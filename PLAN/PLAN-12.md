# 管理员项目管理拆页与固定顶栏改造计划

## 摘要
- 将当前管理员的 `/projects` 从“项目管理”重定义为“新增项目”入口，同时新增一个独立的“项目管理”页面，专门查看和筛选项目状态。
- 新的项目管理页按状态分类展示项目，至少覆盖 `全部 / 进行中(active) / 已完成(completed) / 争议中(disputed) / 草稿(draft)`，并支持按开发者、客户做更细筛选。
- 顶部状态栏从当前的 `sticky` 改为真正固定在视口顶部，滚动时完全不移动；主内容区补足顶部偏移，避免被遮挡。
- “已完成项目”不再散落在别处，而是在新的管理员项目管理页中单独归类收纳，管理员可以集中查看。

## 关键改动
### 路由与导航
- 保留现有 [ProjectsView.vue](/C:/Users/yce/Desktop/c_last/frontend/src/views/ProjectsView.vue) 作为“新增项目”页面，但更新页面标题、Hero 文案和导航标签，明确它只负责创建项目。
- 新增一个管理员专用项目管理页，例如 `ProjectManagementView`，路由建议为 `/project-management`，标题为“项目管理”。
- 更新 [router/index.js](/C:/Users/yce/Desktop/c_last/frontend/src/router/index.js)：
  - `/projects` 标题改为“新增项目”
  - 新增 `/project-management`，仅管理员可访问
- 更新 [AppShell.vue](/C:/Users/yce/Desktop/c_last/frontend/src/components/AppShell.vue) 的管理员导航顺序：
  - 控制台
  - 成员管理
  - 新增项目
  - 项目管理
  - 流程总览
  - 审计管理
  - 个人中心

### 项目管理页
- 新页面只做“查看与筛选”，不承担创建逻辑。
- 直接复用当前 `GET /api/projects` 返回的全部项目列表，不新增后端接口；因为现有返回已包含：
  - `id`
  - `name`
  - `description`
  - `status`
  - `developerId`
  - `clientId`
  - `developerName`
  - `clientName`
- 前端在项目管理页本地做筛选与分组：
  - 状态筛选：`全部 / active / completed / disputed / draft`
  - 开发者筛选：基于 `developerId`
  - 客户筛选：基于 `clientId`
- 页面结构固定为三段：
  - 顶部摘要区：项目总数、进行中数、已完成数、异常数
  - 筛选区：状态 Tab + 开发者选择器 + 客户选择器 + 清空筛选
  - 项目结果区：按当前筛选后的项目卡片列表展示
- 已完成项目必须有明确可见的收纳方式：
  - 当状态切到“已完成”时只显示 completed 项目
  - 在“全部”视图下，结果区可按“进行中 / 已完成 / 异常项目”分段展示，避免 completed 混在 active 中
- 项目卡片统一展示：
  - 项目名
  - 状态
  - 开发者
  - 客户
  - 项目说明
- 不在本轮加入项目编辑、删除、归档等变更动作，避免超出当前后端能力。

### 新增项目页
- 当前 [ProjectsView.vue](/C:/Users/yce/Desktop/c_last/frontend/src/views/ProjectsView.vue) 改名义不改能力：
  - Hero 标题从“项目编排台”收敛为“新增项目”
  - 下方表单仍用于创建项目
  - 右侧“当前演示项目”列表可以保留为简版参考，也可以收缩为“最近创建项目”小列表
- 页面文案强调“这里只负责新建，不负责状态管理和筛选”。

### 固定顶栏
- 当前 [main.css](/C:/Users/yce/Desktop/c_last/frontend/src/styles/main.css) 中 `.topbar` 为 `position: sticky`，计划改为真正固定：
  - `position: fixed`
  - `top: 0`
  - `left` 与 `width` 按桌面端侧边栏宽度联动
- 为避免遮挡主内容，配套调整：
  - `.content` 增加顶部内边距或使用 CSS 变量定义 `--topbar-height`
  - `.page-slot` 的首屏内容下移，保证标题和 Hero 不被盖住
- 移动端下仍保持顶部栏固定，但宽度改为全屏，左侧抽屉式侧边栏逻辑不变。
- 固定后仍保留现有功能：
  - 钱包入口
  - 会话入口
  - 答辩导览按钮
- 本轮不再改变顶部栏的信息结构，只解决“冻结不移动”和不遮挡页面的问题。

## 接口与数据
- 后端默认不新增新接口。
- 继续使用：
  - `GET /api/projects` 作为项目管理页的数据源
  - `GET /api/users?role=developer`
  - `GET /api/users?role=client`
- 文档更新只需补充：
  - `/projects` 页面现在是“新增项目”
  - 新增前端页面 `/project-management`
  - 管理员可在该页查看 `completed / active / disputed / draft` 项目分类

## 测试与验收
- 管理员登录后，导航中可分别进入“新增项目”和“项目管理”两个独立页面。
- 新增项目页仍可正常创建项目，创建成功后项目能在新项目管理页中看到。
- 项目管理页可正确筛选：
  - 只看已完成项目
  - 只看进行中项目
  - 按指定开发者筛选
  - 按指定客户筛选
  - 组合筛选（例如：某开发者 + 已完成）
- `completed` 项目不会再混在 active 项目里不易辨认。
- 顶部状态栏在页面滚动时保持固定，不随内容滚动离开视口。
- 固定顶栏后，页面首屏内容不被遮挡；桌面端和移动端都不出现新的布局错乱或横向溢出。
- 管理员以外角色不应看到新的管理员项目管理页入口，也不能直接访问该路由。

## 假设与默认项
- 默认采用“拆成两页”的信息架构：`/projects` 为新增项目，新增 `/project-management` 为项目管理。
- 默认把系统已有全部项目状态都纳入项目管理页，而不是只看进行中和已完成。
- 默认项目管理页以只读筛选为主，不在本轮增加编辑/删除/归档能力。
- 默认“查看已完成项目”这一诉求通过新的管理员项目管理页满足，而不是额外再做一个单独完成页。
- 默认“上方状态栏冻结不移动”解释为真正固定在视口顶部，而不是继续使用 sticky。
