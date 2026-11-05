// game.vm
cvar.register("gamename", "Noire's Mod", 0);
cvar.register("gamedate", new Date().toISOString().slice(0, 10), 0);
// cgame.vm
cvar.register("physR", "128", CVAR.USERINFO | CVAR.ARCHIVE);
cvar.register("physG", "225", CVAR.USERINFO | CVAR.ARCHIVE);
cvar.register("physB", "255", CVAR.USERINFO | CVAR.ARCHIVE);
// ui.vm
// general
cvar.register("language", "en", CVAR.ARCHIVE);