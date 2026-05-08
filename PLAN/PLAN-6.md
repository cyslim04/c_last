# 顶部交互与流程总览页优化方案

## Summary
- 修复顶部“系统管理员/用户入口”和 “MetaMask 连接入口”展开后的遮挡问题，改为稳定的顶层弹出层方案。
- 审计页只把“后台日志”折叠一级，默认隐藏，点击“查看日志”后再展开；交易记录和项目矩阵保持直出。
- 顶部用户入口改成“双触点”逻辑：点击头像进入个人中心，点击文字/箭头展开会话菜单。
- 新增“流程总览”页面，提供“双模式”展示：
  - 全局固定流程说明
  - 页面内切换项目的动态流程状态
- 动态流程按项目展示 3 种状态：`completed`、`running`、`pending`。

## Key Changes
### 1. 顶部弹层与遮挡修复
- 将 `AppShell` 中的钱包菜单和用户菜单从当前局部绝对定位改为顶层弹出层。
- 使用 `Teleport` 挂载到 `body`，根据触发按钮 `getBoundingClientRect()` 计算位置，避免被顶部布局、父容器或滚动区域遮挡。
- 所有弹层统一支持：
  - 点击外部关闭
  - `Esc` 关闭
  - 路由切换后自动关闭
  - 小屏时退化为全宽底部抽屉/面板，避免右侧溢出
- 顶部操作区允许换行但不互相覆盖；钱包和用户入口最小/最大宽度固定，防止长文案把另一个入口挤压。

### 2. 用户入口逻辑调整
- 顶部用户区拆成两个明确交互目标：
  - 头像：直接跳转 `/profile`
  - 用户名/角色/箭头区域：打开小型会话菜单
- 会话菜单仅保留会话类操作：
  - `退出登录`
  - 可选保留“查看资料”次入口，但不是主路径
- 不再要求“先点用户，再点个人中心”才能进入资料页。
- 个人中心页继续承担资料修改、改密码、退出登录等完整操作。

### 3. 审计页日志折叠
- 审计页保留 3 个信息层级：
  - 最近链上回写：默认展开
  - 项目闭环状态：默认展开
  - 后台日志：默认折叠
- 在“后台日志”区块头部增加切换按钮：
  - 默认文案：`查看日志`
  - 展开后文案：`收起日志`
- 展开前只显示摘要信息：
  - 日志总数
  - 最近一条时间
  - 最近一条动作摘要
- 展开后再渲染完整日志列表；若实现复杂度更低，也可以预取数据但默认隐藏 DOM 区块。
- 本次只折叠后台日志，不折叠交易记录和项目矩阵，也不扩散到控制台页。

### 4. 新增“流程总览”页面
- 新增统一页面，例如 `/process-flow`，所有登录角色可访问，并加入侧边导航。
- 页面分为两部分：
  - 全局说明模式：固定展示平台标准 8 步流程
  - 项目动态模式：页面内项目选择器，默认选当前可见项目中最新一条
- 动态模式项目来源：
  - 复用当前角色可见项目范围
  - 管理员可看全部项目
  - 开发者/客户只能看与自己相关的项目
- 全局固定流程使用当前文档里的标准链路：
  1. 管理员登录并创建项目
  2. 分配开发者与客户
  3. 开发者绑定钱包
  4. 开发者登记工时
  5. 开发者登记交付物
  6. 发起链上存证/演示回写
  7. 客户确认或驳回
  8. 管理员审计复核结果
- 动态模式每一步展示：
  - 步骤名
  - 步骤说明
  - 当前状态：`completed / running / pending`
  - 对应证据摘要，如项目名、最新工时日期、交付状态、审批意见、交易哈希摘要

### 5. 动态流程状态判定规则
- 新增只读聚合接口，避免前端拼装多份业务规则：
  - `GET /api/process-flow`
  - `GET /api/process-flow?projectId=...`
- 返回内容包含：
  - `globalSteps`
  - `projectOptions`
  - `selectedProject`
  - `projectSteps`
  - 每一步的 `status`、`title`、`description`、`evidenceSummary`
- `projectSteps` 的判定规则固定如下：
  1. `project_created`
     - `completed`：项目存在
  2. `member_assigned`
     - `completed`：项目同时存在 `developer_id` 和 `client_id`
  3. `wallet_bound`
     - `completed`：项目开发者存在主钱包地址
  4. `worklog_recorded`
     - `completed`：该项目至少有 1 条工时
  5. `deliverable_recorded`
     - `completed`：该项目至少有 1 条交付物
  6. `evidence_written`
     - `completed`：该项目存在至少 1 条相关 `evidence_records`
     - `running`：已有工时或交付物，但仍无相关 evidence
  7. `client_confirmed`
     - `completed`：该项目存在至少 1 条 `approval_records` 或交付物状态为 `approved/rejected`
     - `running`：存在 `uploaded/notarized` 交付物但尚未确认
  8. `audit_reviewed`
     - `completed`：项目状态为 `completed`
     - `running`：前 7 步已完成但项目仍为 `active`
- 通用回退规则：
  - 某步未命中“完成”且前面已有完成步骤时，首个关键待办步骤显示 `running`
  - 其后的步骤统一显示 `pending`

## Public APIs / Interfaces
- 新增路由：
  - `/process-flow`
- 新增前端导航项：
  - `流程总览`
- 新增后端只读接口：
  - `GET /api/process-flow`
  - `GET /api/process-flow?projectId=<id>`
- 接口响应新增结构：
  - `globalSteps: Array<{ key, title, description }>`
  - `projectOptions: Array<{ id, name, status }>`
  - `selectedProject: { id, name, status } | null`
  - `projectSteps: Array<{ key, title, description, status, evidenceSummary }>`
- 不涉及数据库 schema 变更；状态聚合基于现有 `projects`、`users`、`wallet_bindings`、`worklogs`、`deliverables`、`evidence_records`、`approval_records`、`audit_logs` 计算。

## Test Plan
- 顶部用户菜单和钱包菜单在桌面宽度下展开时不遮挡、不裁切、不超出视口。
- 小屏下两个弹层都退化为可用的抽屉/全宽面板。
- 点击头像直接进入个人中心；点击用户名/角色区域只展开会话菜单。
- 审计页初始不显示后台日志列表，点击“查看日志”后展开，再次点击后收起。
- 最近交易和项目矩阵始终保持默认可见。
- 流程总览页对三类角色都可访问，导航可见。
- 流程总览页能切换不同项目，并根据项目数据展示 `completed / running / pending`。
- 对于完整闭环项目，后段步骤应显示已完成；对于进行中项目，中间步骤应显示运行中，后续步骤显示未完成。
- 项目切换后步骤摘要同步更新，且不显示当前角色无权访问的项目。
- 路由切换、点击空白、按 `Esc` 都会关闭顶部弹层。

## Assumptions
- “点击用户后在进入个人中心”按你选择的方式执行为：头像直接进入个人中心，文本区域保留会话菜单。
- “日志隐藏一级”仅作用于审计页后台日志，不扩展到交易记录、项目矩阵或控制台最近记录。
- 新的流程页对所有登录角色开放，但动态项目选择范围遵守当前角色可见权限。
- 本轮流程页优先展示现有演示业务闭环，不新增真实链上执行逻辑。
