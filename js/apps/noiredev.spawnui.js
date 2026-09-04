cvar.register("noiredev.spawnui.toolgun.setting[1]", "", 0);
cvar.register("noiredev.spawnui.toolgun.setting[2]", "", 0);
cvar.register("noiredev.spawnui.toolgun.setting[3]", "", 0);
cvar.register("noiredev.spawnui.toolgun.setting[4]", "", 0);
cvar.register("noiredev.spawnui.prop.private", "0", 0);
cvar.register("noiredev.spawnui.prop.physics", "1", 0);
cvar.register("noiredev.spawnui.prop.grid", "25", 0);

function NoireSpawn_Init(appID) {
    var id = ui.window(-1, app.list[appID].nameid, app.list[appID].name, app.list[appID].icon, 0, 756 + 224, 532, color.white, color.windowHeader, color.window);
    if (!ctx[id]) ctx[id] = {};

    ctx[id].self = id;
    ctx[id].close = ui.windowButton(id, -1, 0, "✕", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].close, NoireSpawn_Exit);
    ctx[id].minimize = ui.windowButton(id, -1, 1, "−", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].minimize, NoireSpawn_Minimize);
    ctx[id].pin = ui.windowButton(id, -1, 2, "⚲", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].pin, NoireSpawn_Pin);
    ctx[id].link = ui.windowButton(id, -1, 3, "⌖", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].link, NoireSpawn_Link);
    ctx[id].keyboard = ui.windowButton(id, -1, 4, "⌨", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].keyboard, NoireSpawn_Keyboard);

    ctx[id].sidePanel = ui.button(id, -1, 0, 0, 224, 532, "", UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT | UI.NO_BOTTOM_RIGHT | UI.NO_BOTTOM_LEFT, color.windowSide, 1.00);
    ctx[id].sidePanel2 = ui.button(id, -1, 756, 0, 224, 532, "", UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT | UI.NO_BOTTOM_RIGHT | UI.NO_BOTTOM_LEFT, color.windowSide, 1.00);

    ctx[id].toolgunSetting1 = ui.field(id, -1, 756 + 10, 10 + 0 * 29, 204, 24, "", UI.BOLD, color.windowItem, 0.65, "noiredev.spawnui.toolgun.setting[1]");
    api.element(id, ctx[id].toolgunSetting1, "baseCorner", 4);
    ctx[id].toolgunSetting2 = ui.field(id, -1, 756 + 10, 10 + 1 * 29, 204, 24, "", UI.BOLD, color.windowItem, 0.65, "noiredev.spawnui.toolgun.setting[2]");
    api.element(id, ctx[id].toolgunSetting2, "baseCorner", 4);
    ctx[id].toolgunSetting3 = ui.field(id, -1, 756 + 10, 10 + 2 * 29, 204, 24, "", UI.BOLD, color.windowItem, 0.65, "noiredev.spawnui.toolgun.setting[3]");
    api.element(id, ctx[id].toolgunSetting3, "baseCorner", 4);
    ctx[id].toolgunSetting4 = ui.field(id, -1, 756 + 10, 10 + 3 * 29, 204, 24, "", UI.BOLD, color.windowItem, 0.65, "noiredev.spawnui.toolgun.setting[4]");
    api.element(id, ctx[id].toolgunSetting4, "baseCorner", 4);

    ctx[id].private = ui.checkbox(id, -1, 756 + 10, 10 + 5 * 29, 204, 24, "Private", UI.BOLD, color.windowItem, 0.65, "noiredev.spawnui.prop.private");
    api.element(id, ctx[id].private, "baseCorner", 4);
    ctx[id].physics = ui.checkbox(id, -1, 756 + 10, 10 + 6 * 29, 204, 24, "Physics", UI.BOLD, color.windowItem, 0.65, "noiredev.spawnui.prop.physics");
    api.element(id, ctx[id].physics, "baseCorner", 4);
    ctx[id].grid = ui.field(id, -1, 756 + 10, 10 + 7 * 29, 204, 24, "Grid", UI.BOLD, color.windowItem, 0.65, "noiredev.spawnui.prop.grid");
    api.element(id, ctx[id].grid, "baseCorner", 4);

    ctx[id].tabs = [
        { name: "Props", button: null },
        { name: "Entities", button: null },
        { name: "NPCs", button: null },
        { name: "Items", button: null },
        { name: "Spawnlists", button: null },
        { name: "Scripts", button: null },
        { name: "Tools", button: null },
        { name: "Saves", button: null },
        { name: "Music", button: null },
    ];

    for (var i = 0; i < ctx[id].tabs.length; i++) {
        ctx[id].tabs[i].button = ui.button(id, -1, 10, 10 + i * 29, 204, 24, ctx[id].tabs[i].name, UI.BOLD, color.windowItem, 0.80);
        api.element(id, ctx[id].tabs[i].button, "baseCorner", 4);
    }

    ctx[id].tabStartID = 50;
    ctx[id].tabEndID = 50;
    ctx[id].currentTab = 0;
    ctx[id].currentFolder = "";
    NoireSpawn_Call(id, ctx[id].tabs[0].button, KEY.MOUSE1);
}

function NoireSpawn_UpdateTab(id) {
    var y = 10;
    ctx[id].tabEndID = 50;
    ctx[id].elementHeight = 32;
    ctx[id].elementSpace = 5;
    ctx[id].elementFontScale = 0.80;

    if (ctx[id].tabs[ctx[id].currentTab].name === "Props") {
        if (ctx[id].currentFolder === "") {
            ctx[id].spawnList = ui.listFiles(id, ctx[id].tabEndID, 234, 10, 128, 128, 0.65, 4, 4, LSTYLE.GRID, LCONTENT.MODEL, 0);
            ui.setMargin(id, ctx[id].spawnList, 5, 5, 5, 5);
        } else {
            ctx[id].spawnList = ui.listFiles(id, ctx[id].tabEndID, 234, 10, 128, 128, 0.65, 4, 4, LSTYLE.GRID, LCONTENT.SHADER, 0);
            ui.setMargin(id, ctx[id].spawnList, 26, 20, 26, 32);
        }
        ui.fillListFiles(id, "props/", ".md3", "props/", 0);
        if (ctx[id].currentFolder === "") {
            ui.fillListFiles(id, "props/", ".md3", "props/", 0);
        } else {
            ui.fillListFiles(id, "mtr/" + ctx[id].currentFolder + "/", ".png", "mtr/" + ctx[id].currentFolder + "/", 0);
        }
    }
    if (ctx[id].tabs[ctx[id].currentTab].name === "Items") {
        ctx[id].spawnList = ui.list(id, ctx[id].tabEndID, 234, 10, 128, 128, 0.65, 4, 4, LSTYLE.GRID, LMODE.GAMEITEMS, 0);
        ui.setMargin(id, ctx[id].spawnList, 26, 20, 26, 32);
    }
    if (ctx[id].tabs[ctx[id].currentTab].name === "Spawnlists") {
        ctx[id].spawnList = ui.listFiles(id, ctx[id].tabEndID, 234, 10, 128, 128, 0.65, 4, 4, LSTYLE.GRID, LCONTENT.SHADER, 0);
        ui.setMargin(id, ctx[id].spawnList, 26, 20, 26, 32);
        if (ctx[id].currentFolder === "") {
            ui.fillListFiles(id, "js/spawnlists/", ".cfg", "js/spawnlists/icons/", 0);
        } else {
            ui.fillListFiles(id, "js/spawnlists/" + ctx[id].currentFolder + "/", ".js", "js/spawnlists/" + ctx[id].currentFolder + "/icons/", 0);
        }
    }
    if (ctx[id].tabs[ctx[id].currentTab].name === "Tools") {
        ctx[id].spawnList = ui.list(id, ctx[id].tabEndID, 234, 10, 512, 20, 0.65, 1, 25, LSTYLE.LIST, LMODE.TOOLS, 0);
    }
    if (ctx[id].tabs[ctx[id].currentTab].name === "Music") {
        ctx[id].spawnList = ui.listFiles(id, ctx[id].tabEndID, 234, 10, 512, 20, 0.65, 1, 25, LSTYLE.LIST, LCONTENT.NONE, 0);
        ui.fillListFiles(id, "music/", ".mp3", "", 0);
    }

    for (var i = ctx[id].tabStartID; i <= ctx[id].tabEndID; i++) {
        api.element(id, i, "baseCorner", 4);
        api.element(id, i, "hoverStyle", UI.ACCENT);
    }
}

function NoireSpawn_Key(key, id) {
    if (key == KEY.ESCAPE) {
        qvm.cmd(qvm.ui, EXEC.APPEND, "set toolgun_cmd tm " + api.element(id, ctx[id].toolgunSetting1, "field") + " " + api.element(id, ctx[id].toolgunSetting2, "field") + " " + api.element(id, ctx[id].toolgunSetting3, "field") + " " + api.element(id, ctx[id].toolgunSetting4, "field"));
    }
}

function NoireSpawn_Call(id, eid, key) {
    if (key != KEY.MOUSE1 && key != KEY.MOUSE2) return;
    for (var i = 0; i < ctx[id].tabs.length; i++) {
        if (eid == ctx[id].tabs[i].button) {
            for (var j = 0; j < ctx[id].tabs.length; j++) {
                api.element(id, ctx[id].tabs[j].button, "colorBackground", color.windowItem);
            }
            api.element(id, ctx[id].tabs[i].button, "colorBackground", color.enabled);
            api.window(id, "name", translation.get("SpawnUI") + " - " + translation.get(ctx[id].tabs[i].name));

            ctx[id].currentTab = i;
            ctx[id].currentFolder = "";

            ui.clearWindow(id, ctx[id].tabStartID, ctx[id].tabEndID);
            ctx[id].needsToApply = false;
            NoireSpawn_UpdateTab(id);
        }
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Props" && eid == ctx[id].spawnList) {
        if (ctx[id].currentFolder === "") {
            ui.fillListFiles(id, "mtr/" + api.element(id, ctx[id].spawnList, "field"), ".png", "", 1);
            if (ui.getFileListCount(id, 1) > 0) {
                ctx[id].currentFolder = api.element(id, ctx[id].spawnList, "field");
                NoireSpawn_UpdateTab(id);
                return;
            }
            cvar.set("spawn_cmd", "sl prop null null " + api.element(id, ctx[id].private, "value") + " " + (api.element(id, ctx[id].physics, "value") + 1) + " " + api.element(id, ctx[id].grid, "field") + " null " + "props/" + api.element(id, ctx[id].spawnList, "field") + " " + 0);
            qvm.cmd(qvm.ui, EXEC.APPEND, "vstr spawn_cmd");
        } else {
            cvar.set("spawn_cmd", "sl prop null null " + api.element(id, ctx[id].private, "value") + " " + (api.element(id, ctx[id].physics, "value") + 1) + " " + api.element(id, ctx[id].grid, "field") + " null " + "props/" + ctx[id].currentFolder + " " + api.element(id, ctx[id].spawnList, "field"));
            qvm.cmd(qvm.ui, EXEC.APPEND, "vstr spawn_cmd");
        }
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Items" && eid == ctx[id].spawnList) {
        qvm.cmd(qvm.ui, EXEC.APPEND, "give " + api.element(id, ctx[id].spawnList, "field"));
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Spawnlists" && eid == ctx[id].spawnList) {
        if (ctx[id].currentFolder === "") {
            ctx[id].currentFolder = api.element(id, ctx[id].spawnList, "field");
            NoireSpawn_UpdateTab(id);
        } else {
            qvm.cmd(qvm.ui, EXEC.APPEND, "set spawn_cmd js.open js/spawnlists/" + ctx[id].currentFolder + "/" + api.element(id, ctx[id].spawnList, "field") + ".js" + ";");
            qvm.cmd(qvm.ui, EXEC.APPEND, "vstr spawn_cmd");
        }
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Tools" && eid == ctx[id].spawnList) {
        tool.list[api.element(id, ctx[id].spawnList, "value")].initfunc(api.element(id, ctx[id].spawnList, "value"));
        qvm.cmd(qvm.ui, EXEC.APPEND, "weapon 26");
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Music" && eid == ctx[id].spawnList) {
        qvm.cmd(qvm.ui, EXEC.APPEND, "music \"music/" + api.element(id, ctx[id].spawnList, "field") + "\"");
        api.window(id, "name", translation.get("SpawnUI") + " - " + api.element(id, ctx[id].spawnList, "field"));
    }
}

function NoireSpawn_Update(id) {
    if (api.window(id, "minimized")) api.element(id, ctx[id].minimize, "colorBackground", color.accent3);
    else api.element(id, ctx[id].minimize, "colorBackground", color.windowButton);
    if (api.window(id, "pinned")) api.element(id, ctx[id].pin, "colorBackground", color.accent3);
    else api.element(id, ctx[id].pin, "colorBackground", color.windowButton);
    if (api.window(id, "linked")) api.element(id, ctx[id].link, "colorBackground", color.accent3);
    else api.element(id, ctx[id].link, "colorBackground", color.windowButton);
    if (api.window(id, "keyboardCapture")) api.element(id, ctx[id].keyboard, "colorBackground", color.accent3);
    else api.element(id, ctx[id].keyboard, "colorBackground", color.windowButton);

    var windowScaleFactor = api.window(id, "scaleFactor");
    api.element(id, ctx[id].sidePanel, "baseCorner", cvar.int("shell.window.corner") / windowScaleFactor);

    api.element(id, ctx[id].toolgunSetting1, "text", cvar.string("toolgun_toolset1"));
    api.element(id, ctx[id].toolgunSetting2, "text", cvar.string("toolgun_toolset2"));
    api.element(id, ctx[id].toolgunSetting3, "text", cvar.string("toolgun_toolset3"));
    api.element(id, ctx[id].toolgunSetting4, "text", cvar.string("toolgun_toolset4"));

    if (api.window(id, "linked")) {
        api.element(id, ctx[id].sidePanel, "colorBackground", color.empty);
        api.element(id, ctx[id].sidePanel2, "colorBackground", color.empty);
    } else {
        api.element(id, ctx[id].sidePanel, "colorBackground", color.windowSide);
        api.element(id, ctx[id].sidePanel2, "colorBackground", color.windowSide);
    }
}

function NoireSpawn_BackgroundUpdate(id) {
}

function NoireSpawn_Shutdown(id) {
}

function NoireSpawn_Exit(id) {
    NoireSpawn_Shutdown(id);
    ui.closeWindow(ctx[id].self);
    delete ctx[id];
}

function NoireSpawn_Minimize(id) {
    if (api.window(id, "minimized")) api.window(id, "minimized", 0);
    else api.window(id, "minimized", 1);
}

function NoireSpawn_Pin(id) {
    if (api.window(id, "pinned")) api.window(id, "pinned", 0);
    else api.window(id, "pinned", 1);
}

function NoireSpawn_Link(id) {
    if (api.window(id, "linked")) {
        api.window(id, "linked", 0);
    } else {
        ui.teleportWindowToPlayer(id, 128);
        api.window(id, "linked", 1);
    }
}

function NoireSpawn_Keyboard(id) {
    if (api.window(id, "keyboardCapture")) api.window(id, "keyboardCapture", 0);
    else api.window(id, "keyboardCapture", 1);
}

app.register("noiredev.spawnui", "Spawn UI", NoireSpawn_Init, NoireSpawn_Key, NoireSpawn_Call, NoireSpawn_Update, NoireSpawn_BackgroundUpdate, NoireSpawn_Shutdown);