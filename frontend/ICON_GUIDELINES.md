# Frontend Icon Guidelines

版本：v1.0
生效日期：2026-04-24
范围：frontend/src

## 1. 目标

统一整个前端的图标视觉语言，避免出现 emoji 与线性图标混搭导致的风格割裂。

## 2. 库与限制

- 图标库：lucide-react
- 禁止：在导航、按钮、标题、列表状态中使用 emoji 作为功能图标
- 允许：空状态插画位可少量使用，但优先使用 lucide-react

## 3. 尺寸规范

- 顶部主标题图标：16
- 导航项图标：14
- 操作按钮图标（action-chip）：14
- 弹窗标题图标：17-18
- 消息发送者/元信息图标：12-13
- 空状态主图标：32-44

## 4. 线宽规范

- 默认 strokeWidth：2
- 强调操作（如新增按钮图标）可用 2.2
- 同一区域避免混用过多线宽

## 5. 颜色语义

- 主品牌/重点：cyan 或 sky 系
- 中性文本图标：slate 系
- 成功：emerald
- 警告：amber
- 危险/删除：rose 或 red
- 状态点：
  - online -> status-dot-online
  - busy -> status-dot-busy
  - away -> status-dot-away
  - offline -> status-dot-offline

## 6. 布局与对齐

- 图标+文字组合统一使用 icon-label
- 纯图标按钮统一使用 icon-only
- 图标与文字间距保持 6px 左右
- 同一容器内图标基线必须对齐

## 7. 文案规则

- 系统消息、按钮文本不再添加 emoji 前缀
- 日志输出可保留纯文本标签（例如 [WS] Connected）

## 8. 新增功能检查清单

每次新增页面或组件图标前，至少确认：

- 是否使用 lucide-react
- 尺寸是否匹配本规范档位
- 是否使用了统一颜色语义
- 是否避免引入 emoji 功能图标
- 是否通过前端错误检查

## 9. 例外处理

若确实需要偏装饰性图形，需满足：

- 不影响信息识别优先级
- 不破坏整页图标一致性
- 在 PR 中说明原因和使用范围
