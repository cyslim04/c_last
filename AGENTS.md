# 仓库规范

## 项目结构与模块组织
- `frontend/` 存放 Vue 3 + Vite 前端应用。路由页面放在 `src/views`，共享 UI 放在 `src/components`，Pinia 状态放在 `src/stores`，API 辅助函数放在 `src/api`，Web3 辅助函数放在 `src/web3`，全局样式放在 `src/styles`。
- `backend/` 存放面向 Windows 的 C + SQLite 服务。主要请求处理在 `src/server.c`，通用 C 工具函数在 `src/utils`，头文件在 `include`，运行时数据在 `data/`，上传文件在 `uploads/`。
- `contracts/` 存放 Solidity 合约、Hardhat 配置、部署脚本和测试，目录分别为 `contracts/`、`scripts/`、`test/`。
- `docs/` 用于存放说明文档与版本日志。说明文档统一放在 `docs/reference/`，版本日志统一放在 `docs/logs/YYYY-Www/`。版本日志统一使用 `YYYY-MM-DD-序号-中文主题.md` 命名，同一天从 `01` 开始递增；每次修改后如果需要留痕，请把日志写入对应周目录，不要覆盖旧文件。

## 构建、测试与开发命令
- `cd frontend; npm install; npm run dev` 会在 `http://localhost:5173` 启动 Vite 开发服务器。
- `cd frontend; npm run build` 会将生产构建产物输出到 `frontend/dist`。
- `cd backend; .\build.ps1` 会编译 `trust_work_backend.exe`。它依赖 `gcc` 以及脚本中引用的 SQLite 工具链路径。
- `cd backend; .\run.ps1` 会重建并运行后端，地址为 `http://localhost:8080`，并会先停止同路径下已有的旧后端进程。
- `cd contracts; npm install; npm run test` 用于运行 Hardhat 测试。
- `cd contracts; npm run compile` 用于编译 Solidity 源码。`npm run deploy:sepolia` 会使用 `.env` 中的值执行部署。

## 编码风格与命名约定
- Vue、JavaScript 和 C 代码统一使用 2 空格缩进。
- 保持现有 JS 风格：双引号、分号，以及语义清晰的 computed/store 命名。
- Vue 组件和视图文件使用 PascalCase，例如 `PageHero.vue`、`DashboardView.vue`。
- JS 模块使用小写或 camelCase，例如 `http.js`、`wallet.js`。
- C 语言中函数优先使用 `snake_case`，文件内局部辅助函数尽量声明为 `static`。

## 测试指南
- 合约测试使用 Hardhat + Chai，位于 `contracts/test/*.js`。测试命名应围绕行为来写，例如 `allows only client to confirm deliverable`。
- 仓库中未包含前端或后端的自动化测试套件。涉及这些部分的改动，请在 `frontend` 里运行 `npm run build`，启动 `backend/run.ps1`，并手动验证受影响流程。

## 提交与 Pull Request 规范
- 由于当前工作区不包含 `.git`，无法通过项目历史验证提交规范。请使用简短的祈使句主题，并带上作用域，例如 `frontend: fix process flow switcher`。
- PR 应说明受影响区域、行为变化、手动验证步骤，以及任何 API、数据库结构或种子数据影响。
- UI 改动请附截图；合约相关工作请注明钱包与网络前提。

## 安全与生成文件
- 不要提交真实密钥。`SEPOLIA_RPC_URL` 和 `PRIVATE_KEY` 请以 `contracts/.env.example` 作为模板。
- 优先修改源码，不要修改生成物或运行时产物，例如 `frontend/dist/`、`backend/*.exe`、`backend/data/trust_work.db`，或 `backend/uploads/` 下的文件。

## 修改文件
- 每次修改文件后均写入日志，按照日志要求写入`docs/logs/` 内，按周为界分好。
- 所有文档（.md .txt类似）均使用中文写入
- 每次5篇日志后更新README.md文档/
