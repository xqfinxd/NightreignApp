/**
 * NightreignApp Bootstrap Loader
 * 使用 Emscripten RunDependency 机制管理资源加载
 */

(function() {
    'use strict';

    const BootstrapLoader = {
        // 配置
        config: {
            manifestUrl: 'nightreign/manifest.json',
            retryAttempts: 3,
            retryDelay: 1000
        },

        // 状态
        state: {
            essentialFiles: [],      // 必需文件列表
            downloadQueue: [],       // 需要下载的文件
            loadedCount: 0,
            totalCount: 0,
            currentPhase: 'init'     // init, idbfs, manifest, checking, downloading, persisting, ready
        },

        // 进度回调
        callbacks: {
            onProgress: null,
            onPhaseChange: null,
            onError: null
        },

        /**
         * 初始化 bootstrap 流程
         */
        initialize: function(progressCallback, phaseChangeCallback, errorCallback) {
            console.log('[Bootstrap] Initializing...');
            
            this.callbacks.onProgress = progressCallback;
            this.callbacks.onPhaseChange = phaseChangeCallback;
            this.callbacks.onError = errorCallback;

            // 开始加载流程
            this.mountIDBFS();
        },

        /**
         * 阶段 1: 挂载 IDBFS 并从 IndexedDB 恢复
         */
        mountIDBFS: function() {
            console.log('[Bootstrap] Phase 1: Mounting IDBFS...');
            this.updatePhase('idbfs', 'Initializing file system...');

            Module.addRunDependency('idbfs-mount');

            try {
                FS.mkdir('nightreign');
                FS.mount(IDBFS, {}, 'nightreign');

                // 从 IndexedDB 同步到内存
                FS.syncfs(true, (err) => {
                    if (err) {
                        console.warn('[Bootstrap] IDBFS sync from IndexedDB failed (first run?):', err);
                    } else {
                        console.log('[Bootstrap] IDBFS sync from IndexedDB completed');
                    }
                    
                    // 先添加下一个 dependency 再移除当前的，避免 dependency 计数为 0
                    Module.addRunDependency('manifest.json');
                    Module.removeRunDependency('idbfs-mount');
                    
                    this.loadManifest();
                });
            } catch (e) {
                console.error('[Bootstrap] IDBFS mount error:', e);
                Module.removeRunDependency('idbfs-mount');
                this.handleError('Failed to mount file system', e);
            }
        },

        /**
         * 阶段 2: 下载并解析 manifest.json
         */
        loadManifest: function() {
            console.log('[Bootstrap] Phase 2: Loading manifest...');
            this.updatePhase('manifest', 'Loading resource manifest...');

            // 注意：dependency 已经在 mountIDBFS 中添加了

            // 直接从网络下载 manifest（不缓存，因为文件很小）
            this.downloadFile(this.config.manifestUrl, (data) => {
                try {
                    const content = new TextDecoder('utf-8').decode(data);
                    console.log('[Bootstrap] Manifest downloaded');
                    
                    this.parseManifest(content);
                    
                    // 检查是否需要下载文件，并预先添加 dependencies
                    this.prepareDownload();
                    
                    // 现在可以安全地移除 manifest dependency
                    Module.removeRunDependency('manifest.json');
                    
                    // 执行缓存检查和下载
                    this.checkCache();
                } catch (e) {
                    console.error('[Bootstrap] Failed to process manifest:', e);
                    Module.removeRunDependency('manifest.json');
                    this.handleError('Failed to process manifest', e);
                }
            }, (error) => {
                console.error('[Bootstrap] Failed to download manifest:', error);
                Module.removeRunDependency('manifest.json');
                this.handleError('Failed to download manifest', error);
            });
        },

        /**
         * 解析 manifest.json
         */
        parseManifest: function(content) {
            console.log('[Bootstrap] Parsing manifest...');

            const entries = JSON.parse(content);
            this.state.essentialFiles = [];

            for (const entry of entries) {
                if (entry.priority >= 1) {
                    this.state.essentialFiles.push({
                        path: entry.path,
                        priority: entry.priority,
                        crc: entry.crc,
                    });
                }
            }

            this.state.totalCount = this.state.essentialFiles.length;
            console.log(`[Bootstrap] Found ${this.state.totalCount} essential files`);
        },

        /**
         * 准备下载：提前添加所有必要的 RunDependencies
         */
        prepareDownload: function() {
            // 预检查哪些文件缺失
            const missingFiles = [];
            for (const file of this.state.essentialFiles) {
                const fullPath = '/' + file.path;
                try {
                    const stat = FS.analyzePath(fullPath);
                    if (!stat.exists) {
                        missingFiles.push(file);
                    }
                } catch (e) {
                    missingFiles.push(file);
                }
            }

            if (missingFiles.length > 0) {
                // 提前为所有缺失文件添加 RunDependency
                console.log(`[Bootstrap] Pre-adding dependencies for ${missingFiles.length} files`);
                for (const file of missingFiles) {
                    Module.addRunDependency('file:' + file.path);
                }
            } else {
                // 所有文件都已缓存，添加 persist dependency
                Module.addRunDependency('idbfs-persist');
            }
        },

        /**
         * 计算 CRC32（与 Python zlib.crc32 一致，返回 8 位小写十六进制字符串）
         */
        computeCRC32: function(data) {
            // 预计算查找表
            if (!this._crc32Table) {
                const table = new Uint32Array(256);
                for (let i = 0; i < 256; i++) {
                    let c = i;
                    for (let j = 0; j < 8; j++) {
                        c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
                    }
                    table[i] = c;
                }
                this._crc32Table = table;
            }
            const table = this._crc32Table;
            let crc = 0xFFFFFFFF;
            for (let i = 0; i < data.length; i++) {
                crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >>> 8);
            }
            return ((crc ^ 0xFFFFFFFF) >>> 0).toString(16).padStart(8, '0');
        },

        /**
         * 阶段 3: 检查文件缓存状态（含 CRC 校验）
         */
        checkCache: function() {
            console.log('[Bootstrap] Phase 3: Checking cache...');
            this.updatePhase('checking', 'Checking cached files...');

            this.state.downloadQueue = [];

            for (const file of this.state.essentialFiles) {
                const fullPath = '/' + file.path;
                
                try {
                    const stat = FS.analyzePath(fullPath);
                    if (stat.exists) {
                        // 文件存在时校验 CRC
                        if (file.crc) {
                            try {
                                const existingData = FS.readFile(fullPath);
                                const actualCrc = this.computeCRC32(existingData);
                                if (actualCrc === file.crc) {
                                    console.log(`[Bootstrap] Cached (CRC OK): ${file.path}`);
                                } else {
                                    console.warn(`[Bootstrap] CRC mismatch: ${file.path} (expected=${file.crc}, actual=${actualCrc}), re-downloading`);
                                    this.state.downloadQueue.push(file);
                                }
                            } catch (e) {
                                console.warn(`[Bootstrap] CRC check failed for ${file.path}:`, e);
                                this.state.downloadQueue.push(file);
                            }
                        } else {
                            console.log(`[Bootstrap] Cached: ${file.path}`);
                        }
                    } else {
                        console.log(`[Bootstrap] Missing: ${file.path}`);
                        this.state.downloadQueue.push(file);
                    }
                } catch (e) {
                    console.log(`[Bootstrap] Missing: ${file.path}`);
                    this.state.downloadQueue.push(file);
                }
            }

            const cachedCount = this.state.totalCount - this.state.downloadQueue.length;
            console.log(`[Bootstrap] Cache check: ${cachedCount}/${this.state.totalCount} files cached`);

            if (this.state.downloadQueue.length === 0) {
                console.log('[Bootstrap] All files cached, skipping download phase');
                // dependency 已在 prepareDownload 中添加
                this.persistToIDBFS();
            } else {
                // dependencies 已在 prepareDownload 中添加
                this.downloadMissingFiles();
            }
        },

        /**
         * 阶段 4: 下载缺失的文件
         */
        downloadMissingFiles: function() {
            console.log(`[Bootstrap] Phase 4: Downloading ${this.state.downloadQueue.length} files...`);
            this.updatePhase('downloading', `Downloading files (0/${this.state.downloadQueue.length})...`);

            this.state.loadedCount = 0;

            // 注意：RunDependency 已经在 prepareDownload 中添加了

            // 串行下载（避免过多并发请求）
            this.downloadNextFile(0);
        },

        /**
         * 下载下一个文件（串行）
         */
        downloadNextFile: function(index) {
            if (index >= this.state.downloadQueue.length) {
                console.log('[Bootstrap] All files downloaded');
                // 在调用 persistToIDBFS 之前添加 dependency
                Module.addRunDependency('idbfs-persist');
                this.persistToIDBFS();
                return;
            }

            const file = this.state.downloadQueue[index];
            const fileName = file.path.split('/').pop();
            
            console.log(`[Bootstrap] Downloading [${index + 1}/${this.state.downloadQueue.length}] ${fileName}...`);
            
            this.updatePhase('downloading', 
                `Downloading ${fileName} (${index + 1}/${this.state.downloadQueue.length})...`);

            this.downloadFile(file.path, 
                // Success
                (data) => {
                    this.saveFile(file.path, data);
                    this.state.loadedCount++;
                    
                    this.updateProgress(this.state.loadedCount, this.state.downloadQueue.length);
                    
                    Module.removeRunDependency('file:' + file.path);
                    
                    // 下载下一个文件
                    this.downloadNextFile(index + 1);
                },
                // Error
                (error) => {
                    console.error(`[Bootstrap] Failed to download ${file.path}:`, error);
                    // 即使失败也继续下载（可以选择重试）
                    Module.removeRunDependency('file:' + file.path);
                    this.downloadNextFile(index + 1);
                },
                // 可选的尝试次数
                0
            );
        },

        /**
         * 下载文件（底层函数）
         */
        downloadFile: function(url, onSuccess, onError, attempt = 0) {
            fetch(url)
                .then(response => {
                    if (!response.ok) {
                        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
                    }
                    return response.arrayBuffer();
                })
                .then(arrayBuffer => {
                    onSuccess(new Uint8Array(arrayBuffer));
                })
                .catch(error => {
                    if (attempt < this.config.retryAttempts) {
                        console.warn(`[Bootstrap] Retry ${attempt + 1}/${this.config.retryAttempts} for ${url}`);
                        setTimeout(() => {
                            this.downloadFile(url, onSuccess, onError, attempt + 1);
                        }, this.config.retryDelay);
                    } else {
                        onError(error);
                    }
                });
        },

        /**
         * 保存文件到虚拟文件系统
         */
        saveFile: function(path, data) {
            const fullPath = '/' + path;
            
            // 创建父目录
            const dirs = fullPath.split('/');
            let currentPath = '';
            for (let i = 1; i < dirs.length - 1; i++) {
                currentPath += '/' + dirs[i];
                try {
                    FS.mkdir(currentPath);
                } catch (e) {
                    // 目录已存在
                }
            }

            // 写入文件
            FS.writeFile(fullPath, data);
            console.log(`[Bootstrap] Saved: ${path} (${data.length} bytes)`);
        },

        /**
         * 阶段 5: 持久化到 IndexedDB
         */
        persistToIDBFS: function() {
            console.log('[Bootstrap] Phase 5: Persisting to IndexedDB...');
            this.updatePhase('persisting', 'Saving cache...');

            // 注意：dependency 已经提前添加了

            // 从内存同步到 IndexedDB
            FS.syncfs(false, (err) => {
                if (err) {
                    console.warn('[Bootstrap] IndexedDB sync failed:', err);
                } else {
                    console.log('[Bootstrap] IndexedDB sync completed');
                }
                
                Module.removeRunDependency('idbfs-persist');
                this.complete();
            });
        },

        /**
         * 完成启动流程
         */
        complete: function() {
            console.log('[Bootstrap] Phase 6: Bootstrap complete! 🚀');
            this.updatePhase('ready', 'Ready!');
            
            // 此时所有 RunDependency 已清除，Emscripten 会自动调用 main()
        },

        /**
         * 更新阶段
         */
        updatePhase: function(phase, message) {
            this.state.currentPhase = phase;
            if (this.callbacks.onPhaseChange) {
                this.callbacks.onPhaseChange(phase, message);
            }
        },

        /**
         * 更新进度
         */
        updateProgress: function(loaded, total) {
            if (this.callbacks.onProgress) {
                this.callbacks.onProgress(loaded, total);
            }
        },

        /**
         * 错误处理
         */
        handleError: function(message, error) {
            console.error(`[Bootstrap] Error: ${message}`, error);
            if (this.callbacks.onError) {
                this.callbacks.onError(message, error);
            }
        }
    };

    // 导出到全局
    window.BootstrapLoader = BootstrapLoader;

})();
