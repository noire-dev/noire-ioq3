const cgui = {};

const UI = {
    LEFT: 0,
    CENTER: 1,
    RIGHT: 2,
    BOLD: 4,
    ITALIC: 8,
    UNDERLINE: 16,
    STRIKETHROUGH: 32,
    MAGIC: 64,
    SHAKE: 128,
    LOCKSTYLE: 256,
    LOCKEFFECTS: 512,
    LOCKCOLOR: 1024,
    LOCKEMOJI: 2048,
    ACCENT: 4096,
    DROPSHADOW: 8192,

    NO_TOP_LEFT: 16384,
    NO_TOP_RIGHT: 32768,
    NO_BOTTOM_LEFT: 65536,
    NO_BOTTOM_RIGHT: 131072,
    NODRAW: 262144,

    NOTITLE: 1,
    NOSCALE: 2,
    NOZORDER: 4,
    NOSAVE: 8,

    LOCKALL: 3840
};

cgui.string = function (vmIndex, x, y, text, style, color, size, maxChars) {
    qvm.call(vm.drawString, vmIndex, x, y, text, style, color, size, maxChars);
}

cgui.stringAdjusted = function (vmIndex, x, y, text, style, color, size, maxChars) {
    qvm.call(vm.drawStringAdjusted, vmIndex, x, y, text, style, color, size, maxChars);
}

cgui.rect = function (vmIndex, x, y, w, h, corner, color, style) {
    qvm.call(vm.drawRectangle, vmIndex, x, y, w, h, corner, color, style);
}

cgui.shader = function (vmIndex, x, y, w, h, shader, color) {
    qvm.call(vm.drawShader, vmIndex, x, y, w, h, shader, color);
}
