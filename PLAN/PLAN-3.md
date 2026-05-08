## 全站标题与系统文案极简化方案

### Summary
- 把所有业务页的首屏统一收缩为“页面名称 + 必要按钮”，去掉进入页面后的大段说明、右侧解释卡、首屏统计解释和营销化命名。
- 系统级固定文案统一精简，只保留简单功能名称、状态值、数字值和必要操作；项目说明、审计意见、日志详情等业务内容继续保留。
- 不改后端接口和功能流转；保留现有功能，只做展示层减法，并保留当前工作区里已经存在的项目管理跳转/紧凑 Hero 改动。

### Key Changes
- 共享首屏组件：
  - 在 [PageHero.vue](C:\Users\yce\Desktop\c_last\frontend\src\components\PageHero.vue) 增加 `variant="minimal"`，默认仍为 `default`。
  - `minimal` 只渲染标题和 `actions` 插槽；隐藏 `eyebrow`、`description`、`badges`、`stats` 和默认视觉区。
  - 所有使用 `PageHero` 的页面统一切到 `minimal`，并停止传入长说明文案。
- 页面标题统一规则：
  - 各页首屏标题统一采用路由/菜单中的中文功能名，不再使用“品牌级链路地图”“审计剧场”“角色资源台”这类包装性标题。
  - `DashboardView` 固定显示 `控制台`，不再优先使用后端 `overview.headline/subline`。
  - `ProcessFlowView` 固定显示 `流程总览`，不再显示后端 `globalHint.title/summary` 作为首屏标题说明。
  - 登录页不走 `PageHero`，但同步改成极简头部，只保留品牌名/登录入口/必要按钮。
- 系统固定说明精简：
  - 在 [AppShell.vue](C:\Users\yce\Desktop\c_last\frontend\src\components\AppShell.vue) 去掉品牌副标题、侧边栏上下文说明段、导航小字 `hint`、顶部 `cue` 说明、导览 banner 的说明段，只保留功能名称、角色、当前页名和操作按钮。
  - 各业务页去掉首屏 `hero-side-card`、`SignalRing` 的 `subtitle`、`state-explain-card` 里纯说明型句子，以及空状态的长解释句；空状态改成短语式提示，例如“暂无项目”“暂无记录”“暂无待审项目”。
  - 保留表单标签、按钮文案、状态标签、数字统计值、项目/成员/日志等业务数据；保留 `project.description`、`approvalComment`、`summary`、`log.detail`、`tx.comment` 这类业务内容，不做删除。
- 页面层落地范围：
  - `frontend/src/views` 下所有业务页统一移除首屏长说明，并把首屏收缩为标题+按钮。
  - 登录页同步去掉欢迎说明、模式说明卡中的长段文字；快捷登录、普通登录、主题切换、导览开关这些基础功能保留。
  - 已存在的 `ProjectManagementView` / `ProcessFlowView` 跳转与紧凑化改动不回退，只在其上继续做“文案减法”。

### Public Interfaces
- 新增组件接口：`PageHero.variant: "default" | "minimal"`，默认 `"default"`。
- 首屏标题以路由 `meta.title` 或与导航一致的中文功能名为准，不再依赖后端返回的营销化 headline/subline。
- 无后端接口、数据结构、数据库变更。

### Test Plan
- 构建验证：`frontend` 执行 `npm run build` 通过。
- 页面首屏检查：
  - `控制台 / 成员管理 / 新增项目 / 项目管理 / 工时记录 / 交付物 / 客户确认 / 流程总览 / 审计管理 / 个人中心 / 登录` 首屏都只保留简单标题和必要按钮。
  - 不再显示 Hero description、右侧解释卡、首屏统计小字说明、英文 eyebrow。
- 系统壳层检查：
  - 侧边栏导航只显示功能名称，不再显示 hint。
  - 顶部栏不再显示长 `cue` 文案。
  - 导览 banner 若保留，只有幕标题与操作按钮，不再有说明段落。
- 文案边界检查：
  - 业务内容仍可见：项目说明、审计意见、日志详情、交付摘要、确认意见不被误删。
  - 纯系统说明被压缩：空状态、提交后说明、模式说明、引导说明等变成短提示或直接移除。
- 回归检查：
  - 登录、快捷角色切换、项目管理筛选、项目卡片跳流程总览、流程总览项目切换、审计操作等现有功能不受影响。

### Assumptions
- “仅保留基础功能”解释为：保留现有能力与操作入口，只压缩展示层说明文案，不删业务功能。
- “去除大量文字说明”只针对系统固定文案，不删除用户录入或业务返回的内容字段。
- 全站标题以中文功能名统一，不再保留英文副标题或品牌化包装标题。
