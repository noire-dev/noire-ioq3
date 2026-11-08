const weapon = {};

// weaponType_t (game/bg_public.h)
const WT = {
    NONE: 0,
    MELEE: 1,
    BULLET: 2,
    SHOTGUN: 3,
    BEAM: 4,
    PHYSGUN: 5,
    GRAVITYGUN: 6,
    TOOLGUN: 7,
    MISSILE: 8,
    EMPTY: 9,
};

// weaponAnimType_t (game/bg_public.h)
const WAT = {
    NONE: 0,
    MELEE: 1,
    GRENADE: 2,
};

// animType_t (game/bg_public.h) — for jsd_weapon "animType" (locomotion block)
const AMTYPE = {
    ALL: 0,
    AR2: 1,
    CAMERA: 2,
    CROSSBOW: 3,
    DUAL: 4,
    FIST: 5,
    GRENADE: 6,
    KNIFE: 7,
    MAGIC: 8,
    MELEE: 9,
    MELEE2: 10,
    PASSIVE: 11,
    PHYSGUN: 12,
    PISTOL: 13,
    REVOLVER: 14,
    RPG: 15,
    SHOTGUN: 16,
    SLAM: 17,
    SMG1: 18,
};

// particle flags (cgame) — for weaponFx particle "flags"
const PF = {
    SMOOTH_FADE: 0x01,
    GRAVITY: 0x02,
};

weapon.register = function (wType, wAnimType, classname, delay, count, damage, splashDamage, splashRadius, range, speed, speedRandom, spread, timeout, gravity, bounce, bounceModifier, zOffset) {
    var weaponCount = api.weapon(weapon.vmIndex, 0, "weaponCount");

    api.weapon(weapon.vmIndex, weaponCount, "wType", wType);
    api.weapon(weapon.vmIndex, weaponCount, "wAnimType", wAnimType);
    api.weapon(weapon.vmIndex, weaponCount, "classname", classname ? classname : "");
    api.weapon(weapon.vmIndex, weaponCount, "delay", delay);
    api.weapon(weapon.vmIndex, weaponCount, "count", count);
    api.weapon(weapon.vmIndex, weaponCount, "damage", damage);
    api.weapon(weapon.vmIndex, weaponCount, "splashDamage", splashDamage);
    api.weapon(weapon.vmIndex, weaponCount, "splashRadius", splashRadius);
    api.weapon(weapon.vmIndex, weaponCount, "range", range);
    api.weapon(weapon.vmIndex, weaponCount, "speed", speed);
    api.weapon(weapon.vmIndex, weaponCount, "speedRandom", speedRandom);
    api.weapon(weapon.vmIndex, weaponCount, "spread", spread);
    api.weapon(weapon.vmIndex, weaponCount, "timeout", timeout);
    api.weapon(weapon.vmIndex, weaponCount, "gravity", gravity ? 1 : 0);
    api.weapon(weapon.vmIndex, weaponCount, "bounce", bounce ? 1 : 0);
    api.weapon(weapon.vmIndex, weaponCount, "bounceModifier", bounceModifier);
    api.weapon(weapon.vmIndex, weaponCount, "zOffset", zOffset);

    api.weapon(weapon.vmIndex, 0, "weaponCount", weaponCount + 1);
    return weaponCount;
}

// weaponFx lives only in cgame.vm (jsd_weaponFx), so do nothing in game.vm/ui.vm
weapon.fx = function (weaponID, fieldName, value, i1, i2, i3, i4) {
    if (weapon.vmIndex !== qvm.cgame) return false;
    return api.weaponFx(weaponID, fieldName, value, i1, i2, i3, i4);
}

// jsd_weapon exists in every VM, so the write happens in game.vm, cgame.vm and ui.vm
weapon.set = function (weaponID, fieldName, value, i1, i2, i3, i4) {
    return api.weapon(weapon.vmIndex, weaponID, fieldName, value, i1, i2, i3, i4);
}

// Vector/color shortcut: writes x,y,z (and optional w) components via i1
weapon.fxv = function (weaponID, fieldName, x, y, z, w) {
    weapon.fx(weaponID, fieldName, x, 0);
    weapon.fx(weaponID, fieldName, y, 1);
    weapon.fx(weaponID, fieldName, z, 2);
    if (w !== undefined) weapon.fx(weaponID, fieldName, w, 3);
}

weapon.initSystem = function (vmIndex) {
    weapon.vmIndex = vmIndex;
    api.weapon(weapon.vmIndex, 0, "weaponCount", 0);
    weapon.none = weapon.register(WT.NONE, WAT.NONE, null, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, false, 0.0, 0.0);
    openjs.folder("js/weapons", "weapons");
}
