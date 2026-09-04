cvar.register("noiredev.settings.playerModelSelect", "0", 0);

function NoireSettings_Init(appID) {
    var id = ui.window(-1, app.list[appID].nameid, app.list[appID].name, app.list[appID].icon, 0, 800, 600, color.white, color.windowHeader, color.window);
    if (!ctx[id]) ctx[id] = {};

    ctx[id].self = id;
    ctx[id].close = ui.windowButton(id, -1, 0, "✕", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].close, NoireSettings_Exit);
    ctx[id].minimize = ui.windowButton(id, -1, 1, "−", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].minimize, NoireSettings_Minimize);
    ctx[id].pin = ui.windowButton(id, -1, 2, "⚲", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].pin, NoireSettings_Pin);
    ctx[id].link = ui.windowButton(id, -1, 3, "⌖", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].link, NoireSettings_Link);
    ctx[id].keyboard = ui.windowButton(id, -1, 4, "⌨", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].keyboard, NoireSettings_Keyboard);

    ctx[id].sidePanel = ui.button(id, -1, 0, 0, 224, 600, "", UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT | UI.NO_BOTTOM_RIGHT, color.windowSide, 1.00);
    api.element(id, ctx[id].sidePanel, "baseCorner", cvar.int("shell.window.corner"));

    ctx[id].tabs = [
        { name: "General", button: null },
        { name: "Profile", button: null },
        { name: "Controls-1", button: null },
        { name: "Controls-2", button: null },
        { name: "Graphics", button: null },
        { name: "Sound", button: null },
        { name: "Game", button: null },
        { name: "Appearance", button: null },
    ];

    for (var i = 0; i < ctx[id].tabs.length; i++) {
        ctx[id].tabs[i].button = ui.button(id, -1, 10, 10 + i * 29, 204, 24, ctx[id].tabs[i].name, UI.BOLD, color.windowItem, 0.80);
        api.element(id, ctx[id].tabs[i].button, "baseCorner", 4);
    }

    ctx[id].tabStartID = 50;
    ctx[id].tabEndID = 50;
    ctx[id].currentTab = 0;
    NoireSettings_Call(id, ctx[id].tabs[0].button, KEY.MOUSE1);
}

function NoireSettings_UpdateTab(id) {
    var y = 10;
    ctx[id].tabEndID = 50;
    ctx[id].elementHeight = 32;
    ctx[id].elementSpace = 4;
    ctx[id].elementFontScale = 0.80;

    if (ctx[id].tabs[ctx[id].currentTab].name === "General") {
        ctx[id].lang = ctx[id].tabEndID;
        api.element(id, ctx[id].lang, "options", "en", 0);
        api.element(id, ctx[id].lang, "options", "ru", 1);
        api.element(id, ctx[id].lang, "optionsCount", 6);
        ui.spin(id, ctx[id].lang, 234, y, 800 - 224 - 20, ctx[id].elementHeight, "Language", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "language", EMODE.STRING);
        ctx[id].scaleFactor = ctx[id].tabEndID += 1;
        ui.slider(id, ctx[id].scaleFactor, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Shell scale", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "", 1, api.cgui(qvm.ui, "scale") * 2, EMODE.INT);
        ui.func(id, ctx[id].scaleFactor, NoireSettings_ApplyScale);
        api.element(id, ctx[id].scaleFactor, "value", cvar.float("shell.global.scale") * 2);
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Profile") {
        ctx[id].playerName = ui.field(id, ctx[id].tabEndID, 234, y, 800 - 224 - 20, ctx[id].elementHeight, "Player name", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "name");
        ctx[id].playerModelSelect = ctx[id].tabEndID += 1;
        api.element(id, ctx[id].playerModelSelect, "options", "All", 0);
        api.element(id, ctx[id].playerModelSelect, "options", "Torso", 1);
        api.element(id, ctx[id].playerModelSelect, "options", "Head", 2);
        api.element(id, ctx[id].playerModelSelect, "options", "Legs", 3);
        api.element(id, ctx[id].playerModelSelect, "optionsCount", 4);
        ui.spin(id, ctx[id].playerModelSelect, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Player model select", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "noiredev.settings.playerModelSelect", EMODE.INT);
        ctx[id].playerModelListBG = ui.button(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, 92 * 4, "", 0, color.windowItem, 0.00);
        ctx[id].playerModelList = ui.list(id, ctx[id].tabEndID += 1, 236, y, 92, 92, 0.65, 6, 4, LSTYLE.GRID, LMODE.PLAYERMODELS, 0);
        ui.setMargin(id, ctx[id].playerModelList, 6, 6, 6, 6);
        api.element(id, ctx[id].playerModelList, "colorText", color.empty);
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Controls-1") {
        ui.action(id, ctx[id].tabEndID, 234, y, 800 - 224 - 20, ctx[id].elementHeight, "Move forward", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+forward");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Move left", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+moveleft");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Move back", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+back");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Move right", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+moveright");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Run / Walk", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+speed");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Up / Jump", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+moveup");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Down / Crouch", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+movedown");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Attack", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "vstr attackBtn");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Secondary Attack / Zoom", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+zoom");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Use", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+button2");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Gesture", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+button3");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Show scores", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "+scores");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Prev weapon", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "weapprev");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Next weapon", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "weapnext");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Undone", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "undo");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Flashlight", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "flashlight");
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Controls-2") {
        ui.action(id, ctx[id].tabEndID, 234, y, 800 - 224 - 20, ctx[id].elementHeight, "Chat", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "messagemode");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Chat - Team", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "messagemode2");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Chat - Target", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "messagemode3");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Undone", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "undo");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Noclip", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "noclip");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Toggle run", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "toggle cl_run");
        ui.action(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Switch toolgun mode", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "vstr uitoolmode");
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Graphics") {
        ctx[id].resolution = ctx[id].tabEndID;
        for (var i = 0; i < 64; i++) {
            if (cvar.string("displaymode." + i) === "") {
                api.element(id, ctx[id].resolution, "optionsCount", i);
                break;
            }
            api.element(id, ctx[id].resolution, "options", cvar.string("displaymode." + i), i);
        }
        ui.spin(id, ctx[id].resolution, 234, y, 800 - 224 - 20, ctx[id].elementHeight, "Resolution", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_resolution", EMODE.STRING);
        ctx[id].mode = ui.spin(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Mode", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_fullscreen", EMODE.NUMBER);
        api.element(id, ctx[id].mode, "options", "Window", 0);
        api.element(id, ctx[id].mode, "options", "Borderless window", 1);
        api.element(id, ctx[id].mode, "options", "Fullscreen", 2);
        api.element(id, ctx[id].mode, "options", "Full desktop", 3);
        api.element(id, ctx[id].mode, "optionsCount", 4);
        ctx[id].anisotropy = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Anisotropy", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_ext_texture_filter_anisotropic");
        ctx[id].postfx = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Post-processing", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_postfx");
        ctx[id].hdr = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "HDR", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_hdr");
        ctx[id].entityLimit = ui.spin(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Entity limit", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "ui_effectsLevel", EMODE.NUMBER);
        api.element(id, ctx[id].entityLimit, "options", "Very low", 0);
        api.element(id, ctx[id].entityLimit, "options", "Low", 1);
        api.element(id, ctx[id].entityLimit, "options", "Medium", 2);
        api.element(id, ctx[id].entityLimit, "options", "High", 3);
        api.element(id, ctx[id].entityLimit, "options", "Very High", 4);
        api.element(id, ctx[id].entityLimit, "options", "Ultra", 5);
        api.element(id, ctx[id].entityLimit, "optionsCount", 6);
        ctx[id].textureQuality = ui.spin(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Texture quality", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_picmip", EMODE.NUMBER);
        api.element(id, ctx[id].textureQuality, "options", "High", 0);
        api.element(id, ctx[id].textureQuality, "options", "Medium", 1);
        api.element(id, ctx[id].textureQuality, "options", "Low", 2);
        api.element(id, ctx[id].textureQuality, "options", "Very low", 3);
        api.element(id, ctx[id].textureQuality, "options", "Ultra low", 4);
        api.element(id, ctx[id].textureQuality, "optionsCount", 5);
        api.element(id, ctx[id].tabEndID += 1, "valueMod", 2.00);
        ctx[id].antiAliasing = ui.spin(id, ctx[id].tabEndID, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Anti-aliasing", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_ext_multisample", EMODE.NUMBER);
        api.element(id, ctx[id].antiAliasing, "options", "Off", 0);
        api.element(id, ctx[id].antiAliasing, "options", "1x", 1);
        api.element(id, ctx[id].antiAliasing, "options", "2x", 2);
        api.element(id, ctx[id].antiAliasing, "options", "3x", 3);
        api.element(id, ctx[id].antiAliasing, "options", "4x", 4);
        api.element(id, ctx[id].antiAliasing, "optionsCount", 5);
        api.element(id, ctx[id].tabEndID += 1, "valueMod", 0.05);
        ctx[id].bloomLevel = ui.spin(id, ctx[id].tabEndID, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Bloom level", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_bloom_intensity", EMODE.NUMBER);
        api.element(id, ctx[id].bloomLevel, "options", "Off", 0);
        api.element(id, ctx[id].bloomLevel, "options", "Low", 1);
        api.element(id, ctx[id].bloomLevel, "options", "Medium", 2);
        api.element(id, ctx[id].bloomLevel, "options", "High", 3);
        api.element(id, ctx[id].bloomLevel, "options", "Ultra", 4);
        api.element(id, ctx[id].bloomLevel, "optionsCount", 5);
        ctx[id].gamma = ui.slider(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Gamma", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_gamma", 0.50, 3.00, EMODE.FLOAT);
        ctx[id].vsync = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "V-Sync", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "r_swapInterval");
        ctx[id].maxfps = ui.field(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Max FPS", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "com_maxfps");
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Sound") {
        ctx[id].soundVolume = ui.slider(id, ctx[id].tabEndID, 234, y, 800 - 224 - 20, ctx[id].elementHeight, "Sound volume", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "s_volume", 0.00, 1.50, EMODE.FLOAT);
        ctx[id].musicVolume = ui.slider(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Music volume", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "s_musicVolume", 0.00, 1.50, EMODE.FLOAT);
        ctx[id].soundDriver = ui.spin(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Sound driver", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "", EMODE.NUMBER);
        api.element(id, ctx[id].soundDriver, "options", "dsound", 0);
        api.element(id, ctx[id].soundDriver, "options", "wasapi", 1);
        api.element(id, ctx[id].soundDriver, "optionsCount", 2);
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Game") {
        ctx[id].thirdPerson = ui.checkbox(id, ctx[id].tabEndID, 234, y, 800 - 224 - 20, ctx[id].elementHeight, "Third person", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "cg_thirdPerson");
        ctx[id].thirdPersonOffset = ui.slider(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Third person offset", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "cg_thirdPersonOffset", -50, 50, EMODE.INT);
        ctx[id].thirdPersonRange = ui.slider(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Third person range", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "cg_thirdPersonRange", 20, 100, EMODE.INT);
        ctx[id].cameraEyes = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Camera eyes", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "cg_cameraEyes");
        ctx[id].cameraShake = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Camera shake", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "cg_enableBobbing");
        ctx[id].marks = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Marks", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "cg_addMarks");
        ctx[id].blood = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Blood", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "com_blood");
        ctx[id].fov = ui.field(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "FoV", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "cg_fov");
        ctx[id].zoomFov = ui.field(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Zoom FoV", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "cg_zoomfov");
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Appearance") {
        ctx[id].accentColor = ui.button(id, ctx[id].tabEndID, 234, y, 800 - 224 - 20, ctx[id].elementHeight, "Accent color:", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "");
        ctx[id].accentColors = [
            { selcolor: color.blue3, button: null },
            { selcolor: color.green3, button: null },
            { selcolor: color.yellow3, button: null },
            { selcolor: color.orange3, button: null },
            { selcolor: color.red3, button: null },
            { selcolor: color.purple3, button: null },
            { selcolor: color.brown3, button: null },
            { selcolor: color.dark3, button: null },
            { selcolor: color.magenta3, button: null },
            { selcolor: color.ultraviolet3, button: null },
        ];
        for (var i = 0; i < ctx[id].accentColors.length; i++) {
            ctx[id].accentColors[i].button = ui.button(id, ctx[id].tabEndID += 1, (800 - 10 - ctx[id].elementHeight) - (i * ctx[id].elementHeight), y + (ctx[id].elementHeight * 0.15), ctx[id].elementHeight * 0.70, ctx[id].elementHeight * 0.70, "", UI.BOLD, ctx[id].accentColors[i].selcolor, 1.00);
        }
        ctx[id].windowCorner = ui.slider(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Window corner", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "shell.window.corner", 0, 20, EMODE.INT);
        ctx[id].windowOutline = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Window outline", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "shell.window.outline");
        ctx[id].windowColoredOutline = ui.checkbox(id, ctx[id].tabEndID += 1, 234, y += ctx[id].elementHeight + ctx[id].elementSpace, 800 - 224 - 20, ctx[id].elementHeight, "Window colored outline", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "shell.window.coloredOutline");
    }

    for (var i = ctx[id].tabStartID; i <= ctx[id].tabEndID; i++) {
        api.element(id, i, "baseCorner", 4);
        api.element(id, i, "hoverStyle", UI.ACCENT);
    }
    if (ctx[id].tabs[ctx[id].currentTab].name === "Appearance") {
        for (var i = 0; i < ctx[id].accentColors.length; i++) {
            api.element(id, ctx[id].accentColors[i].button, "baseCorner", 999999);
        }
    }
}

function NoireSettings_ApplyScale(id) {
    cvar.set("shell.global.scale", api.element(id, ctx[id].scaleFactor, "value") / 2);
    api.shell("scale", cvar.float("shell.global.scale"));
    qvm.cmd(qvm.ui, EXEC.INSERT, "shell.restart");
}

function NoireSettings_UpdateBtn(id) {
    if (ctx[id].needsToApply) {
        ctx[id].applyBtn = ui.button(id, -1, 10, 10 + 19 * 29, 204, 24, "Apply", UI.BOLD, color.windowItem, 0.80);
        api.element(id, ctx[id].applyBtn, "baseCorner", 4);
        ui.func(id, ctx[id].applyBtn, NoireSettings_Apply);
    }
}

function NoireSettings_Apply(id) {
    if (ctx[id].tabs[ctx[id].currentTab].name === "Graphics") {
        qvm.cmd(qvm.ui, EXEC.INSERT, "vid_restart");
    }
    if (ctx[id].tabs[ctx[id].currentTab].name === "Appearance") {
        qvm.cmd(qvm.ui, EXEC.INSERT, "shell.restart");
    }
}

function NoireSettings_Key(key, id) {
}

function NoireSettings_Call(id, eid, key) {
    if (key != KEY.MOUSE1 && key != KEY.MOUSE2) return;
    for (var i = 0; i < ctx[id].tabs.length; i++) {
        if (eid == ctx[id].tabs[i].button) {
            for (var j = 0; j < ctx[id].tabs.length; j++) {
                api.element(id, ctx[id].tabs[j].button, "colorBackground", color.windowItem);
            }
            api.element(id, ctx[id].tabs[i].button, "colorBackground", color.enabled);
            api.window(id, "name", translation.get("Settings") + " - " + translation.get(ctx[id].tabs[i].name));

            ctx[id].currentTab = i;

            ui.clearWindow(id, ctx[id].tabStartID, ctx[id].tabEndID);
            ctx[id].needsToApply = false;
            NoireSettings_UpdateTab(id);
            NoireSettings_UpdateBtn(id);
        }
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Profile") {
        if (eid == ctx[id].playerModelList) {
            if (cvar.int("noiredev.settings.playerModelSelect") == 0) {
                qvm.cmd(qvm.ui, EXEC.APPEND, "set headmodel " + api.element(id, ctx[id].playerModelList, "field") + ";");
                qvm.cmd(qvm.ui, EXEC.APPEND, "set model " + api.element(id, ctx[id].playerModelList, "field") + ";");
                qvm.cmd(qvm.ui, EXEC.APPEND, "set legsmodel " + api.element(id, ctx[id].playerModelList, "field") + ";");
            }
            if (cvar.int("noiredev.settings.playerModelSelect") == 1) qvm.cmd(qvm.ui, EXEC.APPEND, "set model " + api.element(id, ctx[id].playerModelList, "field") + ";");
            if (cvar.int("noiredev.settings.playerModelSelect") == 2) qvm.cmd(qvm.ui, EXEC.APPEND, "set headmodel " + api.element(id, ctx[id].playerModelList, "field") + ";");
            if (cvar.int("noiredev.settings.playerModelSelect") == 3) qvm.cmd(qvm.ui, EXEC.APPEND, "set legsmodel " + api.element(id, ctx[id].playerModelList, "field") + ";");
        }
    }

    if (ctx[id].tabs[ctx[id].currentTab].name === "Appearance") {
        for (var i = 0; i < ctx[id].accentColors.length; i++) {
            if (eid == ctx[id].accentColors[i].button) {
                cvar.set("shell.global.accentColor", String(ctx[id].accentColors[i].selcolor));
                color.init(qvm.ui);
            }
        }
    }

    if (!ctx[id].tabs[ctx[id].currentTab]) return;

    if (ctx[id].tabs[ctx[id].currentTab].name === "Graphics") {
        if (eid == ctx[id].resolution) ctx[id].needsToApply = true;
        if (eid == ctx[id].mode) ctx[id].needsToApply = true;
        if (eid == ctx[id].anisotropy) ctx[id].needsToApply = true;
        if (eid == ctx[id].textureQuality) ctx[id].needsToApply = true;
        if (eid == ctx[id].antiAliasing) ctx[id].needsToApply = true;
        if (eid == ctx[id].vsync) ctx[id].needsToApply = true;
    }
    if (ctx[id].tabs[ctx[id].currentTab].name === "Appearance") {
        if (eid == ctx[id].windowCorner) ctx[id].needsToApply = true;
        if (eid == ctx[id].windowOutline) ctx[id].needsToApply = true;
        if (eid == ctx[id].windowColoredOutline) ctx[id].needsToApply = true;
    }

    if (ctx[id].applyBtn) ui.clearWindow(id, ctx[id].applyBtn, ctx[id].applyBtn);
    NoireSettings_UpdateBtn(id);
}

function NoireSettings_Update(id) {
    if (api.window(id, "minimized")) api.element(id, ctx[id].minimize, "colorBackground", color.accent3);
    else api.element(id, ctx[id].minimize, "colorBackground", color.windowButton);
    if (api.window(id, "pinned")) api.element(id, ctx[id].pin, "colorBackground", color.accent3);
    else api.element(id, ctx[id].pin, "colorBackground", color.windowButton);
    if (api.window(id, "linked")) api.element(id, ctx[id].link, "colorBackground", color.accent3);
    else api.element(id, ctx[id].link, "colorBackground", color.windowButton);
    if (api.window(id, "keyboardCapture")) api.element(id, ctx[id].keyboard, "colorBackground", color.accent3);
    else api.element(id, ctx[id].keyboard, "colorBackground", color.windowButton);
}

function NoireSettings_BackgroundUpdate(id) {
}

function NoireSettings_Shutdown(id) {
}

function NoireSettings_Exit(id) {
    NoireSettings_Shutdown(id);
    ui.closeWindow(ctx[id].self);
    delete ctx[id];
}

function NoireSettings_Minimize(id) {
    if (api.window(id, "minimized")) api.window(id, "minimized", 0);
    else api.window(id, "minimized", 1);
}

function NoireSettings_Pin(id) {
    if (api.window(id, "pinned")) api.window(id, "pinned", 0);
    else api.window(id, "pinned", 1);
}

function NoireSettings_Link(id) {
    if (api.window(id, "linked")) {
        api.window(id, "linked", 0);
    } else {
        ui.teleportWindowToPlayer(id, 128);
        api.window(id, "linked", 1);
    }
}

function NoireSettings_Keyboard(id) {
    if (api.window(id, "keyboardCapture")) api.window(id, "keyboardCapture", 0);
    else api.window(id, "keyboardCapture", 1);
}

app.register("noiredev.settings", "Settings", NoireSettings_Init, NoireSettings_Key, NoireSettings_Call, NoireSettings_Update, NoireSettings_BackgroundUpdate, NoireSettings_Shutdown);