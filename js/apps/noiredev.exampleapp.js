function ExampleApp_Init(appID) {
    var id = ui.window(-1, app.list[appID].nameid, app.list[appID].name, app.list[appID].icon, 0, 640, 480, color.white, color.windowHeader, color.window);
    if(!ctx[id]) ctx[id] = {};
    
    ctx[id].self = id;
    ctx[id].close = ui.windowButton(id, -1, 0, "✕", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].close, ExampleApp_Exit);
    ctx[id].minimize = ui.windowButton(id, -1, 1, "−", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].minimize, ExampleApp_Minimize);
    ctx[id].pin = ui.windowButton(id, -1, 2, "⚲", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].pin, ExampleApp_Pin);
    ctx[id].link = ui.windowButton(id, -1, 3, "⌖", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].link, ExampleApp_Link);
    ctx[id].keyboard = ui.windowButton(id, -1, 4, "⌨", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].keyboard, ExampleApp_Keyboard);
}

function ExampleApp_Key(key, id) {
}

function ExampleApp_Call(id, eid, key) {
}

function ExampleApp_Update(id) {
    if(api.window(id, "minimized")) api.element(id, ctx[id].minimize, "colorBackground", color.accent3);
    else api.element(id, ctx[id].minimize, "colorBackground", color.windowButton);
    if(api.window(id, "pinned")) api.element(id, ctx[id].pin, "colorBackground", color.accent3);
    else api.element(id, ctx[id].pin, "colorBackground", color.windowButton);
    if(api.window(id, "linked")) api.element(id, ctx[id].link, "colorBackground", color.accent3);
    else api.element(id, ctx[id].link, "colorBackground", color.windowButton);
    if(api.window(id, "keyboardCapture")) api.element(id, ctx[id].keyboard, "colorBackground", color.accent3);
    else api.element(id, ctx[id].keyboard, "colorBackground", color.windowButton);
}

function ExampleApp_BackgroundUpdate(id) {
}

function ExampleApp_Shutdown(id) {
}

function ExampleApp_Exit(id) {
    ExampleApp_Shutdown(id);
    ui.closeWindow(ctx[id].self);
    delete ctx[id];
}

function ExampleApp_Minimize(id) {
    if(api.window(id, "minimized")) api.window(id, "minimized", 0);
    else api.window(id, "minimized", 1);
}

function ExampleApp_Pin(id) {
    if(api.window(id, "pinned")) api.window(id, "pinned", 0);
    else api.window(id, "pinned", 1);
}

function ExampleApp_Link(id) {
    if(api.window(id, "linked")) {
        api.window(id, "linked", 0);
    } else {
        ui.teleportWindowToPlayer(id, 128);
        api.window(id, "linked", 1);
    }
}

function ExampleApp_Keyboard(id) {
    if(api.window(id, "keyboardCapture")) api.window(id, "keyboardCapture", 0);
    else api.window(id, "keyboardCapture", 1);
}

app.register("noiredev.exampleapp", "Example App", ExampleApp_Init, ExampleApp_Key, ExampleApp_Call, ExampleApp_Update, ExampleApp_BackgroundUpdate, ExampleApp_Shutdown);