function NoireMapBrowser_Init(appID) {
    var id = ui.window(-1, app.list[appID].nameid, app.list[appID].name, app.list[appID].icon, 0, 788, 532, color.white, color.windowHeader, color.window);
    if (!ctx[id]) ctx[id] = {};

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

    ctx[id].tabStartID = 50;
    ctx[id].tabEndID = 50;

    NoireMapBrowser_UpdateTab(id);
}

function NoireMapBrowser_UpdateTab(id) {
    ctx[id].mapList = ui.listFiles(id, ctx[id].tabEndID, 10, 10, 128, 128, 0.65, 6, 4, LSTYLE.GRID, LCONTENT.SHADER, 0);
    ui.setMargin(id, ctx[id].mapList, 15, 8, 15, 22);
    ui.fillListFiles(id, "maps/", ".bsp", "levelshots/", 0);
}

function NoireMapBrowser_Key(key, id) {
}

function NoireMapBrowser_Call(id, eid, key) {
    if (key != KEY.MOUSE1 && key != KEY.MOUSE2) return;
    if (eid == ctx[id].mapList) {
        qvm.cmd(qvm.ui, EXEC.APPEND, "map " + api.element(id, ctx[id].mapList, "field"));
    }
}

function NoireMapBrowser_Update(id) {
    if (api.window(id, "minimized")) api.element(id, ctx[id].minimize, "colorBackground", color.accent3);
    else api.element(id, ctx[id].minimize, "colorBackground", color.windowButton);
    if (api.window(id, "pinned")) api.element(id, ctx[id].pin, "colorBackground", color.accent3);
    else api.element(id, ctx[id].pin, "colorBackground", color.windowButton);
    if (api.window(id, "linked")) api.element(id, ctx[id].link, "colorBackground", color.accent3);
    else api.element(id, ctx[id].link, "colorBackground", color.windowButton);
    if (api.window(id, "keyboardCapture")) api.element(id, ctx[id].keyboard, "colorBackground", color.accent3);
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
    if (api.window(id, "minimized")) api.window(id, "minimized", 0);
    else api.window(id, "minimized", 1);
}

function NoireMapBrowser_Pin(id) {
    if (api.window(id, "pinned")) api.window(id, "pinned", 0);
    else api.window(id, "pinned", 1);
}

function NoireMapBrowser_Link(id) {
    if (api.window(id, "linked")) {
        api.window(id, "linked", 0);
    } else {
        ui.teleportWindowToPlayer(id, 128);
        api.window(id, "linked", 1);
    }
}

function NoireMapBrowser_Keyboard(id) {
    if (api.window(id, "keyboardCapture")) api.window(id, "keyboardCapture", 0);
    else api.window(id, "keyboardCapture", 1);
}

app.register("noire.mapbrowser", "Maps", NoireMapBrowser_Init, NoireMapBrowser_Key, NoireMapBrowser_Call, NoireMapBrowser_Update, NoireMapBrowser_BackgroundUpdate, NoireMapBrowser_Shutdown);