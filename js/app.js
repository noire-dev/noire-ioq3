const app = {};
app.list = [];
const ctx = {};

app.register = function(nameid, name, initfunc, keyfunc, callfunc, updatefunc, bgupdatefunc, shutdownfunc) {
    var id = app.list.length;
    if(!app.list[id]) app.list[id] = {};
    app.list[id].nameid = nameid;
    app.list[id].name = name;
    app.list[id].icon = "js/apps/" + nameid + "/appicon";
    app.list[id].initfunc = initfunc;
    app.list[id].keyfunc = keyfunc;
    app.list[id].callfunc = callfunc;
    app.list[id].updatefunc = updatefunc;
    app.list[id].bgupdatefunc = bgupdatefunc;
    app.list[id].shutdownfunc = shutdownfunc;
}

app.get = function(id) {
    if(app.list[id] && app.list[id].nameid) return id;
    return -1;
}

app.getByNameID = function(nameID) {
    for (var i = 0; i < app.list.length; i++) {
        if (app.list[i] && app.list[i].nameid === nameID) return i;
    }
    
    return -1;
}

app.getName = function(id) {
    if(app.list[id] && app.list[id].name) return app.list[id].name;
    return null;
}

app.getIcon = function(id) {
    if(app.list[id] && app.list[id].icon) return app.list[id].icon;
    return null;
}

app.launch = function(appID) {
    if(app.list[appID] && app.list[appID].nameid) app.list[appID].initfunc(appID);
}

app.key = function(appID, key, id) {
    if(app.list[appID] && app.list[appID].keyfunc) app.list[appID].keyfunc(key, id);
}

app.call = function(appID, id, eid, key) {
    if(app.list[appID] && app.list[appID].callfunc) app.list[appID].callfunc(id, eid, key);
}

app.update = function(appID, id) {
    if(app.list[appID] && app.list[appID].updatefunc) app.list[appID].updatefunc(id);
}

app.bgupdate = function(appID, id) {
    if(app.list[appID] && app.list[appID].bgupdatefunc) app.list[appID].bgupdatefunc(id);
}

app.shutdown = function(appID, id) {
    if(app.list[appID] && app.list[appID].shutdownfunc) app.list[appID].shutdownfunc(id);
}

openjs.folder("js/apps", "apps");