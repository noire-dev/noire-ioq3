#ifndef BT_WRAPPER_H
#define BT_WRAPPER_H
#ifdef __cplusplus
extern "C" {
#endif
#include "../../qcommon/cm_local.h"
#include "../../qcommon/cm_patch.h"
#include "../../qcommon/q_shared.h"
#include "../../qcommon/qfiles.h"
#ifdef __cplusplus
}
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern cvar_t* phys_fps;
extern cvar_t* phys_steps;
extern cvar_t* phys_solver_iterations;
extern cvar_t* phys_gravity_x;
extern cvar_t* phys_gravity_y;
extern cvar_t* phys_gravity_z;

typedef struct btPhysicsWorld_s btPhysicsWorld_t;
typedef struct btRigidBody_s btRigidBody_t;
typedef struct btCollisionShape_s btCollisionShape_t;

typedef struct {
	float fraction;
	vec3_t endpos;
	vec3_t normal;
	float planeDist;
	int entityNum;
	bool hit;
} physTrace_t;

// Управление миром
btPhysicsWorld_t* BT_Init(void);
void BT_Shutdown(btPhysicsWorld_t* world);

// Создание форм
btCollisionShape_t* BT_CreateBoxShape(float hx, float hy, float hz);

// SV интеграция
void SV_PhysicsInit(void);
void SV_PhysicsStep(float dt);
void SV_PhysicsShutdown(void);

// QVM интеграция
void SV_CreateShape(int modelindex, const char* objPath);
void SV_PhysicsCreateBox(int entityID, float x, float y, float z, float mass);
void SV_PhysicsCreateModel(int entityID, int modelindex, float x, float y, float z, float mass);
void SV_PhysicsDeleteObject(int entityID);
void SV_PhysicsGetPosition(int entityID, float* x, float* y, float* z);
void SV_PhysicsSetPosition(int entityID, float x, float y, float z);
void SV_PhysicsGetAngles(int entityID, float* pitch, float* yaw, float* roll);
void SV_PhysicsSetAngles(int entityID, float pitch, float yaw, float roll);
void SV_PhysicsGetLinearVelocity(int entityID, float* vx, float* vy, float* vz);
void SV_PhysicsSetLinearVelocity(int entityID, float vx, float vy, float vz);
void SV_PhysicsGetAngularVelocity(int entityID, float* vx, float* vy, float* vz);
void SV_PhysicsSetAngularVelocity(int entityID, float vx, float vy, float vz);
void SV_PhysicsApplyCentralForce(int entityID, float fx, float fy, float fz);
void SV_PhysicsFreezeObject(int entityID);
void SV_PhysicsUnfreezeObject(int entityID);
void SV_PhysicsWeld(int entA, int entB, float px, float py, float pz);
bool SV_PhysicsIsSleeping(int entityID);
void BT_PhysicsTrace(const float* start, const float* end, const float* mins, const float* maxs, int skipEntity, physTrace_t* result);

// BSP интеграция
void BT_LoadBSPToWorld(const clipMap_t* cm);

#ifdef __cplusplus
}
#endif

#endif