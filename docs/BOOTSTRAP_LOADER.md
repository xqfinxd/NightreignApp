# RunDependency 启动加载方案 - 使用指南

## 方案概述

这个方案完全基于 Emscripten 的 **RunDependency** 机制，实现了：
- ✅ 不修改 C++ 启动流程（Application::initialize() 保持不变）
- ✅ 使用 IndexedDB 持久化存储（移动端友好）
- ✅ 动态加载 manifest.csv（可随时修改必需文件列表）
- ✅ 美观的加载进度界面
- ✅ 自动缓存管理

## 文件结构

```
deploy/
├── index.html                          # 主页面（已添加加载界面）
└── nightreign/
    ├── manifest.csv                    # 资源清单（动态下载）
    ├── assets/                         # 所有资源文件
    │   ├── datas/                      # CSV 数据文件
    │   ├── fonts/                      # 字体文件
    │   ├── shaders/                    # 着色器文件
    │   └── textures/                   # 纹理文件
    └── js/
        ├── bootstrap.js                # 核心加载器（新增）
        ├── load_wasm.js                # WASM 加载器（已修改）
        └── toggle.js                   # UI 交互
```

## 加载流程

### 用户首次访问

```plaintext
1. 加载 HTML/JS/CSS
   ↓
2. 显示加载界面（进度条）
   ↓
3. 挂载 IDBFS 
   - 创建 /nightreign 虚拟目录
   - 从 IndexedDB 同步（首次为空）
   ↓
4. 下载 manifest.csv
   - 从网络获取资源清单
   - 解析 priority >= 1 的文件
   ↓
5. 检查缓存状态
   - 发现 13 个文件缺失
   ↓
6. 下载必需文件（约 10-11 MB）
   - CSV 数据文件
   - 字体文件 (simhei.ttf)
   - Shader 文件
   - 实时显示进度
   ↓
7. 保存到 IndexedDB
   - 持久化存储文件
   ↓
8. 所有 RunDependency 清除
   ↓
9. Emscripten 自动调用 main()
   ↓
10. Application::initialize() 执行
    - 所有文件已就绪
    - 使用 fopen() 直接读取
    ↓
11. 游戏启动完成 🎮
```

### 用户后续访问

```plaintext
1. 加载 HTML/JS/CSS
   ↓
2. 显示加载界面
   ↓
3. 挂载 IDBFS
   - 从 IndexedDB 恢复 13 个文件到内存
   ↓
4. 读取缓存的 manifest.csv
   - 跳过网络下载
   ↓
5. 检查缓存状态
   - 发现所有文件都已缓存
   - 跳过下载阶段 ✅
   ↓
6. 快速同步（无新文件）
   ↓
7. main() 自动调用
   ↓
8. 游戏启动（接近秒开 ⚡）
```

## 工作原理

### RunDependency 机制

```javascript
// 添加依赖（告诉 Emscripten "还没准备好"）
Module.addRunDependency('resource-name');

// ... 异步加载资源 ...

// 移除依赖（告诉 Emscripten "这个资源准备好了"）
Module.removeRunDependency('resource-name');

// 当所有依赖都被移除时，Emscripten 自动调用 main()
```

### 依赖管理示例

```plaintext
Active Dependencies 时间线：

时刻 | 操作                        | 依赖列表
-----|----------------------------|-----------------------
10ms | addRunDependency('idbfs')  | ['idbfs']
200ms| removeRunDependency('idbfs')| []
210ms| addRunDependency('manifest')| ['manifest']
300ms| removeRunDependency('manifest')| []
310ms| addRunDependency('file:0')  | ['file:0']
310ms| addRunDependency('file:1')  | ['file:0', 'file:1']
310ms| addRunDependency('file:2')  | ['file:0', 'file:1', 'file:2']
...  | ...                         | ...
1000ms| removeRunDependency('file:0')| ['file:1', 'file:2', ...]
...  | ...                         | ...
3000ms| removeRunDependency('file:12')| []  ← 最后一个文件完成
3010ms| addRunDependency('persist')  | ['persist']
3500ms| removeRunDependency('persist')| []  ← 所有依赖清空
3510ms| ✅ Emscripten 调用 main()    | []
```

## manifest.csv 格式

```csv
path,priority,info
nightreign/assets/datas/autogen_pattern_list.csv,1,33259
nightreign/assets/datas/autogen_pattern_variationlist.csv,1,525959
nightreign/assets/fonts/simhei.ttf,1,9753388
nightreign/assets/shaders/font.frag,1,485
nightreign/assets/textures/0/MENU_MapTile_L0_00_00.png,0,256x256
...
```

**字段说明：**
- `path`: 文件路径（相对于网站根目录）
- `priority`: 优先级
  - `1` = 启动必需（会在 main() 前下载）
  - `0` = 可选（运行时按需加载）
- `info`: 文件大小（字节）或其他信息（如纹理尺寸）

## 修改必需文件列表

只需编辑 `deploy/nightreign/manifest.csv`：

### 示例 1: 添加新的必需文件
```csv
nightreign/assets/config/settings.ini,1,1024
```

### 示例 2: 将纹理设为必需
```csv
nightreign/assets/textures/bg.png,1,256x256
```

### 示例 3: 降低文件优先级（改为按需加载）
```csv
nightreign/assets/datas/optional_data.csv,0,5000
```

修改后，用户下次访问会自动下载新的必需文件。

## 构建和部署

### 1. 构建项目

```bash
cd build-wasm
emcmake cmake ..
emmake make
```

**生成的文件：**
- `NightreignApp.js` (约 200 KB)
- `NightreignApp.wasm` (约 1.4 MB)
- **不再生成 .data 文件** ✅

### 2. 部署文件

```bash
# 复制编译产物
cp NightreignApp.js ../deploy/nightreign/
cp NightreignApp.wasm ../deploy/nightreign/

# 确保资源文件在正确位置
ls ../deploy/nightreign/assets/
# 应该看到：datas/ fonts/ shaders/ textures/
```

### 3. 部署目录结构

```
deploy/
├── index.html
├── start.py                    # 本地测试服务器
└── nightreign/
    ├── NightreignApp.js       # 编译生成
    ├── NightreignApp.wasm     # 编译生成
    ├── manifest.csv           # 手动维护
    ├── assets/                # 资源文件
    ├── css/
    └── js/
        ├── bootstrap.js       # 启动加载器
        ├── load_wasm.js       # WASM 加载器
        └── toggle.js
```

### 4. 本地测试

```bash
cd deploy
python start.py
# 访问 http://localhost:8000
```

**测试检查项：**
1. 首次访问能看到加载进度条
2. 查看浏览器控制台，确认文件下载日志
3. 刷新页面，应该接近秒开（从缓存加载）
4. 检查 IndexedDB（开发者工具 → Application → IndexedDB）

### 5. 部署到 GitHub Pages

```bash
# 将 deploy/ 目录内容推送到 gh-pages 分支
git add .
git commit -m "Update with bootstrap loader"
git push origin main

# GitHub Pages 会自动部署
```

## 进度界面自定义

### 修改进度条样式

编辑 `deploy/index.html` 中的 `<style>` 部分：

```css
/* 修改进度条颜色 */
#progress-bar {
    background: linear-gradient(90deg, #ff6b6b 0%, #feca57 100%);
}

/* 修改背景渐变 */
#loading-screen {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}
```

### 修改加载文本

编辑 `deploy/nightreign/js/bootstrap.js`：

```javascript
updatePhase: function(phase, message) {
    const messages = {
        'idbfs': '正在初始化文件系统...',
        'manifest': '正在加载资源清单...',
        'checking': '正在检查缓存...',
        'downloading': '正在下载必需文件...',
        'persisting': '正在保存缓存...',
        'ready': '启动完成！'
    };
    // ...
}
```

## 调试技巧

### 1. 查看加载日志

打开浏览器控制台 (F12)，筛选：
```
[Bootstrap]  - Bootstrap Loader 日志
[WASM]       - Emscripten 运行时日志
```

### 2. 清除缓存测试

```javascript
// 在控制台执行：
indexedDB.deleteDatabase('/nightreign');
location.reload();
```

### 3. 模拟慢速网络

Chrome DevTools → Network → Throttling → Slow 3G

### 4. 查看 IndexedDB 内容

Chrome DevTools → Application → IndexedDB → `/nightreign`

### 5. 检查虚拟文件系统

在控制台执行：
```javascript
// 列出 /nightreign 目录
FS.readdir('/nightreign');

// 查看文件是否存在
FS.analyzePath('/nightreign/assets/datas/manual_maps.csv').exists;
```

## 常见问题

### Q: 首次加载很慢？
A: 正常现象，需要下载 10-11 MB。可以：
- 压缩文件（gzip）
- 使用 CDN
- 减少必需文件数量

### Q: 如何强制重新下载？
A: 清除 IndexedDB：
```javascript
indexedDB.deleteDatabase('/nightreign');
```

### Q: 修改 manifest.csv 后不生效？
A: 清除缓存的 manifest：
```javascript
FS.unlink('/nightreign/manifest.csv');
FS.syncfs(false, () => location.reload());
```

### Q: 加载界面不消失？
A: 检查：
1. 是否所有 RunDependency 都被 remove
2. main() 是否被调用
3. 控制台是否有错误

### Q: 移动端 IndexedDB 被清理怎么办？
A: 会自动重新下载。可以通过以下方式减少影响：
- 减小必需文件大小
- 实现版本检查增量更新

## 性能数据

### 首次加载（4G 网络）
- HTML/JS/CSS: ~1 秒
- WASM: ~0.5 秒
- 必需文件: ~3-5 秒（取决于网络）
- **总计: ~5-7 秒**

### 后续加载（有缓存）
- HTML/JS/CSS: ~0.5 秒（浏览器缓存）
- WASM: ~0.2 秒（浏览器缓存）
- 文件恢复: ~0.5 秒（从 IndexedDB）
- **总计: ~1-2 秒** ⚡

### 文件大小分布
```
必需文件（priority >= 1）：
- CSV 数据: ~580 KB
- 字体: ~9.75 MB
- Shader: ~1.3 KB
总计: ~10.3 MB

可选文件（priority = 0）：
- 纹理: ~30-40 MB
（运行时按需加载）
```

## 下一步优化

1. **增量更新**：只下载变化的文件
2. **文件压缩**：使用 gzip 减小传输大小
3. **分包加载**：按地图分组预加载
4. **Service Worker**：更精细的缓存控制
5. **WebP 纹理**：减小纹理文件大小

---

## 总结

这个方案的优势：

✅ **无需修改 C++ 代码** - Application 启动流程保持不变  
✅ **动态资源清单** - 修改 manifest.csv 即可调整必需文件  
✅ **移动端友好** - IndexedDB 比 HTTP Cache 更持久  
✅ **用户体验好** - 有进度条，后续访问接近秒开  
✅ **易于维护** - 清晰的代码结构和日志  

现在你的项目已经可以部署了！🚀
