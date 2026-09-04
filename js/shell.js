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
    if (!shell.onMap) {
        shell.desktop.background = ui.picture(shell.desktop.window, -1, 0, 0, shell.width, shell.height, "menu/animbg", 0, color.white);
        shell.desktop.vignette = ui.picture(shell.desktop.window, -1, 0, 0, shell.width, shell.height, "menu/vignette", 0, color.transparent192);
    }

    shell.apps = {};
    shell.apps.window = ui.window(1, "", "App launcher", "none", UI.NOTITLE | UI.NOSAVE, 640, 540, color.white, color.empty, color.empty);
    shell.apps.w = api.window(shell.apps.window, "baseW") * shell.scale;
    shell.apps.h = api.window(shell.apps.window, "baseH") * shell.scale;
    shell.apps.background = ui.button(shell.apps.window, -1, 0, 0, 640, 512, "", UI.CENTER | UI.BOLD | UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT, color.background, 1.00);
    shell.apps.quit = ui.button(shell.apps.window, -1, 32, 8, 128, 32, "Quit", UI.CENTER | UI.BOLD, color.accent4, 1.00);
    ui.func(shell.apps.window, shell.apps.quit, function () { qvm.cmd(qvm.ui, EXEC.INSERT, "quit"); });
    shell.apps.text = ui.button(shell.apps.window, -1, 32, 48, 576, 48, "Applications:", UI.LEFT | UI.BOLD | UI.DROPSHADOW, color.empty, 1.20);
    api.element(shell.apps.window, shell.apps.text, "hoverStyle", 0);
    shell.apps.button = ui.button(shell.apps.window, -1, 320 - 80, 512, 160, 28, "Menu", UI.CENTER | UI.BOLD | UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT, color.background, 0.75);
    shell.apps.buttonH = api.element(shell.apps.window, shell.apps.button, "baseH") * shell.scale;
    shell.apps.appList = ui.list(shell.apps.window, -1, 32, 48 + 48, 96, 96, 0.70, 6, 4, LSTYLE.GRID, LMODE.APPS, 0);
    ui.setMargin(shell.apps.window, shell.apps.appList, 16, 16, 16, 16);
    shell.apps.animStatus = 0;
    shell.apps.anim = { y: 0 };
    shell.apps.anim.y = -1 - (shell.apps.h - shell.apps.buttonH);
    api.window(shell.apps.window, "x", (shell.width * 0.5) - (shell.apps.w * 0.5));

    openjs.folder("js/startup", "startup");
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