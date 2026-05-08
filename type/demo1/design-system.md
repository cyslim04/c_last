# SaaS Pricing Page - Design System

基于 **Tech Innovation** 主题的完整设计系统规范

---

## 🎨 Color System

### Primary Colors
```css
--color-primary: #0066ff;           /* Electric Blue - 主要品牌色 */
--color-primary-hover: #0052cc;     /* 悬停状态 */
--color-primary-active: #0047b3;    /* 激活状态 */
--color-primary-light: #3385ff;     /* 浅色变体 */
--color-primary-lighter: #e6f0ff;   /* 背景高亮 */
```

### Secondary Colors
```css
--color-secondary: #00ffff;         /* Neon Cyan - 强调色 */
--color-secondary-hover: #00e6e6;   /* 悬停状态 */
--color-secondary-active: #00cccc;  /* 激活状态 */
--color-secondary-light: #80ffff;   /* 浅色变体 */
--color-secondary-lighter: #e6ffff; /* 背景高亮 */
```

### Neutral Colors
```css
--color-dark: #1e1e1e;              /* Dark Gray - 深色背景 */
--color-dark-lighter: #2a2a2a;      /* 卡片背景 */
--color-dark-lightest: #3a3a3a;     /* 边框/分隔线 */

--color-white: #ffffff;             /* 纯白 - 文本/背景 */
--color-gray-50: #fafafa;           /* 浅灰背景 */
--color-gray-100: #f5f5f5;          /* 次要背景 */
--color-gray-200: #e5e5e5;          /* 边框 */
--color-gray-300: #d4d4d4;          /* 分隔线 */
--color-gray-400: #a3a3a3;          /* 禁用文本 */
--color-gray-500: #737373;          /* 次要文本 */
--color-gray-600: #525252;          /* 正文文本 */
--color-gray-700: #404040;          /* 标题文本 */
--color-gray-800: #262626;          /* 深色文本 */
--color-gray-900: #171717;          /* 最深文本 */
```

### Semantic Colors
```css
--color-success: #10b981;           /* 成功状态 */
--color-success-light: #d1fae5;     /* 成功背景 */
--color-warning: #f59e0b;           /* 警告状态 */
--color-warning-light: #fef3c7;     /* 警告背景 */
--color-error: #ef4444;             /* 错误状态 */
--color-error-light: #fee2e2;       /* 错误背景 */
--color-info: #3b82f6;              /* 信息状态 */
--color-info-light: #dbeafe;        /* 信息背景 */
```

### Surface Colors
```css
--surface-base: #ffffff;            /* 基础表面 */
--surface-raised: #fafafa;          /* 抬升表面 */
--surface-overlay: rgba(0, 0, 0, 0.5); /* 遮罩层 */
```

---

## 📝 Typography System

### Font Families
```css
--font-sans: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Roboto', 'Helvetica Neue', Arial, sans-serif;
--font-mono: 'SF Mono', 'Monaco', 'Inconsolata', 'Fira Code', 'Droid Sans Mono', monospace;
```

### Font Sizes (Type Scale)
```css
--text-xs: 0.75rem;      /* 12px - 标签、辅助文本 */
--text-sm: 0.875rem;     /* 14px - 次要文本 */
--text-base: 1rem;       /* 16px - 正文 */
--text-lg: 1.125rem;     /* 18px - 大正文 */
--text-xl: 1.25rem;      /* 20px - 小标题 */
--text-2xl: 1.5rem;      /* 24px - 标题 */
--text-3xl: 1.875rem;    /* 30px - 大标题 */
--text-4xl: 2.25rem;     /* 36px - 特大标题 */
--text-5xl: 3rem;        /* 48px - 超大标题 */
--text-6xl: 3.75rem;     /* 60px - 巨大标题 */
```

### Font Weights
```css
--font-light: 300;
--font-normal: 400;
--font-medium: 500;
--font-semibold: 600;
--font-bold: 700;
--font-extrabold: 800;
```

### Line Heights
```css
--leading-none: 1;
--leading-tight: 1.25;
--leading-snug: 1.375;
--leading-normal: 1.5;
--leading-relaxed: 1.625;
--leading-loose: 2;
```

### Letter Spacing
```css
--tracking-tighter: -0.05em;
--tracking-tight: -0.025em;
--tracking-normal: 0;
--tracking-wide: 0.025em;
--tracking-wider: 0.05em;
--tracking-widest: 0.1em;
```

---

## 📏 Spacing System (4px Base)

```css
--space-0: 0;
--space-1: 0.25rem;   /* 4px */
--space-2: 0.5rem;    /* 8px */
--space-3: 0.75rem;   /* 12px */
--space-4: 1rem;      /* 16px */
--space-5: 1.25rem;   /* 20px */
--space-6: 1.5rem;    /* 24px */
--space-8: 2rem;      /* 32px */
--space-10: 2.5rem;   /* 40px */
--space-12: 3rem;     /* 48px */
--space-16: 4rem;     /* 64px */
--space-20: 5rem;     /* 80px */
--space-24: 6rem;     /* 96px */
--space-32: 8rem;     /* 128px */
```

---

## 🔲 Border Radius System

```css
--radius-none: 0;
--radius-sm: 0.125rem;    /* 2px */
--radius-base: 0.25rem;   /* 4px */
--radius-md: 0.375rem;    /* 6px */
--radius-lg: 0.5rem;      /* 8px */
--radius-xl: 0.75rem;     /* 12px */
--radius-2xl: 1rem;       /* 16px */
--radius-3xl: 1.5rem;     /* 24px */
--radius-full: 9999px;    /* 完全圆角 */
```

---

## 🌑 Shadow System

### Elevation Shadows
```css
--shadow-sm: 0 1px 2px 0 rgba(0, 0, 0, 0.05);
--shadow-base: 0 1px 3px 0 rgba(0, 0, 0, 0.1), 0 1px 2px -1px rgba(0, 0, 0, 0.1);
--shadow-md: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -2px rgba(0, 0, 0, 0.1);
--shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -4px rgba(0, 0, 0, 0.1);
--shadow-xl: 0 20px 25px -5px rgba(0, 0, 0, 0.1), 0 8px 10px -6px rgba(0, 0, 0, 0.1);
--shadow-2xl: 0 25px 50px -12px rgba(0, 0, 0, 0.25);
```

### Glow Effects (for Tech Innovation theme)
```css
--glow-primary: 0 0 20px rgba(0, 102, 255, 0.3);
--glow-primary-strong: 0 0 30px rgba(0, 102, 255, 0.5);
--glow-secondary: 0 0 20px rgba(0, 255, 255, 0.3);
--glow-secondary-strong: 0 0 30px rgba(0, 255, 255, 0.5);
```

### Inner Shadows
```css
--shadow-inner: inset 0 2px 4px 0 rgba(0, 0, 0, 0.05);
```

---

## 🎯 Component States

### Button States
```css
/* Primary Button */
.btn-primary {
  background: var(--color-primary);
  color: var(--color-white);
  border: none;
  box-shadow: var(--shadow-sm);
  transition: all 150ms ease-out;
}

.btn-primary:hover {
  background: var(--color-primary-hover);
  box-shadow: var(--shadow-md), var(--glow-primary);
  transform: translateY(-1px);
}

.btn-primary:active {
  background: var(--color-primary-active);
  box-shadow: var(--shadow-sm);
  transform: translateY(0);
}

.btn-primary:focus-visible {
  outline: 2px solid var(--color-primary);
  outline-offset: 2px;
}

.btn-primary:disabled {
  background: var(--color-gray-300);
  color: var(--color-gray-500);
  cursor: not-allowed;
  box-shadow: none;
  transform: none;
}

/* Secondary Button */
.btn-secondary {
  background: transparent;
  color: var(--color-primary);
  border: 2px solid var(--color-primary);
  transition: all 150ms ease-out;
}

.btn-secondary:hover {
  background: var(--color-primary-lighter);
  border-color: var(--color-primary-hover);
}

.btn-secondary:active {
  background: var(--color-primary-light);
}
```

### Card States
```css
.card {
  background: var(--surface-base);
  border: 1px solid var(--color-gray-200);
  border-radius: var(--radius-xl);
  box-shadow: var(--shadow-sm);
  transition: all 200ms ease-out;
}

.card:hover {
  box-shadow: var(--shadow-lg);
  transform: translateY(-2px);
  border-color: var(--color-primary-light);
}

.card.featured {
  border: 2px solid var(--color-primary);
  box-shadow: var(--shadow-md), var(--glow-primary);
}
```

### Input States
```css
.input {
  background: var(--surface-base);
  border: 1px solid var(--color-gray-300);
  border-radius: var(--radius-md);
  transition: all 150ms ease-out;
}

.input:hover {
  border-color: var(--color-gray-400);
}

.input:focus {
  outline: none;
  border-color: var(--color-primary);
  box-shadow: 0 0 0 3px var(--color-primary-lighter);
}

.input:disabled {
  background: var(--color-gray-100);
  color: var(--color-gray-400);
  cursor: not-allowed;
}

.input.error {
  border-color: var(--color-error);
}

.input.error:focus {
  box-shadow: 0 0 0 3px var(--color-error-light);
}
```

---

## 📱 Responsive Breakpoints

```css
--breakpoint-sm: 640px;    /* 小屏手机 */
--breakpoint-md: 768px;    /* 平板竖屏 */
--breakpoint-lg: 1024px;   /* 平板横屏/小笔记本 */
--breakpoint-xl: 1280px;   /* 桌面 */
--breakpoint-2xl: 1536px;  /* 大屏桌面 */
```

---

## ⚡ Animation & Transitions

### Duration
```css
--duration-fast: 150ms;
--duration-base: 200ms;
--duration-slow: 300ms;
--duration-slower: 400ms;
```

### Easing Functions
```css
--ease-in: cubic-bezier(0.4, 0, 1, 1);
--ease-out: cubic-bezier(0, 0, 0.2, 1);
--ease-in-out: cubic-bezier(0.4, 0, 0.2, 1);
--ease-bounce: cubic-bezier(0.68, -0.55, 0.265, 1.55);
```

### Common Transitions
```css
--transition-colors: color var(--duration-base) var(--ease-out),
                     background-color var(--duration-base) var(--ease-out),
                     border-color var(--duration-base) var(--ease-out);

--transition-transform: transform var(--duration-base) var(--ease-out);

--transition-shadow: box-shadow var(--duration-base) var(--ease-out);

--transition-all: all var(--duration-base) var(--ease-out);
```

---

## 🎯 Pricing Card Specific Tokens

### Pricing Tiers
```css
/* Starter Tier */
--tier-starter-bg: var(--surface-base);
--tier-starter-border: var(--color-gray-300);
--tier-starter-accent: var(--color-gray-600);

/* Professional Tier (Featured) */
--tier-pro-bg: var(--surface-base);
--tier-pro-border: var(--color-primary);
--tier-pro-accent: var(--color-primary);
--tier-pro-badge-bg: var(--color-primary);
--tier-pro-badge-text: var(--color-white);

/* Enterprise Tier */
--tier-enterprise-bg: var(--color-dark);
--tier-enterprise-border: var(--color-dark-lightest);
--tier-enterprise-accent: var(--color-secondary);
--tier-enterprise-text: var(--color-white);
```

### Pricing Typography
```css
--price-currency-size: var(--text-xl);
--price-amount-size: var(--text-6xl);
--price-period-size: var(--text-base);
--price-label-size: var(--text-sm);
```

---

## ♿ Accessibility Standards

### Contrast Ratios (WCAG AA)
- Normal text: minimum 4.5:1
- Large text (18px+): minimum 3:1
- UI components: minimum 3:1

### Focus Indicators
```css
--focus-ring-width: 2px;
--focus-ring-offset: 2px;
--focus-ring-color: var(--color-primary);
```

### Touch Targets
- Minimum size: 44×44px (iOS) / 48×48px (Material)
- Minimum spacing: 8px between targets

---

## 📋 Usage Guidelines

### Do's ✅
- Use semantic color tokens (primary, secondary, error) instead of raw hex values
- Maintain consistent spacing using the 4px base system
- Apply appropriate shadows for elevation hierarchy
- Use glow effects sparingly for emphasis on key CTAs
- Ensure all interactive elements have clear hover/focus states
- Test contrast ratios in both light and dark contexts

### Don'ts ❌
- Don't use emojis as icons (use SVG icons instead)
- Don't mix different shadow styles arbitrarily
- Don't animate width/height (use transform instead)
- Don't create custom spacing values outside the system
- Don't rely on color alone to convey information
- Don't disable zoom on mobile viewports

---

## 🔗 Integration with Tech Innovation Theme

This design system extends the **Tech Innovation** theme with:
- High-contrast color pairings for clarity
- Glow effects for modern tech aesthetic
- Bold typography scale for impact
- Smooth animations for premium feel
- Dark mode support for versatility

**Primary Use Case:** SaaS pricing pages, product comparison tables, feature showcases, and conversion-focused landing pages.
