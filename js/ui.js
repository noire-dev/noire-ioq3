const ui = {};
ui.funcs = {};

const KEY = {
    TAB: 9,
    ENTER: 13,
    ESCAPE: 27,
    SPACE: 32,
    QUOTE: "'",
    PLUS: "+",
    COMMA: ",",
    MINUS: "-",
    DOT: ".",
    SLASH: "/",
    SEMICOLON: ";",
    EQUAL: "=",
    BACKSLASH: "\\",
    UNDERSCORE: "_",
    BRACKET_OPEN: "[",
    BRACKET_CLOSE: "]",
    A: "a",
    B: "b",
    C: "c",
    D: "d",
    E: "e",
    F: "f",
    G: "g",
    H: "h",
    I: "i",
    J: "j",
    K: "k",
    L: "l",
    M: "m",
    N: "n",
    O: "o",
    P: "p",
    Q: "q",
    R: "r",
    S: "s",
    T: "t",
    U: "u",
    V: "v",
    W: "w",
    X: "x",
    Y: "y",
    Z: "z",
    BACKSPACE: 127,
    COMMAND: 128,
    CAPSLOCK: 129,
    POWER: 130,
    PAUSE: 131,
    UPARROW: 132,
    DOWNARROW: 133,
    LEFTARROW: 134,
    RIGHTARROW: 135,
    ALT: 136,
    CTRL: 137,
    SHIFT: 138,
    INS: 139,
    DEL: 140,
    PGDN: 141,
    PGUP: 142,
    HOME: 143,
    END: 144,
    F1: 145,
    F2: 146,
    F3: 147,
    F4: 148,
    F5: 149,
    F6: 150,
    F7: 151,
    F8: 152,
    F9: 153,
    F10: 154,
    F11: 155,
    F12: 156,
    F13: 157,
    F14: 158,
    F15: 159,
    KP_HOME: 160,
    KP_UPARROW: 161,
    KP_PGUP: 162,
    KP_LEFTARROW: 163,
    KP_5: 164,
    KP_RIGHTARROW: 165,
    KP_END: 166,
    KP_DOWNARROW: 167,
    KP_PGDN: 168,
    KP_ENTER: 169,
    KP_INS: 170,
    KP_DEL: 171,
    KP_SLASH: 172,
    KP_MINUS: 173,
    KP_PLUS: 174,
    KP_NUMLOCK: 175,
    KP_STAR: 176,
    KP_EQUALS: 177,
    MOUSE1: 178,
    MOUSE2: 179,
    MOUSE3: 180,
    MOUSE4: 181,
    MOUSE5: 182,
    MWHEELDOWN: 183,
    MWHEELUP: 184,
    SUPER: 185,
    COMPOSE: 186,
    MODE: 187,
    HELP: 188,
    PRINT: 189,
    SYSREQ: 190,
    SCROLLOCK: 191,
    BREAK: 192,
    MENU: 193,
    EURO: 194,
    UNDO: 195
};

const EMODE = {
    NONE: 0,
    INT: 1,
    FLOAT: 2,
    NUMBER: 1,
    STRING: 2,
};

const LSTYLE = {
    NONE: 0,
    GRID: 1,
    LIST: 2,
}

const LCONTENT = {
    NONE: 0,
    SHADER: 1,
    MODEL: 2,
}

const LMODE = {
    NONE: 0,
    APPS: 1,
    MAPS: 2,
    BOTS: 3,
    PLAYERMODELS: 4,
    GAMEITEMS: 5,
    TOOLS: 6,
}

ui.func = function (windowID, elementID, func) {
    if (!ui.funcs[windowID]) ui.funcs[windowID] = {};
    ui.funcs[windowID][elementID] = func;
};

ui.callFunc = function (windowID, elementID) {
    if (ui.funcs[windowID] && ui.funcs[windowID][elementID]) return ui.funcs[windowID][elementID](windowID);
    return null;
};

ui.itemFocused = function (windowID, elementID) {
    return qvm.call(vm.itemFocused, qvm.ui, windowID, elementID);
}

ui.cursorInWindow = function (windowID, x, y, w, h) {
    return qvm.call(vm.cursorInWindow, qvm.ui, windowID, x, y, w, h);
}

ui.closeWindow = function (windowID) {
    if (ui.funcs[windowID]) delete ui.funcs[windowID]
    return qvm.call(vm.closeWindow, qvm.ui, windowID);
}

ui.clearWindow = function (windowID, min, max) {
    for (var i = min; i <= max; i++) if (ui.funcs[windowID]) ui.funcs[windowID][i] = null;
    return qvm.call(vm.clearWindow, qvm.ui, windowID, min, max);
}

ui.teleportWindowToPlayer = function (windowID, distance) {
    return qvm.call(vm.teleportWindowToPlayer, qvm.ui, windowID, distance);
}

ui.getFreeElement = function (windowID) {
    return qvm.call(vm.getFreeElement, qvm.ui, windowID);
}

ui.window = function (windowID, nameid, name, icon, style, w, h, colorText, colorTitle, colorBackground) {
    for (var i = 0; i < limits.maxElements; i++) if (ui.funcs[windowID]) ui.funcs[windowID][i] = null;
    var translatedName = translation.get(name);
    return qvm.call(vm.createWindow, qvm.ui, windowID, nameid, translatedName, icon, style, w, h, colorText, colorTitle, colorBackground);
}

ui.windowButton = function (windowID, elementID, x, text, style, color, scale) {
    var translatedText = translation.get(text);
    return qvm.call(vm.createWindowButton, qvm.ui, windowID, elementID, x, translatedText, style, color, scale);
}

ui.picture = function (windowID, elementID, x, y, w, h, text, style, color) {
    var translatedText = translation.get(text);
    return qvm.call(vm.createPicture, qvm.ui, windowID, elementID, x, y, w, h, translatedText, style, color);
}

ui.button = function (windowID, elementID, x, y, w, h, text, style, color, scale) {
    var translatedText = translation.get(text);
    return qvm.call(vm.createButton, qvm.ui, windowID, elementID, x, y, w, h, translatedText, style, color, scale);
}

ui.checkbox = function (windowID, elementID, x, y, w, h, text, style, color, scale, cvarString) {
    var translatedText = translation.get(text);
    return qvm.call(vm.createCheckbox, qvm.ui, windowID, elementID, x, y, w, h, translatedText, style, color, scale, cvarString);
}

ui.slider = function (windowID, elementID, x, y, w, h, text, style, color, scale, cvarString, min, max, mode) {
    var translatedText = translation.get(text);
    return qvm.call(vm.createSlider, qvm.ui, windowID, elementID, x, y, w, h, translatedText, style, color, scale, cvarString, min, max, mode);
}

ui.action = function (windowID, elementID, x, y, w, h, text, style, color, scale, cvarString) {
    var translatedText = translation.get(text);
    return qvm.call(vm.createAction, qvm.ui, windowID, elementID, x, y, w, h, translatedText, style, color, scale, cvarString);
}

ui.spin = function (windowID, elementID, x, y, w, h, text, style, color, scale, cvarString, mode) {
    var translatedText = translation.get(text);
    return qvm.call(vm.createSpin, qvm.ui, windowID, elementID, x, y, w, h, translatedText, style, color, scale, cvarString, mode);
}

ui.field = function (windowID, elementID, x, y, w, h, text, style, color, scale, cvarString) {
    var translatedText = translation.get(text);
    return qvm.call(vm.createField, qvm.ui, windowID, elementID, x, y, w, h, translatedText, style, color, scale, cvarString);
}

ui.list = function (windowID, elementID, x, y, w, h, scale, col, row, listStyle, listType, listSubtype) {
    return qvm.call(vm.createList, qvm.ui, windowID, elementID, x, y, w, h, scale, col, row, listStyle, listType, listSubtype);
}

ui.listFiles = function (windowID, elementID, x, y, w, h, scale, col, row, listStyle, listContent, listID) {
    return qvm.call(vm.createListFiles, qvm.ui, windowID, elementID, x, y, w, h, scale, col, row, listStyle, listContent, listID);
}

ui.fillListFiles = function (windowID, folder, ext, drawDir, listID) {
    return qvm.call(vm.fillListFiles, qvm.ui, windowID, folder, ext, drawDir, listID);
}

ui.getFileFromList = function (windowID, listID, index) {
    return qvm.call(vm.getFileFromList, qvm.ui, windowID, listID, index);
}

ui.getFileListCount = function (windowID, listID) {
    return qvm.call(vm.getFileListCount, qvm.ui, windowID, listID);
}

ui.setMargin = function (windowID, elementID, x, y, w, h) {
    return qvm.call(vm.setMargin, qvm.ui, windowID, elementID, x, y, w, h);
}

var Animation = {
    list: [],

    add: function (obj, props, duration, easing) {
        this.list.push({
            active: true,
            startTime: Date.now(),
            duration: duration,
            easing: easing || function (t) { return 1 - Math.pow(1 - t, 3); }, // ease-out-cubic
            obj: obj,
            start: Object.assign({}, obj),
            target: props,
            keys: Object.keys(props)
        });
    },

    update: function () {
        for (var i = this.list.length - 1; i >= 0; i--) {
            var a = this.list[i];
            if (!a.active) {
                this.list.splice(i, 1);
                continue;
            }

            var t = Math.min(1, (Date.now() - a.startTime) / a.duration);
            var ease = a.easing(t);

            for (var j = 0; j < a.keys.length; j++) {
                var key = a.keys[j];
                var start = a.start[key];
                var target = a.target[key];
                a.obj[key] = start + (target - start) * ease;
            }

            if (t >= 1) {
                a.active = false;
                for (var k = 0; k < a.keys.length; k++) {
                    a.obj[a.keys[k]] = a.target[a.keys[k]];
                }
            }
        }
        return this.list.length > 0;
    },

    clear: function () {
        this.list = [];
    }
};