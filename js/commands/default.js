function TestCmd(args) {
    console.log("Новая команда запущена");
}

var moneyStat = 0;
function MoneyAdd(args) {
    console.log("Вы получили 100$");
    moneyStat += 100;
    console.log("Ваш баланс: " + moneyStat + "$");
}

function MoneyWhat(args) {
    console.log("Ваш баланс: " + moneyStat + "$");
}

function ADDITEM(args) {
    item.vmIndex = qvm.ui;
    item.register(args[1], args[2], args[3], args[4], 0, 1, 0);
    item.vmIndex = qvm.cgame;
    item.register(args[1], args[2], args[3], args[4], 0, 1, 0);
    item.vmIndex = qvm.game;
    item.register(args[1], args[2], args[3], args[4], 0, 1, 0);
}

command.register("testCmdJS", TestCmd);
command.register("addmoney", MoneyAdd);
command.register("moneycheck", MoneyWhat);
command.register("additem", ADDITEM);