const entity = {};
const player = {};

entity.delete = function (entityID) {
    return qvm.call(vm.gapiEntityDelete, qvm.game, entityID);
}

entity.spawn = function (x, y, z, modelName) {
    return qvm.call(vm.gapiEntitySpawn, qvm.game, x, y, z, modelName);
}

player.crosshairPoint = function (player, distance, xyz) {
    return qvm.call(vm.gapiPlayerCrosshairPoint, qvm.game, player, distance, xyz);
}