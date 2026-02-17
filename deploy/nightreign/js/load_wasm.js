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
            console.log('[WASM] Starting bootstrap process...');
            
            // 初始化 Bootstrap Loader
            BootstrapLoader.initialize(
                // 进度回调
                function(loaded, total) {
                    var percent = Math.floor(loaded / total * 100);
                    console.log('[WASM] Download progress: ' + loaded + '/' + total + ' (' + percent + '%)');
                    
                    // 更新简化的状态文本
                    var loadingStatus = document.getElementById('loading-status');
                    if (loadingStatus) {
                        loadingStatus.textContent = 'Loading files: ' + loaded + ' / ' + total + ' (' + percent + '%)';
                    }
                },
                // 阶段变化回调
                function(phase, message) {
                    console.log('[WASM] Phase: ' + phase + ' - ' + message);
                    
                    // 更新简化的状态文本
                    var loadingStatus = document.getElementById('loading-status');
                    if (loadingStatus) {
                        loadingStatus.textContent = message;
                    }
                },
                // 错误回调
                function(message, error) {
                    console.error('[WASM] Bootstrap error: ' + message, error);
                    
                    // 显示错误信息
                    var loadingStatus = document.getElementById('loading-status');
                    if (loadingStatus) {
                        loadingStatus.textContent = 'Error: ' + message;
                        loadingStatus.style.background = 'rgba(255, 68, 68, 0.9)';
                    }
                }
            );
        }
    ],
    onRuntimeInitialized: function () {
        console.log('[WASM] Runtime initialized');
        
        // 定期保存缓存到 IndexedDB (每 60 秒)
        setInterval(function() {
            FS.syncfs(false, function(err) {
                if (err) {
                    console.error('[WASM] Failed to sync cache to IndexedDB:', err);
                } else {
                    console.log('[WASM] Cache synced to IndexedDB');
                }
            });
        }, 60000);
        
        // 页面关闭时保存缓存
        window.addEventListener('beforeunload', function() {
            FS.syncfs(false, function(err) {
                if (err) {
                    console.error('[WASM] Failed to save cache on exit:', err);
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