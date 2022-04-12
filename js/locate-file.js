Module['locateFile'] = function(path, prefix) {
    if (path.endsWith(".wasm")) {
        let scope;
        if ('function' === typeof importScripts)
            scope = self;
        else
            scope = window;

        return scope.location.origin + "/dist/" + path;
    }

    return prefix + path;
}
