# SQLite 数据库设计

## 表

- `users`
  - 用户基础信息
  - 关键新增字段：`status`
  - 演示快捷登录字段：`quick_login_password`
- `wallet_bindings`
  - 钱包绑定历史
- `projects`
  - 项目主表
- `project_members`
  - 项目成员关系
- `worklogs`
  - 工时记录
  - 关键新增字段：`stage_no`
- `deliverables`
  - 交付物元数据与文件哈希
  - 关键新增字段：`stage_no`
- `approval_records`
  - 客户确认记录
  - 仍按交付物维度留痕
- `project_audits`
  - 新增
  - 用于统一保存阶段审计与最终总审计
- `evidence_records`
  - 链上交易回写
  - 现支持阶段审计与总审计业务类型
- `audit_logs`
  - 后台审计留痕

## 状态枚举

### 用户

- 角色：`developer | client | admin`
- 状态：`active | inactive`

### 项目

- `draft | active | completed | disputed`

### 交付物

- `uploaded`
- `notarized`
- `approved`
- `rejected`

### 项目审计

- `audit_type`
  - `stage`
  - `final`
- `decision`
  - `pass`
  - `reject`

### 链上回写

- `pending | confirmed | failed`

## 阶段模型说明

- 一个项目由多个 `stage_no` 组成
- `worklogs.stage_no` 与 `deliverables.stage_no` 明确表示该条记录属于哪一阶段
- 后端自动推进阶段：
  - 最近阶段已通过管理员阶段审计，则新记录进入下一阶段
  - 最近阶段仍开放，则新记录继续进入当前阶段
  - 最近阶段已通过客户确认但尚未完成管理员阶段审计，则禁止继续登记

## 演示账号

- 管理员：`admin / admin123`
- 开发者 A：`dev / dev123`
- 开发者 B：`dev2 / dev123`
- 客户 A：`client / client123`
- 客户 B：`client2 / client123`

## 快捷登录说明

- `users.quick_login_password` 仅用于毕业设计演示环境中的登录页快捷登录
- 新增成员时会写入初始密码
- 管理员重置成员密码、用户自己修改密码时会同步更新
- 停用或删除账号后，登录页不会再展示该账号
- 历史自定义账号若没有明文密码，不会自动出现在快捷登录中；下一次重置密码或修改密码后会重新进入列表
