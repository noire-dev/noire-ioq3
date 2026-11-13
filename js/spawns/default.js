function SP_Prop_Physics() {
    var origin = spawnInfo.vector("origin", "0 0 0");
    var angles = spawnInfo.vector("angles", "0 0 0");
    var model = spawnInfo.string("model", "");

    var o = origin.split(" ");
    var x = parseFloat(o[0]) || 0;
    var y = parseFloat(o[1]) || 0;
    var z = parseFloat(o[2]) || 0;

    var ao = angles.split(" ");
    var ax = parseFloat(ao[0]) || 0;
    var ay = parseFloat(ao[1]) || 0;
    var az = parseFloat(ao[2]) || 0;
    entity.spawn(x, y, z, ax, ay, az, model);
}

function SP_Worldspawn() {
    var fogR = spawnInfo.string("r_fogColorR", "0.50");
    var fogG = spawnInfo.string("r_fogColorG", "0.50");
    var fogB = spawnInfo.string("r_fogColorB", "0.50");
    var fogDensity = spawnInfo.string("r_fogDensity", "1.00");
    var fogStart = spawnInfo.string("r_fogStart", "4096");
    var fogEnd = spawnInfo.string("r_fogEnd", "500000");

    cvar.set("r_fogColorR", fogR);
    cvar.set("r_fogColorG", fogG);
    cvar.set("r_fogColorB", fogB);
    cvar.set("r_fogDensity", fogDensity);
    cvar.set("r_fogStart", fogStart);
    cvar.set("r_fogEnd", fogEnd);
}

spawn.register("prop_physics", SP_Prop_Physics);
spawn.register("worldspawn", SP_Worldspawn);