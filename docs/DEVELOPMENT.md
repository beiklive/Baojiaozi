# Baojiaozi 分阶段开发文档

## 1. 文档目的

本文档是 Baojiaozi 的唯一阶段跟踪文档。每完成一个阶段，必须同步更新：

- 阶段状态
- 实际完成的文件和接口
- 验收结果
- 已知限制
- 下一阶段前置条件

文档中的“完成”只表示对应验收标准已经通过，不表示整个产品完成。

## 2. 产品边界

Baojiaozi 为 JiaoZiPi 提供主题描述运行时和可视化设计器，负责：

1. 解析页面、组件、主题和动画 JSON。
2. 根据控件状态计算视觉属性。
3. 根据布局描述计算控件几何位置。
4. 使用 ImGui 渲染预览和桌面设计器。
5. 通过接口清单引用 JiaoZiPi 或核心提供的页面、状态和动作。
6. 在设计器中预览状态变化和动画。

JiaoZiPi 或核心负责业务逻辑、页面功能、接口实现和接口清单生成。Baojiaozi 不执行任意脚本，不直接实现模拟器业务流程。

## 3. 工程分层

```text
baojiaozi_core
  document / parser / manifest / style / layout / animation / runtime / validation

baojiaozi_imgui
  ImGui renderer / input adapter / font loader / preview host

baojiaozi_designer
  project manager / asset panels / tabs / inspector / preview / blueprint / history
```

依赖方向只能从设计器到渲染层，再到核心运行时，核心运行时不能依赖设计器。

## 4. 阶段计划

| 阶段 | 目标 | 状态 | 完成标准 |
|---|---|---|---|
| Phase 0 | 仓库、依赖、构建和文档基线 | 进行中 | CMake 可配置，ImGui/GLFW 子模块存在，字体可被定位 |
| Phase 1 | JSON 文档模型、解析器和校验器 | 已完成 | 能加载项目、组件、页面、主题和清单，并报告错误位置 |
| Phase 2 | 运行时树、基础布局和 ImGui 渲染 | 已完成 | 能渲染 Box、Text、Image、Button、Horizontal、Vertical |
| Phase 3 | 状态系统、插值动画和预览事件 | 已完成 | 能预览基础事件和动画轨道 |
| Phase 4 | JiaoZiPi 清单、主页加载和主题选择 | 已完成 | JiaoZiPi 可构建并读取 Baojiaozi 主页描述 |
| Phase 5 | 设计器项目管理、Tab、属性面板和即时预览 | 已完成 | 可打开主页、编辑属性、保存并实时看到变化 |
| Phase 6 | 最小蓝图、组件复用和依赖刷新 | 延期 | 初版交付后实现，避免阻塞主页和设计器闭环 |
| Phase 7 | 初版主页和设计器交付 | 已完成 | 独立设计器和 JiaoZiPi 主页均可构建 |

## 5. 固定数据模型

主题项目采用以下结构：

```text
<theme>/
  project.json
  manifest.snapshot.json
  theme.json
  components/*.json
  pages/*.json
  animations/*.json
  assets/
```

`manifest.snapshot.json` 记录设计器打开项目时使用的清单版本和哈希。运行时加载最新清单时必须检查兼容性，清单不兼容时禁止静默使用未知动作。

## 6. 控件和状态范围

第一版控件：`box`、`text`、`image`、`button`、`horizontal`、`vertical`。

第一版状态：`normal`、`focused`、`pressed`、`disabled`、`hidden`。

第一版触发：`enter`、`focus_in`、`trigger`、`focus_out`、`exit`。

第一版插值：数值、二维向量、颜色、旋转、尺寸、透明度；缓动支持 linear、step、cubicIn、cubicOut、cubicInOut。

## 7. 阶段记录规则

每次阶段完成时，在对应章节补充：

```text
完成日期：YYYY-MM-DD
变更：列出新增目标和文件
验证：列出命令及结果
限制：列出当前未覆盖项
```

失败的验证必须保留在记录中，不能只记录最后一次成功结果。

## 8. Phase 0 记录

状态：已完成。

已完成：

- 将工作区中的 `Baojiaozi ` 目录规范为 `Baojiaozi`。
- 初始化独立 Git 仓库。
- 加入 ImGui 子模块。
- 加入 GLFW 子模块，用于桌面设计器的第一版窗口后端。
- 保留 `resources/fonts/switch_font.ttf`、`MaterialIcons-Regular.ttf` 和 `switch_icons.ttf`。
- 建立 `include`、`src`、`imgui`、`designer`、`tests` 和 `docs` 目录。

待完成：

- 顶层 CMake 已建立 `baojiaozi_core`、`baojiaozi_imgui`、`baojiaozi_designer` 和 `baojiaozi_core_tests`。
- ImGui/GLFW 最小窗口目标已成功编译。
- 设计器启动代码已检查 `switch_font.ttf` 和 `MaterialIcons-Regular.ttf` 的资源路径，并加载文本字体。
- 核心冒烟测试已通过。

完成日期：2026-10-01。

验证命令：

```bash
cmake -S . -B build/dev -DBAOJIAOZI_BUILD_DESIGNER=ON -DBAOJIAOZI_BUILD_TESTS=ON
cmake --build build/dev -j4
ctest --test-dir build/dev --output-on-failure
```

验证结果：`baojiaozi_designer` 构建成功，`baojiaozi_core_tests` 通过，1/1 测试通过。

当前限制：

- 当前窗口仅显示工程基线状态，不加载页面 JSON。
- Material Icons 目前只校验文件存在，图标字体合并和名称映射放在 ImGui 资源层阶段实现。
- 当前桌面后端使用 GLFW/OpenGL3；JiaoZiPi 的平台后端尚未接入。

## 9. 变更记录

| 日期 | 阶段 | 内容 |
|---|---|---|
| 2026-10-01 | Phase 0 | 创建 Baojiaozi 仓库，加入 ImGui 和 GLFW 子模块，建立开发文档 |
| 2026-10-01 | Phase 1 | 建立文档模型、项目目录解析、节点 ID 校验和默认主题示例 |
| 2026-10-01 | Phase 2 | 建立主题解析、运行时布局树、ImGui 基础渲染和设计器主页预览 |
| 2026-10-01 | Phase 3 | 增加动画文档、缓动插值、设计器事件预览按钮和动画测试 |
| 2026-10-01 | Phase 4 | JiaoZiPi 引入 Baojiaozi 子模块，增加顶层 CMake、主页主题和 UI 清单 |
| 2026-10-01 | Phase 5 | 增加设计器页面选择、节点树、属性检查器、运行时刷新和页面保存 |
| 2026-10-01 | Phase 7 | 完成初版主页和设计器交付验收，Phase 6 蓝图扩展延期 |

## 10. Phase 1 记录

状态：已完成。

已完成：

- 加入 nlohmann/json 3.12.0 子模块。
- 建立 `Node`、`PageDocument`、`ComponentDocument`、`ThemeDocument`、`ManifestDocument` 和 `ProjectDocument`。
- 支持读取 `project.json`、`theme.json`、`manifest.snapshot.json`、`pages/*.json` 和 `components/*.json`。
- 支持基础节点类型：`box`、`text`、`image`、`button`、`horizontal`、`vertical`、`component`。
- 对必填字段、节点类型、属性对象、动画引用、绑定对象和子节点数组进行校验。
- 对页面、组件和节点 ID 做重复检查。
- 对目录文件进行排序后加载，保证页面和组件顺序稳定。
- 增加错误级别、源文件和 JSON 路径诊断信息。
- 增加 `resources/examples/default_theme` 示例项目。

完成日期：2026-10-01。

验证结果：

- 项目目录加载成功。
- 页面节点树解析成功。
- 非法控件类型能够产生错误诊断并使加载失败。
- `cmake --build build/dev -j4` 成功。
- `ctest --test-dir build/dev --output-on-failure`：1/1 通过。

当前限制：

- 当前只解析 JSON 文档，不负责布局和渲染。
- `animations` 和 `bindings` 目前保留为结构化数据，尚未执行。
- 清单动作和页面引用尚未验证是否存在。
- 尚未实现项目文件写回。

## 11. Phase 2 记录

状态：已完成。

已完成：

- 增加 `ThemeResolver`，支持按控件类型和命名样式解析背景色、文字色、字体大小、圆角、内边距和间距。
- 增加 `Runtime` 和 `RuntimeNode`，将文档节点转换为可布局的运行时树。
- 实现垂直布局和水平布局的基础尺寸计算。
- 实现 `box`、`text`、`image`、`button`、`horizontal` 和 `vertical` 的 ImGui 绘制。
- 设计器加载 `resources/examples/default_theme`，并在即时渲染区域显示 `home` 页面。
- 设计器增加菜单栏、控件列表、页面列表和即时预览区域的第一版结构。
- 增加运行时布局冒烟断言。

完成日期：2026-10-01。

验证结果：

- `cmake --build build/dev -j4` 成功。
- `ctest --test-dir build/dev --output-on-failure`：1/1 通过。
- 运行时测试确认页面根节点和文本节点尺寸按布局规则计算。

当前限制：

- 当前渲染器使用占位矩形绘制 Image，尚未加载纹理资源。
- Button 当前只绘制视觉外观，没有输入事件和状态切换。
- 当前布局不支持滚动、弧形容器、尺寸约束和响应式断点。
- 设计器窗口的人工截图验收因当前 macOS 会话处于锁定状态未完成，构建和运行时测试已通过。

## 12. Phase 3 记录

状态：已完成。

已完成：

- 增加 `AnimationDocument`、`AnimationTrack` 和 `AnimationKeyframe` 数据模型。
- 支持从 `animations/*.json` 读取动画定义，并校验持续时间、轨道和关键帧。
- 实现 linear、step、cubicIn、cubicOut 和 cubicInOut 缓动。
- 实现数字和数组值的关键帧插值。
- 运行时根据节点的动画引用和预览事件覆盖视觉属性。
- 设计器增加“普通”“聚焦”“触发”预览按钮和运行时间显示。
- 增加 `button_focus` 和 `button_trigger` 示例动画。
- 增加动画加载和插值测试。

完成日期：2026-10-01。

验证结果：

- `cmake --build build/dev -j4` 成功。
- `ctest --test-dir build/dev --output-on-failure`：1/1 通过。
- 测试确认 `cubicOut` 在中间时间点产生非线性插值结果。

当前限制：

- 当前只把数字、数组、圆角、内边距等属性接入渲染，颜色字符串插值尚未接入。
- 状态机还没有接收真实鼠标、键盘或手柄事件。
- 预览事件只在设计器中手动触发，尚未连接 JiaoZiPi 的接口清单。

## 13. Phase 4 记录

状态：已完成。

已完成：

- JiaoZiPi 以 Git 子模块方式引入 Baojiaozi。
- JiaoZiPi 增加顶层 CMake，构建 Baojiaozi 核心和 ImGui 渲染层。
- 增加 `jiaozi_pi_app` 最小桌面应用入口。
- 增加 JiaoZiPi 的 `resources/ui_manifest.json`，声明 `home`、`library`、`settings` 页面和 `navigate.page` 动作。
- 增加 JiaoZiPi 默认主题项目，并在启动时加载 `home` 页面。
- 支持 `--theme-dir` 指定主题目录。
- 构建时将 UI 清单复制到构建资源目录，作为后续生成清单的过渡接口。

完成日期：2026-10-01。

验证结果：

- `cmake -S . -B build/phase4 -DJIAOZIPI_BUILD_APP=ON` 成功。
- `cmake --build build/phase4 -j4` 成功。
- `jiaozi_pi_app` 链接成功。

当前限制：

- 当前 UI 清单是仓库内的初始静态清单，尚未从实际核心注册表自动生成。
- 当前启动链路只渲染主页，页面跳转动作尚未接入。
- Baojiaozi 子模块使用本地 URL；远程发布后必须更新 `.gitmodules`。
- macOS 会话锁定，尚未完成窗口截图验收。

## 14. Phase 5 记录

状态：已完成。

已完成：

- 增加 `ProjectStore`，支持将页面文档序列化回 `pages/<id>.json`。
- 设计器支持页面列表切换和当前活动页面状态。
- 设计器增加节点树，支持选择页面中的任意节点。
- 设计器增加属性检查器，支持编辑 `text` 和 `fontSize`。
- 文档属性修改直接作用于内存项目模型，预览每帧从模型重新生成运行时树。
- 设计器保存菜单写回当前页面 JSON，并显示成功或失败信息。
- 运行时将节点属性中的 `fontSize` 和 `padding` 合并到样式。
- 增加页面保存测试。

完成日期：2026-10-01。

验证结果：

- `cmake --build build/dev -j4` 成功。
- `ctest --test-dir build/dev --output-on-failure`：1/1 通过。
- 测试确认页面序列化后文件存在且可写入。

当前限制：

- 当前只有主页示例文件，页面列表切换依赖项目中实际存在的页面文件。
- 当前保存只写回单个页面，不保存项目元数据、主题和动画文件。
- 当前没有撤销、重做和未保存状态提示。
- 当前属性编辑器只覆盖文本和字号，尚未实现通用属性 schema。

## 15. Phase 6 记录

状态：延期。

延期原因：初版目标已经由文档解析、运行时渲染、动画预览、主页加载和属性保存组成；蓝图节点编辑、组件依赖图和动作绑定会显著扩大文档模型和交互范围，当前不应阻塞初版闭环。

保留范围：

- 组件文件格式已经由 Phase 1 的 `components/*.json` 预留。
- 节点 `bindings` 已保留结构化数据。
- `manifest.snapshot.json` 已提供动作清单输入。

后续进入条件：

- 设计器有稳定的撤销/重做命令系统。
- 页面和组件引用关系有独立依赖索引。
- 清单动作参数可以生成可编辑的节点端口。

## 16. Phase 7 记录

状态：已完成。

交付内容：

- Baojiaozi 独立仓库已初始化。
- ImGui、GLFW 和 nlohmann/json 已作为子模块加入。
- 文档模型、JSON 解析、诊断、主题解析、布局、运行时树、ImGui 渲染和动画播放器已实现。
- 独立设计器可以打开默认主题项目，显示控件列表、页面列表、主页预览和属性检查器。
- 设计器可以修改主页节点文本和字号，并保存到页面 JSON。
- JiaoZiPi 已加入 Baojiaozi 子模块和顶层 CMake。
- JiaoZiPi 已加载默认主题主页，并支持通过 `--theme-dir` 选择主题目录。
- JiaoZiPi 已提供第一版 UI 清单文件。

验收命令：

```bash
# Baojiaozi
cmake -S . -B build/dev -DBAOJIAOZI_BUILD_DESIGNER=ON -DBAOJIAOZI_BUILD_TESTS=ON
cmake --build build/dev -j4
ctest --test-dir build/dev --output-on-failure

# JiaoZiPi
cmake -S . -B build/phase5 -DJIAOZIPI_BUILD_APP=ON
cmake --build build/phase5 -j4
```

验收结果：

- Baojiaozi 设计器构建成功。
- Baojiaozi 核心测试 1/1 通过。
- JiaoZiPi 前端应用链接成功。
- 设计器和 JiaoZiPi 使用同一 Baojiaozi 子模块版本 `05a2edf`。

已知限制：

- 当前 macOS 会话处于锁定状态，无法完成人工窗口截图验收；窗口构建、链接和核心测试已完成。
- JiaoZiPi 当前只渲染主页，页面跳转和核心业务接口尚未连接。
- UI 清单当前是静态初始清单，核心注册表自动生成将在后续阶段实现。
- Baojiaozi 已发布到 `git@github.com:beiklive/Baojiaozi.git`，JiaoZiPi 的子模块应使用该正式地址。
