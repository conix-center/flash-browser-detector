Module['locateFile'] = function(path, prefix) {
    if (path.endsWith(".wasm")) {
        var pathArray = self.location.href.split('/');

        var newPrefix = "";
        for (i = 0; i < pathArray.length - 1; i++) {
            newPrefix += pathArray[i];
            newPrefix += "/";
        }
        return newPrefix + path;
    }

    return prefix + path;
}
