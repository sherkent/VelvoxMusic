# Velvox Music — Project Context

## 项目概述

- **名称**：Velvox Music
- **平台**：Windows 桌面本地音乐播放器
- **技术栈**：Qt 6.8+ / QML / C++20 / CMake 3.21+
- **音频**：miniaudio（解码 + 播放，商业安全）
- **元数据**：TagLib（LGPL，需动态链接）
- **本地数据库**：QSQLITE
- **UI 方向**：现代、高颜值、动态网格渐变、高斯模糊玻璃拟态、流畅动画
- **硬件目标**：AMD 核显笔记本 / 16GB 内存，要求内存占用小、渲染可分级
- **许可证**：MIT

---

## 已完成模块

### 1. 项目骨架
- CMake 根配置（`CMakeLists.txt`），已加入 Release 构建优化（LTO、去符号、/O2）与 `windeployqt` 自动部署步骤
- Qt6 模块引入：`Quick`、`Qml`、`Sql`、`Svg`、`ShaderTools`、`Effects`、`Widgets`
- 使用 `qt_add_qml_module` 组织 QML 模块（URI: `VelvoxMusic`），未使用手写 `.qrc`
- `.gitignore`（覆盖 build/、CMakeUserPresets.json、Qt Creator 用户文件等）
- MIT `LICENSE`，`THIRD_PARTY_LICENSES.md` 补充 LGPL 来源声明
- `README.md` 发布级模板（功能、截图占位、构建、致谢）
- GitHub Actions workflow（`.github/workflows/build.yml`）自动构建、打包便携 ZIP、生成 Inno Setup 安装包、打 tag 时自动发布 Release
- Inno Setup 脚本（`installer/VelvoxMusic.iss`）：桌面快捷方式、开始菜单、卸载入口
- `.clang-format`：统一 C++ 代码风格

### 2. C++ 后端核心类
- `src/core/AudioEngine.h/.cpp` —— miniaudio 流式解码 + 播放控制，暴露给 QML；已预分配频谱单声道缓冲避免实时线程堆分配
- `src/core/SpectrumAnalyzer.h/.cpp` —— 实时频谱分析（kissfft + SPSC ring buffer）；迭代缓冲已提升为成员变量避免循环内堆分配
- `src/core/Track.h` / `Playlist.h` —— 曲目与播放列表数据结构
- `src/core/MetadataReader.h/.cpp` —— TagLib 元数据读取 + 封面提取与缓存（限制 400×400 JPEG）
- `src/core/DatabaseManager.h/.cpp` —— QSQLITE 连接、迁移与 CRUD（tracks / playlists / playlist_tracks / favorites / recent_plays）
- `src/core/LibraryManager.h/.cpp` —— 本地曲库目录管理 + 后台线程异步扫描 + 进度报告
- `src/core/LyricsParser.h/.cpp` —— LRC / 内嵌歌词解析，支持标准 LRC 与增强 LRC 逐字标签 `<mm:ss.xx>`
- `src/core/WindowsSystemIntegration.h/.cpp` —— Windows 媒体键（WM_APPCOMMAND）+ 任务栏缩略图按钮（ITaskbarList3）+ 系统托盘集成入口
- `src/models/TrackListModel.h/.cpp` —— `QAbstractListModel` 歌曲源模型，仅持有原始曲目数据与基本 CRUD
- `src/models/TrackListProxyModel.h/.cpp` —— `QSortFilterProxyModel` 代理，承担排序/过滤/索引映射，避免 `beginResetModel` 导致的全列表重建
- `src/models/AlbumListModel.h/.cpp` —— 专辑聚合模型（封面/年份/曲目数/总时长）与专辑内曲目查询
- `src/models/ArtistListModel.h/.cpp` —— 艺术家聚合模型（封面/曲目数/专辑数）与艺术家曲目查询
- `src/models/PlaylistModel.h/.cpp` —— 播放列表模型，支持新建/删除/重命名、歌曲拖拽排序、歌曲数缓存
- `src/models/LyricsModel.h/.cpp` —— 歌词数据模型，支持当前行/当前词同步与高亮

### 3. QML 前端骨架
- `qml/main.qml` —— 无边框主窗口（自绘标题栏、可拖动缩放、最小化/最大化/关闭），集成底部播放栏、歌词面板、迷你播放器切换、上一首/下一首队列
- `qml/theme/ThemeManager.qml` —— QML Singleton 设计 token（色板/圆角/间距/尺寸/渐变占位）
- `qml/theme/Motion.qml`（Singleton）—— 全局动画 token（fast/normal/slow 时长 + 统一缓动曲线 + reduceMotion 降级）
- `qml/components/AppButton.qml` —— 可复用按钮
- `qml/components/MeshGradientBackground.qml` —— 动态网格渐变背景（RHI shader + 律动模式 + Low/Medium/High 性能分级 + 窗口失焦暂停）
- `qml/components/AcrylicPanel.qml` —— 基于 `Qt6::QuickEffects` / `MultiEffect` 的毛玻璃面板（可配 blurRadius / tintColor / tintOpacity / renderQuality）
- `qml/components/CrossFadeImage.qml` —— 切歌时封面交叉淡化；支持 `sourceSize`、`asynchronous` 加载、可关闭动画，列表委托中关闭动画避免滚动卡顿
- `qml/components/PlayPauseButton.qml` —— 播放/暂停按钮，带图标交叉淡化 + 缩放过渡
- `qml/components/LyricsView.qml` —— 同步歌词视图，按词渲染并高亮当前词
- `qml/components/MiniPlayer.qml` —— 弹出式迷你播放器，使用 `AcrylicPanel` 作为背景
- `qml/components/MiniPlayerWindow.qml` —— 独立置顶迷你播放器窗口
- 左侧导航 + `StackView` 页面切换（淡入淡出 + 32px 水平位移）：
  - `qml/pages/LibraryPage.qml` —— 音乐库列表，点击播放，右键加入播放列表，封面 `sourceSize: 48×48`，`cacheBuffer` 已限制
  - `qml/pages/AlbumPage.qml` —— 专辑网格浏览，点击专辑显示右侧曲目列表，点击曲目播放
  - `qml/pages/ArtistPage.qml` —— 艺术家网格浏览，点击艺术家显示右侧曲目列表，点击曲目播放
  - `qml/pages/PlaylistsPage.qml` —— 播放列表新建/删除/重命名、歌曲拖拽排序
  - `qml/pages/SearchPage.qml` —— 全局搜索，标题/艺人/专辑模糊匹配 + 150ms 防抖
  - `qml/pages/SettingsPage.qml` —— 设置占位（渲染质量切换）
- `qml/pages/AudioTestPage.qml` —— 音频引擎测试页；频谱可视化已改用 `Canvas` 单帧绘制以避免布局开销
- `qml/pages/LibraryTestPage.qml` —— 本地曲库完整测试页（保留）

### 4. 资源目录
- `resources/icons/app-icon.svg` —— 应用图标
- `resources/shaders/meshgradient.frag` —— 背景着色器源码（构建时编译为 `.qsb`）
- `resources/fonts/.gitkeep` —— 字体目录保留
- `assets/.gitkeep` —— 截图 / demo 目录保留

---

## 关键类名

| 命名空间/位置 | 类名 | 职责 |
|---|---|---|
| `src/core/` | `Velvox::AudioEngine` | miniaudio 上下文与播放控制，实时回调中预分配单声道样本缓冲 |
| `src/core/` | `Velvox::SpectrumAnalyzer` | 实时 FFT 频谱，工作线程中复用样本/FFT/频段缓冲 |
| `src/core/` | `Velvox::MetadataReader` | TagLib 元数据读取、封面提取、歌词提取 |
| `src/core/` | `Velvox::LyricsParser` | LRC 文件与内嵌歌词解析，支持逐字标签 `<mm:ss.xx>` |
| `src/core/` | `Velvox::LibraryManager` | 曲库目录管理与异步扫描 |
| `src/core/` | `Velvox::DatabaseManager` | QSQLITE 连接、迁移与 CRUD |
| `src/core/` | `Velvox::WindowsSystemIntegration` | Windows 媒体键、任务栏缩略图按钮、托盘信号中转 |
| `src/models/` | `Velvox::TrackListModel` | 歌曲源 `QAbstractListModel`（纯数据 + 收藏/删除） |
| `src/models/` | `Velvox::TrackListProxyModel` | `QSortFilterProxyModel` 包装，增量过滤/排序/索引映射 |
| `src/models/` | `Velvox::AlbumListModel` | 专辑聚合 `QAbstractListModel`（封面/年份/曲目数/总时长） |
| `src/models/` | `Velvox::ArtistListModel` | 艺术家聚合 `QAbstractListModel`（封面/曲目数/专辑数） |
| `src/models/` | `Velvox::PlaylistModel` | 播放列表模型（CRUD + 拖拽排序） |
| `src/models/` | `Velvox::LyricsModel` | 歌词同步模型（当前行/当前词） |
| `qml/theme/` | `ThemeManager`（QML Singleton） | 全局颜色、字体、渐变 token |
| `qml/theme/` | `Motion`（QML Singleton） | 全局动画时长/缓动/reduceMotion |

---

## 目录结构

```
VelvoxMusic/
├── CMakeLists.txt              # 根构建配置（含 Release LTO/去符号优化 + windeployqt）
├── LICENSE                     # MIT
├── README.md                   # 项目说明与构建命令
├── THIRD_PARTY_LICENSES.md     # 第三方库许可与 LGPL 来源声明
├── PROJECT_CONTEXT.md          # 本文件
├── vcpkg.json                  # vcpkg 依赖清单（TagLib）
├── .gitignore
├── .clang-format               # C++ 代码风格配置
├── src/
│   ├── app/
│   │   └── main.cpp            # 入口，加载 QML 模块，初始化系统托盘与 Windows 集成
│   ├── core/                   # C++ 后端核心逻辑
│   │   ├── AudioEngine.h/.cpp       # miniaudio 流式解码 + QML 接口
│   │   ├── SpectrumAnalyzer.h/.cpp  # 实时 FFT 频谱
│   │   ├── Track.h / Playlist.h
│   │   ├── MetadataReader.h/.cpp
│   │   ├── DatabaseManager.h/.cpp
│   │   ├── LibraryManager.h/.cpp
│   │   ├── LyricsParser.h/.cpp
│   │   └── WindowsSystemIntegration.h/.cpp
│   └── models/                 # 暴露给 QML 的数据模型
│       ├── TrackListModel.h/.cpp
│       ├── TrackListProxyModel.h/.cpp
│       ├── AlbumListModel.h/.cpp
│       ├── ArtistListModel.h/.cpp
│       ├── PlaylistModel.h/.cpp
│       └── LyricsModel.h/.cpp
├── qml/
│   ├── main.qml                # 无边框主窗口 + 播放栏 + 歌词面板 + 迷你播放器
│   ├── pages/                  # 页面级视图
│   │   ├── HomePage.qml
│   │   ├── LibraryPage.qml
│   │   ├── AlbumPage.qml
│   │   ├── ArtistPage.qml
│   │   ├── PlaylistsPage.qml
│   │   ├── SearchPage.qml
│   │   ├── SettingsPage.qml
│   │   ├── AudioTestPage.qml
│   │   └── LibraryTestPage.qml
│   ├── components/             # 可复用控件
│   │   ├── AppButton.qml
│   │   ├── MeshGradientBackground.qml
│   │   ├── AcrylicPanel.qml
│   │   ├── CrossFadeImage.qml
│   │   ├── PlayPauseButton.qml
│   │   ├── LyricsView.qml
│   │   ├── MiniPlayer.qml
│   │   └── MiniPlayerWindow.qml
│   └── theme/                  # 主题与样式规范
│       ├── ThemeManager.qml    # Singleton 设计 token
│       └── Motion.qml          # Singleton 动画 token
├── resources/
│   ├── icons/                  # SVG 图标
│   ├── fonts/                  # 自定义字体
│   └── shaders/                # GLSL 着色器源码（构建时编译为 .qsb）
├── installer/                  # Inno Setup 安装脚本
│   └── VelvoxMusic.iss
├── .github/workflows/          # CI / 自动发布
│   └── build.yml
└── assets/                     # README 截图 / demo 媒体
```

---

## 性能与内存审计结论

已按以下策略完成一轮优化：

1. **图片解码尺寸控制**：所有 `Image`/`CrossFadeImage` 均设置 `sourceSize`（列表 48×48、播放栏 56×56、迷你播放器 88×88），并开启 `asynchronous: true` 避免主线程解码阻塞。
2. **列表缓存控制**：所有 `ListView` 设置 `cacheBuffer: Math.max(200, height * 1.5)`，避免滚动时创建过多委托。
3. **列表动画降级**：`CrossFadeImage` 新增 `animate` 属性，列表委托中设为 `false`，避免 delegate 复用时触发交叉淡出。
4. **频谱可视化去布局化**：`AudioTestPage.qml` 的 32 个频谱条由 `RowLayout + Repeater` 改为单 `Canvas` 绘制，消除每帧 32 次布局。
5. **实时线程零堆分配**：`AudioEngine` 预分配单声道样本缓冲；`SpectrumAnalyzer` 将 `samples`/`bands`/`fftOut` 提升为成员变量。
6. **Release 构建优化**：`CMakeLists.txt` 已加入 MSVC `/O2 /GL /LTCG /OPT:REF /OPT:ICF` 与 GCC/Clang `-O3 -flto -s` 等选项。
7. **搜索/排序模型增量更新**：`TrackListModel` 拆分为纯源模型，`TrackListProxyModel`（`QSortFilterProxyModel`）接管过滤与排序。搜索时通过 `invalidateFilter()` 触发 `rowsInserted`/`rowsRemoved` 增量更新，排序时触发 `layoutChanged`，避免 `beginResetModel` 导致 ListView 重建全部委托。

当前内存目标：**空闲/播放状态 80–130 MB Private Working Set；扫描峰值不超过 250 MB**。

---

## 待办事项（TODO）

### 高优先级
- [x] 接入 miniaudio：实现音频解码与播放（`AudioEngine` 已可独立播放 mp3/flac/wav/ogg）
- [x] 接入 TagLib：实现元数据读取（`MetadataReader`）
- [x] 设计并实现 QSQLITE 数据库 schema（`DatabaseManager`）
- [x] 实现本地曲库扫描器（后台 worker + `LibraryManager`）
- [x] 实现播放队列基础上一首/下一首（`main.qml` + `WindowsSystemIntegration` 信号）
- [x] 完善 `TrackListModel` 并接入真实数据
- [x] Windows 媒体键 + 任务栏缩略图按钮 + 系统托盘集成

### 中优先级
- [x] 设计并实现现代化无边框窗口拖拽、缩放、标题栏
- [x] 实现动态网格渐变背景（Mesh Gradient，RHI shader + qsb + 性能分级 + 律动模式）
- [x] 实现高斯模糊玻璃拟态组件（`AcrylicPanel`、`MiniPlayer`）
- [x] 实现底部播放控制栏（`PlayerBar`）
- [x] 实现统一动画系统（`Motion.qml` Singleton + StackView 页面切换 + 列表 stagger + 播放按钮/封面过渡 + hover/press 反馈 + 减少动效开关）
- [x] 实现主页、曲库页、播放列表页、搜索页、设置页占位
- [x] 实现播放列表新建/删除/重命名、歌曲拖拽排序、从音乐库右键加入播放列表
- [x] 实现全局搜索（标题/艺人/专辑模糊匹配 + 防抖过滤）
- [x] 歌词显示（LRC 文件 + 内嵌，逐字高亮）
- [x] 性能与内存审计第一轮（sourceSize、cacheBuffer、Canvas 频谱、实时线程零堆分配、Release LTO）
- [x] 实现专辑页、艺术家页（`AlbumPage.qml` / `ArtistPage.qml` + `AlbumListModel` / `ArtistListModel`，网格浏览 + 右侧曲目列表）
- [x] 实现虚拟化歌曲/专辑/艺术家列表（`GridView`/`ListView` + `cacheBuffer` 已提供专辑/艺术家/歌曲列表虚拟化；代理模型避免重置进一步减少委托重建）
- [x] 搜索过滤模型优化：已改用 `TrackListProxyModel`（`QSortFilterProxyModel`）实现增量过滤/排序

### 低优先级 / 后续优化
- [x] 音频频谱可视化（`SpectrumAnalyzer` + `AudioTestPage`）
- [ ] 明暗主题切换与跟随系统
- [ ] Windows SMTC（System Media Transport Controls）集成
- [ ] 音效：均衡器、交叉淡化、无缝播放
- [ ] 国际化（i18n）
- [x] CI（GitHub Actions）自动构建 Windows 安装包
- [x] README 完善截图、功能清单、贡献指南

---

## 关键决策记录

1. **音频库**：使用 miniaudio，避免 BASS 商业授权问题。
2. **构建系统**：CMake 3.21+，`qt_add_qml_module` 组织 QML，不手写 `.qrc`。
3. **元数据**：TagLib 动态链接，符合 LGPL 要求。
4. **数据库**：Qt 内置 QSQLITE，不引入额外数据库依赖。
5. **窗口**：无边框主窗口，自定义标题栏、拖拽、双击最大化、边缘缩放。
6. **资源嵌入**：图标、字体、shader 通过 `qt_add_qml_module` 的 `RESOURCES` 嵌入。
7. **第三方库获取**：miniaudio 与 kissfft 均通过 CMake `FetchContent` 自动拉取源码；kissfft 源文件直接编译进目标，避免动态库部署。
8. **毛玻璃实现**：使用 Qt 6 内置 `Qt6::QuickEffects` 的 `MultiEffect` 实现高斯模糊，不引入已废弃的 `Qt5Compat.GraphicalEffects`；背景层与 AcrylicPanel 均开启 `layer.enabled` 缓存，避免每帧重算模糊。
9. **动画系统**：所有动画时长与缓动曲线统一从 `Motion.qml` Singleton 读取，支持 `reduceMotion` 一键降级为 0ms，兼顾无障碍与低性能设备。
10. **实时音频线程**：禁止在回调与频谱工作线程中堆分配，采用预分配/成员缓冲策略，降低延迟抖动。
11. **图片策略**：封面缓存到本地 400×400 JPEG，不在内存中驻留大图；QML 显示时通过 `sourceSize` 限制解码尺寸，并异步加载。
