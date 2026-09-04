const cg = {};

cg.HUDCounter = function(x, y, value, text) {
    return qvm.call(vm.HUDCounter, qvm.cgame, x, y, value, text);
}

const NOTIFY = {
    NONE: 0,
    INFO: 1,
    UNDO: 2,
    ITEM: 3,
    KILL: 4,
    CUSTOM: 5,
}

cg.notify = function(text, type, number, picPath) {
    return qvm.call(vm.notify, qvm.cgame, text, type, number, picPath);
}