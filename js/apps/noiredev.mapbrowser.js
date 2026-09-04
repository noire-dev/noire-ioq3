function NoireMapBrowser_Init(appID) {
    var id = ui.window(-1, app.list[appID].nameid, app.list[appID].name, app.list[appID].icon, 0, (128*5)+234+10, (128*4)+10, color.white, color.windowHeader, color.window);
    if(!ctx[id]) ctx[id] = {};
    
    ctx[id].self = id;
    ctx[id].close = ui.windowButton(id, -1, 0, "✕", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].close, NoireMapBrowser_Exit);
    ctx[id].minimize = ui.windowButton(id, -1, 1, "−", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].minimize, NoireMapBrowser_Minimize);
    ctx[id].pin = ui.windowButton(id, -1, 2, "⚲", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].pin, NoireMapBrowser_Pin);
    ctx[id].link = ui.windowButton(id, -1, 3, "⌖", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].link, NoireMapBrowser_Link);
    ctx[id].keyboard = ui.windowButton(id, -1, 4, "⌨", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].keyboard, NoireMapBrowser_Keyboard);
    
    ctx[id].currentTab = 0;
    
    ctx[id].sidePanel = ui.button(id, -1, 0, 0, 224, api.window(id, "baseH"), "", UI.NO_TOP_LEFT|UI.NO_TOP_RIGHT|UI.NO_BOTTOM_RIGHT, color.windowSide, 1.00);
    api.element(id, ctx[id].sidePanel, "baseCorner", cvar.int("shell.window.corner"));
 
    ctx[id].tabs = [
        { name: "Sandbox", button: null },
        { name: "Free for All", button: null },
        { name: "Team Deathmatch", button: null },
        { name: "Capture the Flag", button: null },
        { name: "One Flag Capture", button: null },
        { name: "Overload", button: null },
        { name: "Harvester", button: null },
    ];

    for (var i = 0; i < ctx[id].tabs.length; i++) {
        ctx[id].tabs[i].button = ui.button(id, -1, 10, 10 + i * 29, 204, 24, ctx[id].tabs[i].name, UI.BOLD, color.windowItem, 0.65);
        api.element(id, ctx[id].tabs[i].button, "baseCorner", 4);
    }

    ctx[id].allMaps = ui.checkbox(id, -1, 10, 10 + 16.5 * 29, 204, 24, "All maps", UI.BOLD, color.windowItem, 0.65, "noiredev.mapbrowser.showall");
    
    ctx[id].tabStartID = 50;
    ctx[id].tabEndID = 50;
    NoireMapBrowser_Call(id, ctx[id].tabs[0].button, KEY.MOUSE1);
}

function NoireMapBrowser_UpdateTab(id) {
    if(cvar.int("noiredev.mapbrowser.showall")) {
        ctx[id].mapList = ui.listFiles(id, ctx[id].tabEndID, 234, 0, 128, 128, 0.65, 5, 4, LSTYLE.GRID, LCONTENT.SHADER, 0);
        ui.setMargin(id, ctx[id].mapList, 10, 20, 10, 40);
        ui.fillListFiles(id, "maps/", ".bsp", "levelshots/", 0);
    } else {
        ctx[id].mapList = ui.list(id, ctx[id].tabEndID, 234, 0, 128, 128, 0.65, 5, 4, LSTYLE.GRID, LMODE.MAPS, ctx[id].currentTab);
        ui.setMargin(id, ctx[id].mapList, 10, 20, 10, 40);
    }
}

function NoireMapBrowser_Key(key, id) {
}

function NoireMapBrowser_Call(id, eid, key) {
    if(key != KEY.MOUSE1 && key != KEY.MOUSE2) return;
    for (var i = 0; i < ctx[id].tabs.length; i++) {
        if (eid == ctx[id].tabs[i].button) {
            for (var j = 0; j < ctx[id].tabs.length; j++) {
                api.element(id, ctx[id].tabs[j].button, "colorBackground", color.windowItem);
            }
            api.element(id, ctx[id].tabs[i].button, "colorBackground", color.enabled);
            api.window(id, "name", translation.get("Maps") + " - " + translation.get(ctx[id].tabs[i].name));
            
            ctx[id].currentTab = i;
            
            ui.clearWindow(id, ctx[id].tabStartID, ctx[id].tabEndID);
            ctx[id].needsToApply = false;
            NoireMapBrowser_UpdateTab(id);
        }
    }
    if(eid == ctx[id].mapList) {
        qvm.cmd(qvm.ui, EXEC.APPEND, "set g_gametype " + ctx[id].currentTab + ";");
        qvm.cmd(qvm.ui, EXEC.APPEND, "map " + api.element(id, ctx[id].mapList, "field"));
    }
    if(eid == ctx[id].allMaps) {
        NoireMapBrowser_UpdateTab(id);
    }
}

function NoireMapBrowser_Update(id) {
    if(api.window(id, "minimized")) api.element(id, ctx[id].minimize, "colorBackground", color.accent3);
    else api.element(id, ctx[id].minimize, "colorBackground", color.windowButton);
    if(api.window(id, "pinned")) api.element(id, ctx[id].pin, "colorBackground", color.accent3);
    else api.element(id, ctx[id].pin, "colorBackground", color.windowButton);
    if(api.window(id, "linked")) api.element(id, ctx[id].link, "colorBackground", color.accent3);
    else api.element(id, ctx[id].link, "colorBackground", color.windowButton);
    if(api.window(id, "keyboardCapture")) api.element(id, ctx[id].keyboard, "colorBackground", color.accent3);
    else api.element(id, ctx[id].keyboard, "colorBackground", color.windowButton);
}

function NoireMapBrowser_BackgroundUpdate(id) {
}

function NoireMapBrowser_Shutdown(id) {
}

function NoireMapBrowser_Exit(id) {
    NoireMapBrowser_Shutdown(id);
    ui.closeWindow(ctx[id].self);
    delete ctx[id];
}

function NoireMapBrowser_Minimize(id) {
    if(api.window(id, "minimized")) api.window(id, "minimized", 0);
    else api.window(id, "minimized", 1);
}

function NoireMapBrowser_Pin(id) {
    if(api.window(id, "pinned")) api.window(id, "pinned", 0);
    else api.window(id, "pinned", 1);
}

function NoireMapBrowser_Link(id) {
    if(api.window(id, "linked")) {
        api.window(id, "linked", 0);
    } else {
        ui.teleportWindowToPlayer(id, 128);
        api.window(id, "linked", 1);
    }
}

function NoireMapBrowser_Keyboard(id) {
    if(api.window(id, "keyboardCapture")) api.window(id, "keyboardCapture", 0);
    else api.window(id, "keyboardCapture", 1);
}

app.register("noiredev.mapbrowser", "Maps", NoireMapBrowser_Init, NoireMapBrowser_Key, NoireMapBrowser_Call, NoireMapBrowser_Update, NoireMapBrowser_BackgroundUpdate, NoireMapBrowser_Shutdown);