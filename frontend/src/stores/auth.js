import { defineStore } from "pinia";
import { api } from "../api/http";

const STORAGE_KEY = "trust-work-session";

function saveSession(session) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(session));
}

function loadSession() {
  return JSON.parse(localStorage.getItem(STORAGE_KEY) || "null");
}

export const useAuthStore = defineStore("auth", {
  state: () => ({
    session: loadSession(),
    loading: false,
  }),
  getters: {
    isAuthenticated: (state) => Boolean(state.session?.token),
    role: (state) => state.session?.user?.role || "guest",
    token: (state) => state.session?.token || "",
    user: (state) => state.session?.user || null,
  },
  actions: {
    persist(session) {
      this.session = session;
      saveSession(session);
    },
    patchUser(user) {
      if (!this.session) {
        return;
      }
      this.persist({ ...this.session, user });
    },
    clear() {
      this.session = null;
      localStorage.removeItem(STORAGE_KEY);
    },
    async login(credentials) {
      this.loading = true;
      try {
        const payload = await api.login(credentials);
        this.persist(payload);
        return payload;
      } finally {
        this.loading = false;
      }
    },
    async refreshMe() {
      if (!this.session?.token) {
        return null;
      }
      const payload = await api.getMe();
      this.patchUser(payload.user);
      return payload.user;
    },
    async updateProfile(form) {
      const payload = await api.updateProfile(form);
      this.patchUser(payload.user);
      return payload.user;
    },
    async changePassword(form) {
      return api.changePassword(form);
    },
    async uploadAvatar(payload) {
      const response = await api.uploadAvatar(payload);
      this.patchUser(response.user);
      return response.user;
    },
    async bindWallet(payload) {
      const response = await api.bindWallet(payload);
      this.patchUser(response.user);
      return response.user;
    },
    async logout() {
      try {
        if (this.session?.token) {
          await api.logout();
        }
      } finally {
        this.clear();
      }
    },
  },
});
