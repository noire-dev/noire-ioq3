addon.register("Weapons content", "weaponsContent", AddonConfig_WeaponsContent);

function AddonConfig_WeaponsContent(id, x, w, addonID) {
    var y = 10;
    ctx[id].enabled = ui.checkbox(id, ctx[id].tabEndID, 234, y, w-20, ctx[id].elementHeight, "Enabled", UI.BOLD, color.windowItem, ctx[id].elementFontScale, "addon.enabled." + addonID);
}