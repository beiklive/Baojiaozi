# 字体资源

设计器启动时从这里加载三个字体文件（见 `designer/Fonts.cpp`）：

| 文件 | 用途 | 码位 |
|---|---|---|
| `switch_font.ttf` | 主文本字体（HOS 转出，含 CJK） | 全部文本字形 |
| `switch_icons.ttf` | NintendoExt 按键图标 | `U+E0E0`…`U+E105` 等 |
| `MaterialIcons-Regular.ttf` | 谷歌 Material 图标 | `U+E001`…`U+EA28` |

`switch_icons.ttf` 的 1024 个私用区码位里有 553 个与 `MaterialIcons` 重叠，两侧都是实心字形
（例如 `play_arrow` `U+E037`、`save` `U+E161`），而 ImGui 合并字体时先声明的源占住码位。
因此按键图标与 Material 图标按 `designer/Fonts.h` 的码位表分配，加载时用
`ImFontConfig::GlyphExcludeRanges` 让每个字体只提供自己那份，归属与
`GUI_DEV/framework/ui/Icons.h` 一致。

主字体缺失时退回系统 CJK 字体（macOS `PingFang.ttc`、Windows `msyh.ttc`、Linux Noto），
再退回 ImGui 内置字体；此时界面仍可运行，但图标会缺字形。
