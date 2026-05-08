# 基于区块链的独立开发者可信工时记录与交付物存证管理系统完整使用示例

## 1. 演示目标

本文档给出一个可直接答辩演示的完整案例，覆盖三类用户：

- 管理员
- 开发者
- 客户

通过一个具体项目，走通以下完整链路：

1. 管理员创建项目
2. 开发者绑定钱包
3. 开发者登记工时
4. 开发者登记交付物
5. 开发者发起链上存证
6. 客户确认或驳回交付物
7. 管理员查看审计结果

---

## 2. 演示案例设定

### 2.1 项目背景

假设有一个独立开发者接到一个外包项目：

- 项目名称：企业官网 2.0 外包开发
- 项目目标：完成官网首页重构、后台管理页开发、部署交付

### 2.2 三类用户

系统内置三个演示账号：

- 管理员：`admin / admin123`
- 开发者：`dev / dev123`
- 客户：`client / client123`

### 2.3 三类角色职责

#### 管理员

- 创建项目
- 分配开发者与客户
- 查看系统总览与审计记录

#### 开发者

- 绑定钱包
- 记录工时
- 上传交付物元数据
- 发起链上工时存证和交付物存证

#### 客户

- 查看交付物说明与哈希
- 链上确认交付通过或驳回

---

## 3. 启动系统

### 3.1 启动后端

在项目根目录执行：

```powershell
cd backend
.\run.ps1
```

启动成功后，后端服务地址为：

```text
http://localhost:8080
```

### 3.2 启动前端

在项目根目录执行：

```powershell
cd frontend
npm run dev
```

启动成功后，在浏览器打开前端地址，一般为：

```text
http://localhost:5173
```

### 3.3 启动合约开发环境

如果你要演示真实链上交互，可以进入合约目录：

```powershell
cd contracts
npm run test
```

如需部署到 Sepolia：

```powershell
npm run deploy:sepolia
```

---

## 4. 完整业务流程示例

## 4.1 管理员操作流程

### 第一步：管理员登录

使用以下账号登录系统：

- 用户名：`admin`
- 密码：`admin123`

登录后进入控制台页面。

### 第二步：创建项目

进入“项目管理”页面，填写如下内容：

- 项目名称：`企业官网 2.0 外包开发`
- 项目说明：`官网重构、后台管理页开发与部署交付`
- 开发者 ID：`2`
- 客户 ID：`3`

点击“保存项目”。

### 第三步：管理员看到的结果

此时系统中会生成一条项目记录，状态通常为：

- `active`

项目列表中可以看到：

- 项目名称
- 项目说明
- 开发者 ID
- 客户 ID
- 当前项目状态

### 第四步：如果演示真实链上项目创建

管理员或演示人员可通过部署好的合约调用：

```solidity
createProject(projectId, developerWallet, clientWallet)
```

其作用是：

- 在链上写入项目 ID
- 绑定开发者钱包地址
- 绑定客户钱包地址
- 触发 `ProjectCreated` 事件

---

## 4.2 开发者操作流程

### 第一步：开发者登录

退出管理员账号后，用开发者账号登录：

- 用户名：`dev`
- 密码：`dev123`

### 第二步：绑定钱包

进入控制台，点击“连接 MetaMask”。

此时会发生以下事情：

1. 前端请求浏览器中的 MetaMask
2. 用户授权连接钱包
3. 前端拿到钱包地址
4. 前端调用后端接口 `/api/auth/wallet`
5. 后端把钱包地址写入：
   - `users`
   - `wallet_bindings`

绑定成功后，侧边栏会显示当前钱包地址。

### 第三步：登记工时

进入“工时记录”页面，填写：

- 项目 ID：假设为 `1`
- 日期：当天日期
- 工时：`4`
- 任务描述：`完成官网首页重构并联调登录接口`

点击“生成工时摘要”。

### 第四步：工时记录背后发生了什么

系统会自动完成以下处理：

1. 将工时数据整理成规范化 JSON
2. 使用 `SHA-256` 计算摘要
3. 写入数据库表 `worklogs`
4. 初始状态写为：
   - `pending`

示例摘要原始内容大致为：

```json
{
  "projectId": 1,
  "userId": 2,
  "workDate": "2026-04-13",
  "hours": 4,
  "taskDescription": "完成官网首页重构并联调登录接口"
}
```

其摘要哈希会在页面上显示，用于后续链上存证。

### 第五步：登记交付物

进入“交付物管理”页面，填写：

- 项目 ID：`1`
- 版本：`v1.0.0`
- 文件：选择一个演示文件，例如 `homepage-v1.zip`
- 文件说明：`官网首页与后台管理首页首版交付`

点击“登记交付物”。

### 第六步：交付物登记背后发生了什么

系统会自动完成以下步骤：

1. 浏览器读取文件内容
2. 前端计算该文件的 `SHA-256`
3. 将如下信息发送到后端：
   - 项目 ID
   - 版本号
   - 文件名
   - 文件说明
   - 文件哈希
4. 后端写入数据库表 `deliverables`

这时交付物状态一般为：

- `uploaded`

---

## 4.3 开发者链上存证流程

这一部分分成两种演示方式：

- 方式 A：论文答辩演示版
- 方式 B：真实 MetaMask 上链版

### 方式 A：论文答辩演示版

当前项目已经支持通过后端接口模拟链上回写。

开发者在成功发起链上交易后，可以把以下内容回写到后端：

- 业务类型
- 业务 ID
- 交易哈希
- 区块号
- 状态

例如：

```json
{
  "businessType": "worklog",
  "businessId": 1,
  "txHash": "0xabc123",
  "blockNumber": 123456,
  "status": "confirmed",
  "comment": "工时摘要已上链"
}
```

后端收到后会：

1. 写入 `evidence_records`
2. 把 `worklogs.evidence_status` 更新为 `confirmed`

如果是交付物回写：

```json
{
  "businessType": "deliverable",
  "businessId": 1,
  "txHash": "0xdef456",
  "blockNumber": 123457,
  "status": "confirmed",
  "comment": "交付物文件哈希已上链"
}
```

后端会把交付物状态更新为：

- `notarized`

### 方式 B：真实 MetaMask 上链版

如果要真实展示 Web3 交互，则开发者钱包应调用智能合约：

#### 工时上链

```solidity
recordWorklog(projectId, worklogId, worklogHash, periodStart, periodEnd)
```

用途：

- 把工时摘要写入链上
- 记录开始时间和结束时间
- 触发 `WorklogRecorded` 事件

#### 交付物上链

```solidity
recordDeliverable(projectId, deliverableId, fileHash, version, fileNameHash)
```

用途：

- 把交付物文件哈希写入链上
- 记录版本信息
- 触发 `DeliverableRecorded` 事件

前端项目中已经预留了对应调用封装：

- `submitWorklogEvidence`
- `submitDeliverableEvidence`

真实答辩时可以将页面按钮进一步绑定到这些函数。

---

## 4.4 客户操作流程

### 第一步：客户登录

退出开发者账号，用客户账号登录：

- 用户名：`client`
- 密码：`client123`

### 第二步：客户查看交付物

进入“客户确认”页面，客户可以看到：

- 文件名
- 版本号
- 文件说明
- 文件哈希
- 当前状态

### 第三步：客户核验交付物

客户可以核对两件事：

1. 收到的本地文件是否正确
2. 页面显示的哈希是否与开发者提交一致

如果一致，则点击：

- “确认通过”

如果发现问题，则点击：

- “驳回交付”

### 第四步：客户确认通过时发生什么

当前演示版中，点击“确认通过”会向后端写入一条确认记录，例如：

```json
{
  "businessType": "deliverable_confirmation",
  "businessId": 1,
  "txHash": "0xconfirm1ok",
  "blockNumber": 1710000000,
  "status": "confirmed",
  "comment": "客户确认通过"
}
```

后端收到后会：

1. 写入 `evidence_records`
2. 写入 `approval_records`
3. 更新 `deliverables.status = approved`

### 第五步：客户驳回时发生什么

如果点击“驳回交付”，系统会写入类似：

```json
{
  "businessType": "deliverable_confirmation",
  "businessId": 1,
  "txHash": "0xconfirm1ng",
  "blockNumber": 1710000001,
  "status": "failed",
  "comment": "客户驳回，需补充材料"
}
```

此时后端会把交付物状态改为：

- `rejected`

### 第六步：如果做真实链上确认

客户钱包可直接调用：

```solidity
confirmDeliverable(projectId, deliverableId, approved, commentHash)
```

该方法只允许客户钱包调用，作用是：

- 记录客户是否确认通过
- 记录确认意见摘要
- 触发 `DeliverableConfirmed` 事件

---

## 5. 管理员查看最终结果

### 5.1 回到管理员账号

再次登录管理员账号：

- 用户名：`admin`
- 密码：`admin123`

### 5.2 查看控制台

控制台可以看到：

- 项目总数
- 工时记录数
- 交付物数量
- 待处理事项
- 最近交易记录

### 5.3 查看审计页

进入“审计管理”，可以看到：

- 最近的链上交易记录
- 当前待处理交付物
- 哪些交付已确认
- 哪些交付被驳回

管理员可以通过这些信息说明：

1. 谁提交了工时
2. 谁上传了交付物
3. 哪些摘要已上链
4. 客户是否确认通过
5. 系统是否形成了完整审计闭环

---

## 6. 数据流总结

## 6.1 管理员创建项目

写入：

- `projects`
- `project_members`
- `audit_logs`

## 6.2 开发者绑定钱包

写入：

- `users`
- `wallet_bindings`
- `audit_logs`

## 6.3 开发者登记工时

写入：

- `worklogs`
- `audit_logs`

## 6.4 开发者登记交付物

写入：

- `deliverables`
- `audit_logs`

## 6.5 开发者回写链上存证

写入：

- `evidence_records`

并更新：

- `worklogs.evidence_status`
- `deliverables.status`

## 6.6 客户确认交付

写入：

- `approval_records`
- `evidence_records`

并更新：

- `deliverables.status`

---

## 7. 一次完整成功演示的最终结果

如果这次案例全部走通，最终应该看到以下状态：

### 项目状态

- `active`

### 工时记录状态

- `confirmed`

### 交付物状态

- `approved`

### 审计层面

数据库中应存在：

- 项目创建记录
- 工时摘要记录
- 交付物哈希记录
- 链上回写记录
- 客户确认记录
- 全部关键操作的审计日志

### 链上层面

合约事件应至少出现：

- `ProjectCreated`
- `WorklogRecorded`
- `DeliverableRecorded`
- `DeliverableConfirmed`

---

## 8. 答辩时推荐讲解顺序

建议答辩时按以下顺序展示：

1. 先说明系统背景与三类角色
2. 管理员登录并创建项目
3. 开发者登录并绑定钱包
4. 开发者录入工时并展示工时摘要
5. 开发者上传交付物并展示文件哈希
6. 展示链上存证接口或交易哈希
7. 客户登录并确认交付物
8. 管理员回到审计页查看完整闭环

这样讲解时逻辑最完整，也最符合论文中“可信工时记录 + 交付物存证 + 客户确认 + 审计追溯”的主题。

---

## 9. 当前版本说明

当前项目已经可以完整演示“业务闭环”：

- 登录
- 项目创建
- 钱包绑定
- 工时登记
- 交付物登记
- 链上回写
- 客户确认
- 后台审计

如果要进一步升级为“页面按钮直接发起真实链上交易”的版本，只需要把前端页面按钮继续接入已经预留好的合约调用函数即可。
