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

// read: entity.stat(id, STAT.HEALTH) / write: entity.stat(id, STAT.HEALTH, 100)
entity.stat = function (entityID, statID, value) {
    return qvm.call(vm.gapiEntityStat, qvm.game, entityID, statID, value);
}

// read: entity.speed(id) / write: entity.speed(id, 600) — 0 = default g_speed,
// the override is cleared on respawn
entity.speed = function (entityID, value) {
    return qvm.call(vm.gapiEntitySpeed, qvm.game, entityID, value);
}

// inventory (swep_ammo): read entity.ammo(id, weapon.weaponID) / write with value
// ammo 9999 is infinite
entity.ammo = function (entityID, weaponID, value) {
    return qvm.call(vm.gapiEntityAmmo, qvm.game, entityID, weaponID, value);
}

// inventory (swep_list): read entity.weapon(id, weapon.weaponID) / write with value
// WS.NONE 0 / WS.HAVE 1 / WS.NOAMMO 2
entity.weapon = function (entityID, weaponID, value) {
    return qvm.call(vm.gapiEntityWeapon, qvm.game, entityID, weaponID, value);
}

// currently selected weapon: read entity.activeWeapon(id) / write with value
entity.activeWeapon = function (entityID, value) {
    return qvm.call(vm.gapiEntityActiveWeapon, qvm.game, entityID, value);
}

// owned weapons as a string "weaponID:ammo,weaponID:ammo,..." (one call)
entity.inventory = function (entityID) {
    return qvm.call(vm.gapiEntityInventory, qvm.game, entityID);
}

// spawns an npc (bot), returns its entity number
// npc.spawn("Grunt", "sarge", 4, x, y, z, yaw) — skill 1..5, model is a bot model name
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
