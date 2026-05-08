# Trust Work 平台重构方案

## Summary
- 以“角色闭环演示 + MetaMask 状态完善 + 正常个人中心 + 答辩风视觉升级”为目标重构当前演示系统。
- 保留现有前后端技术栈，优先修正现有缺口：钱包连接状态只停留在“连接”、无账户/网络切换、无个人中心接口、列表权限过粗、数据库演示数据过空、中文文案乱码。
- 本轮不把工时/交付/确认按钮全面接入真实链上交易；保留现有演示型 evidence 回写逻辑，只把钱包连接、网络切换、状态同步和链前置校验做完整。

## Key Changes
### 1. 全局壳层与角色导航
- 将 `AppShell` 改为“侧边导航 + 顶部状态栏”结构。
- 顶部状态栏固定包含：
  - 钱包状态胶囊：未连接时显示“连接 MetaMask”；已连接时显示 `地址缩写 + 网络名`，例如 `0x12ab...89ef · Sepolia`。
  - 用户入口：头像、姓名、角色，展开后进入个人中心和退出登录。
- 导航按角色收敛：
  - `admin`：控制台、项目管理、审计管理、个人中心。
  - `developer`：控制台、工时记录、交付物、个人中心。
  - `client`：控制台、客户确认、个人中心。
- 修正所有中文乱码文案，统一为 UTF-8 正常中文。

### 2. MetaMask 状态与快捷操作
- 钱包 store 扩展为：`address`、`shortAddress`、`chainId`、`chainName`、`connected`、`supportedNetwork`、`availableNetworks`。
- 连接成功后立即把按钮替换为账户缩写和网络名，不再显示“连接”。
- 接入 `accountsChanged` 和 `chainChanged` 监听；状态变化时同步更新 UI。
- 钱包菜单固定提供 3 个动作：
  - 切换账户：调用 `wallet_requestPermissions` / `eth_requestAccounts` 触发 MetaMask 账户选择。
  - 切换网络：支持 `Sepolia` 和本地 `Hardhat`，通过 `wallet_switchEthereumChain`，本地链缺失时走 `wallet_addEthereumChain` 回退。
  - 退出连接：前端清空钱包状态，并对 `wallet_revokePermissions` 做 best-effort 调用；不删除后端历史绑定记录。
- 当前登录用户在钱包账户切换后，自动把新地址重新绑定为当前主地址，并写入 `wallet_bindings` 历史。
- 所有需要钱包前置条件的按钮都显示明确禁用态和错误提示：未连接、网络不支持、账户缺失。

### 3. 个人中心与认证完善
- 新增独立 `/profile` 页面，采用正常网页个人中心布局：
  - 基本资料卡：头像、姓名、邮箱、手机号、公司/团队、个人简介。
  - 账号安全卡：用户名只读、修改密码。
  - 会话卡：退出登录。
- 用户表新增字段：`email`、`phone`、`company`、`bio`、`avatar_path`、`updated_at`。
- 扩展会话中的 `user` 对象，统一返回以上资料字段和 `avatarUrl`。
- 新增接口：
  - `GET /api/auth/me`
  - `PUT /api/auth/profile`，JSON：`{ name, email, phone, company, bio }`
  - `PUT /api/auth/password`，JSON：`{ currentPassword, newPassword }`
  - `POST /api/auth/avatar`，JSON：`{ fileName, mimeType, contentBase64 }`
  - `POST /api/auth/logout`
- 头像上传采用“前端读取文件为 base64，后端落地到 `backend/uploads/avatars/`，数据库保存相对路径”的本地上传方案。
- 登出流程统一为：调用后端 logout 记录审计，再清空本地 session 和钱包状态并跳回登录页。

### 4. 业务逻辑与数据闭环
- 后端接口按角色做权限和可见范围控制：
  - `admin` 才能创建项目和查看审计。
  - `developer` 只能查看/创建自己参与项目下的工时、交付物、钱包绑定。
  - `client` 只能查看自己参与项目的待确认交付物并发起确认/驳回。
- 列表接口补充展示字段，前端不再暴露纯 ID 驱动的表单：
  - 项目返回开发者/客户姓名。
  - 工时返回项目名、提交人姓名。
  - 交付物返回项目名、提交人、确认状态摘要。
- 新增管理员可用的用户选择接口：`GET /api/users?role=developer|client`，用于项目创建下拉选择。
- 表单全部从“手输 ID”改为“下拉选择项目/成员”。
- 客户确认页补充确认意见输入；审批结果与交付物状态、审计记录、evidence 记录保持一致。
- 后端初始化改为版本化演示种子：
  - 增加 `app_meta.seed_version`。
  - 当数据库为空或为旧版稀疏演示数据时，清空业务表并重建一套完整演示流，只执行一次。
- 重建后的演示数据至少包含：
  - 3 个基础账号：管理员、开发者、客户，均带完整个人资料。
  - 2 个项目：1 个“已走完整闭环”的完成案例，1 个“进行中待确认”的案例。
  - 多条工时：覆盖 `pending`、`confirmed`。
  - 多条交付物：覆盖 `uploaded`、`notarized`、`approved`、`rejected`。
  - 对应的 `wallet_bindings`、`evidence_records`、`approval_records`、`audit_logs`，保证控制台和审计页都能直观看到流程。

### 5. 视觉与动画升级
- 视觉方向按“强展示感答辩风”重做，但保持后台可读性。
- 采用统一设计变量：更强层次背景、玻璃面板、强调色状态体系、定制中文字体栈。
- 动效统一而克制：
  - 页面进入时分层渐入和轻微上移动画。
  - 指标卡、列表卡片 hover 抬升与光泽扫过。
  - 钱包菜单、用户菜单、弹层使用短时 easing 过渡。
  - 登录页和控制台加入背景光斑/流动渐变。
- 响应式必须覆盖桌面和移动端，避免动画影响表单输入和数据阅读。

## Public API / Data Changes
- `users` 表新增：`email`、`phone`、`company`、`bio`、`avatar_path`、`updated_at`。
- 新增 `app_meta` 表用于 `seed_version`。
- 认证返回的 `user` DTO 扩展为：`id`、`username`、`name`、`role`、`walletAddress`、`email`、`phone`、`company`、`bio`、`avatarUrl`。
- 新增接口：
  - `GET /api/auth/me`
  - `PUT /api/auth/profile`
  - `PUT /api/auth/password`
  - `POST /api/auth/avatar`
  - `POST /api/auth/logout`
  - `GET /api/users?role=...`
- 现有项目/工时/交付物接口在保留原 ID 字段的同时补充关联展示字段，供前端直接展示。

## Test Plan
- 登录后按不同角色跳转正确，导航项与页面权限符合角色限制。
- MetaMask 连接成功后，顶部钱包胶囊立即显示账户缩写和网络名。
- 在 MetaMask 中切换账户后，UI 自动刷新，新地址重新绑定成功。
- 在 `Sepolia` 与本地 `Hardhat` 之间切换网络时，状态与提示正确；不支持网络时出现明确提示。
- 钱包退出连接只清空当前连接态，不破坏用户账号登录态。
- 个人中心可查看并更新姓名、邮箱、手机号、公司、简介；修改密码需校验旧密码；头像上传后可回显。
- 退出登录会清空 session 与钱包状态，并返回登录页。
- 管理员可创建项目且使用成员下拉选择；开发者和客户无权访问该入口。
- 开发者只能看到自己项目的工时与交付物；客户只能看到自己待确认交付物。
- 重建后的演示数据能在控制台、工时、交付、确认、审计页形成完整可视化闭环。
- 页面在桌面和移动端均可用，动画不会遮挡交互或造成内容跳动。

## Assumptions
- 本轮默认不把所有业务按钮改为真实链上交易，只完成钱包连接体验、网络切换和链前置校验。
- 当前数据库中的稀疏演示数据允许被一次性重建为新的完整演示数据。
- 头像采用本地文件落盘方案，文件保存于 `backend/uploads/avatars/`，不接第三方对象存储。
