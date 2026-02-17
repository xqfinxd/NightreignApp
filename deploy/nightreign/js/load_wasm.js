var canvasElement = document.getElementById('canvas');

// As a default initial behavior, pop up an alert when webgl context is lost. To make your
// application robust, you may want to override this behavior before shipping!
// See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
canvasElement.addEventListener('webglcontextlost', (e) => {
    alert('WebGL context lost. You will need to reload the page.');
    e.preventDefault();
}, false);

var Module = {
    canvas: canvasElement,
    setStatus: function (text) {
        console.log("status: " + text);
    },
    preRun: [
        function () {
            console.log('Preparing IndexedDB file system...');
            FS.mkdir('nightreign');
            FS.mount(IDBFS, {}, 'nightreign');
            // Synchronize from IndexedDB to memory
            FS.syncfs(true, function (err) {
                if (err) {
                    console.error('IDBFS sync from IndexedDB failed:', err);
                } else {
                    console.log('IDBFS sync from IndexedDB completed');
                }
            });
        }
    ],
    onRuntimeInitialized: function () {
        console.log('Runtime initialized');
        
        // 定期保存缓存到 IndexedDB (每 30 秒)
        setInterval(function() {
            FS.syncfs(false, function(err) {
                if (err) {
                    console.error('Failed to sync cache to IndexedDB:', err);
                } else {
                    console.log('Cache synced to IndexedDB');
                }
            });
        }, 30000);
        
        // 页面关闭时保存缓存
        window.addEventListener('beforeunload', function() {
            FS.syncfs(false, function(err) {
                if (err) {
                    console.error('Failed to save cache on exit:', err);
                }
            });
        });
    }
};
window.onerror = function (event) {
    console.log("onerror: " + event);
};

var xhr = new XMLHttpRequest();
xhr.open('GET', 'nightreign/NightreignApp.wasm', true)
xhr.responseType = 'arraybuffer';
xhr.onload = function () {
    Module.wasmBinary = xhr.response;
    var script = document.createElement('script');
    script.src = "nightreign/NightreignApp.js";
    document.body.appendChild(script);
};
xhr.send(null);