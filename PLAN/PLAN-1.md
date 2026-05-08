# 基于区块链的独立开发者可信工时记录与交付物存证管理系统开发计划

## Summary
目标版本定位为“论文答辩可演示版”，采用 `Vue 3 + Vite` 前端、`C` 语言 REST 后端、`SQLite` 本地业务库、`Solidity` 智能合约，区块链环境按 `Ethereum Sepolia 测试网` 规划。

系统按三类角色设计：
- 开发者：记录工时、提交交付物、发起链上存证
- 客户：查看工时与交付物、链上确认交付结果
- 平台管理员：管理用户、项目、异常记录和审计视图

核心闭环为：`项目创建 -> 钱包绑定 -> 工时记录 -> 工时摘要上链 -> 交付物上传 -> 文件哈希上链 -> 客户确认 -> 后台审计追溯`。

## Key Changes
### 1. 工程与技术结构
采用单仓结构：
- `frontend/`：Vue 3、Pinia、Vue Router、Element Plus、ethers.js
- `backend/`：C REST 服务，使用 `libmicrohttpd`（HTTP）、`sqlite3`、`cJSON`
- `contracts/`：Solidity 合约，使用 `Hardhat` 编译、部署和测试
- `docs/`：接口文档、数据库设计、答辩演示脚本

后端职责明确为：
- 负责账号体系、项目/工时/交付物 CRUD、SQLite 持久化、文件上传、链上记录回写
- 不直接代替用户发链上交易；链上交易由前端通过 MetaMask 发起，后端只保存交易哈希和确认状态

### 2. 业务模块与页面
前端页面固定为：
- 登录/注册页：账号密码登录、钱包绑定/更换
- 控制台首页：项目数、待确认交付物、已存证工时、链上交易状态
- 项目管理页：创建项目、分配开发者/客户、查看项目状态
- 工时记录页：按日期录入工时、任务描述、关联项目、提交存证
- 交付物管理页：上传文件、版本说明、生成哈希、提交链上存证
- 客户确认页：查看交付物详情、链上确认通过/驳回
- 审计管理页：管理员查看用户、项目、交易、异常状态

后端 API 按以下边界设计：
- `/api/auth/*`：注册、登录、登出、钱包绑定
- `/api/users/*`：用户信息、角色、钱包信息
- `/api/projects/*`：项目创建、成员关联、项目详情
- `/api/worklogs/*`：工时记录新增、编辑、查询、提交存证
- `/api/deliverables/*`：交付物上传、版本管理、摘要查询、确认状态
- `/api/evidence/*`：链上交易回写、交易状态查询、审计记录
- `/api/admin/*`：用户审核、项目总览、异常记录查询

SQLite 数据表固定为：
- `users`
- `wallet_bindings`
- `projects`
- `project_members`
- `worklogs`
- `deliverables`
- `evidence_records`
- `approval_records`
- `audit_logs`

### 3. 合约与链上数据设计
智能合约只承担“可信存证”和“确认留痕”，不存完整业务数据。链上采用“文件离链、哈希上链”方案。

合约接口固定为：
- `createProject(projectId, developerWallet, clientWallet)`
- `recordWorklog(projectId, worklogId, worklogHash, periodStart, periodEnd)`
- `recordDeliverable(projectId, deliverableId, fileHash, version, fileNameHash)`
- `confirmDeliverable(projectId, deliverableId, approved, commentHash)`
- `getEvidence(projectId, recordId)`

合约事件固定为：
- `ProjectCreated`
- `WorklogRecorded`
- `DeliverableRecorded`
- `DeliverableConfirmed`

约束规则：
- 只有项目绑定的钱包能提交对应链上记录
- 开发者提交工时和交付物存证
- 客户提交交付确认
- 管理员只做链下管理，不替用户写链

哈希策略固定为：
- 工时记录：对规范化 JSON 做 `SHA-256`
- 交付物：对文件二进制内容做 `SHA-256`
- 链上保存摘要、业务 ID、时间戳、提交地址、交易哈希

### 4. 实施顺序
按 5 个迭代实施：
1. 搭建目录、前后端基础框架、SQLite 初始化、合约脚手架
2. 完成账号体系、角色权限、项目管理、钱包绑定
3. 完成工时记录、工时摘要计算、链上工时存证闭环
4. 完成交付物上传、文件哈希计算、链上交付物存证与客户确认
5. 完成管理员审计页、统计看板、联调、演示数据与答辩流程

## Public APIs / Interfaces
对外接口和类型约束固定如下：
- 用户角色枚举：`developer | client | admin`
- 项目状态枚举：`draft | active | completed | disputed`
- 交付物状态枚举：`uploaded | notarized | approved | rejected`
- 链上记录状态：`pending | confirmed | failed`

前端与后端的关键 DTO 固定为：
- `ProjectDTO`: 项目基本信息、成员、状态、链上项目 ID
- `WorklogDTO`: 日期、工时、任务描述、摘要哈希、交易哈希、确认状态
- `DeliverableDTO`: 文件名、版本、说明、文件哈希、交易哈希、确认状态
- `EvidenceDTO`: 业务类型、链上地址、交易哈希、区块号、时间戳

## Test Plan
必须覆盖以下测试：
- 后端单元测试：登录鉴权、权限校验、SQLite 事务、工时摘要生成、文件哈希计算
- 合约单元测试：角色访问控制、重复提交拦截、客户确认权限、事件正确触发
- 前端联调测试：钱包绑定、MetaMask 发交易、交易结果回写、状态刷新
- 端到端流程测试：
  1. 管理员创建项目并分配成员
  2. 开发者录入工时并提交链上存证
  3. 开发者上传交付物并提交链上存证
  4. 客户查看交付物并链上确认
  5. 管理员在审计页看到完整链下+链上追溯记录
- 异常场景测试：钱包未绑定、交易失败、重复上传、客户越权确认、文件被篡改后校验失败

## Assumptions
- 测试网默认采用 `Sepolia`
- 钱包默认使用 `MetaMask`
- 上传文件仅保存在本地服务器目录，不接入 IPFS
- 每个用户默认只绑定一个主钱包地址
- v1 不做支付结算、发票、消息通知、多组织租户
- 交付物文件大小默认限制为 `100MB`
- 登录采用“账号密码 + JWT”，密码哈希使用 `Argon2id`
- 演示环境默认单机部署：前端、后端、SQLite、合约部署脚本均在同一开发机完成
