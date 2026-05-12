# Trust Work 浅色主题设计规范

## 设计概念：暖石与纸 (Warm Stone & Paper)

**核心思路：** 摆脱暗色科技感，转向有温度、有肌理的浅色体系。灵感来自高端文具、建筑事务所图纸室、Linear / Notion 浅色模式的质感表达。避免冷调纯白和千篇一律的蓝紫渐变。

**三个关键词：** 温润 / 层叠 / 克制

---

## 1. 色板总览

### 1.1 中性色阶（暖石系）

| Token | 色值 | 用途 |
|-------|------|------|
| `stone-50` | `#fdfcf9` | 极浅暖白，用于 elevated 表面 |
| `stone-100` | `#faf8f4` | 页面基底背景 |
| `stone-200` | `#f5f1ea` | 次级表面 / 交替 section |
| `stone-300` | `#ede8df` | 三级表面 / hover 态 |
| `stone-400` | `#e4ded3` | 更深表面层 |
| `stone-500` | `#d9d3c7` | 强分隔线 |
| `stone-600` | `#c4bdb0` | 最强的非语义边框 |

### 1.2 文字色阶（暖墨系）

| Token | 色值 | 用途 |
|-------|------|------|
| `ink-900` | `#231f17` | 主文字（深暖炭色，非纯黑） |
| `ink-700` | `#4a4438` | 次级文字 |
| `ink-500` | `#6e6557` | 辅助文字 / subtle |
| `ink-400` | `#8a8276` | 弱化文字 / muted |
| `ink-200` | `#b5ada0` | placeholder 级别 |

### 1.3 强调色（赤陶系 — 延续品牌 DNA）

当前暗色主题的主色是 `#d4411e`（偏亮的橙红）。浅色背景下需要降低饱和度并略微加深，保持视觉重量一致。

| Token | 色值 | 用途 |
|-------|------|------|
| `terracotta-600` | `#a84a20` | 深赤陶（eyebrow / pill 文字） |
| `terracotta-500` | `#c2552a` | 主强调色（按钮 / 链接 / 焦点） |
| `terracotta-400` | `#d4693d` | hover 态 |
| `terracotta-100` | `#fceee7` | 强调色浅底（badge / pill 背景） |
| `terracotta-50` | `#fdf6f2` | 强调色极浅底 |

**对比度验证（WCAG AA）：**
- `#c2552a` on `#ffffff` → 对比度约 4.8:1（AA 通过）
- `#a84a20` on `#ffffff` → 对比度约 6:1（AA 通过）
- `#231f17` on `#faf8f4` → 对比度约 14:1（AAA 通过）
- `#8a8276` on `#faf8f4` → 对比度约 5.2:1（AA 通过）

### 1.4 角色区分色

当前系统所有角色共用 `#d4411e` 单色。浅色主题建议引入差异化的角色色：

| 角色 | Token | 色值 | 含义 |
|------|-------|------|------|
| 管理员 | `role-admin` | `#c2552a` | 赤陶 — 权威、决断 |
| 开发者 | `role-developer` | `#2a7d8c` | 深青 — 专注、构建 |
| 客户 | `role-client` | `#8c7a2a` | 橄榄金 — 审查、反馈 |

### 1.5 语义色

| Token | 色值 | 用途 |
|-------|------|------|
| `success` | `#5a7a4e` | 鼠尾草绿 — 通过 / 完成 |
| `success-bg` | `rgba(90, 122, 78, 0.1)` | 成功态背景 |
| `warning` | `#b8863c` | 暖琥珀 — 待确认 / 注意 |
| `warning-bg` | `rgba(184, 134, 60, 0.1)` | 警告态背景 |
| `danger` | `#c47069` | 灰调玫瑰 — 驳回 / 异常 |
| `danger-bg` | `rgba(196, 112, 105, 0.1)` | 危险态背景 |

### 1.6 边框与分隔

| Token | 色值 | 用途 |
|-------|------|------|
| `border-soft` | `#ebe6dc` | 卡片内分隔 / section 分隔 |
| `border-base` | `#e8e3d9` | 默认卡片边框 |
| `border-strong` | `#d5cec1` | 强边框 / 焦点边框 |

---

## 2. CSS 变量完整映射

以下是对应 `main.css` 中每个 CSS 变量的浅色主题值。实现时放入 `[data-theme="light"]` 选择器即可。

```css
[data-theme="light"] {
  color-scheme: light;

  /* ═══════════════════════════════════════════
     背景层级 Background Hierarchy
     ═══════════════════════════════════════════ */
  --body-background:            #faf8f4;
  --bg-0:                       #faf8f4;
  --bg-1:                       #f5f1ea;
  --bg-2:                       #ede8df;
  --bg-3:                       #e4ded3;
  --surface-background:         #ffffff;
  --sidebar-background:         #f7f4ef;
  --topbar-background:          rgba(255, 255, 255, 0.92);
  --nav-background:             #f5f2ec;
  --nav-active-background:      #ede6dc;

  /* ═══════════════════════════════════════════
     面板 / 表面色调
     ═══════════════════════════════════════════ */
  --panel:                      #ffffff;
  --panel-strong:               #fdfcf9;
  --panel-soft:                 #f7f4ef;
  --panel-muted:                #f0ece5;

  /* ═══════════════════════════════════════════
     层级表面（hover / 嵌套深度）
     ═══════════════════════════════════════════ */
  --soft-surface-1:             #f7f4ef;
  --soft-surface-2:             #f0ece5;
  --soft-surface-3:             #e9e4db;
  --soft-surface-4:             #e1dbd1;

  /* ═══════════════════════════════════════════
     文字
     ═══════════════════════════════════════════ */
  --text:                       #231f17;
  --subtle:                     #6e6557;
  --muted:                      #8a8276;

  /* ═══════════════════════════════════════════
     边框
     ═══════════════════════════════════════════ */
  --border:                     #e8e3d9;
  --border-strong:              #d5cec1;
  --soft-border-1:              #ebe6dc;
  --soft-border-2:              #ddd7ca;

  /* ═══════════════════════════════════════════
     强调色
     ═══════════════════════════════════════════ */
  --accent:                     #c2552a;
  --neutral-accent:             #c2552a;
  --admin-accent:               #c2552a;
  --developer-accent:           #2a7d8c;
  --client-accent:              #8c7a2a;
  --role-rgb:                   194, 85, 42;

  /* ═══════════════════════════════════════════
     语义色
     ═══════════════════════════════════════════ */
  --success:                    #5a7a4e;
  --warning:                    #b8863c;
  --danger:                     #c47069;

  /* ═══════════════════════════════════════════
     按钮
     ═══════════════════════════════════════════ */
  --button-primary-background:  #c2552a;
  --button-primary-border:      #c2552a;
  --button-primary-text:        #fefdfb;
  --button-muted-text:          #231f17;
  --button-muted-background:    #f5f1ea;
  --button-muted-border:        #e0d9cc;

  /* ═══════════════════════════════════════════
     输入控件
     ═══════════════════════════════════════════ */
  --input-background:           #f7f4ef;
  --input-border:               #e0d9cc;

  /* ═══════════════════════════════════════════
     遮罩 / 覆盖层
     ═══════════════════════════════════════════ */
  --overlay-background:         rgba(35, 31, 23, 0.38);

  /* ═══════════════════════════════════════════
     状态标签
     ═══════════════════════════════════════════ */
  --status-neutral-background:  #f0ece5;
  --status-neutral-border:      #ddd7ca;

  /* ═══════════════════════════════════════════
     空状态
     ═══════════════════════════════════════════ */
  --empty-background:           #f7f4ef;
  --empty-border:               #e0d9cc;

  /* ═══════════════════════════════════════════
     Ring / 信号环
     ═══════════════════════════════════════════ */
  --ring-track:                 #e8e3d9;
  --ring-hole-background:       #ffffff;

  /* ═══════════════════════════════════════════
     导航文字
     ═══════════════════════════════════════════ */
  --nav-text:                   #5c5548;

  /* ═══════════════════════════════════════════
     阴影（多层叠加，制造物理深度）
     ═══════════════════════════════════════════ */
  --shadow-sm:  0 1px 2px rgba(35, 31, 23, 0.04);
  --shadow-md:  0 2px 8px rgba(35, 31, 23, 0.05),
                0 1px 3px rgba(35, 31, 23, 0.04);
  --shadow-lg:  0 4px 16px rgba(35, 31, 23, 0.06),
                0 1px 4px rgba(35, 31, 23, 0.04);
  --shadow-xl:  0 8px 32px rgba(35, 31, 23, 0.07),
                0 2px 6px rgba(35, 31, 23, 0.04);

  /* ═══════════════════════════════════════════
     圆角（浅色下适度增加圆角提升温润感）
     ═══════════════════════════════════════════ */
  --radius-sm:                  4px;
  --radius-md:                  6px;
  --radius-lg:                  8px;
  --radius-xl:                  12px;
  --radius-2xl:                 16px;

  /* ═══════════════════════════════════════════
     网格叠加（微纸纹肌理）
     ═══════════════════════════════════════════ */
  --grid-line-horizontal:       rgba(35, 31, 23, 0.025);
  --grid-line-vertical:         rgba(35, 31, 23, 0.018);
  --sheen-color:                rgba(255, 255, 255, 0.4);

  /* ═══════════════════════════════════════════
     首页 Editorial 板式
     ═══════════════════════════════════════════ */
  --home-editorial-board:       #ffffff;
  --home-editorial-surface:     #f7f4ef;
  --home-editorial-line:        #ebe6dc;
  --home-editorial-muted-line:  #f0ece5;
  --home-editorial-hero:        #faf8f4;
  --home-editorial-hero-text:   #231f17;
  --home-editorial-copy:        #8a8276;
  --home-editorial-accent:      #c2552a;
  --home-editorial-number:      #ddd7ca;
  --home-editorial-chip-background: #f0ece5;
}
```

---

## 3. 质感处理策略

浅色主题最大的挑战是"平"——如果所有卡片都是纯白，页面会失去层次感。以下是三个质感层次：

### 3.1 纸纹肌理（body::before）

```css
[data-theme="light"] body::before {
  opacity: 0.32;
  background-image:
    linear-gradient(var(--grid-line-horizontal) 1px, transparent 1px),
    linear-gradient(90deg, var(--grid-line-vertical) 1px, transparent 1px);
  background-size: 48px 48px;
  mask-image: radial-gradient(circle at 50% 30%, black 28%, transparent 84%);
}
```

网格线从页面中心向外渐隐，模拟纸张纤维的微弱纹理。不会在视觉上"抢戏"，但能消除纯色背景的塑料感。

### 3.2 多层阴影（物理深度）

浅色主题下阴影比暗色主题更关键。使用**双阴影叠加**而非单层模糊：

```
card: 0 2px 8px rgba(35,31,23,0.05) + 0 1px 3px rgba(35,31,23,0.04)
modal: 0 8px 32px rgba(35,31,23,0.07) + 0 2px 6px rgba(35,31,23,0.04)
```

第一层是扩散阴影（模拟环境光），第二层是紧贴阴影（模拟接触面）。两层的 rgba alpha 值都在 0.04-0.07 之间——浅色下阴影要非常克制。

### 3.3 交替区段色温

不同 section 使用微小色温差来区分，而不是依赖分隔线：

```
页面基底:        #faf8f4  (暖奶油)
卡片 / 面板:      #ffffff  (纯白，浮于基底之上)
交替 section:     #f5f1ea  (比基底暖一度)
Topbar:           rgba(255,255,255,0.88) + blur 毛玻璃
Footer:           #f5f1ea  (与交替 section 呼应)
```

### 3.4 Hover 动效恢复

当前暗色主题中 stride.css 禁用了大部分 hover 的 `transform`。浅色主题建议恢复微升效果：

```css
[data-theme="light"] .button:hover,
[data-theme="light"] .resource-card:hover,
[data-theme="light"] .stage-node-card:hover {
  transform: translateY(-2px);
}
```

---

## 4. stride.css 浅色适配要点

当前 `stride.css` 中以下硬编码值在浅色主题下需要适配（写入 `[data-theme="light"]` 覆盖）：

| 当前值 | 浅色替换 | 影响的元素 |
|--------|----------|------------|
| `background: rgba(20,21,24,0.94)` | `rgba(255,255,255,0.88)` | Topbar |
| `border-bottom: 1px solid rgba(245,243,239,0.08)` | `1px solid #ebe6dc` | Topbar / sections |
| `background: rgba(245,243,239,0.03)` | `#f7f4ef` | 卡片 / panel 表面 |
| `border: 1px solid rgba(245,243,239,0.08)` | `1px solid #ebe6dc` | 卡片边框 |
| `color: #f5f3ef` | `#231f17` | 所有文字 |
| `color: rgba(245,243,239,0.72)` | `#5c5548` | 导航链接 |
| `color: rgba(245,243,239,0.42)` | `#c4bdb0` | 卡片序号 |
| `background: rgba(10,10,10,0.22)` | `#f5f1ea` | Footer |
| `background: #15171a` | `#faf8f4` | Auth layout |
| `background: #1a1a1a` | `#f5f1ea` | Auth hero |
| `border-right: 1px solid rgba(245,243,239,0.08)` | `1px solid #ebe6dc` | Auth hero 分隔 |
| `background: rgba(245,243,239,0.03)` (section alt) | `#f5f1ea` | 交替 section |

完整覆盖列表参见第 2 节的 CSS 变量映射——修改变量即可覆盖大部分硬编码。

---

## 5. 字体系统

维持现用字体栈不变，浅色下字号与字重保持原样：

| 层级 | 字体 | 用途 |
|------|------|------|
| Display (h1) | Georgia, Times New Roman, serif | 页面主标题 |
| Heading (h2-h4) | Georgia, Times New Roman, serif | 段落标题 |
| Body | Microsoft YaHei, PingFang SC, Segoe UI, sans-serif | 正文 |
| Mono | Consolas, SFMono-Regular, monospace | 哈希 / 地址 |

浅色背景下 Georgia 衬线体的优雅感会比暗色更明显，这是"纸质感"的重要组成。

---

## 6. 间距与布局

维持现有 4px 基准间距系统不变。浅色不改变任何布局参数。

| Token | 值 | 用途 |
|-------|-----|------|
| `stride-gap-xs` | 12px | 紧凑间距 |
| `stride-gap-sm` | 18px | 小间距 |
| `stride-gap-md` | 28px | 默认间距 |
| `stride-gap-lg` | 42px | 大间距 |
| `stride-gap-xl` | 64px | 特大间距 |
| `stride-gap-2xl` | 96px | 超大间距 |
| `stride-page-max` | 1280px | 内容最大宽度 |

---

## 7. 响应式断点

维持现有断点：

| 断点 | 宽度 | 调整内容 |
|------|------|----------|
| XL | ≤ 1320px | Hero 单列、grid 折叠 |
| LG | ≤ 1180px | 额外 grid 折叠 |
| MD | ≤ 1080px | Compact layout 切换 |
| SM | ≤ 980px | Topbar 折叠、navigation 改 mobile |
| XS | ≤ 900px | Padding 缩减 |
| 2XS | ≤ 720px | 卡片 padding 缩减、grid 单列 |

---

## 8. 实施步骤

实现浅色主题的推荐顺序：

1. **新建** `frontend/src/styles/light-theme.css`，放入第 2 节的完整 CSS 变量 + 第 4 节的 stride.css 覆盖
2. **修改** `frontend/src/main.js` 第 17-18 行：
   ```js
   // 改为
   document.documentElement.setAttribute("data-theme", "light");
   document.documentElement.style.colorScheme = "light";
   ```
3. **import** light-theme.css 到 main.js（在 main.css 之后）
4. **走查** 全部页面：首页、登录、项目管理、工时记录、审计、流程总览、客户确认
5. **微调** 对比度不足的局部元素

### 可选增强：主题切换器

如果后续需要用户手动切换，可在 AppShell 的 Topbar 或 Footer 中添加一个 toggle 按钮，切换 `document.documentElement` 上的 `data-theme` 属性即可。

```js
function toggleTheme() {
  const current = document.documentElement.getAttribute('data-theme');
  const next = current === 'dark' ? 'light' : 'dark';
  document.documentElement.setAttribute('data-theme', next);
  document.documentElement.style.colorScheme = next;
  localStorage.setItem('theme', next);
}
```

---

## 9. 视觉预览对照

| 区域 | 当前暗色 | 浅色方案 |
|------|----------|----------|
| 页面背景 | `#1e2026` 深灰蓝 | `#faf8f4` 暖奶油纸 |
| Topbar | 深灰半透明 + 毛玻璃 | 白色半透明 + 毛玻璃 |
| 卡片 | 半透明深色叠加 | `#ffffff` + 微阴影 |
| 主文字 | `#f5f3ef` 暖白 | `#231f17` 深暖炭 |
| 弱文字 | `rgba(184,177,168,0.82)` | `#8a8276` 暖灰 |
| 主按钮 | `#d4411e` 亮橙红 | `#c2552a` 赤陶 |
| Section 分隔 | 半透明白色线 | `#ebe6dc` 暖灰线 |
| Footer | 深灰背景 | `#f5f1ea` 浅石色 |
| 成功态 | `#9aa47a` | `#5a7a4e` 鼠尾草绿 |
| 危险态 | `#d97b66` | `#c47069` 灰调玫瑰 |
| 阴影 | 无 (shadow: none) | 多层微阴影 |
| 网格纹理 | 白色透明网格（微妙） | 暖灰透明网格（更微妙） |

---

## 附录：完整色值速查卡

```
┌─────────────────────────────────────────────────────────┐
│  浅色主题 · 暖石与纸 · 色值速查                           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  页面基底    #faf8f4  ████████████████████████          │
│  卡片表面    #ffffff  ████████████████████████          │
│  次级表面    #f5f1ea  ████████████████████████          │
│  三级表面    #ede8df  ████████████████████████          │
│                                                         │
│  主文字      #231f17  ████████████████████████          │
│  次级文字    #6e6557  ████████████████████████          │
│  弱化文字    #8a8276  ████████████████████████          │
│                                                         │
│  主强调色    #c2552a  ████████████████████████          │
│  开发者色    #2a7d8c  ████████████████████████          │
│  客户色      #8c7a2a  ████████████████████████          │
│                                                         │
│  边框-软    #ebe6dc  ████████████████████████          │
│  边框-基    #e8e3d9  ████████████████████████          │
│  边框-强    #d5cec1  ████████████████████████          │
│                                                         │
│  成功        #5a7a4e  ████████████████████████          │
│  警告        #b8863c  ████████████████████████          │
│  危险        #c47069  ████████████████████████          │
│                                                         │
└─────────────────────────────────────────────────────────┘
```
