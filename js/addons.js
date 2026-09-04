const addon = {};
addon.list = [];

addon.buildlist = function() {
    for(var i = 0; i < limits.maxAddons; i++) cvar.set("addon.name." + i, "");
    for(var i = 0; i < addon.list.length; i++) {
        if(cvar.int("addon.enabled." + i)) cvar.set("addon.name." + i, addon.list[i].nameid);
    }
    cvar.set("addon.count", addon.list.length);
}

addon.register = function(addonName, addonNameID, configfunc) {
    var id = addon.list.length;
    if(id >= limits.maxAddons) return;
    if(!addon.list[id]) addon.list[id] = {};
    addon.list[id].name = addonName;
    addon.list[id].nameid = addonNameID;
    addon.list[id].configfunc = configfunc;
    cvar.register("addon.enabled." + id, "1", CVAR.ARCHIVE);
    addon.buildlist();
}

addon.getByNameID = function(nameID) {
    for (var i = 0; i < addon.list.length; i++) {
        if (addon.list[i] && addon.list[i].nameid === nameID) return i;
    }
    
    return -1;
}

openjs.folder("js/addons", "addons");