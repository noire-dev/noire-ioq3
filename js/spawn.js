const spawn = {};
spawn.data = {};

spawn.register = function (classname, func) {
    if (!spawn.data[classname]) spawn.data[classname] = func;
}

spawn.use = function (classname) {
    if (typeof spawn.data[classname] === 'function')
        return spawn.data[classname]();

    return -1;
}

openjs.folder("js/spawns", "spawn");