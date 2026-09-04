var GMONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

function getGTime(offset) {
    var tz = offset || 0;
    var d = new Date();

    var customDate = new Date(Date.UTC(d.getUTCFullYear(), d.getUTCMonth(), d.getUTCDate(), d.getUTCHours() + tz, d.getUTCMinutes()));

    var day = customDate.getUTCDate();
    var month = GMONTHS[customDate.getUTCMonth()];
    var h = ("0" + customDate.getUTCHours()).slice(-2);
    var m = ("0" + customDate.getUTCMinutes()).slice(-2);

    return day + " " + month + " " + h + ":" + m;
}