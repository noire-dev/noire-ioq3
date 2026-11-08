const entity = {};
const player = {};
const npc = {};
const spawnInfo = {};

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

// spawns an npc (bot), returns its entity number
// npc.spawn("Grunt", "sarge", 4, x, y, z, yaw) — skill 1..5, model is a bot model name
// npcs spawned here are removed automatically when they die
npc.spawn = function (npcName, npcModel, skill, x, y, z, yaw) {
    return qvm.call(vm.gapiNPCSpawn, qvm.game, npcName, npcModel, skill, x, y, z, yaw);
}

player.crosshairPoint = function (player, distance, xyz) {
    return qvm.call(vm.gapiPlayerCrosshairPoint, qvm.game, player, distance, xyz);
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
