const api = {};
const apiFields = {};
const apiField = -1;

apiField = -1;
apiFields.item = {
    "classname": apiField += 1,
    "pickup_sound": apiField += 1,
    "pickup_name": apiField += 1,
    "model": apiField += 1,
    "icon": apiField += 1,
    "quantity": apiField += 1,
    "giType": apiField += 1,
    "giTag": apiField += 1,
    "precaches": apiField += 1,
    "sounds": apiField += 1,

    "itemCount": apiField += 1,
};

apiField = -1;
apiFields.weapon = {
    "wType": apiField += 1,
    "wAnimType": apiField += 1,
    "classname": apiField += 1,
    "delay": apiField += 1,
    "count": apiField += 1,
    "damage": apiField += 1,
    "splashDamage": apiField += 1,
    "splashRadius": apiField += 1,
    "range": apiField += 1,
    "speed": apiField += 1,
    "speedRandom": apiField += 1,
    "spread": apiField += 1,
    "timeout": apiField += 1,
    "gravity": apiField += 1,
    "bounce": apiField += 1,
    "bounceModifier": apiField += 1,
    "zOffset": apiField += 1,

    "weaponCount": apiField += 1,
};

// cgame.vm only (jsd_weaponFx), colors/vectors are per-component with i1
apiField = -1;
apiFields.weaponFx = {
    "animType": apiField += 1,
    "weaponSection": apiField += 1,
    "sectionOrder": apiField += 1,
    "colorizeItemIcon": apiField += 1,

    "hitLightColor": apiField += 1, // [3]
    "hitLightRadius": apiField += 1,
    "hitLightTime": apiField += 1,
    "hitParticleColor": apiField += 1, // [4]
    "hitParticleCount": apiField += 1,
    "hitParticleSpeed": apiField += 1,
    "hitParticleFlags": apiField += 1,
    "hitParticleGravity": apiField += 1, // [3]
    "hitParticleSize": apiField += 1,
    "hitParticleLifetime": apiField += 1,
    "hitParticleShader": apiField += 1,

    "continuousFire": apiField += 1,
    "suppressRepeatFire": apiField += 1,
    "fireLightColor": apiField += 1, // [3]
    "fireLightRadius": apiField += 1,
    "fireLightTime": apiField += 1,
    "fireParticleColor": apiField += 1, // [4]
    "fireParticleCount": apiField += 1,
    "fireParticleSpeed": apiField += 1,
    "fireParticleFlags": apiField += 1,
    "fireParticleGravity": apiField += 1, // [3]
    "fireParticleSize": apiField += 1,
    "fireParticleLifetime": apiField += 1,
    "fireParticleShader": apiField += 1,

    "trailColor": apiField += 1, // [4]
    "trailCount": apiField += 1,
    "trailSpeed": apiField += 1,
    "trailFlags": apiField += 1,
    "trailGravity": apiField += 1, // [3]
    "trailSize": apiField += 1,
    "trailLifetime": apiField += 1,
    "trailShader": apiField += 1,
    "trailInterval": apiField += 1,

    "missileLightColor": apiField += 1, // [3]
    "missileLightRadius": apiField += 1,

    "tracerChance": apiField += 1,
    "tracerLength": apiField += 1,
    "tracerParticleShader": apiField += 1,
    "tracerParticleColor": apiField += 1, // [4]
    "tracerParticleCount": apiField += 1,
    "tracerParticleSpeed": apiField += 1,
    "tracerParticleFlags": apiField += 1,
    "tracerParticleGravity": apiField += 1, // [3]
    "tracerParticleSize": apiField += 1,
    "tracerParticleLifetime": apiField += 1,
    "tracerSound": apiField += 1,

    "isBeam": apiField += 1,
    "beamShader": apiField += 1,
    "beamColor": apiField += 1, // [4]
    "beamColorizeByClient": apiField += 1,
    "beamStartOffset": apiField += 1, // [3]
    "beamEndOffset": apiField += 1, // [3]

    "readySound": apiField += 1,
    "fireSound": apiField += 1, // [4]
    "continuousFireSound": apiField += 1,
    "missileFlySound": apiField += 1,
    "missileBounceSound": apiField += 1,
    "missileExplodeSound": apiField += 1,

    "missileModel": apiField += 1,
    "missileSprite": apiField += 1,
    "missileRenderfx": apiField += 1,

    "modelColorSource": apiField += 1,
    "modelColor": apiField += 1, // [3]
    "explodeOnFleshHit": apiField += 1,

    "flashModel": apiField += 1,
    "muzzleBone": apiField += 1,

    "brassModel": apiField += 1,
    "brassCount": apiField += 1,
    "brassOffset": apiField += 1, // [3]
    "brassVelocity": apiField += 1, // [3]
    "brassVelocitySpread": apiField += 1, // [3]
    "brassBounceFactor": apiField += 1,
    "brassLifetimeMult": apiField += 1,
    "brassSpinDelta": apiField += 1, // [3]
};

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

api.item = function (vmIndex, itemID, fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.item[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.item: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiItem, vmIndex, itemID, fieldID, newValue, i1, i2, i3, i4);
}

api.weapon = function (vmIndex, weaponID, fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.weapon[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.weapon: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiWeapon, vmIndex, weaponID, fieldID, newValue, i1, i2, i3, i4);
}

api.weaponFx = function (weaponID, fieldName, value, i1, i2, i3, i4) {
    var fieldID = apiFields.weaponFx[fieldName];
    var newValue = value;
    if (fieldID === undefined) {
        console.log("#ff5api.weaponFx: invalid field '" + fieldName + "'");
        return false;
    }
    return qvm.call(vm.apiWeaponFx, qvm.cgame, weaponID, fieldID, newValue, i1, i2, i3, i4);
}

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
