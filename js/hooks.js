// Hook system (works like command.js, but one hook name can hold many
// functions and every event calls all of them in registration order)
//
// Events fired by the game module:
//   hook.register("frame",       function (frameTime, frameNum) {})
//   hook.register("playerDeath", function (victim, attacker, method) {})
//   hook.register("itemPickup",  function (player, itemID) {})
//   hook.register("weaponFire",  function (player, weaponID) {})
//   hook.register("damage",      function (victim, attacker, method) {})
//
// All entity/player/npc arguments are entity numbers (g_entities index).
// attacker == -1 means the world (falling, lava, crush, trigger hurt...).
// method is a meansOfDeath id (see MOD in js/game.js, MOD.WEAPONS + weaponID
// for weapon kills).

const hook = {};
hook.data = {};

hook.register = function (name, func) {
    if (typeof func !== 'function') {
        console.log("#ff5hook.register: '" + name + "' needs a function");
        return;
    }
    if (!hook.data[name]) hook.data[name] = [];
    if (hook.data[name].indexOf(func) === -1) hook.data[name].push(func);
}

hook.remove = function (name, func) {
    var list = hook.data[name];
    if (!list) return;
    var index = list.indexOf(func);
    if (index !== -1) list.splice(index, 1);
}

hook.count = function (name) {
    return hook.data[name] ? hook.data[name].length : 0;
}

// args[0] is the hook name, the rest are the event arguments
hook.run = function (args) {
    var funcs = hook.data[args[0]];
    if (!funcs) return -1;

    var eventArgs = args.slice(1);
    for (var i = 0; i < funcs.length; i++) {
        try {
            funcs[i].apply(null, eventArgs);
        } catch (error) {
            console.log("#ff5hook '" + args[0] + "' handler " + i + " error: " + error);
        }
    }

    return 0;
}
