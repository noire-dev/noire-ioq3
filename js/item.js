const item = {};
item.list = [];

item.register = function (classname, model, icon, pickup_name, quantity, giType, giTag) {
    var itemCount = api.item(item.vmIndex, 0, "itemCount");

    api.item(item.vmIndex, itemCount, "classname", classname);
    api.item(item.vmIndex, itemCount, "pickup_sound", "sound/items/ammo_pickup.wav");
    api.item(item.vmIndex, itemCount, "pickup_name", pickup_name);
    api.item(item.vmIndex, itemCount, "model", model);
    api.item(item.vmIndex, itemCount, "icon", icon);
    api.item(item.vmIndex, itemCount, "quantity", quantity);
    api.item(item.vmIndex, itemCount, "giType", giType);
    api.item(item.vmIndex, itemCount, "giTag", giTag);
    api.item(item.vmIndex, itemCount, "precaches", "");
    api.item(item.vmIndex, itemCount, "sounds", "");

    api.item(item.vmIndex, 0, "itemCount", itemCount + 1);
    return itemCount;
}

item.initSystem = function (vmIndex) {
    item.vmIndex = vmIndex;
    api.item(item.vmIndex, 0, "itemCount", 0);
    item.register(null, null, null, null, 0, 0, 0);
    openjs.folder("js/items", "items");
}