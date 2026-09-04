const jsCount = -1;
const js = {
    shellInit: jsCount += 1,
    shellDraw: jsCount += 1,
    shellKey: jsCount += 1,
    shellCallback: jsCount += 1,
    shellUpdate: jsCount += 1,
    shellBackgroundUpdate: jsCount += 1,
    shellShutdown: jsCount += 1,
    getApp: jsCount += 1,
    getAppName: jsCount += 1,
    getAppIcon: jsCount += 1,
    launchApp: jsCount += 1,
    HUDInit: jsCount += 1,
    HUDDraw: jsCount += 1,
    getTool: jsCount += 1,
    getToolNameID: jsCount += 1,
    getToolName: jsCount += 1,
    initTool: jsCount += 1,
    callTool: jsCount += 1,
    gameRunFrame: jsCount += 1,
};

const vmCount = -1;
const vm = {
    // General
    cmd: vmCount += 1,

    // game.qvm
    apiGameEntity: vmCount += 1,
    gapiEntityDelete: vmCount += 1,

    // cgame.qvm + ui.qvm
    apiCGUI: vmCount += 1,
    apiGLConfig: vmCount += 1,
    drawString: vmCount += 1,
    drawStringAdjusted: vmCount += 1,
    drawRectangle: vmCount += 1,
    drawShader: vmCount += 1,

    // cgame.qvm
    apiHUD: vmCount += 1,
    apiClientPlayer: vmCount += 1,
    HUDCounter: vmCount += 1,
    notify: vmCount += 1,

    // ui.qvm
    apiShell: vmCount += 1,
    apiWindow: vmCount += 1,
    apiElement: vmCount += 1,
    itemFocused: vmCount += 1,
    cursorInWindow: vmCount += 1,
    closeWindow: vmCount += 1,
    clearWindow: vmCount += 1,
    teleportWindowToPlayer: vmCount += 1,
    getFreeElement: vmCount += 1,
    createWindow: vmCount += 1,
    createWindowButton: vmCount += 1,
    createPicture: vmCount += 1,
    createButton: vmCount += 1,
    createCheckbox: vmCount += 1,
    createSlider: vmCount += 1,
    createAction: vmCount += 1,
    createSpin: vmCount += 1,
    createField: vmCount += 1,
    createList: vmCount += 1,
    createListFiles: vmCount += 1,
    fillListFiles: vmCount += 1,
    getFileFromList: vmCount += 1,
    getFileListCount: vmCount += 1,
    setMargin: vmCount += 1,
};

const CVAR = {
    ARCHIVE: 0x0001,        // 1
    USERINFO: 0x0002,       // 2
    SERVERINFO: 0x0004,     // 4
    SYSTEMINFO: 0x0008,     // 8
    INIT: 0x0010,           // 16
    LATCH: 0x0020,          // 32
    ROM: 0x0040,            // 64
    USER_CREATED: 0x0080,   // 128
    TEMP: 0x0100,           // 256
    CHEAT: 0x0200,          // 512
    NORESTART: 0x0400,      // 1024
    SERVER_CREATED: 0x0800, // 2048
    VM_CREATED: 0x1000,     // 4096
    PROTECTED: 0x2000,      // 8192
    NODEFAULT: 0x4000,      // 16384
    PRIVATE: 0x8000,        // 32768
    DEVELOPER: 0x10000,     // 65536
    NOTABCOMPLETE: 0x20000, // 131072
    MODIFIED: 0x40000000,   // 1073741824
    NONEXISTENT: 0x80000000 // 2147483648
};

const limits = {
    maxElements: 512,
    maxAddons: 4096,
};

const EXEC = {
    NOW: 0,
    INSERT: 1,
    APPEND: 2
};

qvm.cmd = function (vmIndex, type, command) {
    qvm.call(vm.cmd, vmIndex, type, command);
}

cvar.register("hud.module", "default", CVAR.ARCHIVE);

openjs.file("js/addons");
openjs.file("js/cvar");
openjs.file("js/translation");
openjs.file("js/api");
openjs.file("js/color");
openjs.file("js/cgui");
openjs.file("js/ui");
openjs.file("js/utils");
openjs.file("js/app");
openjs.file("js/tool");
openjs.file("js/shell");
openjs.file("js/cg");
openjs.file("js/hud/" + cvar.string("hud.module"));
openjs.file("js/game");
openjs.file("js/gameMain");
openjs.file("js/spawnlists/spawnlists");