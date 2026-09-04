function NoireModManager_Init(appID) {
    var id = ui.window(-1, app.list[appID].nameid, app.list[appID].name, app.list[appID].icon, 0, 800, (29*16)+17, color.white, color.windowHeader, color.window);
    if(!ctx[id]) ctx[id] = {};
    
    ctx[id].self = id;
    ctx[id].close = ui.windowButton(id, -1, 0, "✕", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].close, NoireModManager_Exit);
    ctx[id].minimize = ui.windowButton(id, -1, 1, "−", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].minimize, NoireModManager_Minimize);
    ctx[id].pin = ui.windowButton(id, -1, 2, "⚲", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].pin, NoireModManager_Pin);
    ctx[id].link = ui.windowButton(id, -1, 3, "⌖", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].link, NoireModManager_Link);
    ctx[id].keyboard = ui.windowButton(id, -1, 4, "⌨", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].keyboard, NoireModManager_Keyboard);
    
    ctx[id].currentTab = -1;
    ctx[id].scrollTab = 0;
    
    ctx[id].sidePanel = ui.button(id, -1, 0, 0, 224, api.window(id, "baseH"), "", UI.NO_TOP_LEFT|UI.NO_TOP_RIGHT|UI.NO_BOTTOM_RIGHT, color.windowSide, 1.00);
    api.element(id, ctx[id].sidePanel, "baseCorner", cvar.int("shell.window.corner"));
    
    ctx[id].addonStartID = 5;
    ctx[id].addonEndID = 5;
    
    NoireModManager_UpdateTab(id);
    
    ctx[id].tabStartID = 50;
    ctx[id].tabEndID = 50;
    ctx[id].pageX = 224;
    ctx[id].pageW = 800-224;
    
    for (var j = 0; j < ctx[id].tabs.length; j++) {
        if(!cvar.int("addon.enabled." + addon.getByNameID(api.element(id, ctx[id].tabs[j].button, "field")))) api.element(id, ctx[id].tabs[j].button, "style", UI.BOLD|UI.STRIKETHROUGH);
    }
    
    NoireModManager_Call(id, ctx[id].tabs[0].button, KEY.MOUSE1);
}

function NoireModManager_Key(key, id) {
    if(ui.cursorInWindow(id, 0, 0, 224, 500)) {
        if(key == KEY.MWHEELUP) {
            ctx[id].scrollTab -= 1;
            if(ctx[id].scrollTab < 0) ctx[id].scrollTab = 0;
            NoireModManager_UpdateTab(id);
        }
        if(key == KEY.MWHEELDOWN) {
            ctx[id].scrollTab += 1;
            if(ctx[id].scrollTab > addon.list.length-16) ctx[id].scrollTab = addon.list.length-16;
            if(addon.list.length < 16) ctx[id].scrollTab = 0;
            NoireModManager_UpdateTab(id);
        }
    }
}

function NoireModManager_Call(id, eid, key) {
    if(key != KEY.MOUSE1 && key != KEY.MOUSE2) return;
    for (var i = ctx[id].scrollTab; i < ctx[id].tabs.length; i++) {
        if (eid == ctx[id].tabs[i].button) {
            ctx[id].currentTab = i;
            NoireModManager_UpdateTab(id);
            
            ctx[id].elementHeight = 32;
            ctx[id].elementSpace = 5;
            ctx[id].elementFontScale = 0.80;
            ui.clearWindow(id, ctx[id].tabStartID, ctx[id].tabEndID);
            addon.list[addon.getByNameID(api.element(id, ctx[id].tabs[i].button, "field"))].configfunc(id, ctx[id].pageX, ctx[id].pageW, addon.getByNameID(api.element(id, ctx[id].tabs[i].button, "field")));
        }
    }
    
    for(var i = ctx[id].tabStartID; i <= ctx[id].tabEndID; i++) {
        api.element(id, i, "baseCorner", 4);
        api.element(id, i, "hoverStyle", UI.ACCENT);
    }
    
    if(eid == ctx[id].enabled) {
        addon.buildlist();
        NoireModManager_Call(id, ctx[id].tabs[ctx[id].currentTab].button, KEY.MOUSE1);
    }
}

function NoireModManager_Update(id) {
    if(api.window(id, "minimized")) api.element(id, ctx[id].minimize, "colorBackground", color.accent3);
    else api.element(id, ctx[id].minimize, "colorBackground", color.windowButton);
    if(api.window(id, "pinned")) api.element(id, ctx[id].pin, "colorBackground", color.accent3);
    else api.element(id, ctx[id].pin, "colorBackground", color.windowButton);
    if(api.window(id, "linked")) api.element(id, ctx[id].link, "colorBackground", color.accent3);
    else api.element(id, ctx[id].link, "colorBackground", color.windowButton);
    if(api.window(id, "keyboardCapture")) api.element(id, ctx[id].keyboard, "colorBackground", color.accent3);
    else api.element(id, ctx[id].keyboard, "colorBackground", color.windowButton);
}

function NoireModManager_UpdateTab(id) {
    ctx[id].tabs = [];
    ui.clearWindow(id, ctx[id].addonStartID, ctx[id].addonEndID);

    var currentItem = 0;
    for (var i = ctx[id].scrollTab; i < 16+ctx[id].scrollTab && i < addon.list.length; i++) {
        if(!ctx[id].tabs[i]) ctx[id].tabs[i] = {};
        ctx[id].tabs[i].button = ui.button(id, ctx[id].addonEndID + currentItem, 10, 10 + (currentItem * 29), 204, 24, addon.list[i].name, UI.BOLD, color.windowItem, 0.70);
        api.element(id, ctx[id].tabs[i].button, "field", addon.list[i].nameid);
        api.element(id, ctx[id].tabs[i].button, "baseCorner", 4);
        api.element(id, ctx[id].tabs[i].button, "hoverStyle", UI.ACCENT);
        currentItem += 1;
    }
    for (var i = ctx[id].scrollTab; i < ctx[id].tabs.length; i++) {
        if(!cvar.int("addon.enabled." + addon.getByNameID(api.element(id, ctx[id].tabs[i].button, "field")))) api.element(id, ctx[id].tabs[i].button, "style", UI.BOLD|UI.STRIKETHROUGH);
        if(!cvar.int("addon.enabled." + addon.getByNameID(api.element(id, ctx[id].tabs[i].button, "field")))) api.element(id, ctx[id].tabs[i].button, "colorBackground", color.windowItemDisabled);
        if(i == ctx[id].currentTab) {
            api.element(id, ctx[id].tabs[i].button, "colorBackground", color.enabled);
            api.element(id, ctx[id].tabs[i].button, "hoverStyle", 0);
        }
    }
}

function NoireModManager_BackgroundUpdate(id) {
}

function NoireModManager_Shutdown(id) {
}

function NoireModManager_Exit(id) {
    NoireModManager_Shutdown(id);
    ui.closeWindow(ctx[id].self);
    delete ctx[id];
}

function NoireModManager_Minimize(id) {
    if(api.window(id, "minimized")) api.window(id, "minimized", 0);
    else api.window(id, "minimized", 1);
}

function NoireModManager_Pin(id) {
    if(api.window(id, "pinned")) api.window(id, "pinned", 0);
    else api.window(id, "pinned", 1);
}

function NoireModManager_Link(id) {
    if(api.window(id, "linked")) {
        api.window(id, "linked", 0);
    } else {
        ui.teleportWindowToPlayer(id, 128);
        api.window(id, "linked", 1);
    }
}

function NoireModManager_Keyboard(id) {
    if(api.window(id, "keyboardCapture")) api.window(id, "keyboardCapture", 0);
    else api.window(id, "keyboardCapture", 1);
}

app.register("noiredev.modmanager", "Mod Manager", NoireModManager_Init, NoireModManager_Key, NoireModManager_Call, NoireModManager_Update, NoireModManager_BackgroundUpdate, NoireModManager_Shutdown);