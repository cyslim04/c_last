# 高层泳道总体逻辑修正方案

## Summary
- 只修正 `高层流程` 这张总览泳道图的逻辑表达，不改 `阶段循环` 和 `当前快照` 的运行态结构。
- 目标有两点：
  - 高层图改成“总体说明图”，不再把节点画成 `已完成/未解锁/进行中` 这种运行态结果。
  - 明确展示开发者与客户围绕“工时/交付物 -> 客户确认 -> 驳回后补充修订 -> 再确认”的回环。
- 正式页与 `test/process-flow-prototype/` 同步修正，避免再次分叉。

## Key Changes
- 后端流程语义拆分：
  - 保留现有 `globalSteps`、`stageSummaries`、`currentStage`、`finalAuditStatus` 字段不删除，避免兼容性风险。
  - `/api/process-flow` 新增一个仅供高层总览使用的 `globalOverview` 字段，前端高层泳道只消费它，不再直接拿 `globalSteps.state` 画总览节点。
  - `globalOverview` 包含：
    - `lanes`：固定三项 `admin / developer / client`
    - `nodes`：固定七个节点
      - `project_setup`
      - `wallet_ready`
      - `developer_log_submit`
      - `client_review`
      - `developer_revise`
      - `admin_stage_audit`
      - `final_audit`
    - `edges`：前进边与回环边
    - `focusKey`：当前主焦点节点
  - `nodes[*]` 不再返回 `completed/running/locked`，改为更适合总览图的结构性焦点字段：
    - `tone`: `focus / available / neutral`
    - `kind`: `setup / loop / audit`
    - `isCurrentFocus`: `0/1`
    - `summary`：结构说明，不写“已完成”
    - `detail`：当前为何聚焦到这里
  - `edges[*]` 至少包含：
    - `from`
    - `to`
    - `kind`: `forward / loop / approval / rework`
    - `tone`: `focus / neutral`

- 高层泳道节点与回环定义：
  - 总览固定使用七节点结构，不再把“开发者阶段登记”压成一个单节点。
  - 开发者泳道拆成两个节点：
    - `developer_log_submit`：登记工时与提交交付
    - `developer_revise`：客户驳回后的补充修订
  - 客户泳道使用一个确认节点：
    - `client_review`
  - 回环边固定存在：
    - `developer_log_submit -> client_review`
    - `client_review -> developer_revise`
    - `developer_revise -> client_review`
    - `client_review -> admin_stage_audit`
  - 这样无论当前项目是否真的刚发生驳回，高层图都能正确解释“这套流程本身存在开发者与客户的循环”。

- 焦点判定规则：
  - 无项目时：`focusKey = project_setup`
  - 有项目但未绑钱包：`focusKey = wallet_ready`
  - 当前阶段 `open` 且最近交付状态为 `uploaded/notarized`：`focusKey = client_review`
  - 当前阶段 `open` 且最近交付状态为 `rejected`：`focusKey = developer_revise`
  - 当前阶段 `open` 且尚未形成待确认交付：`focusKey = developer_log_submit`
  - 当前阶段 `pending_stage_audit`：`focusKey = admin_stage_audit`
  - 所有阶段闭环且项目可总审计：`focusKey = final_audit`
  - 项目已 `completed/disputed`：高层图仍显示结构图，但焦点落在 `final_audit`，并用详情说明结果，不把其他节点画成“已完成”。

- 前端高层流程重构：
  - `frontend/src/views/ProcessFlowView.vue` 的高层泳道视图改为只消费 `globalOverview`。
  - 节点卡不再显示状态标签 `已完成/未解锁`，只显示：
    - 序号
    - 标题
    - 简短结构说明
  - 右侧详情面板改为展示：
    - 当前焦点
    - 当前角色
    - 当前所处说明
    - 下一步流转
  - 左侧泳道标签只跟随 `focusKey` 或用户当前点击节点高亮，不再和旧 `isCurrent` 混用。
  - 节点尺寸保持统一，文本超出截断；完整逻辑放右侧详情。

- 原型与文档同步：
  - `test/process-flow-prototype/` 同步改成基于 `globalOverview` mock 数据的七节点泳道图。
  - `docs/reference/api.md` 增补 `globalOverview` 字段说明。
  - 按仓库规范补版本日志，必要时同步 README 最近更新摘要。

## Public Interfaces
- `/api/process-flow` 增量新增：
  - `globalOverview.lanes`
  - `globalOverview.nodes`
  - `globalOverview.edges`
  - `globalOverview.focusKey`
- 现有字段 `globalSteps / roleProgress / stageSummaries / currentStage / finalAuditStatus` 保留不删。
- 高层泳道正式页改为优先使用 `globalOverview`；旧 `globalSteps` 保留给兼容或过渡用途。

## Test Plan
- 高层总览语义验证：
  - 进入 `/process-flow` 高层流程后，不再出现“已完成/未解锁”这类总览状态标签。
  - 管理员、开发者、客户三条泳道始终固定可见。
  - 开发者与客户之间存在明确回环：
    - 登记/提交
    - 客户确认
    - 补充修订
    - 再确认
- 焦点场景验证：
  - 无项目：聚焦 `project_setup`
  - 已建项目未绑钱包：聚焦 `wallet_ready`
  - 开发中无待确认交付：聚焦 `developer_log_submit`
  - 交付待客户确认：聚焦 `client_review`
  - 客户驳回后：聚焦 `developer_revise`
  - 待阶段审计：聚焦 `admin_stage_audit`
  - 可最终总审计：聚焦 `final_audit`
- 数据与兼容验证：
  - `/api/process-flow` 仍返回旧字段
  - 新增 `globalOverview` 后，前端高层图不再依赖 `globalSteps.state`
  - `npm run build` 通过
  - 原型和正式页都不再残留“总体说明图里显示已完成”的逻辑

## Assumptions
- 高层泳道图是“结构解释图”，不是“运行状态甘特图”，因此不再展示完成态标签。
- `deliverables.status = 'rejected'` 作为“客户驳回后进入补充修订”的唯一明确后端信号。
- 即便当前项目没有处于驳回状态，`developer_revise` 节点和其回环边仍要在高层图中固定存在，因为它表达的是流程机制本身。
- `阶段循环` 和 `当前快照` 继续保留运行态表达，本次不一起重写逻辑。
