# HTML 模块基础功能缺口分析

审计对象：`src/formats/html/`（`src/formats/html/litehtml/` 为 litehtml 替换层，不含 `src/thirdparty`）
规模：约 **60,439 行** C++ / **111 个** CSS 源文件 / 74 个 `HTML*` 类绑定
约束前提：**不支持 JavaScript，脚本后端只有 GDScript**

---

## 0. 总体结论

这个模块的"渲染层"完成度远高于预期，不是半成品：

| 层 | 状态 |
| --- | --- |
| DOM/CSSOM API（查询、增删、克隆、adopt/import、Range、Selection、MutationObserver） | **接近完整**，有 WPT 移植测试 |
| CSS（Grid + Subgrid、容器查询、`:has()/is()/where()`、`@layer`、`@scope`、`oklch/color-mix`、masks、3D transform、scroll snap） | **接近现代浏览器** |
| 表单（22 种 input 状态、日期/时间/取色器、`<datalist>`、约束校验、`method=dialog`） | **完整** |
| 媒体（`<video>`/`<audio>` 控件、字幕除外） | **基本完整** |
| **交互 / 事件层** | **几乎为 0** |
| **脚本运行时（GDScript 后端）** | **只有一个"挂载钩子"** |

一句话：**缺的不是"能画什么"，而是"页面怎么响应"和"后端怎么运行"。**
布局引擎已经是浏览器级的，但事件与运行时还停在"库"的水平——这对一个以 GDScript 为唯一后端的方案是致命的，因为 JS 消失后，事件分发和运行时必须由引擎侧自己补上，不能指望宿主项目各自实现。

---

## 1. 最大缺口：DOM 事件模型（完全缺失）

### 1.1 现状

全仓库（`src` 排除 thirdparty）检索结果：

| 关键词 | 命中 | 说明 |
| --- | --- | --- |
| `addEventListener` / `removeEventListener` | **0** | — |
| `dispatchEvent` | **0** | — |
| `Event` 类（DOM 意义） | **0** | 115 处 `Event` 全是 `godot::InputEvent` |
| `onclick` / `onload` / `onsubmit` 等内联属性 | **0** | 属性被当普通字符串保留，从不解释 |
| `bubbl` / `capture` / `stopPropagation` | **0** | 无传播模型 |
| `MouseEvent` / `KeyboardEvent` / `FocusEvent` / `CustomEvent` | **0** | — |
| `dblclick` / `上下文菜单事件` | **0** | 连双击检测都不存在 |
| `preventDefault` | **2** | 仅 `HTMLDialogCloseRequest::prevent_default()` |

`HTMLNode` / `HTMLElement` / `HTMLDocumentNode` **没有注册监听器的能力**，也没有任何 `Callable` 成员。
`HTMLElement` 上甚至没有 `click()` / `focus()` / `blur()` / `scrollIntoView()` 方法（这些能力散落在 `HTMLLabel` 上）。

### 1.2 现在能"观察到"什么

全部可观察交互 = **`HTMLLabel` 的 14 个信号 + 3 个元素级信号**：

- `HTMLLabel`：`element_clicked`、`link_pressed`、`button_pressed`、`input_changed`、`input_committed`、`textarea_changed`、`textarea_committed`、`select_changed`、`form_submitted`、`form_validation_failed`、`form_reset`、`layout_changed`、`video_failed`、`video_fullscreen_changed`
- 元素级：`HTMLDialogElement.close_requested/closed`、`HTMLDetailsElement.toggled`
- `HTMLMutationObserver.mutations_delivered`（只报 DOM 变更，不报输入）

注意 `element_clicked` 的语义是"**默认行为已经执行完之后**的通知"（`html_label.cpp:462-481`：先跑链接跳转/按钮/表单，再发信号），且文本输入框和开关控件会**抑制**该信号（`:393`）。

### 1.3 事件覆盖差距表

| 事件 | 现状 | 机制 / 原因 |
| --- | --- | --- |
| `click` | 部分 | 只有 label 级 `element_clicked`，无元素订阅；文本框/toggle 不发；仅左键；需按下抬起同元素且未拖动 |
| `dblclick` | **无** | 全仓库无双击检测，连双击选词都没有 |
| `contextmenu` | **无** | 右键只弹内置文本编辑菜单，不可取消、不可观察 |
| `mousedown` / `mouseup` | **无** | 被内部消费（`pressed_element`/`pressed_button`/`pressed_link`） |
| `mouseenter` / `mouseleave` | **无** | 只有 CSS `:hover`；无 hovered 元素 getter，也无信号 |
| `focus` / `blur` | **无** | `set_focused_element()` 静默改伪类，只能轮询 `get_focused_element()` |
| `keydown` / `keyup` | **无** | `handle_dom_key()` 只处理 Tab/Enter/Space/Escape；任何键码都不进 DOM、不进脚本 |
| `input` / `change` | 部分 | label 级信号，只带元素句柄，无事件对象 |
| `submit` | 部分，**不可取消** | `form_submitted` 在校验通过后已提交才发；无法否决 |
| `reset` | 部分，**不可取消** | `form_reset` 是重置完之后的通知 |
| `load` / `DOMContentLoaded` | 部分 | 只能靠 `layout_changed`（但 hover 回流也会触发）或 `HTMLBehavior._attach_document` |
| `scroll` | **无** | 滚动偏移变化完全静默，只能轮询 `get_scroll_offset()` |
| `error` | 只能轮询 | `get_render_error()`；唯一的异步错误信号是 `video_failed` |

### 1.4 为什么这是"基础功能"缺口

没有事件层时，GDScript 后端要写一个按钮回调，必须：

```gdscript
func _attach(label: HTMLLabel) -> void:
    label.element_clicked.connect(func(el):
        if el.closest("#save-btn"): _save())
```

也就是说：**每个行为脚本都要自己重新实现一遍事件分发**，还必须连 label 才能做（`_attach_document` 拿不到 label）。这与 JS 是否存在无关——事件冒泡、焦点事件、键盘事件是"文档 UI"的公共基础设施，属于引擎该提供的部分，而不是留给每个使用者。

参考实现已经存在：`HTMLDialogElement.close_requested` + `prevent_default()` 就是这套 API 应有的形状（同步派发、可取消、带 reason），只是没有被抽象成通用 `Event`/`EventTarget`。

---

## 2. 脚本宿主（GDScript 后端）本身的缺口

整个运行时约 **124 行**（`src/nodes/html_label_behaviors.cpp`）。

### 2.1 选择机制

```cpp
for (auto& element : query_selector_all("script")) {
    if (declaration->get_type().nocasecmp_to("text/gdscript") != 0) continue;
    const String name = declaration->get_src().strip_edges();
    auto script = label->behavior_scripts.get(StringName(name));   // 字典 key
    ...
}
```

- `type` 必须**恰好**是 `text/gdscript`（带 `;charset=utf-8` 就不匹配），`language=` 被忽略
- `src` **不是路径、不是 URL**，是字典 key 的字符串字面量。`<script src="res://ui/app.gd">` 只有在字典里存在 `"res://ui/app.gd"` 这个 key 时才工作
- 对比：`<img src>`、`<link href>`、`<audio src>`、`<video src>`、`@font-face` 全都有 `html_resource_paths.cpp` 的完整解析器（`res://`、`user://`、`uid://`、`<base href>`、相对路径），并且导入时会收集依赖。**只有 `<script>` 没有解析器**
- 内联 `<script>…</script>` 的 body 永不执行，只当数据
- `<template>` 内和 SVG foreign content 内的 `<script>` 不会被选中
- 挂载失败**完全静默**：未注册的 `src`、基类不对、抽象类、`_init` 有必填参数 → 直接跳过，无 `push_warning`、无 `get_render_error()` 条目
- 任何一个 `render_error` 非空都会**整体禁用**所有 behavior（`html_label_behaviors.cpp:79`）

### 2.2 能力矩阵

| 能力 | 状态 | 说明 |
| --- | --- | --- |
| `setTimeout` / `setInterval` | **无** | behavior 是 `RefCounted` 不是 `Node`，只能 `Engine.get_main_loop().create_timer()` 自己造；`_detach` 没有统一清理 |
| `requestAnimationFrame` | **无** | 没有逐帧钩子；只有声明式 `element.animate()` |
| `fetch` / `XMLHttpRequest` | **无** | `http_requests.h` 是 C++ 命名空间函数，`RemoteImage`/`RemoteStylesheet` 非 `GDCLASS`，只服务图片和 CSS。GDScript 拿不到，也无法复用文档 base URL |
| `localStorage` / `sessionStorage` | **无** | 无任何存储类；只能 `user://` + `FileAccess`/`ConfigFile`，全局无命名空间、无变更事件 |
| `location` / `navigate` / `history` | **部分（只读）** | `get_base_uri()` 只读；`link_pressed` 只是建议；只有 `#fragment` 有内置行为。无 `pushState`/前进后退/URL 变更事件 |
| `console` / 错误上报 | **部分（只能用 Godot 的）** | 无页面日志缓冲、无按 `<script>` 归因、无运行期错误捕获（GDScript 无 try/catch） |
| 多脚本顺序 | 可用 | 文档顺序、同步、每个匹配元素一个独立实例 |
| `defer` / `async` / `nomodule` | **属性是死的** | 只反射属性（`html_metadata_elements.cpp:125-138`），挂载只看 `type` 和 `src` |
| 模块 / import | **无** | 扁平 `Dictionary[StringName, GDScript]`，无解析、无 import map、无依赖图 |
| 动态插入 `<script>` | **无** | `create_element("script")` + append 不执行；改 `src` 不重跑 |
| 热重载 / remount API | **无** | 全仓库无文件监听；`set_behavior_scripts` 字典相等就早退；编辑器里直接禁用 behavior。改一行 .gd 要换文档或切 `behaviors_enabled`，会销毁全部实例 |
| 拿到宿主 label | **部分** | 只有 `_attach(label)` 能拿到；`_attach_document(dom)` 是死路（`HTMLBehavior` 没有 `get_label()`，`HTMLDocumentNode` 无 owner 访问器）。而文档却把 `_attach_document` 写成首选 |

---

## 3. 元素（标签）缺口

元素分派有两层：容器（`src/formats/html/backend/litehtml_container.cpp:141-161`）特化 `ruby/details/dialog/output/template/video/audio/meter/progress/select/option/textarea/input/script/style/link/source/img`；之后 litehtml 自己再特化 `br/p/img/table/td/th/link/title/a/tr/style/base/body/div/script/font`（`litehtml/document/src/document.cpp:363-436`）。其余标签成为通用 `litehtml::html_tag`——**有盒模型、可查询、可样式化，但无专属行为**。DOM 句柄层注册了 51 个元素类（`src/register_types.cpp:85-138`），未映射的标签拿到裸 `HTMLElement`。

注意：标签名不在 `css_string_id.h:18-141` 的表里也**不会被丢弃**——`_id()` 会内联任意字符串（`css_string_id.cpp:43-51`），未知标签照样成为同名通用元素。下面说的是"引擎支持度"，不是"能否解析"。

**没有对应 `HTML*` 类或专属代码路径的标准元素**：

| 缺失 | 严重度 | 说明 |
| --- | --- | --- |
| `<canvas>` | **高** | 整个 HTML 模块只有 `_canvas_` 这个 tag id 和 CSS 系统色 `canvas`（`css_string_id.h:40`、`css_web_color.cpp:259`），**无 `getContext`、无 2D 上下文、无 `toDataURL`**。`<canvas>` 是普通 inline 元素，只有 fallback 子节点会流式渲染。JS 不支持时，这里本该是"GDScript 直接画"的入口 |
| `contenteditable` | **高** | 全仓库 0 命中。富文本编辑（在线文档、批注、笔记）完全不可用；只有 `<textarea>` 纯文本 |
| `drag` / `drop`（HTML5 DnD） | 中 | 无 `draggable`、无 `dragstart/dragover/drop`，文件不能拖入 |
| `<iframe>` | 中 | 无浏览上下文、无 `srcdoc`/`sandbox`；Gumbo 把其内容当 RAWTEXT（`parser.c:3950-3956`），DOM 侧是 raw text（`html_dom.cpp:83-87`），最终按普通 inline 文本流式渲染 |
| `<object>` / `<embed>` / `<param>` | 中 | 惰性，无插件/嵌入对象处理；`object` 的子节点只当 fallback flow |
| `<math>`（MathML） | 低* | Gumbo 认识该标签并打 `mathml` 命名空间，但**无 tag id、无 UA 规则、无 MathML 布局**，公式退化为普通文本。（*本项目 LaTeX 覆盖数学需求，故标低） |
| `<map>` / `<area>` | 低 | 无图像地图、无热点命中测试；`area` 仅通过 `area[href]` 参与 `:link/:any-link` 匹配 |
| `<slot>` / Shadow DOM / 自定义元素 | 中 | 文档明说"不支持 declarative shadow root"；无 `attachShadow`/`customElements`/`getRootNode`。连字符自定义标签能创建能查询，但**无升级、无生命周期回调、无样式边界** |
| 内联 `<svg>` 图形 | 中 | SVG **确实有真实命名空间**（`element_namespace::svg`），几何解析完整（path/rect/circle/ellipse/polygon/polyline/line + transform + `<use>` + `<tspan>`），**但没有任何内联绘制通路**：形状永远画不出来，几何只服务 `clip-path`/`mask`；mask 是把内联 SVG 子树重新序列化后用 `Image::load_svg_from_string` 光栅化（`html_svg_mask.cpp:152-245`）。外部 `.svg` 仅能当 `<img>`/CSS 背景图光栅化使用 |
| `<track>`（字幕） | 低 | 仅列为 void 元素，无 TextTrack/字幕代码 |
| `<frameset>` / `<frame>` / `<noframes>` | 低 | 无 frame 布局；`get_body()` 只是把 frameset 当 body 替身 |
| `accesskey` | 低 | 无 |

已确认**支持**、无需担心：`<picture>`/`srcset`、`<datalist>`、`<details>/<summary>`、`<dialog>`（含 top layer/backdrop）、`<ruby>/<rt>/<rp>`、`<template>`（含 `content` fragment，行为完整且有 WPT 测试）、`<meter>/<progress>`、`<output>`、`<video>/<audio>`、`<table>` 全家桶（含 insert_row/insert_cell）、`<pre>`、`<wbr>`、`<plaintext>`/`<xmp>`（解析层）、`autofocus`、22 种 input。

### 3.1 顺带发现的细节缺陷（都不是大功能，但属于"不完整"）

- **`<noscript>` 的 fallback 内容总是渲染**：Gumbo 以 `scripting disabled` 解析，导致 `<noscript>` 里的内容直接被当普通 DOM 渲染（`parser.c:3962-3965`）。在"永远没有脚本"的前提下这算符合预期，但如果将来接 GDScript 行为，语义就是错的。
- **`<optgroup>` 的 label 不绘制**：下拉绘制循环只用 option 的 label（`html_select_node.cpp:216-228`），分组标题看不见。
- **`<big>` 无 UA 规则**：退化成普通字号（`css_default_stylesheet.h:54-70` 里没有 `big`）。
- **`<meta charset>` 被忽略**：源码一律按 UTF-8 解码（`litehtml_document_adapter.cpp:56-58`）；`<base target>` 只反射不使用。
- **表格遗留属性未映射**：`cellpadding`、`valign`、`td/th` 的 `align` 都不映射（`el_table.cpp:23-46`），只有 `width/height/cellspacing/border/bgcolor` 生效。
- **`<hr>` 的遗留属性只反射不生效**（align/color/noshade/size/width）。
- **`title` / `abbr[title]` 没有悬停提示气泡**：`HTMLLabel` 侧无 tooltip 代码（MarkdownLabel 只有链接/图片的 tooltip）。

---

## 4. CSS 缺口（按"文档用途"排序）

现有实现已覆盖 Grid+Subgrid、容器查询、`:has/:is/:where`、`@layer`、`@scope`、`oklch/lab/lch/color-mix`、相对颜色、mask、clip-path、3D transform、scroll snap、`@counter-style`、`::marker`。真正的缺口偏"文档型 / CJK / 无障碍"：

| # | 缺口 | 证据 |
| --- | --- | --- |
| 1 | **分页 / 打印媒体整体缺失** | 无 `@page`、无 `break-before/after/inside`、无 `orphans`/`widows`、无分页布局。且后端硬编码 `media_type_screen`（`backend/litehtml_container.cpp:647`），所以 `@media print` **永久不匹配**——作者的打印样式表全部失效 |
| 2 | **多列布局缺失** | `columns` / `column-count` / `column-width` / `column-rule*` / `column-span` / `column-fill` 不在属性表中（`css/css_declarations.cpp:74-386`） |
| 3 | **竖排缺失** | 无 `writing-mode`、无 `text-orientation`；`writing-mode: vertical-rl` 被列为 `unknown_property` 的示例（`html_css_support.md:776`）；引擎显式只做水平书写（`css_declarations.cpp:570`、`css_units.cpp:46`）。古籍/竖排诗词不可行 |
| 4 | **无障碍/输入类媒体特性缺失** | 只有 `prefers-color-scheme`；`prefers-reduced-motion`、`forced-colors`、`prefers-contrast`、`hover`、`pointer`、`any-hover` 全无（`css/css_media_query.cpp:546-598`）。Windows 高对比度（本项目主力平台）无响应 |
| 5 | `@property` | 无类型化自定义属性，`@property` 报 unknown rule |
| 6 | `::first-line` / `::first-letter` | 在解析器里被注释掉（`css/css_selectors.cpp:566,587`），首字下沉/首行样式不可表达 |
| 7 | `:dir()` / `unicode-bidi` | `:dir()` 不可解析；`unicode-bidi` 非支持属性（只在 `all` 重置里被排除）。`direction: rtl` 的逻辑属性映射是有的，但文本 bidi 重排主要靠 Godot 单个 text run |
| 8 | `shape-outside` / `shape-margin` | 无属性，非矩形绕排不可用 |
| 9 | 低频：`mask-border`、`background-clip: text`、`@namespace`、`@starting-style`、scroll-driven `animation-timeline`、`::part`/`::slotted` | 各自已标注为 pending 或直接不支持 |

---

## 5. 其他基础能力

- **无障碍**：只有 `get_accessible_text()` 返回一整段纯文本。没有元素级 a11y 树、没有 role/state/action，屏幕阅读器无法按标题/表格/表单结构导航。
- **HTML → PDF / 分页导出**：PDF 模块只做 PDF 的读写与表单，没有 HTML→PDF 通路；配合缺 `@page` 与分页，一个"文档渲染器"实际无法产出分页版面。可以用 `SubViewport` + `HTMLLayout.draw()` 出整页位图，但拿不到页级语义。
- **坐标/文档模型**：一个 label 一个文档、一个 selection，无多文档、无跨文档选区。
- **文档错误**：`doc_classes/HTMLLayout.xml` 明显过时（说边框只有 4 种样式、transform 只有 2D、选择器只有基础类型），会让人严重低估实现；而且 `html_css_support.md` 里没有"已知不支持"章节（`writing-mode` 只能从诊断示例里反推）。

---

## 6. 建议优先级

**P0 — 事件层（这是"GDScript 后端"的前提，不是可选项）**

1. 引入 `HTMLEvent`（`type`/`target`/`current_target`/`phase`/`cancelable`/`default_prevented`）与 `HTMLElement.add_event_listener(type, callable, options)` / `remove_event_listener` / `dispatch_event`。
2. 先覆盖最小可用集：`click`、`dblclick`、`contextmenu`、`mousedown/up`、`mouseenter/leave`、`focus/blur`、`keydown/up`、`input`、`change`、`submit`（可取消）、`scroll`、`DOMContentLoaded`。
3. 冒泡沿祖先链走（`closest` 已经有，事件委托是文档 UI 的关键）；`preventDefault` 要在默认行为之前生效——现在默认行为先跑，这个顺序必须改。
4. 内联 `on*` 属性可以解释为"在该元素上按 `type="text/gdscript"` 挂一个行为"，这样保持零 JS 且不自造表达式语言。

**P1 — 脚本宿主**

5. `<script src>` 走 `html_resource_paths.cpp` 的资源解析（`.gd` 或 `.gd` 打包资源），支持相对路径与 `<base href>`；内联 body 可选支持为一段 GDScript。
6. 挂载失败必须 `push_warning` + 进入诊断；`_attach_document` 增加 `get_label()`，别让首选钩子成死路。
7. `HTMLLabel.fetch(url, callback)`（复用现成的 `queue_http_request`）、`set_timeout/set_interval/on_frame`（`_detach` 自动清理）、`remount()`/热重载。
8. `storage` 与 `location/history`：`user://` 下按文档标识命名空间化；`navigate(url)` + 前进后退栈。

**P2 — 文档形态**

9. `@media print` 真正生效 + `@page`/`break-*`/`orphans`/`widows` + 分页 API（配合已有的 PDF 写出，可做 HTML→PDF）。
10. 多列布局；竖排 `writing-mode`。
11. `contenteditable`（若目标是"文档编辑器"则提升到 P1）。
12. `<canvas>` 的 GDScript 绘制 API（若目标包含图表/白板）。
13. `prefers-reduced-motion` / `forced-colors` 与元素级无障碍树。

---

## 附：关键文件索引

| 主题 | 文件 |
| --- | --- |
| 元素工厂 / 标签特化 | `src/formats/html/backend/litehtml_container.cpp:141-162` |
| DOM 变更 → 布局失效 | `src/formats/html/dom/html_dom_mutations.cpp`、`html_dom.cpp:1429+` |
| 行为挂载 | `src/nodes/html_label_behaviors.cpp`（无 `.h`，成员在 `html_label.h:430-437, 478-482`） |
| 行为基类 | `src/formats/html/html_behavior.h/.cpp` |
| 输入与信号派发 | `src/nodes/html_label.cpp:165-482`（鼠标）、`:584-708`（键盘） |
| 表单 | `src/nodes/html_label_forms.cpp`、`src/formats/html/elements/html_form_state.cpp` |
| 网络（内部） | `src/formats/http_requests.h`、`src/formats/remote_images.cpp`、`src/formats/html/resources/html_remote_stylesheets.cpp` |
| 资源路径解析（无 script 分支） | `src/formats/html/resources/html_resource_paths.cpp` |
| 媒体特性白名单 | `src/formats/html/css/css_media_query.cpp:546-598` |
| 属性总表 | `src/formats/html/css/css_declarations.cpp:74-386` |
| CSS 支持文档 | `addons/docrender/docs/html_css_support.md` |
| API / Web 标准对照 | `addons/docrender/docs/html_api_reference.md` |
