const item = {};
item.list = [];

item.register = function (classname, model, icon, pickup_name, quantity, giType, giTag) {
    var id = item.list.length;
    if (!item.list[id]) item.list[id] = {};
    item.list[id].classname = classname;

    var itemCount = api.item(item.vmIndex, 0, "itemCount");

    api.item(item.vmIndex, itemCount, "classname", classname);
    api.item(item.vmIndex, itemCount, "pickup_sound", "sound/misc/w_pkup.wav");
    api.item(item.vmIndex, itemCount, "pickup_name", pickup_name);
    api.item(item.vmIndex, itemCount, "model", model);
    api.item(item.vmIndex, itemCount, "icon", icon);
    api.item(item.vmIndex, itemCount, "quantity", quantity);
    api.item(item.vmIndex, itemCount, "giType", giType);
    api.item(item.vmIndex, itemCount, "giTag", giTag);
    api.item(item.vmIndex, itemCount, "precaches", "");
    api.item(item.vmIndex, itemCount, "sounds", "");

    api.item(item.vmIndex, 0, "itemCount", itemCount + 1);

    console.log("added item: " + classname + " with id " + itemCount);
}

item.initSystem = function (vmIndex) {
    item.vmIndex = vmIndex;
    api.item(item.vmIndex, 0, "itemCount", 0);
    openjs.folder("js/items", "items");
}