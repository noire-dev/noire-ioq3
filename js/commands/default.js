function TestCmd(args) {
    console.log("Новая команда запущена");
}

var cmdPlace = [];

var moneyStat = 0;
function MoneyAdd(args) {
    if (cmdPlace[args[0]] === undefined) cmdPlace[args[0]] = {};
    if (cmdPlace[args[0]].moneyStat === undefined) cmdPlace[args[0]].moneyStat = 0;

    console.log("Вы получили 100$");
    cmdPlace[args[0]].moneyStat += 100;
    console.log("Ваш баланс: " + cmdPlace[args[0]].moneyStat + "$");
}

function MoneyWhat(args) {
    console.log("Ваш баланс: " + cmdPlace[args[0]].moneyStat + "$");
}

function ADDITEM(args) {
    item.vmIndex = qvm.ui;
    item.register(args[2], args[3], args[4], args[5], 0, 1, 0);
    item.vmIndex = qvm.cgame;
    item.register(args[2], args[3], args[4], args[5], 0, 1, 0);
    item.vmIndex = qvm.game;
    item.register(args[2], args[3], args[4], args[5], 0, 1, 0);
}

command.register("testCmdJS", TestCmd);
command.register("addmoney", MoneyAdd);
command.register("moneycheck", MoneyWhat);
command.register("additem", ADDITEM);