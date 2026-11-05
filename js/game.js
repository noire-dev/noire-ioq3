const entity = {};
const player = {};
const npc = {};
const spawnInfo = {};
const level = {};
const phys = {};

// playerState->stats[] indexes (game/bg_public.h statIndex_t)
const STAT = {
    HEALTH: 0,
    ARMOR: 1,
    AMMO: 2,
};

// meansOfDeath_t (game/bg_public.h)
// weapon kills come as MOD.WEAPONS + weaponID, so method - MOD.WEAPONS is the weapon id
const MOD = {
    UNKNOWN: 0,
    WEAPONS: 2048,
    WATER: 2049,
    SLIME: 2050,
    LAVA: 2051,
    CRUSH: 2052,
    TELEFRAG: 2053,
    FALLING: 2054,
    SUICIDE: 2055,
    TRIGGER_HURT: 2056,
};

// swep_list status values (game/bg_public.h)
const WS = {
    NONE: 0,
    HAVE: 1,
    NOAMMO: 2,
};

// clientConnected_t (game/g_local.h) — pers.connected values
const CON = {
    DISCONNECTED: 0,
    CONNECTING: 1,
    CONNECTED: 2,
};

// entity / player / npc api — everything is addressed by entity number,
// the same call works for players, npcs and props
entity.delete = function (entityID) {
    return qvm.call(vm.gapiEntityDelete, qvm.game, entityID);
}

// returns the new entity number
entity.spawn = function (x, y, z, ax, ay, az, modelName) {
    return qvm.call(vm.gapiEntitySpawn, qvm.game, x, y, z, ax, ay, az, modelName);
}

// 1 if the entity is a player or an npc (has a client), 0 for props and empty slots
entity.isClient = function (entityID) {
    return qvm.call(vm.gapiEntityIsClient, qvm.game, entityID);
}

// spawns an npc (bot), returns its entity number
// npc.spawn("Grunt", "sarge", 4, x, y, z, yaw) — skill 1..5, model is a bot model name
// npcs spawned here are removed automatically when they die
npc.spawn = function (npcName, npcModel, skill, x, y, z, yaw) {
    return qvm.call(vm.gapiNPCSpawn, qvm.game, npcName, npcModel, skill, x, y, z, yaw);
}

player.crosshairPoint = function (player, distance, xyz) {
    return qvm.call(vm.gapiPlayerCrosshairPoint, qvm.game, player, distance, xyz);
}

// entity model name, "" if the entity has none
entity.getModel = function (entityID) {
    return qvm.call(vm.gapiEntityGetModel, qvm.game, entityID);
}

// live world position of the entity (r.currentOrigin), xyz: 0/1/2
entity.getPosition = function (entityID, xyz) {
    return qvm.call(vm.gapiEntityGetPosition, qvm.game, entityID, xyz);
}

// entity angles (s.angles), xyz: 0/1/2
entity.getAngles = function (entityID, xyz) {
    return qvm.call(vm.gapiEntityGetAngles, qvm.game, entityID, xyz);
}

// physics api — velocity, forces, freeze/weld, all by entity number
// xyz: 0/1/2 for the getters
phys.getLinearVelocity = function (entityID, xyz) {
    return qvm.call(vm.gapiPhysGetLinearVelocity, qvm.game, entityID, xyz);
}

phys.setLinearVelocity = function (entityID, x, y, z) {
    return qvm.call(vm.gapiPhysSetLinearVelocity, qvm.game, entityID, x, y, z);
}

phys.getAngularVelocity = function (entityID, xyz) {
    return qvm.call(vm.gapiPhysGetAngularVelocity, qvm.game, entityID, xyz);
}

phys.setAngularVelocity = function (entityID, x, y, z) {
    return qvm.call(vm.gapiPhysSetAngularVelocity, qvm.game, entityID, x, y, z);
}

phys.applyCentralForce = function (entityID, fx, fy, fz) {
    return qvm.call(vm.gapiPhysApplyCentralForce, qvm.game, entityID, fx, fy, fz);
}

phys.freezeObject = function (entityID) {
    return qvm.call(vm.gapiPhysFreezeObject, qvm.game, entityID);
}

phys.unfreezeObject = function (entityID) {
    return qvm.call(vm.gapiPhysUnfreezeObject, qvm.game, entityID);
}

// welds two entities together at a local point (px, py, pz)
phys.weld = function (entA, entB, px, py, pz) {
    return qvm.call(vm.gapiPhysWeld, qvm.game, entA, entB, px, py, pz);
}

// 1 if the physics object is sleeping (at rest), 0 otherwise
phys.isSleeping = function (entityID) {
    return qvm.call(vm.gapiPhysIsSleeping, qvm.game, entityID);
}

spawnInfo.int = function (key, defaultString) {
    return qvm.call(vm.gapiSpawnInt, qvm.game, key, defaultString);
}

spawnInfo.float = function (key, defaultString) {
    return qvm.call(vm.gapiSpawnFloat, qvm.game, key, defaultString);
}

spawnInfo.string = function (key, defaultString) {
    return qvm.call(vm.gapiSpawnString, qvm.game, key, defaultString);
}

spawnInfo.vector = function (key, defaultString) {
    return qvm.call(vm.gapiSpawnVector, qvm.game, key, defaultString);
}

//
// direct struct apis — field order must match the API_FIELD order in
// qcommon/vm_javascript.c
//
// read: entity.field(id, "health") / write: entity.field(id, "health", 50)
// array and vector fields take the component index as the last argument:
// entity.ps(id, "origin", undefined, 0) reads ps.origin[0]
//

apiField = -1;
apiFields.level = {
    "num_entities": apiField += 1,
    "maxclients": apiField += 1,
    "framenum": apiField += 1,
    "time": apiField += 1,
    "previousTime": apiField += 1,
    "startTime": apiField += 1,
    "restarted": apiField += 1,
};

apiField = -1;
apiFields.gameEntity = {
    "inuse": apiField += 1,
    "classname": apiField += 1,
    "model": apiField += 1,
    "model2": apiField += 1,
    "physicsObject": apiField += 1,
    "physicsBounce": apiField += 1,
    "health": apiField += 1,
    "takedamage": apiField += 1,
    "count": apiField += 1,
    "waterlevel": apiField += 1,
    "item": apiField += 1, // item id or -1, read only
    "swep_list": apiField += 1, // [weaponID] WS.NONE / WS.HAVE / WS.NOAMMO
    "swep_ammo": apiField += 1, // [weaponID], 9999 = infinite
    "flashlight": apiField += 1,
    "owner": apiField += 1, // entity id or -1
    "grabbedEntity": apiField += 1, // entity id or -1
    "isGrabbed": apiField += 1,
    "lastPlayer": apiField += 1, // entity id or -1
    "sb_phys": apiField += 1, // PHYS_STATIC 1 / PHYS_DYNAMIC 2 / PHYS_SLEEPING 3
};

apiField = -1;
apiFields.entityState = {
    "number": apiField += 1, // read only
    "eType": apiField += 1,
    "eFlags": apiField += 1,

    "pos.trType": apiField += 1,
    "pos.trTime": apiField += 1,
    "pos.trDuration": apiField += 1,
    "pos.trBase": apiField += 1, // [xyz]
    "pos.trDelta": apiField += 1, // [xyz]

    "apos.trType": apiField += 1,
    "apos.trTime": apiField += 1,
    "apos.trDuration": apiField += 1,
    "apos.trBase": apiField += 1, // [xyz]
    "apos.trDelta": apiField += 1, // [xyz]

    "time": apiField += 1,
    "time2": apiField += 1,

    "origin": apiField += 1, // [xyz]
    "origin2": apiField += 1, // [xyz]

    "angles": apiField += 1, // [xyz]
    "angles2": apiField += 1, // [xyz]

    "otherEntityNum": apiField += 1,
    "otherEntityNum2": apiField += 1,

    "groundEntityNum": apiField += 1, // -1 = in air

    "constantLight": apiField += 1,
    "loopSound": apiField += 1,

    "modelindex": apiField += 1,
    "modelindex2": apiField += 1,
    "clientNum": apiField += 1,
    "frame": apiField += 1,

    "solid": apiField += 1,

    "event": apiField += 1,
    "eventParm": apiField += 1,

    "weapon": apiField += 1,
    "legsAnim": apiField += 1,
    "torsoAnim": apiField += 1,

    "generic1": apiField += 1,
};

apiField = -1;
apiFields.entityShared = {
    "linked": apiField += 1, // read only
    "linkcount": apiField += 1, // read only
    "svFlags": apiField += 1, // SVF_NOCLIENT / SVF_BROADCAST / ...
    "singleClient": apiField += 1,
    "bmodel": apiField += 1, // read only
    "mins": apiField += 1, // [xyz]
    "maxs": apiField += 1, // [xyz]
    "contents": apiField += 1,
    "absmin": apiField += 1, // [xyz]
    "absmax": apiField += 1, // [xyz]
    "currentOrigin": apiField += 1, // [xyz]
    "currentAngles": apiField += 1, // [xyz]
    "ownerNum": apiField += 1,
};

apiField = -1;
apiFields.gameClient = {
    "noclip": apiField += 1,
    "buttons": apiField += 1,
    "respawnTime": apiField += 1,
};

apiField = -1;
apiFields.clientPersistant = {
    "connected": apiField += 1, // CON.DISCONNECTED / CON.CONNECTING / CON.CONNECTED
    "localClient": apiField += 1,
    "netname": apiField += 1,
};

apiField = -1;
apiFields.clientCommand = {
    "serverTime": apiField += 1,
    "angles": apiField += 1, // [2]
    "buttons": apiField += 1, // BUTTON_ATTACK 1 / BUTTON_USE 2 / BUTTON_WALKING 4
    "weapon": apiField += 1,
    "forwardmove": apiField += 1,
    "rightmove": apiField += 1,
    "upmove": apiField += 1,
};

apiField = -1;
apiFields.clientState = {
    "commandTime": apiField += 1,
    "pm_type": apiField += 1, // PM_NORMAL 0 / PM_NOCLIP 1 / PM_DEAD 2 / PM_FREEZE 3
    "bobCycle": apiField += 1,
    "pm_flags": apiField += 1,
    "pm_time": apiField += 1,

    "origin": apiField += 1, // [xyz]
    "velocity": apiField += 1, // [xyz]
    "weaponTime": apiField += 1,
    "gravity": apiField += 1,
    "speed": apiField += 1,
    "delta_angles": apiField += 1, // [2]

    "groundEntityNum": apiField += 1, // -1 = in air

    "legsTimer": apiField += 1,
    "legsAnim": apiField += 1,

    "torsoTimer": apiField += 1,
    "torsoAnim": apiField += 1,

    "movementDir": apiField += 1,

    "eFlags": apiField += 1,

    "eventSequence": apiField += 1,
    "events": apiField += 1, // [2]
    "eventParms": apiField += 1, // [2]

    "externalEvent": apiField += 1,
    "externalEventParm": apiField += 1,
    "externalEventTime": apiField += 1,

    "clientNum": apiField += 1,
    "weapon": apiField += 1,
    "weaponstate": apiField += 1,

    "viewangles": apiField += 1, // [xyz]
    "viewheight": apiField += 1,

    "stats": apiField += 1, // [STAT.HEALTH, STAT.ARMOR, STAT.AMMO, ...]
    "persistant": apiField += 1, // [16]

    "generic1": apiField += 1,
    "loopSound": apiField += 1,

    "ping": apiField += 1,
    "pmove_framecount": apiField += 1,
    "entityEventSequence": apiField += 1,
};

// level_locals_t: read level.field("time") / write level.field("time", 1000)
level.field = function (fieldName, value) {
    var fieldID = apiFields.level[fieldName];
    if (fieldID === undefined) {
        console.log("#ff5level.field: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiLevel, qvm.game, fieldID, value);
}

// gentity_s fields
entity.field = function (entityID, fieldName, value, i1) {
    var fieldID = apiFields.gameEntity[fieldName];
    if (fieldID === undefined) {
        console.log("#ff5entity.field: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiGameEntity, qvm.game, entityID, fieldID, value, i1);
}

// entityState_t (entity->s) fields
entity.state = function (entityID, fieldName, value, i1) {
    var fieldID = apiFields.entityState[fieldName];
    if (fieldID === undefined) {
        console.log("#ff5entity.state: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiEntityState, qvm.game, entityID, fieldID, value, i1);
}

// entityShared_t (entity->r) fields
entity.shared = function (entityID, fieldName, value, i1) {
    var fieldID = apiFields.entityShared[fieldName];
    if (fieldID === undefined) {
        console.log("#ff5entity.shared: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiEntityShared, qvm.game, entityID, fieldID, value, i1);
}

// gclient_s fields, players and npcs only
entity.client = function (entityID, fieldName, value) {
    var fieldID = apiFields.gameClient[fieldName];
    if (fieldID === undefined) {
        console.log("#ff5entity.client: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiGameClient, qvm.game, entityID, fieldID, value);
}

// clientPersistant_t (client->pers) fields
entity.pers = function (entityID, fieldName, value) {
    var fieldID = apiFields.clientPersistant[fieldName];
    if (fieldID === undefined) {
        console.log("#ff5entity.pers: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiClientPersistant, qvm.game, entityID, fieldID, value);
}

// usercmd_t (client->pers.cmd) fields
entity.cmd = function (entityID, fieldName, value, i1) {
    var fieldID = apiFields.clientCommand[fieldName];
    if (fieldID === undefined) {
        console.log("#ff5entity.cmd: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiClientCommand, qvm.game, entityID, fieldID, value, i1);
}

// playerState_t (client->ps) fields
entity.ps = function (entityID, fieldName, value, i1) {
    var fieldID = apiFields.clientState[fieldName];
    if (fieldID === undefined) {
        console.log("#ff5entity.ps: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiClientState, qvm.game, entityID, fieldID, value, i1);
}
