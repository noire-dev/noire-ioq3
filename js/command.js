const command = {};
command.data = {};

command.register = function (name, func) {
    if (!command.data[name]) command.data[name] = func;
}

command.run = function (args) {
    if (typeof command.data[args[1]] === 'function')
        return command.data[args[1]](args);

    return -1;
}

openjs.folder("js/commands", "commands");