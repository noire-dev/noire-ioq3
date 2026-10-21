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

spawn.register("prop_physics", SP_Prop_Physics);