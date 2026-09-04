const tool = {};
tool.list = [];
const ctx = {};

tool.register = function(nameid, name, initfunc, callfunc) {
    var id = tool.list.length;
    if(!tool.list[id]) tool.list[id] = {};
    tool.list[id].nameid = nameid;
    tool.list[id].name = name;
    tool.list[id].initfunc = initfunc;
    tool.list[id].callfunc = callfunc;
}

tool.get = function(id) {
    if(tool.list[id] && tool.list[id].nameid) return id;
    return -1;
}

tool.getNameID = function(id) {
    if(tool.list[id] && tool.list[id].nameid) return tool.list[id].nameid;
    return null;
}

tool.getName = function(id) {
    if(tool.list[id] && tool.list[id].name) return tool.list[id].name;
    return null;
}

tool.init = function(toolID) {
    if(tool.list[toolID] && tool.list[toolID].nameid) tool.list[toolID].initfunc(toolID);
}

tool.call = function(toolID, targetID, playerID, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) {
    console.log("tool.call");
    if(tool.list[toolID] && tool.list[toolID].callfunc) tool.list[toolID].callfunc(toolID, targetID, playerID, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

openjs.folder("js/tools", "tools");