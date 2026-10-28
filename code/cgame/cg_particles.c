/*
===========================================================================
cg_particles.c -- simplified particle system
===========================================================================
*/
#include "cg_local.h"

#define MAX_PARTICLES 262144
#define MAX_PARTICLE_SHADERS 256
#define PARTICLE_LIFETIME_BASE 1.5f
#define PARTICLE_LIFETIME_VARIANCE 0.5f
#define PARTICLE_SIZE 4.0f
#define PARTICLE_SHADER (cgs.media.smokePuffShader)

typedef struct particle_s {
	struct particle_s* next;
	vec3_t birthPos;
	vec3_t velocity;
	vec3_t gravity;
	vec4_t color;
	float birthTime;
	float lifetime;
	float size;
	int flags;
	qhandle_t shader;
} particle_t;

static particle_t particlePool[MAX_PARTICLES];
static particle_t* activeParticles;
static particle_t* freeParticles;
static bool particlesReady = false;

/*
===============
CG_ClearParticles
===============
*/
void CG_ClearParticles(void) {
	int i;

	memset(particlePool, 0, sizeof(particlePool));
	for(i = 0; i < MAX_PARTICLES - 1; i++) {
		particlePool[i].next = &particlePool[i + 1];
	}
	particlePool[MAX_PARTICLES - 1].next = NULL;

	freeParticles = &particlePool[0];
	activeParticles = NULL;
	particlesReady = true;
}

/*
===============
CG_AddParticles

Спавнит count частиц в origin со сферическим разлётом.
  origin   — точка спавна
  color    — RGBA (0..1)
  count    — сколько частиц
  speed    — максимальная компонента скорости (юниты/сек)
  flags    — PF_SMOOTH_FADE / PF_GRAVITY
  gravity  — вектор ускорения, применяется только при PF_GRAVITY
===============
*/
void CG_AddParticles(const vec3_t origin, const vec4_t color, int count, float speed, int flags, const vec3_t gravity, const char* shader) {
	int i;

	if(!particlesReady) CG_ClearParticles();
	if(count <= 0) return;

	for(i = 0; i < count && freeParticles; i++) {
		particle_t* p = freeParticles;
		freeParticles = p->next;

		p->next = activeParticles;
		activeParticles = p;

		VectorCopy(origin, p->birthPos);

		if(speed > 0.0f) {
			float lenSq;
			do {
				p->velocity[0] = crandom() * speed;
				p->velocity[1] = crandom() * speed;
				p->velocity[2] = crandom() * speed;
				lenSq = DotProduct(p->velocity, p->velocity);
			} while(lenSq > speed * speed);
		} else {
			VectorClear(p->velocity);
		}

		if((flags & PF_GRAVITY) && gravity) {
			VectorCopy(gravity, p->gravity);
		} else {
			VectorClear(p->gravity);
		}

		Vector4Copy(color, p->color);

		p->birthTime = cg.time;
		p->lifetime = PARTICLE_LIFETIME_BASE + crandom() * PARTICLE_LIFETIME_VARIANCE;
		p->size = PARTICLE_SIZE;
		p->flags = flags;
		if(strlen(shader))
			p->shader = trap_R_RegisterShader(shader);
		else
			p->shader = PARTICLE_SHADER;
	}
}

/*
===============
CG_EmitParticleQuad

Строит billboard-квад одной частицы прямо в polyVert_t.
Позиция вычисляется по кинематике (x0 + v·t + ½·g·t²),
ориентация — по осям cg.refdef.viewaxis (right/up), alpha — с fade.

Порядок вершин (TR, TL, BL, BR) совпадает с тем, что делает
RB_AddQuadStamp для RT_SPRITE, поэтому winding и fan-триангуляция
в RB_SurfacePolychain дают те же два треугольника и та же
видимая сторона при включённом culling.
===============
*/
static void CG_EmitParticleQuad(const particle_t* p, polyVert_t* out) {
	float age = (cg.time - p->birthTime) * 0.001f;
	vec3_t pos, right, up;
	float alpha = 1.0f;
	byte r, g, b, a;
	int i;

	pos[0] = p->birthPos[0] + p->velocity[0] * age + 0.5f * p->gravity[0] * age * age;
	pos[1] = p->birthPos[1] + p->velocity[1] * age + 0.5f * p->gravity[1] * age * age;
	pos[2] = p->birthPos[2] + p->velocity[2] * age + 0.5f * p->gravity[2] * age * age;

	if(p->flags & PF_SMOOTH_FADE) {
		float t = (cg.time - p->birthTime) / (p->lifetime * 1000.0f);
		if(t < 0.0f) t = 0.0f;
		if(t > 1.0f) t = 1.0f;
		alpha = 1.0f - t;
	}

	VectorScale(cg.refdef.viewaxis[1], p->size, right);
	VectorScale(cg.refdef.viewaxis[2], p->size, up);

	out[0].xyz[0] = pos[0] + right[0] + up[0];
	out[0].xyz[1] = pos[1] + right[1] + up[1];
	out[0].xyz[2] = pos[2] + right[2] + up[2];
	out[0].st[0] = 0.0f;
	out[0].st[1] = 0.0f;

	out[1].xyz[0] = pos[0] - right[0] + up[0];
	out[1].xyz[1] = pos[1] - right[1] + up[1];
	out[1].xyz[2] = pos[2] - right[2] + up[2];
	out[1].st[0] = 1.0f;
	out[1].st[1] = 0.0f;

	out[2].xyz[0] = pos[0] - right[0] - up[0];
	out[2].xyz[1] = pos[1] - right[1] - up[1];
	out[2].xyz[2] = pos[2] - right[2] - up[2];
	out[2].st[0] = 1.0f;
	out[2].st[1] = 1.0f;

	out[3].xyz[0] = pos[0] + right[0] - up[0];
	out[3].xyz[1] = pos[1] + right[1] - up[1];
	out[3].xyz[2] = pos[2] + right[2] - up[2];
	out[3].st[0] = 0.0f;
	out[3].st[1] = 1.0f;

	r = (byte)(p->color[0] * 255);
	g = (byte)(p->color[1] * 255);
	b = (byte)(p->color[2] * 255);
	a = (byte)(p->color[3] * alpha * 255);

	for(i = 0; i < 4; i++) {
		out[i].modulate[0] = r;
		out[i].modulate[1] = g;
		out[i].modulate[2] = b;
		out[i].modulate[3] = a;
	}
}

/*
===============
CG_RenderParticles

Батчим живые частицы по шейдерам: на каждый уникальный шейдер —
один вызов trap_R_AddPolysToScene. Внутри одного шейдера всё
уходит единым RB_SurfacePolychain-батчем, как и раньше.

Алгоритм:
  проход 1 — собираем множество используемых шейдеров и их счётчики;
  вычисляем смещения в общем массиве batch[] (prefix sum);
  проход 2 — фильтруем мёртвых, эмитим живых в свой «слот»;
  flush — один trap_R_AddPolysToScene на каждый непустой шейдер.

Частицы с шейдером за пределами MAX_PARTICLE_SHADERS рендерятся
по одной (крайне редкий overflow-случай) — это не ломает
остальной батч.
===============
*/
void CG_RenderParticles(void) {
	particle_t *p, *next;
	particle_t *active = NULL, *tail = NULL;
	static polyVert_t batch[MAX_PARTICLES * 4];
	polyVert_t overflowQuad[4];

	qhandle_t shaderKeys[MAX_PARTICLE_SHADERS];
	int shaderCounts[MAX_PARTICLE_SHADERS];
	int shaderOffsets[MAX_PARTICLE_SHADERS];
	int shaderWritePos[MAX_PARTICLE_SHADERS];

	int numShaders, totalVerts, i, idx, outIdx;

	if(!particlesReady) return;

	numShaders = 0;
	totalVerts = 0;

	for(p = activeParticles; p; p = p->next) {
		if(cg.time >= p->birthTime + p->lifetime * 1000.0f) continue;

		idx = -1;
		for(i = 0; i < numShaders; i++) {
			if(shaderKeys[i] == p->shader) {
				idx = i;
				break;
			}
		}
		if(idx < 0) {
			if(numShaders >= MAX_PARTICLE_SHADERS) continue;
			idx = numShaders++;
			shaderKeys[idx] = p->shader;
			shaderCounts[idx] = 0;
		}
		shaderCounts[idx]++;
	}

	for(i = 0; i < numShaders; i++) {
		shaderOffsets[i] = totalVerts;
		shaderWritePos[i] = 0;
		totalVerts += shaderCounts[i];
	}

	for(p = activeParticles; p; p = next) {
		next = p->next;

		if(cg.time >= p->birthTime + p->lifetime * 1000.0f) {
			p->next = freeParticles;
			freeParticles = p;
			continue;
		}

		idx = -1;
		for(i = 0; i < numShaders; i++) {
			if(shaderKeys[i] == p->shader) {
				idx = i;
				break;
			}
		}

		if(idx >= 0) {
			outIdx = shaderOffsets[idx] + shaderWritePos[idx];
			CG_EmitParticleQuad(p, &batch[outIdx * 4]);
			shaderWritePos[idx]++;
		} else {
			CG_EmitParticleQuad(p, overflowQuad);
			trap_R_AddPolysToScene(p->shader, 4, overflowQuad, 1);
		}

		p->next = NULL;
		if(!tail) {
			active = tail = p;
		} else {
			tail->next = p;
			tail = p;
		}
	}

	activeParticles = active;

	for(i = 0; i < numShaders; i++) {
		if(shaderWritePos[i] > 0) {
			trap_R_AddPolysToScene(shaderKeys[i], 4, &batch[shaderOffsets[i] * 4], shaderWritePos[i]);
		}
	}
}