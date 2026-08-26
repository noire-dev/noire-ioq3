#include "btBulletDynamicsCommon.h"
#include "bt_wrapper.h"

struct btPhysicsWorld_s {
	btDefaultCollisionConfiguration* m_config;
	btCollisionDispatcher* m_dispatcher;
	btDbvtBroadphase* m_broadphase;
	btSequentialImpulseConstraintSolver* m_solver;
	btDiscreteDynamicsWorld* m_world;
};

struct btRigidBody_s {
	btRigidBody* m_body;
	btCollisionShape* m_shape;            // храним чтобы удалить
	btDefaultMotionState* m_motionState;  // храним чтобы удалить
};

struct btCollisionShape_s {
	btCollisionShape* m_shape;
};

btPhysicsWorld_t* BT_Init(void) {
	btPhysicsWorld_t* phys = new btPhysicsWorld_t();

	phys->m_config = new btDefaultCollisionConfiguration();
	phys->m_dispatcher = new btCollisionDispatcher(phys->m_config);
	phys->m_broadphase = new btDbvtBroadphase();
	phys->m_solver = new btSequentialImpulseConstraintSolver();
	phys->m_world = new btDiscreteDynamicsWorld(phys->m_dispatcher, phys->m_broadphase, phys->m_solver, phys->m_config);

	phys->m_world->getSolverInfo().m_numIterations = phys_solver_iterations->integer;
	phys->m_world->getSolverInfo().m_solverMode |= SOLVER_RANDMIZE_ORDER | SOLVER_FRICTION_SEPARATE | SOLVER_USE_2_FRICTION_DIRECTIONS | SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
	phys->m_world->getSolverInfo().m_splitImpulse = true;
	phys->m_world->getSolverInfo().m_splitImpulsePenetrationThreshold = -0.04f;
	phys->m_world->getSolverInfo().m_erp = 0.65f;
	phys->m_world->getSolverInfo().m_globalCfm = 0.0f;

	return phys;
}

void BT_Shutdown(btPhysicsWorld_t* phys) {
	if(!phys) return;
	delete phys->m_world;
	delete phys->m_solver;
	delete phys->m_broadphase;
	delete phys->m_dispatcher;
	delete phys->m_config;
	delete phys;
}

btCollisionShape_t* BT_CreateBoxShape(float hx, float hy, float hz) {
	btCollisionShape_t* shape = new btCollisionShape_t();
	shape->m_shape = new btBoxShape(btVector3(hx, hy, hz));
	return shape;
}
