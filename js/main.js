JSCall = function(func_id) {
    var args = Array.prototype.slice.call(arguments, 1);
    switch(func_id) {
        case js.shellInit: return Shell_Init();
        case js.shellDraw: return Shell_Draw();
        case js.shellKey: return Shell_Key(args[0], args[1]);
        case js.shellCallback: return Shell_Callback(args[0], args[1], args[2]);
        case js.shellUpdate: return Shell_Update(args[0]);
        case js.shellBackgroundUpdate: return Shell_BackgroundUpdate(args[0]);
        case js.shellShutdown: return Shell_Shutdown(args[0]);
        case js.getApp: return app.get(args[0]);
        case js.getAppName: return app.getName(args[0]);
        case js.getAppIcon: return app.getIcon(args[0]);
        case js.launchApp: return app.launch(app.getByNameID(args[0]));
        case js.HUDInit: return Hud_Init();
        case js.HUDDraw: return Hud_Draw();
        case js.getTool: return tool.get(args[0]);
        case js.getToolNameID: return tool.getNameID(args[0]);
        case js.getToolName: return tool.getName(args[0]);
        case js.initTool: return tool.init(args[0]);
        case js.callTool: return tool.call(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11]);
        case js.gameRunFrame: return Game_RunFrame(args[0]);
    }
};
