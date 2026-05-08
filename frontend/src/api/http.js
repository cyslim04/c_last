const API_BASE = import.meta.env.VITE_API_BASE || "http://localhost:8080";

function getSession() {
  return JSON.parse(localStorage.getItem("trust-work-session") || "null");
}

function buildUrl(path) {
  return `${API_BASE}${path}`;
}

async function request(path, options = {}) {
  const session = getSession();
  const headers = {
    "Content-Type": "application/json",
    ...(session?.token ? { Authorization: `Bearer ${session.token}` } : {}),
    ...(options.headers || {}),
  };

  const response = await fetch(buildUrl(path), { ...options, headers });
  const payload = await response.json().catch(() => ({}));

  if (!response.ok) {
    throw new Error(payload.message || "请求失败");
  }

  return payload;
}

export { API_BASE, buildUrl };

export const api = {
  login(payload) {
    return request("/api/auth/login", { method: "POST", body: JSON.stringify(payload) });
  },
  getQuickAccounts() {
    return request("/api/auth/quick-accounts");
  },
  getMe() {
    return request("/api/auth/me");
  },
  updateProfile(payload) {
    return request("/api/auth/profile", { method: "PUT", body: JSON.stringify(payload) });
  },
  changePassword(payload) {
    return request("/api/auth/password", { method: "PUT", body: JSON.stringify(payload) });
  },
  uploadAvatar(payload) {
    return request("/api/auth/avatar", { method: "POST", body: JSON.stringify(payload) });
  },
  logout() {
    return request("/api/auth/logout", { method: "POST", body: JSON.stringify({}) });
  },
  bindWallet(payload) {
    return request("/api/auth/wallet", { method: "POST", body: JSON.stringify(payload) });
  },
  getDashboard() {
    return request("/api/dashboard");
  },
  getAudit() {
    return request("/api/audit");
  },
  submitStageAudit(payload) {
    return request("/api/audit/stage", { method: "POST", body: JSON.stringify(payload) });
  },
  submitFinalAudit(payload) {
    return request("/api/audit/final", { method: "POST", body: JSON.stringify(payload) });
  },
  getProcessFlow(projectId) {
    const suffix = projectId ? `?projectId=${encodeURIComponent(projectId)}` : "";
    return request(`/api/process-flow${suffix}`);
  },
  getUsers(role, status) {
    const params = new URLSearchParams();
    if (role) {
      params.set("role", role);
    }
    if (status) {
      params.set("status", status);
    }
    const suffix = params.size ? `?${params.toString()}` : "";
    return request(`/api/users${suffix}`);
  },
  createUser(payload) {
    return request("/api/users", { method: "POST", body: JSON.stringify(payload) });
  },
  updateUser(payload) {
    return request("/api/users", { method: "PUT", body: JSON.stringify(payload) });
  },
  resetUserPassword(payload) {
    return request("/api/users/password/reset", { method: "POST", body: JSON.stringify(payload) });
  },
  updateUserStatus(payload) {
    return request("/api/users/status", { method: "POST", body: JSON.stringify(payload) });
  },
  deleteUser(userId) {
    return request(`/api/users?id=${encodeURIComponent(userId)}`, { method: "DELETE" });
  },
  getProjects() {
    return request("/api/projects");
  },
  createProject(payload) {
    return request("/api/projects", { method: "POST", body: JSON.stringify(payload) });
  },
  getWorklogs() {
    return request("/api/worklogs");
  },
  createWorklog(payload) {
    return request("/api/worklogs", { method: "POST", body: JSON.stringify(payload) });
  },
  getDeliverables() {
    return request("/api/deliverables");
  },
  createDeliverable(payload) {
    return request("/api/deliverables", { method: "POST", body: JSON.stringify(payload) });
  },
  saveEvidence(payload) {
    return request("/api/evidence", { method: "POST", body: JSON.stringify(payload) });
  },
};
