const shell = {};

cvar.register("shell.global.scale", "1.00", CVAR.ARCHIVE);
cvar.register("shell.window.corner", "16", CVAR.ARCHIVE);
cvar.register("shell.window.outline", "1", CVAR.ARCHIVE);
cvar.register("shell.window.coloredOutline", "1", CVAR.ARCHIVE);
cvar.register("shell.element.corner", "8", CVAR.ARCHIVE);

function Shell_Init() {
    shell.width = api.glconfig(qvm.ui, "width");
    shell.height = api.glconfig(qvm.ui, "height");
    shell.onMap = api.shell("onMap");
    shell.scale = api.shell("scale");
    if (cvar.float("shell.global.scale") > api.cgui(qvm.ui, "scale")) {
        cvar.set("shell.global.scale", api.cgui(qvm.ui, "scale"));
        qvm.cmd(qvm.ui, EXEC.INSERT, "shell.restart");
    }
    color.init(qvm.ui);

    shell.desktop = {};
    shell.desktop.window = ui.window(0, "", "Desktop", "none", UI.NOTITLE | UI.NOSCALE | UI.NOZORDER | UI.NOSAVE, shell.width, shell.height, color.white, color.empty, color.empty);
    shell.desktop.appColor = 1024 + (1024 * shell.desktop.window);
    if (!shell.onMap) {
        shell.desktop.background = ui.picture(shell.desktop.window, -1, 0, 0, shell.width, shell.height, "menu/animbg", 0, color.white);
    }

    shell.desktop.vignette = ui.picture(shell.desktop.window, -1, 0, 0, shell.width, shell.height, "menu/vignette", 0, color.transparent192);

    Shell_MainMenu();

    shell.apps = {};
    shell.apps.window = ui.window(1, "", "App launcher", "none", UI.NOTITLE | UI.NOSAVE, 640, 508, color.white, color.empty, color.empty);
    shell.apps.appColor = 1024 + (1024 * shell.apps.window);
    shell.apps.w = api.window(shell.apps.window, "baseW") * shell.scale;
    shell.apps.h = api.window(shell.apps.window, "baseH") * shell.scale;
    shell.apps.background = ui.button(shell.apps.window, -1, 0, 0, 640, 512 - 32, "", UI.CENTER | UI.BOLD | UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT, color.background, 1.00);
    shell.apps.text = ui.button(shell.apps.window, -1, 32, 12, 576, 48, "Applications:", UI.LEFT | UI.BOLD | UI.DROPSHADOW, color.empty, 1.20);
    api.element(shell.apps.window, shell.apps.text, "hoverStyle", 0);
    shell.apps.button = ui.button(shell.apps.window, -1, 320 - 80, 512 - 32, 160, 28, "Apps", UI.CENTER | UI.BOLD | UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT, color.background, 0.75);
    shell.apps.buttonH = api.element(shell.apps.window, shell.apps.button, "baseH") * shell.scale;
    shell.apps.appList = ui.list(shell.apps.window, -1, 32, 48 + 12, 96, 96, 0.70, 6, 4, LSTYLE.GRID, LMODE.APPS, 0);
    ui.setMargin(shell.apps.window, shell.apps.appList, 16, 16, 16, 16);
    shell.apps.animStatus = 0;
    shell.apps.anim = { y: 0 };
    shell.apps.anim.y = -1 - (shell.apps.h - shell.apps.buttonH);
    api.window(shell.apps.window, "x", (shell.width * 0.5) - (shell.apps.w * 0.5));

    openjs.folder("js/startup", "startup");
}

function Shell_MainMenu() {
    var scaleFactor = shell.scale * 1.25;
    var x = 72 * scaleFactor;
    var y = 200 * scaleFactor;
    var space = 28 * scaleFactor;
    var pagespace = 32 * scaleFactor;
    var size = 1.00 * scaleFactor;

    var logox = x + (8 * scaleFactor);
    var logobgx = x - (8 * scaleFactor);
    var logotextx = x + (120 * scaleFactor);
    var logoy = 64 * scaleFactor;
    var logobgy = (64 + 16) * scaleFactor;
    var logosize = 96 * scaleFactor;
    var logotextsize = 1.16 * scaleFactor;
    var logobgw = 320 * scaleFactor;
    var logotextw = 174 * scaleFactor;
    var logobgh = 64 * scaleFactor;
    var logocorner = 8 * scaleFactor;

    color.logobg = color.create(qvm.ui, shell.desktop.appColor + 0, 0, 0, 0, 164);
    color.logobgOutline = color.create(qvm.ui, shell.desktop.appColor + 1, 192, 192, 192, 48);

    shell.desktop.logobgOut = ui.button(shell.desktop.window, -1, logobgx - 1, logobgy - 1, logobgw + 2, logobgh + 2, "", 0, color.logobgOutline);
    api.element(shell.desktop.window, shell.desktop.logobgOut, "baseCorner", logocorner);
    shell.desktop.logobg = ui.button(shell.desktop.window, -1, logobgx, logobgy, logobgw, logobgh, "", 0, color.logobg);
    api.element(shell.desktop.window, shell.desktop.logobg, "baseCorner", logocorner);
    shell.desktop.logoText = ui.button(shell.desktop.window, -1, logotextx, logobgy, logotextw, logobgh, "noire's mod", UI.CENTER | UI.BOLD, color.empty, logotextsize);
    api.element(shell.desktop.window, shell.desktop.logoText, "baseCorner", logocorner);
    api.element(shell.desktop.window, shell.desktop.logoText, "hoverStyle", 0);
    shell.desktop.logo = ui.picture(shell.desktop.window, -1, logox, logoy, logosize, logosize, "menu/logo", 0, color.white);

    if (shell.onMap) {
        shell.desktop.resume = ui.button(shell.desktop.window, -1, x, y, 338, 28, "Resume game", UI.LEFT | UI.BOLD, color.empty, size);
        api.element(shell.desktop.window, shell.desktop.resume, "hoverStyle", UI.ACCENT);
        ui.func(shell.desktop.window, shell.desktop.resume, function () { qvm.cmd(qvm.ui, EXEC.INSERT, "shell.close"); });
        y += space;
        y += pagespace;
    }
    shell.desktop.create = ui.button(shell.desktop.window, -1, x, y, 338, 28, "Start New Game", UI.LEFT | UI.BOLD, color.empty, size);
    api.element(shell.desktop.window, shell.desktop.create, "hoverStyle", UI.ACCENT);
    ui.func(shell.desktop.window, shell.desktop.create, function () { app.launch(app.getByNameID("noire.mapbrowser")); });
    shell.desktop.connect = ui.button(shell.desktop.window, -1, x, y += space, 338, 28, "Find Multiplayer Game", UI.LEFT | UI.BOLD, color.empty, size);
    api.element(shell.desktop.window, shell.desktop.connect, "hoverStyle", UI.ACCENT);
    y += pagespace;
    shell.desktop.profile = ui.button(shell.desktop.window, -1, x, y += space, 338, 28, "Profile", UI.LEFT | UI.BOLD, color.empty, size);
    api.element(shell.desktop.window, shell.desktop.profile, "hoverStyle", UI.ACCENT);
    shell.desktop.mods = ui.button(shell.desktop.window, -1, x, y += space, 338, 28, "Mods", UI.LEFT | UI.BOLD, color.empty, size);
    api.element(shell.desktop.window, shell.desktop.mods, "hoverStyle", UI.ACCENT);
    shell.desktop.demos = ui.button(shell.desktop.window, -1, x, y += space, 338, 28, "Demos", UI.LEFT | UI.BOLD, color.empty, size);
    api.element(shell.desktop.window, shell.desktop.demos, "hoverStyle", UI.ACCENT);
    y += pagespace;
    shell.desktop.options = ui.button(shell.desktop.window, -1, x, y += space, 338, 28, "Options", UI.LEFT | UI.BOLD, color.empty, size);
    api.element(shell.desktop.window, shell.desktop.options, "hoverStyle", UI.ACCENT);
    ui.func(shell.desktop.window, shell.desktop.options, function () { app.launch(app.getByNameID("noire.settings")); });
    y += pagespace;
    if (shell.onMap) {
        shell.desktop.disconnect = ui.button(shell.desktop.window, -1, x, y += space, 338, 28, "Disconnect", UI.LEFT | UI.BOLD, color.empty, size);
        api.element(shell.desktop.window, shell.desktop.disconnect, "hoverStyle", UI.ACCENT);
        ui.func(shell.desktop.window, shell.desktop.disconnect, function () { qvm.cmd(qvm.ui, EXEC.INSERT, "disconnect"); });
    }
    shell.desktop.quit = ui.button(shell.desktop.window, -1, x, y += space, 338, 28, "Quit", UI.LEFT | UI.BOLD, color.empty, size);
    api.element(shell.desktop.window, shell.desktop.quit, "hoverStyle", UI.ACCENT);
    ui.func(shell.desktop.window, shell.desktop.quit, function () { qvm.cmd(qvm.ui, EXEC.INSERT, "quit"); });
}

function Shell_Draw() {
    Animation.update();
    api.window(shell.apps.window, "y", shell.apps.anim.y);
}

function Shell_Key(key, windowID) {
    var nameID = api.window(windowID, "nameID");
    app.key(app.getByNameID(nameID), key, windowID);
}

function Shell_Callback(windowID, elementID, key) {
    var nameID = api.window(windowID, "nameID");
    app.call(app.getByNameID(nameID), windowID, elementID, key);
    if (key != KEY.MOUSE1 && key != KEY.MOUSE2 && key != KEY.ENTER) return;

    ui.callFunc(windowID, elementID);

    if (windowID == shell.apps.window) {
        if (elementID == shell.apps.button) {
            if (shell.apps.animStatus == 0) {
                shell.apps.animStatus = 1;
                Animation.add(shell.apps.anim, { y: 0 }, 500);
            } else {
                shell.apps.animStatus = 0;
                Animation.add(shell.apps.anim, { y: -1 - (shell.apps.h - shell.apps.buttonH) }, 500);
            }
        }
        if (elementID == shell.apps.appList) {
            Shell_Callback(shell.apps.window, shell.apps.button, KEY.MOUSE1);
            var selectedApp = api.element(shell.apps.window, shell.apps.appList, "value");
            app.launch(selectedApp);
        }
    }
}

function Shell_Update(windowID) {
    var nameID = api.window(windowID, "nameID");
    app.update(app.getByNameID(nameID), windowID);
}

function Shell_BackgroundUpdate(windowID) {
    var nameID = api.window(windowID, "nameID");
    app.bgupdate(app.getByNameID(nameID), windowID);
}

function Shell_Shutdown(windowID) {
    var nameID = api.window(windowID, "nameID");
    app.shutdown(app.getByNameID(nameID), windowID);
}