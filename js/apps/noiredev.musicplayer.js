cvar.register("noiredev.musicplayer.prev", "0", 0);
cvar.register("noiredev.musicplayer.next", "0", 0);
cvar.register("noiredev.musicplayer.pause", "0", 0);
cvar.register("noiredev.musicplayer.albumSize", "4", CVAR.ARCHIVE);
cvar.register("noiredev.musicplayer.accentCoverBG", "0", CVAR.ARCHIVE);
cvar.register("noiredev.musicplayer.coverBGinPlaylist", "1", CVAR.ARCHIVE);
cvar.register("noiredev.musicplayer.coverBGinAlbums", "0", CVAR.ARCHIVE);
cvar.register("noiredev.musicplayer.notify", "1", CVAR.ARCHIVE);
cvar.register("noiredev.musicplayer.artwork", "artwork", CVAR.ARCHIVE);

function NoireMusic_Init(appID) {
    var id = ui.window(-1, app.list[appID].nameid, app.list[appID].name, app.list[appID].icon, 0, 512 + 20, 512 + 20 + 64, color.white, color.windowHeader, color.window);
    if (!ctx[id]) ctx[id] = {};

    ctx[id].self = id;
    ctx[id].close = ui.windowButton(id, -1, 0, "✕", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].close, NoireMusic_Exit);
    ctx[id].minimize = ui.windowButton(id, -1, 1, "−", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].minimize, NoireMusic_Minimize);
    ctx[id].pin = ui.windowButton(id, -1, 2, "⚲", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].pin, NoireMusic_Pin);
    ctx[id].link = ui.windowButton(id, -1, 3, "⌖", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].link, NoireMusic_Link);
    ctx[id].keyboard = ui.windowButton(id, -1, 4, "⌨", UI.BOLD, color.windowButton, 1.00);
    ui.func(id, ctx[id].keyboard, NoireMusic_Keyboard);

    ctx[id].playerMinimized = 0;
    cvar.set("s_musicEnded", "0");

    ctx[id].playerPage = 0;

    ctx[id].albumFolderScroll = 0;
    ctx[id].currentFolder = "";
    ctx[id].currentPlayingFolder = "";
    ctx[id].currentMusic = "";
    ctx[id].currentCover = "";
    ctx[id].currentPlaylist = [];
    ctx[id].currentPlaylistCover = [];
    ctx[id].currentPlaylistFolder = [];
    ctx[id].currentPlaylistLength = 0;
    ctx[id].currentPlaylistPosition = 0;
    ctx[id].currentPlayMode = 0;
    ctx[id].tabStartID = 50;
    ctx[id].tabEndID = 50;

    NoireMusic_SwitchPage(id);
    NoireMusic_SwitchBottom(id);

    var openedPlaylist = file.open("playlistLast.json");
    if (String(openedPlaylist).length > 4) NoireMusic_LoadPlaylist(id);
}

function NoireMusic_SwitchPage(id) {
    ui.clearWindow(id, ctx[id].tabStartID, ctx[id].tabEndID);
    ctx[id].tabEndID = 50;

    if (ctx[id].playerMinimized == 1) return;

    if (ctx[id].playerPage == 0) {
        if (ctx[id].currentFolder === "") {
            if (cvar.int("noiredev.musicplayer.coverBGinAlbums")) {
                if (cvar.int("noiredev.musicplayer.accentCoverBG")) ctx[id].bgCover = ui.picture(id, ctx[id].tabEndID, 0, 0, 512 + 20, 512 + 20, "music/" + ctx[id].currentPlayingFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.accent3);
                else ctx[id].bgCover = ui.picture(id, ctx[id].tabEndID, 0, 0, 512 + 20, 512 + 20, "music/" + ctx[id].currentPlayingFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.transparent32);
            }
            ctx[id].musicList = ui.listFiles(id, ctx[id].tabEndID += 1, 10, 10, 512 / cvar.int("noiredev.musicplayer.albumSize"), 512 / cvar.int("noiredev.musicplayer.albumSize"), 0.50, cvar.int("noiredev.musicplayer.albumSize"), cvar.int("noiredev.musicplayer.albumSize"), LSTYLE.GRID, LCONTENT.SHADER, 0);
            ui.setMargin(id, ctx[id].musicList, 15, 5, 15, 25);
            ui.fillListFiles(id, "music/", "/", "music/", 0);
            api.element(id, ctx[id].musicList, "scroll", ctx[id].albumFolderScroll);
            api.element(id, ctx[id].musicList, "style", UI.DROPSHADOW | UI.BOLD);
        } else {
            if (cvar.int("noiredev.musicplayer.coverBGinPlaylist")) {
                if (cvar.int("noiredev.musicplayer.accentCoverBG")) ctx[id].bgCover = ui.picture(id, ctx[id].tabEndID += 1, 0, 0, 512 + 20, 512 + 20, "music/" + ctx[id].currentFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.accent3);
                else ctx[id].bgCover = ui.picture(id, ctx[id].tabEndID += 1, 0, 0, 512 + 20, 512 + 20, "music/" + ctx[id].currentFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.transparent32);
            }
            ctx[id].topPanel = ui.button(id, ctx[id].tabEndID += 1, 0, 0, 512 + 20, 128, "", UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT | UI.NO_BOTTOM_RIGHT | UI.NO_BOTTOM_LEFT, color.windowItem, 1.00);
            ctx[id].coverArtShadow = ui.picture(id, ctx[id].tabEndID += 1, 12, 12, 108, 108, "music/" + ctx[id].currentFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.black);
            ctx[id].coverArt = ui.picture(id, ctx[id].tabEndID += 1, 10, 10, 108, 108, "music/" + ctx[id].currentFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.white);
            ctx[id].albumName = ui.button(id, ctx[id].tabEndID += 1, 10 + 108, 10, 512 - 128, 108, ctx[id].currentFolder, UI.BOLD | UI.DROPSHADOW, color.empty, 0.80);
            api.element(id, ctx[id].albumName, "hoverStyle", 0);
            ctx[id].addToPlaylist = ui.button(id, ctx[id].tabEndID += 1, 108, 108, 18, 18, "+", UI.CENTER, color.dark4, 1.00);
            api.element(id, ctx[id].addToPlaylist, "hoverStyle", UI.ACCENT);
            api.element(id, ctx[id].addToPlaylist, "baseCorner", 999999);
            ctx[id].musicList = ui.listFiles(id, ctx[id].tabEndID += 1, 10, 10 + 128, 512, 20, 0.50, 1, 19, LSTYLE.LIST, LCONTENT.SHADER, 0);
            ui.fillListFiles(id, "music/" + ctx[id].currentFolder, ".mp3", "", 0);
            api.element(id, ctx[id].musicList, "style", UI.DROPSHADOW | UI.BOLD);
        }
    }
    if (ctx[id].playerPage == 1) {
        var y = 10;
        ctx[id].playerSize = ui.slider(id, ctx[id].tabEndID, 10, y, 512, 20, "Album grid size", UI.BOLD | UI.LEFT, color.windowItem, 0.50, "noiredev.musicplayer.albumSize", 1, 8, EMODE.INT);
        ctx[id].albumAccent = ui.checkbox(id, ctx[id].tabEndID += 1, 10, y += 20 + 5, 512, 20, "Album background accent", UI.BOLD | UI.LEFT, color.windowItem, 0.50, "noiredev.musicplayer.accentCoverBG");
        ctx[id].playlistCoverBG = ui.checkbox(id, ctx[id].tabEndID += 1, 10, y += 20 + 5, 512, 20, "Album background in playlist view", UI.BOLD | UI.LEFT, color.windowItem, 0.50, "noiredev.musicplayer.coverBGinPlaylist");
        ctx[id].albumCoverBG = ui.checkbox(id, ctx[id].tabEndID += 1, 10, y += 20 + 5, 512, 20, "Album background in main view", UI.BOLD | UI.LEFT, color.windowItem, 0.50, "noiredev.musicplayer.coverBGinAlbums");
        ctx[id].doNotify = ui.checkbox(id, ctx[id].tabEndID += 1, 10, y += 20 + 5, 512, 20, "Track notifications", UI.BOLD | UI.LEFT, color.windowItem, 0.50, "noiredev.musicplayer.notify");
        ctx[id].artwork = ui.field(id, ctx[id].tabEndID += 1, 10, y += 20 + 5, 512, 20, "Album cover file name", UI.BOLD | UI.LEFT, color.windowItem, 0.50, "noiredev.musicplayer.artwork");
        ui.action(id, ctx[id].tabEndID += 1, 10, y += 20 + 5, 512, 20, "Previous track", UI.BOLD, color.windowItem, 0.50, "noiredev.musicplayer.prev 1");
        ui.action(id, ctx[id].tabEndID += 1, 10, y += 20 + 5, 512, 20, "Next track", UI.BOLD, color.windowItem, 0.50, "noiredev.musicplayer.next 1");
        ui.action(id, ctx[id].tabEndID += 1, 10, y += 20 + 5, 512, 20, "Pause / Play", UI.BOLD, color.windowItem, 0.50, "noiredev.musicplayer.pause 1");
        ctx[id].clearPlaylist = ui.button(id, ctx[id].tabEndID += 1, 10, 512 - 32, 246, 32, "Clear playlist", UI.BOLD | UI.CENTER, color.red3, 0.80);
        ctx[id].addAllToPlaylist = ui.button(id, ctx[id].tabEndID += 1, 276, 512 - 32, 246, 32, "Add all to playlist", UI.BOLD | UI.CENTER, color.accent3, 0.80);
    }
    if (ctx[id].playerPage == 2) {
        if (cvar.int("noiredev.musicplayer.accentCoverBG")) ctx[id].bgCover = ui.picture(id, ctx[id].tabEndID, 0, 0, 512 + 20, 512 + 20, "music/" + ctx[id].currentPlayingFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.accent3);
        else ctx[id].bgCover = ui.picture(id, ctx[id].tabEndID, 0, 0, 512 + 20, 512 + 20, "music/" + ctx[id].currentPlayingFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.transparent128);
        ctx[id].topPanel = ui.button(id, ctx[id].tabEndID += 1, 0, 0, 512 + 20, 164, "", UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT | UI.NO_BOTTOM_RIGHT | UI.NO_BOTTOM_LEFT, color.windowItem, 1.00);
        ctx[id].albumName = ui.button(id, ctx[id].tabEndID += 1, 10, 10, 512 - 128, 108, ctx[id].currentPlayingFolder, UI.BOLD | UI.DROPSHADOW, color.empty, 0.80);
        ctx[id].trackName = ui.button(id, ctx[id].tabEndID += 1, 10, 10, 512 - 128, 188, ctx[id].currentMusic, UI.BOLD | UI.DROPSHADOW, color.empty, 0.60);
    }
}

function NoireMusic_SwitchBottom(id) {
    if (ctx[id].playerMinimized == 0) var y = 512;
    else var y = -20;
    if (ctx[id].playerMinimized == 0) ctx[id].bottomPanel = ui.button(id, 10, 0, y + 20, 512 + 20, 64, "", UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT | UI.NO_BOTTOM_RIGHT | UI.NO_BOTTOM_LEFT, color.windowSide, 1.00);
    else ctx[id].bottomPanel = ui.button(id, 10, 0, y + 20, 512 + 20, 64, "", UI.NO_TOP_LEFT | UI.NO_TOP_RIGHT, color.windowSide, 1.00);
    ctx[id].coverPlayingArt = ui.picture(id, 11, 10, y + 20 + 10, 44, 44, "music/" + ctx[id].currentPlayingFolder + "/" + cvar.string("noiredev.musicplayer.artwork"), 0, color.white);
    ctx[id].trackName = ui.button(id, 12, 10 + 44, y + 20 + 10, 380, 44, ctx[id].currentMusic, UI.BOLD, color.empty, 0.50);
    ctx[id].playMode = ui.button(id, 14, 532 - 130, y + 30 + 10, 24, 24, "", UI.BOLD | UI.CENTER, color.empty, 0.80);
    api.element(id, ctx[id].playMode, "hoverStyle", 0);
    ctx[id].prevTrack = ui.button(id, 15, 532 - 130 + 24, y + 30 + 10, 24, 24, "\u23EE", UI.BOLD | UI.CENTER, color.empty, 0.80);
    api.element(id, ctx[id].prevTrack, "hoverStyle", 0);
    ctx[id].pauseTrack = ui.button(id, 16, 532 - 130 + 48, y + 30 + 10, 24, 24, "", UI.BOLD | UI.CENTER, color.empty, 0.80);
    api.element(id, ctx[id].pauseTrack, "hoverStyle", 0);
    ctx[id].nextTrack = ui.button(id, 17, 532 - 130 + 72, y + 30 + 10, 24, 24, "\u23ED", UI.BOLD | UI.CENTER, color.empty, 0.80);
    api.element(id, ctx[id].nextTrack, "hoverStyle", 0);
    ctx[id].settingsBtn = ui.button(id, 18, 532 - 130 + 96, y + 30 + 10, 24, 24, "&e\u2699", UI.BOLD | UI.CENTER, color.empty, 0.80);
    api.element(id, ctx[id].settingsBtn, "hoverStyle", 0);
    ctx[id].playerVolume = ui.slider(id, 19, 532 - 130 - 112, y + 30 + 36, 120 * 2, 12, "", UI.BOLD, color.empty, 0.36, "s_musicVolume", 0, 1.5, EMODE.FLOAT);
    api.element(id, ctx[id].playerVolume, "hoverStyle", 0);
}

function NoireMusic_Key(key, id) {
    if (key == KEY.SPACE) NoireMusic_Call(id, ctx[id].pauseTrack, KEY.MOUSE1);
    if (key == KEY.LEFTARROW) NoireMusic_Call(id, ctx[id].prevTrack, KEY.MOUSE1);
    if (key == KEY.RIGHTARROW) NoireMusic_Call(id, ctx[id].nextTrack, KEY.MOUSE1);
}

function NoireMusic_Call(id, eid, key) {
    if (key != KEY.MOUSE1) return;
    if (eid == ctx[id].musicList) {
        if (ctx[id].currentFolder === "") {
            ctx[id].albumFolderScroll = api.element(id, ctx[id].musicList, "scroll");
            ctx[id].currentFolder = api.element(id, ctx[id].musicList, "field");
            NoireMusic_SwitchPage(id);
        } else {
            ctx[id].currentCover = "music/" + ctx[id].currentFolder + "/" + cvar.string("noiredev.musicplayer.artwork");
            ctx[id].currentMusic = api.element(id, ctx[id].musicList, "field");
            ctx[id].currentPlayingFolder = ctx[id].currentFolder;
            qvm.cmd(qvm.ui, EXEC.INSERT, "music \"music/" + ctx[id].currentFolder + "/" + api.element(id, ctx[id].musicList, "field") + "\"");
            qvm.cmd(qvm.ui, EXEC.INSERT, "s_musicSpeed 1");
            NoireMusic_SwitchBottom(id);
            if (cvar.int("noiredev.musicplayer.notify")) cg.notify(ctx[id].currentMusic, NOTIFY.CUSTOM, 0, "music/" + ctx[id].currentPlayingFolder + "/" + cvar.string("noiredev.musicplayer.artwork"));
            ctx[id].currentPlaylistLength = 0;
            for (var i = 0; i < ui.getFileListCount(id, 0); i++) {
                ctx[id].currentPlaylist[i] = ui.getFileFromList(id, 0, i);
                ctx[id].currentPlaylistCover[i] = "music/" + ctx[id].currentFolder + "/" + cvar.string("noiredev.musicplayer.artwork");
                ctx[id].currentPlaylistFolder[i] = ctx[id].currentFolder;
                ctx[id].currentPlaylistLength += 1;
            }
            for (var i = 0; i < ctx[id].currentPlaylistLength; i++) {
                if (ctx[id].currentPlaylist[i] === ctx[id].currentMusic) {
                    ctx[id].currentPlaylistPosition = i;
                    break;
                }
            }
            api.window(id, "name", "Music Player - " + ctx[id].currentPlaylistLength);
        }
    }
    if (eid == ctx[id].addAllToPlaylist) {
        ctx[id].playerPage = 0;
        ctx[id].currentFolder = "";
        NoireMusic_SwitchPage(id);
        ctx[id].currentPlaylistLength = 0;
        for (var i = 0; i < ui.getFileListCount(id, 0); i++) {
            if (ui.getFileFromList(id, 0, i) === "" || ui.getFileFromList(id, 0, i) === "." || ui.getFileFromList(id, 0, i) === "..") continue;
            ui.fillListFiles(id, "music/" + ui.getFileFromList(id, 0, i), ".mp3", "", 1);
            var oldPlaylistLength = ctx[id].currentPlaylistLength;
            for (var j = 0; j < ui.getFileListCount(id, 1); j++) {
                ctx[id].currentPlaylist[j + oldPlaylistLength] = ui.getFileFromList(id, 1, j);
                ctx[id].currentPlaylistCover[j + oldPlaylistLength] = "music/" + ui.getFileFromList(id, 0, i) + "/" + cvar.string("noiredev.musicplayer.artwork");
                ctx[id].currentPlaylistFolder[j + oldPlaylistLength] = ui.getFileFromList(id, 0, i);
                ctx[id].currentPlaylistLength += 1;
            }
        }
        if (ctx[id].currentMusic === "") {
            ctx[id].currentPlaylistPosition = -1;
            NoireMusic_Call(id, ctx[id].nextTrack, KEY.MOUSE1);
        }
        api.window(id, "name", "Music Player - " + ctx[id].currentPlaylistLength);
    }
    if (eid == ctx[id].clearPlaylist) {
        ctx[id].playerPage = 0;
        ctx[id].currentFolder = "";
        NoireMusic_SwitchPage(id);
        ctx[id].currentPlaylist = [];
        ctx[id].currentPlaylistCover = [];
        ctx[id].currentPlaylistFolder = [];
        ctx[id].currentPlaylistLength = 0;
        ctx[id].currentPlaylistPosition = 0;
        api.window(id, "name", "Music Player");
    }
    if (eid == ctx[id].addToPlaylist) {
        var oldPlaylistLength = ctx[id].currentPlaylistLength;
        var duplicateFound = 0;
        for (var i = 0; i < ui.getFileListCount(id, 0); i++) {
            for (var j = 0; j < ctx[id].currentPlaylistLength; j++) {
                if (ctx[id].currentPlaylist[j] === ui.getFileFromList(id, 0, i)) {
                    duplicateFound = 1;
                    break;
                }
                duplicateFound = 0;
            }
            if (!duplicateFound) {
                ctx[id].currentPlaylist[i + oldPlaylistLength] = ui.getFileFromList(id, 0, i);
                ctx[id].currentPlaylistCover[i + oldPlaylistLength] = "music/" + ctx[id].currentFolder + "/" + cvar.string("noiredev.musicplayer.artwork");
                ctx[id].currentPlaylistFolder[i + oldPlaylistLength] = ctx[id].currentFolder;
                ctx[id].currentPlaylistLength += 1;
            }
        }
        if (ctx[id].currentMusic === "") {
            ctx[id].currentPlaylistPosition = -1;
            NoireMusic_Call(id, ctx[id].nextTrack, KEY.MOUSE1);
        }
        api.window(id, "name", "Music Player - " + ctx[id].currentPlaylistLength);
    }
    if (eid == ctx[id].coverArt) {
        ctx[id].currentFolder = "";
        NoireMusic_SwitchPage(id);
    }
    if (eid == ctx[id].coverPlayingArt || eid == ctx[id].trackName) {
        ctx[id].albumFolderScroll = api.element(id, ctx[id].musicList, "scroll");
        ctx[id].currentFolder = ctx[id].currentPlayingFolder;
        NoireMusic_SwitchPage(id);
    }
    if (eid == ctx[id].prevTrack && ctx[id].currentPlaylistLength) {
        if (ctx[id].currentPlayMode == 0) ctx[id].currentPlaylistPosition -= 1;
        if (ctx[id].currentPlayMode == 1) ctx[id].currentPlaylistPosition = Math.floor(Math.random() * ctx[id].currentPlaylistLength);
        if (ctx[id].currentPlaylistPosition < 0) ctx[id].currentPlaylistPosition = ctx[id].currentPlaylistLength - 1;
        ctx[id].currentMusic = ctx[id].currentPlaylist[ctx[id].currentPlaylistPosition];
        ctx[id].currentPlayingFolder = ctx[id].currentPlaylistFolder[ctx[id].currentPlaylistPosition];
        ctx[id].currentCover = ctx[id].currentPlaylistCover[ctx[id].currentPlaylistPosition];
        qvm.cmd(qvm.ui, EXEC.INSERT, "music \"music/" + ctx[id].currentPlayingFolder + "/" + ctx[id].currentPlaylist[ctx[id].currentPlaylistPosition] + "\"");
        NoireMusic_SwitchBottom(id);
        ctx[id].albumFolderScroll = api.element(id, ctx[id].musicList, "scroll");
        NoireMusic_SwitchPage(id);
        if (cvar.int("noiredev.musicplayer.notify")) cg.notify(ctx[id].currentMusic, NOTIFY.CUSTOM, 0, "music/" + ctx[id].currentPlayingFolder + "/" + cvar.string("noiredev.musicplayer.artwork"));
    }
    if (eid == ctx[id].nextTrack && ctx[id].currentPlaylistLength) {
        if (ctx[id].currentPlayMode == 0) ctx[id].currentPlaylistPosition += 1;
        if (ctx[id].currentPlayMode == 1) ctx[id].currentPlaylistPosition = Math.floor(Math.random() * ctx[id].currentPlaylistLength);
        if (ctx[id].currentPlaylistPosition >= ctx[id].currentPlaylistLength) ctx[id].currentPlaylistPosition = 0;
        ctx[id].currentMusic = ctx[id].currentPlaylist[ctx[id].currentPlaylistPosition];
        ctx[id].currentPlayingFolder = ctx[id].currentPlaylistFolder[ctx[id].currentPlaylistPosition];
        ctx[id].currentCover = ctx[id].currentPlaylistCover[ctx[id].currentPlaylistPosition];
        qvm.cmd(qvm.ui, EXEC.INSERT, "music \"music/" + ctx[id].currentPlayingFolder + "/" + ctx[id].currentPlaylist[ctx[id].currentPlaylistPosition] + "\"");
        NoireMusic_SwitchBottom(id);
        ctx[id].albumFolderScroll = api.element(id, ctx[id].musicList, "scroll");
        NoireMusic_SwitchPage(id);
        if (cvar.int("noiredev.musicplayer.notify")) cg.notify(ctx[id].currentMusic, NOTIFY.CUSTOM, 0, "music/" + ctx[id].currentPlayingFolder + "/" + cvar.string("noiredev.musicplayer.artwork"));
    }
    if (eid == ctx[id].pauseTrack && ctx[id].currentPlaylistLength) {
        qvm.cmd(qvm.ui, EXEC.NOW, "toggle s_musicSpeed");
        if (cvar.int("s_musicSpeed") && cvar.int("noiredev.musicplayer.notify")) cg.notify("Music play", NOTIFY.INFO, 0, "");
        if (!cvar.int("s_musicSpeed") && cvar.int("noiredev.musicplayer.notify")) cg.notify("Music pause", NOTIFY.INFO, 0, "");
    }
    if (eid == ctx[id].playMode) {
        if (ctx[id].currentPlayMode == 0) ctx[id].currentPlayMode = 1;
        else if (ctx[id].currentPlayMode == 1) ctx[id].currentPlayMode = 2;
        else if (ctx[id].currentPlayMode == 2) ctx[id].currentPlayMode = 0;
    }
    if (eid == ctx[id].settingsBtn) {
        if (ctx[id].playerPage == 0) ctx[id].playerPage = 1;
        else if (ctx[id].playerPage == 1) ctx[id].playerPage = 2;
        else if (ctx[id].playerPage == 2) ctx[id].playerPage = 0;
        NoireMusic_SwitchPage(id);
    }
}

function NoireMusic_Update(id) {
    if (api.window(id, "minimized")) api.element(id, ctx[id].minimize, "colorBackground", color.accent3);
    else api.element(id, ctx[id].minimize, "colorBackground", color.windowButton);
    if (api.window(id, "pinned")) api.element(id, ctx[id].pin, "colorBackground", color.accent3);
    else api.element(id, ctx[id].pin, "colorBackground", color.windowButton);
    if (api.window(id, "linked")) api.element(id, ctx[id].link, "colorBackground", color.accent3);
    else api.element(id, ctx[id].link, "colorBackground", color.windowButton);
    if (api.window(id, "keyboardCapture")) api.element(id, ctx[id].keyboard, "colorBackground", color.accent3);
    else api.element(id, ctx[id].keyboard, "colorBackground", color.windowButton);

    if (cvar.float("s_musicSpeed") == 1.00) {
        api.element(id, ctx[id].pauseTrack, "text", "\u{23F8}");
    }
    if (cvar.float("s_musicSpeed") == 0.00) {
        api.element(id, ctx[id].pauseTrack, "text", "\u{25B6}");
    }

    if (ctx[id].playerMinimized == 0) {
        api.window(id, "baseH", 512 + 20 + 64);
    } else {
        api.window(id, "baseH", 64);
    }

    if (ctx[id].currentPlayMode == 0) api.element(id, ctx[id].playMode, "text", "\u{1F501}");
    if (ctx[id].currentPlayMode == 1) api.element(id, ctx[id].playMode, "text", "\u{1F500}");
    if (ctx[id].currentPlayMode == 2) api.element(id, ctx[id].playMode, "text", "\u{1F502}");
}

function NoireMusic_BackgroundUpdate(id) {
    if (cvar.int("s_musicEnded")) {
        cvar.set("s_musicEnded", "0");
        NoireMusic_Call(id, ctx[id].nextTrack, KEY.MOUSE1);
    }

    if (cvar.int("noiredev.musicplayer.prev")) {
        cvar.set("noiredev.musicplayer.prev", "0");
        NoireMusic_Call(id, ctx[id].prevTrack, KEY.MOUSE1);
    }

    if (cvar.int("noiredev.musicplayer.next")) {
        cvar.set("noiredev.musicplayer.next", "0");
        NoireMusic_Call(id, ctx[id].nextTrack, KEY.MOUSE1);
    }

    if (cvar.int("noiredev.musicplayer.pause")) {
        cvar.set("noiredev.musicplayer.pause", "0");
        NoireMusic_Call(id, ctx[id].pauseTrack, KEY.MOUSE1);
    }
}

function NoireMusic_Shutdown(id) {
    NoireMusic_SavePlaylist(id);
}

function NoireMusic_Exit(id) {
    NoireMusic_Shutdown(id);
    ui.closeWindow(ctx[id].self);
    delete ctx[id];
}

function NoireMusic_Minimize(id) {
    if (ctx[id].playerMinimized) ctx[id].playerMinimized = 0;
    else ctx[id].playerMinimized = 1;

    NoireMusic_SwitchPage(id);
    NoireMusic_SwitchBottom(id);
}

function NoireMusic_Pin(id) {
    if (api.window(id, "pinned")) api.window(id, "pinned", 0);
    else api.window(id, "pinned", 1);
}

function NoireMusic_Link(id) {
    if (api.window(id, "linked")) {
        api.window(id, "linked", 0);
    } else {
        ui.teleportWindowToPlayer(id, 128);
        api.window(id, "linked", 1);
    }
}

function NoireMusic_Keyboard(id) {
    if (api.window(id, "keyboardCapture")) api.window(id, "keyboardCapture", 0);
    else api.window(id, "keyboardCapture", 1);
}

function NoireMusic_SavePlaylist(id) {
    var dataToSave = {
        currentPlaylist: ctx[id].currentPlaylist,
        currentPlaylistCover: ctx[id].currentPlaylistCover,
        currentPlaylistFolder: ctx[id].currentPlaylistFolder,
        currentPlaylistLength: ctx[id].currentPlaylistLength,
        currentPlaylistPosition: ctx[id].currentPlaylistPosition,
    }
    var jsonToSave = JSON.stringify(dataToSave, null, 4);
    file.save("playlistLast.json", jsonToSave);
}

function NoireMusic_LoadPlaylist(id) {
    var dataToLoad = file.open("playlistLast.json");
    var jsonToLoad = JSON.parse(dataToLoad);

    ctx[id].currentPlaylist = jsonToLoad.currentPlaylist,
        ctx[id].currentPlaylistCover = jsonToLoad.currentPlaylistCover,
        ctx[id].currentPlaylistFolder = jsonToLoad.currentPlaylistFolder,
        ctx[id].currentPlaylistLength = jsonToLoad.currentPlaylistLength,
        ctx[id].currentPlaylistPosition = jsonToLoad.currentPlaylistPosition,

        ctx[id].currentPlaylistPosition -= 1;
    NoireMusic_Call(id, ctx[id].nextTrack, KEY.MOUSE1);
    NoireMusic_Call(id, ctx[id].pauseTrack, KEY.MOUSE1);
    api.window(id, "name", "Music Player - " + ctx[id].currentPlaylistLength);
}

app.register("noiredev.musicplayer", "Music Player", NoireMusic_Init, NoireMusic_Key, NoireMusic_Call, NoireMusic_Update, NoireMusic_BackgroundUpdate, NoireMusic_Shutdown);