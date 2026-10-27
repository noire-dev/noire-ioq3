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

	phys->m_world->setForceUpdateAllAabbs(false);
	phys->m_world->getSolverInfo().m_numIterations = phys_solver_iterations->integer;
	phys->m_world->getSolverInfo().m_solverMode |= SOLVER_RANDMIZE_ORDER | SOLVER_FRICTION_SEPARATE | SOLVER_USE_2_FRICTION_DIRECTIONS | SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
	phys->m_world->getSolverInfo().m_splitImpulse = true;
	phys->m_world->getSolverInfo().m_splitImpulsePenetrationThreshold = -0.04f;
	phys->m_world->getSolverInfo().m_erp = 0.25f;
	phys->m_world->getSolverInfo().m_globalCfm = 0.0f;
	// Do not permit extra penetration in Bullet's CCD sweep. This matters for
	// very thin static geometry; the normal contact solver still handles contacts.
	phys->m_world->getDispatchInfo().m_allowedCcdPenetration = 0.0f;

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
