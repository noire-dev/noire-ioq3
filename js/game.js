const entity = {};
const player = {};
const spawnInfo = {};

entity.delete = function (entityID) {
    return qvm.call(vm.gapiEntityDelete, qvm.game, entityID);
}

entity.spawn = function (x, y, z, ax, ay, az, modelName) {
    return qvm.call(vm.gapiEntitySpawn, qvm.game, x, y, z, ax, ay, az, modelName);
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