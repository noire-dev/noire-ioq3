/*
===========================================================================
cg_particles.c -- simplified particle system
===========================================================================
*/
#include "cg_local.h"

#define MAX_PARTICLES 2048
#define PARTICLE_LIFETIME_BASE 1.5f
#define PARTICLE_LIFETIME_VARIANCE 0.5f
#define PARTICLE_SIZE 4.0f
#define PARTICLE_SHADER (cgs.media.smokePuffShader)

typedef struct particle_s {
	struct particle_s* next;
	vec3_t birthPos;  // позиция в момент спавна
	vec3_t velocity;  // начальная скорость (юниты/сек)
	vec3_t gravity;   // ускорение (юниты/сек²)
	vec4_t color;     // RGBA 0..1
	float birthTime;  // cg.time при спавне
	float lifetime;   // секунд
	float size;       // радиус в world units
	int flags;
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
void CG_AddParticles(const vec3_t origin, const vec4_t color, int count, float speed, int flags, const vec3_t gravity) {
	int i;

	if(!particlesReady) CG_ClearParticles();
	if(count <= 0) return;

	for(i = 0; i < count && freeParticles; i++) {
		particle_t* p = freeParticles;
		freeParticles = p->next;

		p->next = activeParticles;
		activeParticles = p;

		VectorCopy(origin, p->birthPos);

		// сферический разлёт: равномерно в кубе, но по модулю ограничено скоростью
		p->velocity[0] = crandom() * speed;
		p->velocity[1] = crandom() * speed;
		p->velocity[2] = crandom() * speed;

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
	}
}

/*
===============
CG_DrawParticle
===============
*/
static void CG_DrawParticle(const particle_t* p) {
	refEntity_t ent;
	float age;  // секунд
	float alpha;
	vec3_t pos;

	age = (cg.time - p->birthTime) * 0.001f;

	// позиция по кинематике: x = x0 + v*t + 0.5*g*t²
	pos[0] = p->birthPos[0] + p->velocity[0] * age + 0.5f * p->gravity[0] * age * age;
	pos[1] = p->birthPos[1] + p->velocity[1] * age + 0.5f * p->gravity[1] * age * age;
	pos[2] = p->birthPos[2] + p->velocity[2] * age + 0.5f * p->gravity[2] * age * age;

	alpha = 1.0f;
	if(p->flags & PF_SMOOTH_FADE) {
		float t = (cg.time - p->birthTime) / (p->lifetime * 1000.0f);
		if(t < 0.0f) t = 0.0f;
		if(t > 1.0f) t = 1.0f;
		alpha = 1.0f - t;
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(pos, ent.origin);
	ent.reType = RT_SPRITE;
	ent.customShader = PARTICLE_SHADER;
	ent.radius = p->size;
	ent.rotation = 0;
	ent.shaderRGBA[0] = (byte)(p->color[0] * 255);
	ent.shaderRGBA[1] = (byte)(p->color[1] * 255);
	ent.shaderRGBA[2] = (byte)(p->color[2] * 255);
	ent.shaderRGBA[3] = (byte)(p->color[3] * alpha * 255);

	trap_R_AddRefEntityToScene(&ent);
}

/*
===============
CG_RenderParticles
===============
*/
void CG_RenderParticles(void) {
	particle_t *p, *next;
	particle_t *active = NULL, *tail = NULL;

	if(!particlesReady) return;

	for(p = activeParticles; p; p = next) {
		next = p->next;

		if(cg.time >= p->birthTime + p->lifetime * 1000.0f) {
			p->next = freeParticles;
			freeParticles = p;
			continue;
		}

		CG_DrawParticle(p);

		p->next = NULL;
		if(!tail) {
			active = tail = p;
		} else {
			tail->next = p;
			tail = p;
		}
	}

	activeParticles = active;
}
