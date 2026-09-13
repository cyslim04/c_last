# Trust Work

Trust Work 是一个面向可信协作的业务系统，用于记录工时、管理交付物、完成客户确认、阶段审计并保存链上证据。

## 项目结构

- `frontend/`：Vue 3 + Vite 前端应用
- `backend/`：C11 + Winsock + SQLite 后端服务
- `contracts/`：Solidity 合约、Hardhat 测试与部署脚本
- `docs/guide/`：系统、部署、接口和数据说明
- `docs/logs/`：按周保存的变更记录

## 快速启动

首次使用时先初始化管理员：

```powershell
cd backend
.\init-admin.ps1
```

启动后端：

```powershell
cd backend
.\run.ps1
```

启动前端：

```powershell
cd frontend
npm install
npm run dev
```

默认地址：

- 前端：`http://localhost:5173`
- 后端：`http://localhost:8080`

## 业务流程

1. 管理员创建项目并分配开发者、客户。
2. 开发者登记工时和交付物，并生成文件摘要。
3. 客户查看阶段材料并确认或驳回。
4. 管理员执行阶段审计和最终审计。
5. 需要链上留痕时，连接支持的钱包提交存证。

## 开发与验证

```powershell
cd frontend
npm run build

cd ..\backend
.\build.ps1

cd ..\contracts
npm install
npm run test
```

更完整的说明见：

- [操作手册](docs/guide/操作手册.md)
- [系统说明](docs/guide/系统说明.md)
- [部署与运维](docs/guide/部署与运维.md)
- [接口与数据](docs/guide/接口与数据.md)
