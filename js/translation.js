const translation = {};
translation.data = {};

translation.add = function(string, lang, text) {
    if(!translation.data[string]) translation.data[string] = {};
    translation.data[string][lang] = text;
}

translation.get = function(string) {
    var lang = cvar.string("language");
    if(translation.data[string] && translation.data[string][lang]) return translation.data[string][lang];
    return string;
}

openjs.folder("js/translations", "translations");