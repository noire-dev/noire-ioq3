const api = {};
const apiFields = {};
const apiField = -1;

apiField = -1;
apiFields.gameEntity = {
};

apiField = -1;
apiFields.hud = {
    "counterW": apiField += 1,
    "counterH": apiField += 1,
    "counterCorner": apiField += 1,
    "counterColor": apiField += 1,
    "counterTextX": apiField += 1,
    "counterTextY": apiField += 1,
    "counterTextScale": apiField += 1,
    "counterTextColor": apiField += 1,
    "counterTextStyle": apiField += 1,
    "counterValueX": apiField += 1,
    "counterValueY": apiField += 1,
    "counterValueScale": apiField += 1,
    "counterValueColor": apiField += 1,
    "counterValueStyle": apiField += 1,
};

apiField = -1;
apiFields.clientPlayer = {
    "stats": apiField += 1, // [16]
};

apiField = -1;
apiFields.cgui = {
    "whiteShader": apiField += 1,
    "corner": apiField += 1,
    "scale": apiField += 1,
    "wideoffset": apiField += 1,
    "colors": apiField += 1, // [1024][4]
    "binds": apiField += 1, // [256]
};

apiField = -1;
apiFields.glconfig = {
    "width": apiField += 1,
    "height": apiField += 1,
    "isFullscreen": apiField += 1,
};

apiField = -1;
apiFields.shell = {
    "cursorX": apiField += 1,
    "cursorY": apiField += 1,
    "cursorIsDragging": apiField += 1,
    "cursorIsMovingDesktop": apiField += 1,
    "rendered": apiField += 1,
    "debug": apiField += 1,
    "onMap": apiField += 1,
    "appCount": apiField += 1,
    "focusedWindow": apiField += 1,
    "activeWindow": apiField += 1,
    "zOrder": apiField += 1, // [16]
    "scale": apiField += 1,
    "windowCorner": apiField += 1,
    "windowOutline": apiField += 1,
    "windowColoredOutline": apiField += 1,
};

apiField = -1;
apiFields.window = {
    "created": apiField += 1,
    "minimized": apiField += 1,
    "pinned": apiField += 1,
    "linked": apiField += 1,
    "keyboardCapture": apiField += 1,
    "id": apiField += 1,
    "nameID": apiField += 1,
    "name": apiField += 1,
    "icon": apiField += 1,
    "style": apiField += 1,
    "baseW": apiField += 1,
    "baseH": apiField += 1,
    "x": apiField += 1,
    "y": apiField += 1,
    "w": apiField += 1,
    "h": apiField += 1,
    "colorText": apiField += 1,
    "colorTitle": apiField += 1,
    "colorBackground": apiField += 1,
    "scaleFactor": apiField += 1,
    "focusedElement": apiField += 1,
    "keyCapture": apiField += 1,
    "worldX": apiField += 1,
    "worldY": apiField += 1,
    "worldZ": apiField += 1,
    "worldScale": apiField += 1,
    "worldDisable": apiField += 1,
    "worldCursorScale": apiField += 1,
    "worldMoveDist": apiField += 1,
};

apiField = -1;
apiFields.element = {
    "parentWindow": apiField += 1,
    "created": apiField += 1,
    "type": apiField += 1,
    "id": apiField += 1,
    "baseX": apiField += 1,
    "baseY": apiField += 1,
    "baseW": apiField += 1,
    "baseH": apiField += 1,
    "x": apiField += 1,
    "y": apiField += 1,
    "w": apiField += 1,
    "h": apiField += 1,
    "text": apiField += 1,
    "style": apiField += 1,
    "hoverStyle": apiField += 1,
    "baseScale": apiField += 1,
    "scale": apiField += 1,
    "colorText": apiField += 1,
    "colorBackground": apiField += 1,
    "baseCorner": apiField += 1,
    "corner": apiField += 1,
    "baseMargin": apiField += 1,
    "margin": apiField += 1,
    "cvar": apiField += 1,
    "value": apiField += 1,
    "valueMod": apiField += 1,
    "min": apiField += 1,
    "max": apiField += 1,
    "mode": apiField += 1,
    "bind": apiField += 1,
    "options": apiField += 1, // [64]
    "optionsCount": apiField += 1,
    "field": apiField += 1,
    "fieldPosition": apiField += 1,
    "listStyle": apiField += 1,
    "listContent": apiField += 1,
    "listType": apiField += 1,
    "listSubtype": apiField += 1,
    "scroll": apiField += 1,
    "itemW": apiField += 1,
    "itemH": apiField += 1,
    "col": apiField += 1,
    "row": apiField += 1,
    "listID": apiField += 1,
};

api.gameEntity = function (entityID, fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.gameEntity[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.gameEntity: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiGameEntity, qvm.game, entityID, fieldID, newValue, i1, i2, i3, i4);
}

api.hud = function (fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.hud[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.hud: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiHUD, qvm.cgame, fieldID, newValue, i1, i2, i3, i4);
}

api.clientPlayer = function (fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.clientPlayer[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.clientPlayer: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiClientPlayer, qvm.cgame, fieldID, newValue, i1, i2, i3, i4);
}

api.cgui = function (vmIndex, fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.cgui[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.cgui: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiCGUI, vmIndex, fieldID, newValue, i1, i2, i3, i4);
}

api.glconfig = function (vmIndex, fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.glconfig[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.glconfig: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiGLConfig, vmIndex, fieldID, newValue, i1, i2, i3, i4);
}

api.shell = function (fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.shell[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.shell: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiShell, qvm.ui, fieldID, newValue, i1, i2, i3, i4);
}

api.window = function (windowID, fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.window[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.window: invalid field '" + fieldName + "'");
        return false;
    }
    if (fieldName === "name") newValue = translation.get(value);
    return qvm.call(vm.apiWindow, qvm.ui, windowID, fieldID, newValue, i1, i2, i3, i4);
}

api.element = function (windowID, elementID, fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.element[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.element: invalid field '" + fieldName + "'");
        return false;
    }
    if (fieldName === "text") newValue = translation.get(value);
    if (fieldName === "options") newValue = translation.get(value);
    return qvm.call(vm.apiElement, qvm.ui, windowID, elementID, fieldID, newValue, i1, i2, i3, i4);
}
