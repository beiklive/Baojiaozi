# Baojiaozi

Baojiaozi 是 JiaoZiPi 的主题描述运行时和可视化设计器。

当前开发顺序和验收标准见 [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)。

## 依赖初始化

```bash
git submodule update --init --recursive
```

## 构建

```bash
cmake -S . -B build/dev -DBAOJIAOZI_BUILD_DESIGNER=ON
cmake --build build/dev
```

## 设计原则

- 页面、组件、样式和动画由 JSON 描述。
- 业务动作来自 JiaoZiPi 或核心生成的接口清单。
- 设计器预览使用和模拟器相同的运行时与 ImGui 渲染器。
- 设计器本身使用 ImGui 编写，不要求设计器外壳也由 JSON 描述。
