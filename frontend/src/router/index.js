import { createRouter, createWebHistory } from "vue-router";
import { useAuthStore } from "../stores/auth";
import LoginView from "../views/LoginView.vue";
import HomeView from "../views/HomeView.vue";
import ProjectsView from "../views/ProjectsView.vue";
import ProjectManagementView from "../views/ProjectManagementView.vue";
import MembersView from "../views/MembersView.vue";
import WorklogsView from "../views/WorklogsView.vue";
import DeliverablesView from "../views/DeliverablesView.vue";
import ConfirmationsView from "../views/ConfirmationsView.vue";
import AuditView from "../views/AuditView.vue";
import ProfileView from "../views/ProfileView.vue";
import ProcessFlowView from "../views/ProcessFlowView.vue";

const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: "/login", component: LoginView, meta: { public: true, title: "登录" } },
    { path: "/", redirect: "/home" },
    { path: "/home", component: HomeView, meta: { title: "首页" } },
    { path: "/dashboard", redirect: "/home" },
    { path: "/members", component: MembersView, meta: { roles: ["admin"], title: "成员管理" } },
    { path: "/projects", component: ProjectsView, meta: { roles: ["admin"], title: "新增项目" } },
    { path: "/project-management", component: ProjectManagementView, meta: { roles: ["admin"], title: "项目管理" } },
    { path: "/worklogs", component: WorklogsView, meta: { roles: ["developer"], title: "工时记录" } },
    { path: "/deliverables", component: DeliverablesView, meta: { roles: ["developer"], title: "交付物" } },
    { path: "/confirmations", component: ConfirmationsView, meta: { roles: ["client"], title: "客户确认" } },
    { path: "/audit", component: AuditView, meta: { roles: ["admin"], title: "审计管理" } },
    { path: "/process-flow", component: ProcessFlowView, meta: { title: "流程总览" } },
    { path: "/profile", component: ProfileView, meta: { title: "个人中心" } },
  ],
});

router.beforeEach((to) => {
  const auth = useAuthStore();
  if (!to.meta.public && !auth.isAuthenticated) {
    return "/login";
  }
  if (to.meta.roles && !to.meta.roles.includes(auth.role)) {
    return "/home";
  }
  if (to.path === "/login" && auth.isAuthenticated) {
    return "/home";
  }
  return true;
});

export default router;
