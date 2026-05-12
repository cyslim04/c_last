# Stride 模板 Vue 化接入可行性方案

## Summary
可行，但**只能按“重组迁移”做，不适合直接搬运**。  
原因已经明确：

- 现项目前端是 `Vue 3 + Vite + vue-router + Pinia`
- `stride-dev-1.3.0` 是 `Eleventy + Nunjucks + Bootstrap 5 + Gulp` 的静态落地页模板
- `http://localhost:3000/` 当前展示的是一套营销型单页，不是可直接嵌入现有应用壳的 Vue 页面

最终采用的实现方向应为：

- 保留当前多路由应用结构与权限守卫
- 全站视觉统一向 Stride 靠拢，而不是只替换首页
- 以 **Vue 重写 + 引入 Bootstrap 5 utility/grid** 的方式落地
- 按 **业务语义化改写**，不做对 `localhost:3000` 的 1:1 复刻

## Key Changes
### 1. 接入边界与技术路线
- 不引入 Eleventy、Nunjucks、Gulp 进入当前仓库构建链。
- 仅引入 Bootstrap 5 作为布局与 utility 层；交互仍由 Vue 控制，不依赖 Bootstrap JS 作为主交互框架。
- 不直接拷贝 `index.njk` 成 HTML 页面；按其区块结构拆成 Vue 路由页与可复用展示组件。
- 保留现有后端接口、登录态、角色权限、路由地址与 API 调用方式，不改业务协议。

### 2. 全站信息架构
- 导航模型保持“多路由为主”。
- 首页改造成完整 Stride 风格落地页，承担平台介绍、角色导流、流程预览、信任背书与 CTA。
- 业务页继续独立路由存在，但统一改成“落地页化业务页面”：
  - 每页保留明显 Hero
  - 表单、列表、说明区拆成纵向 section
  - 降低当前控制台式密度，增加模板化留白、分段、图片/图形占位与叙事节奏
- 不把全部业务塞回单页锚点，不改变当前 `members / projects / worklogs / audit / process-flow` 的路由职责。

### 3. 设计系统与组件策略
- 以当前全局样式为基础，重建一套 Stride 化设计 token：
  - Bootstrap 栅格与间距节奏
  - 更强的纵向 section rhythm
  - 更接近模板的字体层级、标题比例、容器宽度、图片占位方式、分段背景切换
- 顶部壳层改成 Bootstrap 风格 navbar 语义，但保留当前角色导航与会话/钱包行为。
- 首页建议按 Stride 的 `navbar + hero + feature + gallery + stats + testimonial/credibility + CTA + footer` 逻辑重构为业务版本。
- 业务页统一采用 1 个页面模板骨架：
  - Hero 区
  - 主操作区
  - 最近记录/当前状态区
  - 说明或可信背书区
- 当前 `PageHero`、共享 panel/card、导航壳层需要一起升级，否则只能得到“首页像模板，其他页不像”的半套结果。

### 4. 模板资源迁移策略
- 可迁移：
  - 区块结构
  - 容器/网格比例
  - Bootstrap utility 用法
  - section 节奏与视觉语言
- 需重写：
  - 所有 Nunjucks include
  - navbar/footer 逻辑
  - AOS 动画触发
  - 表单与业务交互
- 默认不直接整包迁移模板中的人物/室内图片；优先替换为项目相关视觉、系统截图、流程插图或抽象背景，以避免资产语义不匹配和版权噪音。
- 若后续要追求高保真视觉，可单独评估是否筛选迁移字体与少量背景图，但不作为第一阶段前提。

## Public Interfaces / Constraints
- 路由地址不变：`/home`、`/projects`、`/worklogs`、`/audit` 等继续保留。
- 认证与权限守卫不变。
- 后端 API 与前端 `http.js` 调用方式不变。
- 仅新增前端样式/依赖层：
  - 引入 Bootstrap 5 CSS/SCSS
  - 可能新增少量展示组件与页面骨架组件
- 不引入第二套 dev server，不把 `localhost:3000` 模板项目并入当前运行链。

## Test Plan
- 构建验证：
  - `frontend` 执行 `npm run build` 通过
  - 新引入的 Bootstrap 样式不导致现有 Vue 编译或资源引用异常
- 路由与权限：
  - 管理员、开发者、客户三种角色登录后，导航项与跳转仍正确
  - 非授权角色访问受限页仍会被守卫重定向
- 页面表现：
  - 首页具备完整 Stride 风格落地页节奏
  - 至少抽查 `projects`、`worklogs`、`audit`、`process-flow` 四类典型页，确认“业务可操作 + 视觉统一”
- 交互稳定性：
  - 钱包菜单、会话菜单、表单提交、列表展示、状态标签、流程总览入口都不受样式重构影响
- 响应式：
  - 重点验证桌面宽屏、1366 宽度、平板、窄屏手机下的 navbar、hero、section 间距与业务表单布局
- 样式扫尾：
  - 二次检查残留的旧式 glow / blur / role-color / gradient / radius 碎片，避免出现首页和业务页风格断层

## Assumptions
- 默认采用“**Bootstrap 5 仅作为布局/utility 层**，Vue 仍是唯一页面与交互实现层”。
- 默认采用“**语义化改写**”，即 Trust Work 业务内容替换模板原始营销文案与虚构内容。
- 默认采用“**全站统一视觉，但不改后端、不改路由、不改单页应用核心结构**”。
- 默认第一阶段不直接复用模板全部图片资产，而是先完成结构与视觉语言迁移。
- 如果后续实施，建议按三步推进：
  1. 先改全局壳层与首页
  2. 再改典型业务页模板
  3. 最后批量统一剩余页面与样式碎片
