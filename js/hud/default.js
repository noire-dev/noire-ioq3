const hud = {};

function Hud_Init() {
    color.init(qvm.cgame);
    hud.width = api.glconfig(qvm.cgame, "width");
    hud.height = api.glconfig(qvm.cgame, "height");
    
    Hud_Settings();
}

function Hud_Settings() {
    api.hud("counterW", 78.0);
    api.hud("counterH", 36.0);
    api.hud("counterCorner", 4.0);
    api.hud("counterColor", color.background);
    api.hud("counterTextX", 0.10);
    api.hud("counterTextY", 0.85);
    api.hud("counterTextScale", 0.40);
    api.hud("counterTextColor", color.white);
    api.hud("counterTextStyle", UI.LEFT|UI.DROPSHADOW);
    api.hud("counterValueX", 0.925);
    api.hud("counterValueY", 0.50);
    api.hud("counterValueScale", 0.80);
    api.hud("counterValueColor", color.white);
    api.hud("counterValueStyle", UI.BOLD|UI.RIGHT|UI.DROPSHADOW);
}

function Hud_Draw() {
    var wideoffset = api.cgui(qvm.cgame, "wideoffset");
    hud.left = 0-wideoffset;
    hud.right = 640+wideoffset;
    hud.bottom = 480;
    hud.top = 0;
    var xMargin = 5;
    var yMargin = 5;
    cg.HUDCounter(xMargin-wideoffset, (hud.bottom - api.hud("counterH")) - yMargin, String(api.clientPlayer("stats", null, 0)), translation.get("Health"));
    cg.HUDCounter(((xMargin-wideoffset)+api.hud("counterW"))+xMargin, (hud.bottom - api.hud("counterH"))- yMargin, String(api.clientPlayer("stats", null, 1)), translation.get("Armor"));
    cg.HUDCounter((hud.right-api.hud("counterW"))-xMargin, (hud.bottom - api.hud("counterH"))- yMargin, String(api.clientPlayer("stats", null, 2)), translation.get("Ammo"));
}