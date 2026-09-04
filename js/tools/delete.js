function GameTool_Delete_Init(toolID) {
    //Tool ID
    cvar.set("toolgun_tool", toolID);

    //Tool text
    cvar.set("toolgun_tooltext", "Delete");
    cvar.set("toolgun_tooltip1", "Deletes an object");
    cvar.set("toolgun_tooltip2", " ");
    cvar.set("toolgun_tooltip3", " ");
    cvar.set("toolgun_tooltip4", " ");

    //Tool mode name
    cvar.set("toolgun_toolmode1", " ");
    cvar.set("toolgun_toolmode2", " ");
    cvar.set("toolgun_toolmode3", " ");
    cvar.set("toolgun_toolmode4", " ");

    //Tool properties name
    cvar.set("toolgun_toolset1", "--------:");
    cvar.set("toolgun_toolset2", "--------:");
    cvar.set("toolgun_toolset3", "--------:");
    cvar.set("toolgun_toolset4", "--------:");

    //Tool default setting
    cvar.set("uis_tg1", "0");
    cvar.set("uis_tg2", "0");
    cvar.set("uis_tg3", "0");
    cvar.set("uis_tg4", "0");

    //Tool modes toggle
    cvar.set("uitoolmode", "toggle toolgun_mod5 0");

    console.log("tool - " + tool.list[toolID].name + " initialized");
}

function GameTool_Delete_Call(toolID, targetID, playerID, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) {
    entity.delete(targetID);
}

tool.register("noiredev.delete", "Delete", GameTool_Delete_Init, GameTool_Delete_Call);