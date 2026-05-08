# API 设计

## 认证

- `POST /api/auth/login`
- `GET /api/auth/quick-accounts`
- `GET /api/auth/me`
- `PUT /api/auth/profile`
- `PUT /api/auth/password`
- `POST /api/auth/avatar`
- `POST /api/auth/logout`
- `POST /api/auth/wallet`

其中：

- `GET /api/auth/quick-accounts`
  - 公开接口，无需登录
  - 返回：`items[]`
  - 每项包含：
    - `id`
    - `role`
    - `displayName`
    - `username`
    - `password`
    - `description`
  - 仅返回 `active` 且已写入 `quick_login_password` 的账号

登录返回的 `user` 已包含：

- `id`
- `username`
- `name`
- `role`
- `status`
- `walletAddress`
- `email`
- `phone`
- `company`
- `bio`
- `avatarUrl`

## 成员管理

- `GET /api/users?role=<developer|client>&status=<active|inactive|all>`
- `POST /api/users`
- `PUT /api/users`
- `POST /api/users/password/reset`
- `POST /api/users/status`
- `DELETE /api/users?id=<id>`

成员列表返回项补充：

- `status`
- `referencedProjectCount`
- `canDelete`

成员新增补充约束：

- `POST /api/users` 中 `name` 现为可选
- 若未提供 `name` 或为空，后端会自动用 `username` 回填
- `username` 会与系统中所有用户做全局唯一校验

## 项目

- `GET /api/projects`
- `POST /api/projects`

项目仍保留：

- `developerId`
- `clientId`
- `developerName`
- `clientName`

创建项目时仅允许选择 `active` 的开发者和客户。

前端项目管理补充：

- 管理员新增项目页继续使用 `POST /api/projects`
- 管理员项目管理页继续使用 `GET /api/projects`
- 项目状态分类由前端基于返回字段 `status` 本地完成：
  - `active`
  - `completed`
  - `disputed`
  - `draft`

## 工时

- `GET /api/worklogs`
- `POST /api/worklogs`

工时 DTO 现已包含：

- `stageNo`
- `evidenceStatus`
- `txHash`

`stageNo` 由后端自动分配，不要求前端提交。

## 交付物

- `GET /api/deliverables`
- `POST /api/deliverables`

交付物 DTO 现已包含：

- `stageNo`
- `confirmationComment`
- `txHash`

`stageNo` 同样由后端自动分配。

## 阶段确认与审计

- 客户阶段确认仍通过 `POST /api/evidence`
  - `businessType = deliverable_confirmation`
- 管理员阶段审计：
  - `POST /api/audit/stage`
  - 请求体：`{ projectId, stageNo, decision, comment }`
- 管理员最终总审计：
  - `POST /api/audit/final`
  - 请求体：`{ projectId, decision, comment }`

## 仪表盘与流程

- `GET /api/dashboard`
- `GET /api/audit`
- `GET /api/process-flow`

其中：

- `GET /api/audit` 现返回：
  - `transactions`
  - `logs`
  - `stageQueue`
  - `finalQueue`
  - `projects`
- `GET /api/process-flow` 现返回：
  - `globalHint`
  - `globalOverview`
  - `globalSteps`
  - `projectOptions`
  - `selectedProject`
  - `roleProgress`
  - `stageSummaries`
  - `currentStage`
  - `finalAuditStatus`

其中新增或增强字段说明如下：

- `globalOverview`
  - `lanes`：高层总览固定泳道，当前返回 `admin / developer / client`
  - `nodes`：高层总览的结构性节点列表，不再表达“已完成/未解锁”这类运行态结果
  - `edges`：高层总览节点间的前进边与回环边
  - `focusKey`：当前高层总览默认聚焦节点
- `globalOverview.nodes[*]`
  - `actor`：该结构节点所属角色泳道
  - `kind`：节点类别，当前返回 `setup / loop / audit`
  - `tone`：结构焦点层级，当前返回 `focus / available / neutral`
  - `isCurrentFocus`：当前是否为高层总览主焦点
  - `summary`：该节点在整体流程中的结构说明
  - `detail`：当前项目为什么聚焦到该节点的说明
- `globalOverview.edges[*]`
  - `from` / `to`：边的起止节点 key
  - `kind`：边类型，当前返回 `forward / approval / rework`
  - `tone`：边的高亮层级，当前返回 `focus / neutral`
- `globalSteps[*]`
  - 该字段继续保留，用于兼容旧前端或其他运行态消费场景
  - `actor`：当前步骤的责任角色
  - `state`：当前步骤状态，前端用于驱动流程图节点态
  - `detail`：当前项目在该步骤下的运行态说明
  - `gate`：进入或完成该步骤所需条件
  - `isCurrent`：当前是否为全局主焦点步骤
- `roleProgress[*]`
  - `role`：角色标识
  - `title`：角色标题与当前承担对象
  - `stageNo`：当前关联阶段
  - `status`：角色当前状态
  - `summary`：角色当前所处位置摘要
  - `nextAction`：角色下一步动作
  - `blockedBy`：若存在阻塞，则给出阻塞来源
- `stageSummaries[*]`
  - `isCurrent`：是否为当前阶段
  - `phase`：当前阶段所处子状态
  - `currentOwner`：当前主要责任角色
  - `requirement`：该阶段继续推进的门禁条件
  - `nextAction`：该阶段下一步动作
  - `blockingReason`：该阶段阻塞原因
  - `roleStates`：开发者、客户、管理员在该阶段下的状态摘要数组
- `finalAuditStatus`
  - `blockingReason`：最终总审计未解锁时的直接原因
  - `blockingStageNo`：对应阻塞阶段号
  - `requirements`：最终总审计门禁条件列表，包含每条条件是否满足

## 链上回写

- `POST /api/evidence`
- `GET /api/admin/overview`

支持的 `businessType`：

- `worklog`
- `deliverable`
- `deliverable_confirmation`
- `stage_audit`
- `final_audit`
