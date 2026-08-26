#include <cstddef>
#include <vector>
#include <cmath>

#include "bt_wrapper.h"

struct btPhysicsWorld_s {
	btDefaultCollisionConfiguration* m_config;
	btCollisionDispatcher* m_dispatcher;
	btDbvtBroadphase* m_broadphase;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld* m_world;
};

struct btCollisionShape_s {
	btCollisionShape* m_shape;
};

struct btRigidBody_s {
	btRigidBody* m_body;
	btCollisionShape* m_shape;
	btDefaultMotionState* m_motionState;
};

typedef struct btConstraint_s {
	btTypedConstraint* m_constraint;
} btConstraint_t;

// Углы Эйлера в кватернион для Q3 формата
static void AnglesToQuat(float pitch, float yaw, float roll, float* qx, float* qy, float* qz, float* qw) {
	float p = pitch * M_PI / 180.0f;
	float y = yaw * M_PI / 180.0f;
	float r = roll * M_PI / 180.0f;

	float cp = cosf(p * 0.5f);
	float sp = sinf(p * 0.5f);
	float cy = cosf(y * 0.5f);
	float sy = sinf(y * 0.5f);
	float cr = cosf(r * 0.5f);
	float sr = sinf(r * 0.5f);

	*qw = cy * cp * cr + sy * sp * sr;
	*qx = cy * sp * cr + sy * cp * sr;
	*qy = sy * cp * cr - cy * sp * sr;
	*qz = cy * cp * sr - sy * sp * cr;
}

// Индексы углов из движка
#define PITCH 0
#define YAW 1
#define ROLL 2

// Кватернион в углы Эйлера для Q3 формата
static void QuatToAngles(float qx, float qy, float qz, float qw, float* pitch, float* yaw, float* roll) {
	float sin_pitch = 2.0f * (qw * qy - qz * qx);

	if(fabsf(sin_pitch) >= 1.0f - 1e-6f) {
		float sign = copysignf(1.0f, sin_pitch);
		*roll = 0.0f;
		*yaw = -2.0f * sign * atan2f(qx, qw) * 180.0f / M_PI;
		*pitch = sign * 90.0f;
	} else {
		*roll = atan2f(2.0f * (qw * qx + qy * qz), 1.0f - 2.0f * (qx * qx + qy * qy)) * 180.0f / M_PI;
		*pitch = asinf(sin_pitch) * 180.0f / M_PI;
		*yaw = atan2f(2.0f * (qw * qz + qx * qy), 1.0f - 2.0f * (qy * qy + qz * qz)) * 180.0f / M_PI;
	}
}

#define MAX_WELDS 4096

btPhysicsWorld_t* physWorld = NULL;
static btCollisionShape_t* boxShape = NULL;
static btCollisionShape_t* shapes[MAX_MODELS];
static btRigidBody_t* bodies[MAX_GENTITIES];
static float bodies_mass[MAX_GENTITIES];
static btConstraint_t* welds[MAX_WELDS];
static int weldBodyA[MAX_WELDS];
static int weldBodyB[MAX_WELDS];
static int numWelds = 0;
static btTriangleMesh* bspPatchMesh = nullptr;

void SV_PhysicsInit(void) {
	if(physWorld) return;
	physWorld = BT_Init();
	boxShape = BT_CreateBoxShape(25, 25, 25);

	physWorld->m_world->setGravity(btVector3(phys_gravity_x->integer, phys_gravity_y->integer, phys_gravity_z->integer));

	for(int i = 0; i < MAX_GENTITIES; i++) bodies[i] = NULL;
	for(int i = 0; i < MAX_GENTITIES; i++) bodies_mass[i] = 0.0f;
	for(int i = 0; i < MAX_MODELS; i++) shapes[i] = NULL;
	for(int i = 0; i < MAX_WELDS; i++) {
		welds[i] = NULL;
		weldBodyA[i] = -1;
		weldBodyB[i] = -1;
	}
	numWelds = 0;

	BT_LoadBSPToWorld(&cm);
}

void SV_CreateShape(int modelindex, const char* md3Path) {
	if(!physWorld || modelindex < 0 || modelindex >= MAX_MODELS || !md3Path) return;
	if(shapes[modelindex]) return;

	void* buffer = NULL;
	int length = FS_ReadFile(md3Path, &buffer);

	if(length <= 0 || !buffer) {
		Com_Printf(S_COLOR_YELLOW "SV_CreateShape: cannot open %s\n", md3Path);
		return;
	}

	if(length < (int)sizeof(md3Header_t)) {
		Com_Printf(S_COLOR_YELLOW "SV_CreateShape: file too small %s\n", md3Path);
		FS_FreeFile(buffer);
		return;
	}

	const md3Header_t* header = (const md3Header_t*)buffer;
	if(header->ident != MD3_IDENT || header->version != MD3_VERSION) {
		Com_Printf(S_COLOR_YELLOW "SV_CreateShape: invalid MD3 in %s\n", md3Path);
		FS_FreeFile(buffer);
		return;
	}

	// Для Convex Hull собираем УНИКАЛЬНЫЕ вершины
	std::vector<btVector3> convexPoints;

	int totalTris = 0;
	int totalVerts = 0;

	const char* surfacePtr = (const char*)buffer + header->ofsSurfaces;
	const char* bufferEnd = (const char*)buffer + length;

	for(int s = 0; s < header->numSurfaces; s++) {
		if(surfacePtr + sizeof(md3Surface_t) > bufferEnd) break;
		const md3Surface_t* surf = (const md3Surface_t*)surfacePtr;
		if(surf->ident != MD3_IDENT) break;
		if(surf->ofsTriangles == 0 || surf->ofsXyzNormals == 0 || surf->ofsEnd == 0) {
			if(surf->ofsEnd == 0) break;
			surfacePtr += surf->ofsEnd;
			continue;
		}

		const md3XyzNormal_t* verts = (const md3XyzNormal_t*)(surfacePtr + surf->ofsXyzNormals);

		// Сначала собираем ВСЕ вершины поверхности (для Convex Hull)
		for(int i = 0; i < surf->numVerts; i++) {
			btVector3 p(verts[i].xyz[0] * MD3_XYZ_SCALE, verts[i].xyz[1] * MD3_XYZ_SCALE, verts[i].xyz[2] * MD3_XYZ_SCALE);
			convexPoints.push_back(p);
		}

		totalTris += surf->numTriangles;

		totalVerts += surf->numVerts;
		if(surf->ofsEnd == 0) break;
		surfacePtr += surf->ofsEnd;
	}

	FS_FreeFile(buffer);

	Com_Printf("SV_CreateShape: %s -> %d verts, %d tris (mode: %s)\n", md3Path, totalVerts, totalTris, "ConvexHull");

	if(totalVerts == 0) {
		Com_Printf(S_COLOR_YELLOW "  FAILED, no geometry in %s\n", md3Path);
		return;
	}

	btCollisionShape* finalShape = nullptr;

	// ConvexHull для простых объектов
	btConvexHullShape* hull = new btConvexHullShape();
	for(const auto& p : convexPoints) hull->addPoint(p, false);  // false = не пересчитывать AABB каждый раз
	hull->recalcLocalAabb();
	finalShape = hull;

	btCollisionShape_t* wrappedShape = new btCollisionShape_t();
	wrappedShape->m_shape = finalShape;
	shapes[modelindex] = wrappedShape;

	btVector3 aabbMin, aabbMax;
	finalShape->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
	btVector3 size = aabbMax - aabbMin;
}

static void SV_PhysicsSetupCCD(int entityID) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;

	btRigidBody* body = bodies[entityID]->m_body;

	// Работаем только с динамикой
	if(body->getInvMass() == 0.0f) return;

	btCollisionShape* shape = bodies[entityID]->m_shape;

	// 1. Получаем точные размеры объекта
	btVector3 aabbMin, aabbMax;
	shape->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
	btVector3 size = aabbMax - aabbMin;

	// 2. Находим самую тонкую часть объекта
	float minDim = fminf(size.x(), fminf(size.y(), size.z()));

	// 3. Рассчитываем радиус сферы CCD
	// Берем 50% от минимального размера. Это максимально возможный радиус,
	// при котором сфера гарантированно поместится внутри объекта.
	float sphereRadius = minDim * 0.5f;

	// Если объект нулевой толщины (баг модели), ставим минимум чтобы не крашнуло Bullet
	if(sphereRadius <= 0.0f) sphereRadius = 0.1f;

	// 4. Применяем настройки
	body->setCcdSweptSphereRadius(sphereRadius);

	// Порог движения равен радиусу.
	// CCD включится всегда, когда объект смещается быстрее чем на половину своей толщины за кадр.
	body->setCcdMotionThreshold(sphereRadius);
}

void SV_PhysicsCreateBox(int entityID, float x, float y, float z, float mass) {
	if(!physWorld || entityID < 0 || entityID >= MAX_GENTITIES) return;

	if(bodies[entityID]) {  // Не спавним объект снова чтобы не сломать указатели
		Com_Error(ERR_DROP, "SV_PhysicsCreateBox: attempt to recreate physics object\n");
		return;
	}

	// Выбор формы объекта
	btCollisionShape* shape = boxShape->m_shape;

	// Расчёт момента инерции для динамики
	btVector3 inertia(0, 0, 0);
	if(mass > 0.0f) shape->calculateLocalInertia(mass, inertia);
	bodies_mass[entityID] = mass;

	// Настройка положения
	btTransform trans;
	trans.setIdentity();
	trans.setOrigin(btVector3(x, y, z));

	// Выбор состояния движения
	btDefaultMotionState* ms = new btDefaultMotionState(trans);
	btRigidBody::btRigidBodyConstructionInfo info(mass, ms, shape, inertia);

	// Свойства объекта
	info.m_restitution = 0.15f;
	info.m_friction = 0.95f;
	info.m_rollingFriction = 0.10f;
	info.m_spinningFriction = 0.05f;

	// Создание физического объекта
	btRigidBody* bulletBody = new btRigidBody(info);
	bulletBody->setSleepingThresholds(1.00f, 1.00f);
	bulletBody->setDamping(0.10f, 0.10f);
	bulletBody->setDeactivationTime(0.25f);
	bulletBody->setUserIndex(entityID);
	physWorld->m_world->addRigidBody(bulletBody);

	// Враппер моделей
	btRigidBody_t* wrapper = new btRigidBody_t();
	wrapper->m_body = bulletBody;
	wrapper->m_shape = shape;
	wrapper->m_motionState = ms;
	bodies[entityID] = wrapper;

	SV_PhysicsSetupCCD(entityID);

	return;
}

void SV_PhysicsCreateModel(int entityID, int modelindex, float x, float y, float z, float mass) {
	if(!physWorld || entityID < 0 || entityID >= MAX_GENTITIES) return;

	if(modelindex < 0 || modelindex >= MAX_MODELS || !shapes[modelindex]) {  // Проверяем границы!
		Com_Error(ERR_DROP, "Invalid modelindex %i or shape not loaded\n", modelindex);
		return;
	}

	if(bodies[entityID]) {  // Не спавним объект снова чтобы не сломать указатели
		Com_Error(ERR_DROP, "SV_PhysicsCreateModel: attempt to recreate physics object\n");
		return;
	}

	// Выбор формы объекта
	btCollisionShape* shape = shapes[modelindex]->m_shape;

	// Расчёт момента инерции для динамики
	btVector3 inertia(0, 0, 0);
	if(mass > 0.0f) shape->calculateLocalInertia(mass, inertia);
	bodies_mass[entityID] = mass;

	// Настройка положения
	btTransform trans;
	trans.setIdentity();
	trans.setOrigin(btVector3(x, y, z));

	// Выбор состояния движения
	btDefaultMotionState* ms = new btDefaultMotionState(trans);
	btRigidBody::btRigidBodyConstructionInfo info(mass, ms, shape, inertia);

	// Свойства объекта
	info.m_restitution = 0.15f;
	info.m_friction = 0.95f;
	info.m_rollingFriction = 0.10f;
	info.m_spinningFriction = 0.05f;

	// Создание физического объекта
	btRigidBody* bulletBody = new btRigidBody(info);
	bulletBody->setSleepingThresholds(1.00f, 1.00f);
	bulletBody->setDamping(0.10f, 0.10f);
	bulletBody->setDeactivationTime(0.25f);
	bulletBody->setUserIndex(entityID);
	physWorld->m_world->addRigidBody(bulletBody);

	// Враппер моделей
	btRigidBody_t* wrapper = new btRigidBody_t();
	wrapper->m_body = bulletBody;
	wrapper->m_shape = shape;
	wrapper->m_motionState = ms;
	bodies[entityID] = wrapper;

	SV_PhysicsSetupCCD(entityID);

	return;
}

static void SV_PhysicsRemoveWeldsForEntity(int entityID) {
	for(int i = 0; i < MAX_WELDS; i++) {
		if(!welds[i]) continue;
		if(weldBodyA[i] == entityID || weldBodyB[i] == entityID) {
			physWorld->m_world->removeConstraint(welds[i]->m_constraint);
			delete welds[i]->m_constraint;
			delete welds[i];
			welds[i] = NULL;
			weldBodyA[i] = -1;
			weldBodyB[i] = -1;
			numWelds--;
		}
	}
}

void SV_PhysicsDeleteObject(int entityID) {
	if(!physWorld || entityID < 0 || entityID >= MAX_GENTITIES) return;

	if(bodies[entityID]) {
		bodies_mass[entityID] = 0.0f;
		SV_PhysicsRemoveWeldsForEntity(entityID);
		physWorld->m_world->removeRigidBody(bodies[entityID]->m_body);
		delete bodies[entityID]->m_motionState;
		delete bodies[entityID]->m_body;
		delete bodies[entityID];
		bodies[entityID] = NULL;
	}
}

void SV_PhysicsGetPosition(int entityID, float* x, float* y, float* z) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) {
		*x = *y = *z = 0;
		return;
	}
	btVector3 pos = bodies[entityID]->m_body->getCenterOfMassPosition();
	*x = pos.x();
	*y = pos.y();
	*z = pos.z();
}

void SV_PhysicsSetPosition(int entityID, float x, float y, float z) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;
	bodies[entityID]->m_body->activate(true);
	btTransform& t = bodies[entityID]->m_body->getWorldTransform();
	t.setOrigin(btVector3(x, y, z));
	bodies[entityID]->m_body->setCenterOfMassTransform(t);
}

void SV_PhysicsGetAngles(int entityID, float* pitch, float* yaw, float* roll) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) {
		*pitch = *yaw = *roll = 0;
		return;
	}
	float qx, qy, qz, qw;
	btQuaternion rot = bodies[entityID]->m_body->getOrientation();
	qx = rot.x();
	qy = rot.y();
	qz = rot.z();
	qw = rot.w();
	QuatToAngles(qx, qy, qz, qw, pitch, yaw, roll);
}

void SV_PhysicsSetAngles(int entityID, float pitch, float yaw, float roll) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;
	float qx, qy, qz, qw;
	bodies[entityID]->m_body->activate(true);
	AnglesToQuat(pitch, yaw, roll, &qx, &qy, &qz, &qw);
	btTransform& t = bodies[entityID]->m_body->getWorldTransform();
	t.setRotation(btQuaternion(qx, qy, qz, qw));
	bodies[entityID]->m_body->setWorldTransform(t);
}

void SV_PhysicsGetLinearVelocity(int entityID, float* vx, float* vy, float* vz) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;
	btVector3 vel = bodies[entityID]->m_body->getLinearVelocity();
	*vx = vel.x();
	*vy = vel.y();
	*vz = vel.z();
}

void SV_PhysicsSetLinearVelocity(int entityID, float vx, float vy, float vz) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;
	bodies[entityID]->m_body->activate(true);
	bodies[entityID]->m_body->setLinearVelocity(btVector3(vx, vy, vz));
}

void SV_PhysicsGetAngularVelocity(int entityID, float* vx, float* vy, float* vz) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;
	btVector3 vel = bodies[entityID]->m_body->getAngularVelocity();
	*vx = vel.x();
	*vy = vel.y();
	*vz = vel.z();
}

void SV_PhysicsSetAngularVelocity(int entityID, float vx, float vy, float vz) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;
	bodies[entityID]->m_body->activate(true);
	bodies[entityID]->m_body->setAngularVelocity(btVector3(vx, vy, vz));
}

void SV_PhysicsApplyCentralForce(int entityID, float fx, float fy, float fz) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;
	bodies[entityID]->m_body->activate(true);
	bodies[entityID]->m_body->applyCentralForce(btVector3(fx, fy, fz));
}

void SV_PhysicsFreezeObject(int entityID) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;
	bodies[entityID]->m_body->setMassProps(0.0f, btVector3(0, 0, 0));
	bodies[entityID]->m_body->setLinearVelocity(btVector3(0, 0, 0));
	bodies[entityID]->m_body->setAngularVelocity(btVector3(0, 0, 0));
	bodies[entityID]->m_body->setActivationState(DISABLE_DEACTIVATION);
}

void SV_PhysicsUnfreezeObject(int entityID) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return;

	btVector3 inertia(0, 0, 0);
	bodies[entityID]->m_body->getCollisionShape()->calculateLocalInertia(bodies_mass[entityID], inertia);
	bodies[entityID]->m_body->setMassProps(bodies_mass[entityID], inertia);
	bodies[entityID]->m_body->setActivationState(ACTIVE_TAG);
	bodies[entityID]->m_body->activate(true);
}

void SV_PhysicsWeld(int entA, int entB, float px, float py, float pz) {
	if(entA < 0 || entA >= MAX_GENTITIES || !bodies[entA]) return;
	if(entB < 0 || entB >= MAX_GENTITIES || !bodies[entB]) return;

	int slot = -1;
	for(int i = 0; i < MAX_WELDS; i++) {
		if(welds[i] == NULL) {
			slot = i;
			break;
		}
	}
	if(slot < 0) {
		Com_Printf(S_COLOR_YELLOW "SV_PhysicsWeld: no free weld slots\n");
		return;
	}

	btVector3 pivot(px, py, pz);
	btTransform frameA = btTransform::getIdentity();
	btTransform frameB = btTransform::getIdentity();
	frameA.setOrigin(pivot - bodies[entA]->m_body->getCenterOfMassPosition());
	frameB.setOrigin(pivot - bodies[entB]->m_body->getCenterOfMassPosition());

	btConstraint_t* con = new btConstraint_t();
	con->m_constraint = new btFixedConstraint(*bodies[entA]->m_body, *bodies[entB]->m_body, frameA, frameB);
	physWorld->m_world->addConstraint(con->m_constraint, true);

	welds[slot] = con;
	weldBodyA[slot] = entA;
	weldBodyB[slot] = entB;
	numWelds++;

	return;
}

qboolean SV_PhysicsIsSleeping(int entityID) {
	if(entityID < 0 || entityID >= MAX_GENTITIES || !bodies[entityID]) return qfalse;
	int state = bodies[entityID]->m_body->getActivationState();
	return (state == ISLAND_SLEEPING || state == WANTS_DEACTIVATION) ? qtrue : qfalse;
}

struct PhysicsTraceCallback : public btCollisionWorld::ClosestRayResultCallback {
	int skipEntity;

	PhysicsTraceCallback(const btVector3& from, const btVector3& to, int skip) : ClosestRayResultCallback(from, to), skipEntity(skip) {}

	bool needsCollision(btBroadphaseProxy* proxy0) const override {
		// Пропускаем static/BSP тела (userIndex == -1)
		btCollisionObject* obj = (btCollisionObject*)proxy0->m_clientObject;
		int idx = obj->getUserIndex();
		if(idx < 0) return false;            // BSP/static — пропускаем
		if(idx == skipEntity) return false;  // skip entity
		return true;
	}
};

void BT_PhysicsTrace(const float* start, const float* end, const float* mins, const float* maxs, int skipEntity, physTrace_t* result) {
	result->fraction = 1.0f;
	result->hit = qfalse;
	result->entityNum = -1;
	VectorCopy(end, result->endpos);
	VectorClear(result->normal);
	result->planeDist = 0.0f;

	if(!physWorld || !physWorld->m_world) return;

	btVector3 from(start[0], start[1], start[2]);
	btVector3 to(end[0], end[1], end[2]);

	float dx = maxs[0] - mins[0];
	float dy = maxs[1] - mins[1];
	float dz = maxs[2] - mins[2];
	bool hasVolume = (dx > 0.1f && dy > 0.1f && dz > 0.1f);

	if(hasVolume) {
		btVector3 halfExtents(dx * 0.5f, dy * 0.5f, dz * 0.5f);
		btVector3 centerOffset((mins[0] + maxs[0]) * 0.5f, (mins[1] + maxs[1]) * 0.5f, (mins[2] + maxs[2]) * 0.5f);

		btBoxShape sweepShape(halfExtents);
		sweepShape.setMargin(0.0f);  // Обязательно 0, чтобы не увеличивать bbox

		btTransform fromTrans, toTrans;
		fromTrans.setIdentity();
		toTrans.setIdentity();
		fromTrans.setOrigin(from + centerOffset);
		toTrans.setOrigin(to + centerOffset);

		struct SweepCallback : public btCollisionWorld::ClosestConvexResultCallback {
			int skipEnt;
			SweepCallback(const btVector3& from, const btVector3& to, int skip) : ClosestConvexResultCallback(from, to), skipEnt(skip) {}

			bool needsCollision(btBroadphaseProxy* proxy0) const override {
				btCollisionObject* obj = (btCollisionObject*)proxy0->m_clientObject;
				int idx = obj->getUserIndex();
				if(idx < 0) return false;         // BSP/static
				if(idx == skipEnt) return false;  // skip
				return true;
			}
		};

		SweepCallback cb(fromTrans.getOrigin(), toTrans.getOrigin(), skipEntity);
		physWorld->m_world->convexSweepTest(&sweepShape, fromTrans, toTrans, cb);

		if(cb.hasHit()) {
			result->fraction = cb.m_closestHitFraction;
			result->hit = qtrue;
			btVector3 hp = cb.m_hitPointWorld;
			btVector3 hn = cb.m_hitNormalWorld;

			// endpos = позиция origin'а игрока в момент удара
			result->endpos[0] = start[0] + result->fraction * (end[0] - start[0]);
			result->endpos[1] = start[1] + result->fraction * (end[1] - start[1]);
			result->endpos[2] = start[2] + result->fraction * (end[2] - start[2]);

			result->normal[0] = hn.x();
			result->normal[1] = hn.y();
			result->normal[2] = hn.z();

			// plane.dist вычисляется из РЕАЛЬНОЙ точки контакта
			result->planeDist = hp.x() * hn.x() + hp.y() * hn.y() + hp.z() * hn.z();

			result->entityNum = cb.m_hitCollisionObject ? cb.m_hitCollisionObject->getUserIndex() : -1;
		}
	} else {
		PhysicsTraceCallback cb(from, to, skipEntity);
		physWorld->m_world->rayTest(from, to, cb);

		if(cb.hasHit()) {
			result->fraction = cb.m_closestHitFraction;
			result->hit = qtrue;
			btVector3 hp = cb.m_hitPointWorld;
			btVector3 hn = cb.m_hitNormalWorld;

			// Для raycast интерполяция тоже корректна
			result->endpos[0] = start[0] + result->fraction * (end[0] - start[0]);
			result->endpos[1] = start[1] + result->fraction * (end[1] - start[1]);
			result->endpos[2] = start[2] + result->fraction * (end[2] - start[2]);

			result->normal[0] = hn.x();
			result->normal[1] = hn.y();
			result->normal[2] = hn.z();

			result->planeDist = hp.x() * hn.x() + hp.y() * hn.y() + hp.z() * hn.z();

			result->entityNum = cb.m_collisionObject ? cb.m_collisionObject->getUserIndex() : -1;
		}
	}
}

void SV_PhysicsStep(float dt) {
	if(physWorld) physWorld->m_world->stepSimulation(dt, phys_steps->integer, 1.00f / phys_fps->integer);
}

void SV_PhysicsShutdown(void) {
	if(!physWorld) return;

	// Освобождаем сварки
	for(int i = 0; i < MAX_WELDS; i++) {
		if(welds[i]) {
			physWorld->m_world->removeConstraint(welds[i]->m_constraint);
			delete welds[i]->m_constraint;
			delete welds[i];
			welds[i] = NULL;
			weldBodyA[i] = -1;
			weldBodyB[i] = -1;
		}
	}
	numWelds = 0;

	// Освобождаем динамические объекты (игровые энтити)
	for(int i = 0; i < MAX_GENTITIES; i++) SV_PhysicsDeleteObject(i);

	// Освобождаем модели
	for(int i = 0; i < MAX_MODELS; i++) {
		if(shapes[i]) {
			delete shapes[i]->m_shape;
			delete shapes[i];
			shapes[i] = NULL;
		}
	}

	if(bspPatchMesh) {  // Освобождаем BSP патчи
		delete bspPatchMesh;
		bspPatchMesh = nullptr;
	}

	// Перед освобождением Bullet Physics нужно удалить ВСЕ оставшиеся объекты
	btDiscreteDynamicsWorld* world = physWorld->m_world;
	for(int i = world->getNumCollisionObjects() - 1; i >= 0; i--) {
		btCollisionObject* obj = world->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);

		world->removeCollisionObject(obj);

		// Освобождаем motion state, если он есть
		if(body && body->getMotionState()) delete body->getMotionState();

		// Освобождаем BSP
		if(body && body->getCollisionShape()) delete body->getCollisionShape();

		delete obj;
	}

	if(boxShape) {
		delete boxShape->m_shape;
		delete boxShape;
		boxShape = NULL;
	}

	// Освобождаем Bullet Physics
	BT_Shutdown(physWorld);

	physWorld = NULL;
}

void BT_LoadBSPToWorld(const clipMap_t* cm) {
	if(!physWorld || !cm) return;

	int brushCount = 0;

	// ЗАГРУЗКА БРАШЕЙ
	for(int i = 0; i < cm->numBrushes; i++) {
		const cbrush_t* brush = &cm->brushes[i];

		if(!(brush->contents & (CONTENTS_SOLID))) continue;  // Пропускаем браши без коллизии

		// Создаём box коллизию из bounds браша (используется как fallback)
		btVector3 halfExtents((brush->bounds[1][0] - brush->bounds[0][0]) * 0.5f, (brush->bounds[1][1] - brush->bounds[0][1]) * 0.5f, (brush->bounds[1][2] - brush->bounds[0][2]) * 0.5f);

		if(halfExtents.x() <= 0 || halfExtents.y() <= 0 || halfExtents.z() <= 0) continue;

		btVector3 center((brush->bounds[0][0] + brush->bounds[1][0]) * 0.5f, (brush->bounds[0][1] + brush->bounds[1][1]) * 0.5f, (brush->bounds[0][2] + brush->bounds[1][2]) * 0.5f);

		btCollisionShape* shape = nullptr;

		// Пытаемся построить Convex Hull коллизию на основе плоскостей браша
		if(brush->numsides >= 4 && brush->sides != nullptr) {
			// Определяем направление нормалей (внутрь или наружу)
			bool outwards = true;
			const cplane_t* p0 = brush->sides[0].plane;
			if(p0) {
				btVector3 n0(p0->normal[0], p0->normal[1], p0->normal[2]);
				float d_center = n0.dot(center) - p0->dist;
				if(d_center > 0.0f) outwards = false;  // Нормали смотрят внутрь объема
			}

			bool allNonSolid = true;
			for(int s = 0; s < brush->numsides; s++) {
				if(!(brush->sides[s].surfaceFlags & (SURF_NONSOLID | SURF_NODRAW))) {
					allNonSolid = false;
					break;
				}
			}
			if(allNonSolid) continue;

			// Перебираем все тройки плоскостей для поиска вершин многогранника
			std::vector<btVector3> verts;
			for(int j = 0; j < brush->numsides; j++) {
				for(int k = j + 1; k < brush->numsides; k++) {
					for(int l = k + 1; l < brush->numsides; l++) {
						const cplane_t* p1 = brush->sides[j].plane;
						const cplane_t* p2 = brush->sides[k].plane;
						const cplane_t* p3 = brush->sides[l].plane;

						if(!p1 || !p2 || !p3) continue;

						btVector3 n1(p1->normal[0], p1->normal[1], p1->normal[2]);
						btVector3 n2(p2->normal[0], p2->normal[1], p2->normal[2]);
						btVector3 n3(p3->normal[0], p3->normal[1], p3->normal[2]);
						float d1 = p1->dist;
						float d2 = p2->dist;
						float d3 = p3->dist;

						// Ищем точку пересечения трех плоскостей
						btVector3 n2n3 = n2.cross(n3);
						float det = n1.dot(n2n3);

						if(std::fabs(det) > 1e-4f) {
							btVector3 point = (n2n3 * d1 + n3.cross(n1) * d2 + n1.cross(n2) * d3) / det;

							// Проверяем, находится ли точка внутри всех остальных плоскостей
							bool valid = true;
							for(int m = 0; m < brush->numsides; m++) {
								if(m == j || m == k || m == l) continue;
								const cplane_t* pm = brush->sides[m].plane;
								if(!pm) continue;
								btVector3 nm(pm->normal[0], pm->normal[1], pm->normal[2]);
								float dist = nm.dot(point) - pm->dist;

								if(outwards) {
									if(dist > 0.1f) {
										valid = false;
										break;
									}
								} else {
									if(dist < -0.1f) {
										valid = false;
										break;
									}
								}
							}
							if(valid) verts.push_back(point - center);
						}
					}
				}
			}

			// Если удалось найти достаточно вершин для объема, создаем Convex Hull
			if(verts.size() >= 4) {
				btConvexHullShape* convexShape = new btConvexHullShape();
				for(size_t v_idx = 0; v_idx < verts.size(); v_idx++) convexShape->addPoint(verts[v_idx], false);
				convexShape->recalcLocalAabb();
				shape = convexShape;
			}
		}

		// Если не удалось построить Hull, используем обычный Box
		if(!shape) shape = new btBoxShape(halfExtents);

		btTransform trans;
		trans.setIdentity();
		trans.setOrigin(center);

		btDefaultMotionState* ms = new btDefaultMotionState(trans);
		btRigidBody::btRigidBodyConstructionInfo info(0, ms, shape);
		btRigidBody* body = new btRigidBody(info);

		physWorld->m_world->addRigidBody(body);
		brushCount++;
	}

	Com_Printf("BSP converted to physics: %d brushes\n", brushCount);

	// ЗАГРУЗКА ПАТЧЕЙ
	if(bspPatchMesh) {
		delete bspPatchMesh;
		bspPatchMesh = nullptr;
	}
	int patchCount = 0;
	int triangleCount = 0;

	if(cm->numSurfaces > 0 && cm->surfaces != nullptr) {
		for(int i = 0; i < cm->numSurfaces; i++) {
			cPatch_t* patch = cm->surfaces[i];
			if(!patch || !patch->pc) continue;

			// Проверяем contents патча
			if(!(patch->contents & (CONTENTS_SOLID))) continue;
			if(patch->surfaceFlags & (SURF_NONSOLID | SURF_NODRAW)) continue;

			patchCollide_t* pc = patch->pc;

			for(int f = 0; f < pc->numFacets; f++) {  // Для каждой стороны патча извлекаем треугольники
				facet_t* facet = &pc->facets[f];

				// Получаем surface plane стороны
				if(facet->surfacePlane < 0 || facet->surfacePlane >= pc->numPlanes) continue;

				patchPlane_t* surfPlane = &pc->planes[facet->surfacePlane];
				btVector3 surfNormal(surfPlane->plane[0], surfPlane->plane[1], surfPlane->plane[2]);
				float surfDist = surfPlane->plane[3];

				// Собираем вершины стороны через пересечение border planes
				std::vector<btVector3> facetVerts;
				for(int b = 0; b < facet->numBorders; b++) {
					int borderIdx = facet->borderPlanes[b];
					if(borderIdx < 0 || borderIdx >= pc->numPlanes) continue;

					int nextB = (b + 1) % facet->numBorders;
					int nextBorderIdx = facet->borderPlanes[nextB];
					if(nextBorderIdx < 0 || nextBorderIdx >= pc->numPlanes) continue;

					patchPlane_t* borderPlane = &pc->planes[borderIdx];
					patchPlane_t* nextBorderPlane = &pc->planes[nextBorderIdx];

					btVector3 borderNormal(borderPlane->plane[0], borderPlane->plane[1], borderPlane->plane[2]);
					btVector3 nextBorderNormal(nextBorderPlane->plane[0], nextBorderPlane->plane[1], nextBorderPlane->plane[2]);

					float borderDist = borderPlane->plane[3];
					float nextBorderDist = nextBorderPlane->plane[3];

					// Находим точку пересечения трех плоскостей: surface + 2 borders
					btVector3 n2n3 = borderNormal.cross(nextBorderNormal);
					float det = surfNormal.dot(n2n3);

					if(std::fabs(det) > 1e-4f) {
						btVector3 point = (n2n3 * surfDist + nextBorderNormal.cross(surfNormal) * borderDist + surfNormal.cross(borderNormal) * nextBorderDist) / det;

						if(facet->borderInward[b]) {  // Учитываем направление нормали границы (borderInward)
							// Нормаль смотрит внутрь - инвертируем проверку
							float dist = borderNormal.dot(point) - borderDist;
							if(dist < -0.5f) continue;
						} else {
							float dist = borderNormal.dot(point) - borderDist;
							if(dist > 0.5f) continue;
						}

						facetVerts.push_back(point);
					}
				}

				// Создаем треугольники из вершин (fan triangulation)
				if(facetVerts.size() >= 3) {
					if(!bspPatchMesh) bspPatchMesh = new btTriangleMesh();

					for(size_t v = 1; v < facetVerts.size() - 1; v++) {
						bspPatchMesh->addTriangle(facetVerts[0], facetVerts[v], facetVerts[v + 1]);
						triangleCount++;
					}
				}
			}

			patchCount++;
		}
	}

	// Создаем collision shape для всех патчей
	if(bspPatchMesh && bspPatchMesh->getNumTriangles() > 0) {
		btCollisionShape* patchShape = new btBvhTriangleMeshShape(bspPatchMesh, true);

		btTransform trans;
		trans.setIdentity();
		trans.setOrigin(btVector3(0, 0, 0));

		btDefaultMotionState* ms = new btDefaultMotionState(trans);
		btRigidBody::btRigidBodyConstructionInfo info(0, ms, patchShape);
		btRigidBody* body = new btRigidBody(info);

		physWorld->m_world->addRigidBody(body);

		Com_Printf("Patches converted to physics: %d patches with %d triangles\n", patchCount, triangleCount);
	} else if(bspPatchMesh) {
		delete bspPatchMesh;
		bspPatchMesh = nullptr;
	}
}
