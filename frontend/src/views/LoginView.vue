<script setup>
import { computed, onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import { api } from "../api/http";
import { useAuthStore } from "../stores/auth";

const router = useRouter();
const auth = useAuthStore();
const errorMessage = ref("");
const form = reactive({ username: "", password: "" });
const demoAccounts = ref([]);
const demoLoading = ref(true);
const demoUnavailableMessage = ref("");
const demoSearch = ref("");
const pendingDemoAccountId = ref(null);

const roleGroups = [
  { role: "admin", label: "管理员" },
  { role: "developer", label: "开发者" },
  { role: "client", label: "客户" },
];

const visibleRoleGroups = computed(() => {
  const query = demoSearch.value.trim().toLocaleLowerCase();

  return roleGroups
    .map((group) => ({
      ...group,
      accounts: demoAccounts.value.filter((account) => {
        if (account.role !== group.role) {
          return false;
        }
        if (!query) {
          return true;
        }
        return `${account.name || ""} ${account.username || ""}`.toLocaleLowerCase().includes(query);
      }),
    }))
    .filter((group) => group.accounts.length > 0);
});

onMounted(loadDemoAccounts);

async function loadDemoAccounts() {
  demoLoading.value = true;
  demoUnavailableMessage.value = "";
  try {
    const payload = await api.getDemoAccounts();
    demoAccounts.value = Array.isArray(payload.items) ? payload.items : [];
  } catch (error) {
    demoAccounts.value = [];
    demoUnavailableMessage.value = error.message?.includes("未启用")
      ? "本地演示快捷登录未启用，仍可使用账号密码登录。"
      : "快捷账号暂时无法加载，请使用账号密码登录。";
  } finally {
    demoLoading.value = false;
  }
}

async function submit() {
  errorMessage.value = "";
  try {
    await auth.login(form);
    router.push("/home");
  } catch (error) {
    errorMessage.value = error.message;
  }
}

async function loginWithDemo(account) {
  errorMessage.value = "";
  pendingDemoAccountId.value = account.id;
  try {
    await auth.loginWithDemo(account.id);
    router.push("/home");
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    pendingDemoAccountId.value = null;
  }
}
</script>

<template>
  <div class="auth-layout">
    <section class="auth-hero">
      <div>
        <div class="hero-kicker">Trust Work</div>
        <h1>让协作过程成为可信记录。</h1>
        <p>统一管理工时、交付物、客户确认、阶段审计与链上证据。</p>
      </div>
    </section>

    <section class="auth-panel">
      <div class="panel auth-card">
        <div class="auth-card-head">
          <div>
            <div class="eyebrow">账户登录</div>
            <h2>进入工作系统</h2>
          </div>
        </div>

        <section class="login-demo-panel" aria-labelledby="login-demo-title" aria-live="polite">
          <div class="login-demo-heading">
            <div>
              <div id="login-demo-title" class="eyebrow">快捷切换</div>
              <p>按账号身份分组，点击即可进入</p>
            </div>
            <span v-if="demoAccounts.length" class="login-demo-count">{{ demoAccounts.length }} 个账号</span>
          </div>

          <div v-if="demoLoading" class="login-demo-message">正在读取本地演示账号…</div>
          <div v-else-if="demoUnavailableMessage" class="login-demo-message">{{ demoUnavailableMessage }}</div>
          <div v-else-if="demoAccounts.length === 0" class="login-demo-message">当前没有可用账号，请先创建或初始化账号。</div>
          <template v-else>
            <label class="login-demo-search">
              <span class="visually-hidden">按姓名或账号筛选</span>
              <input v-model="demoSearch" type="search" autocomplete="off" placeholder="按姓名或账号搜索" />
            </label>

            <div v-if="visibleRoleGroups.length" class="login-demo-groups">
              <section v-for="group in visibleRoleGroups" :key="group.role" class="login-demo-group" :aria-label="group.label">
                <div class="login-demo-group-heading">
                  <h3>{{ group.label }}</h3>
                  <span>{{ group.accounts.length }}</span>
                </div>
                <div class="login-demo-account-list">
                  <button
                    v-for="account in group.accounts"
                    :key="account.id"
                    class="login-demo-account"
                    type="button"
                    :disabled="auth.loading"
                    :aria-label="`以${group.label}身份登录：${account.name || account.username}（${account.username}）`"
                    @click="loginWithDemo(account)"
                  >
                    <span class="login-demo-avatar" aria-hidden="true">{{ (account.name || account.username || "?").slice(0, 1) }}</span>
                    <span class="login-demo-account-copy">
                      <strong>{{ pendingDemoAccountId === account.id ? "正在登录…" : account.name || account.username }}</strong>
                      <small>{{ account.username }}</small>
                    </span>
                    <span class="login-demo-account-arrow" aria-hidden="true">→</span>
                  </button>
                </div>
              </section>
            </div>
            <div v-else class="login-demo-message">没有匹配的账号，请更换搜索内容。</div>
          </template>
        </section>

        <div class="login-auth-divider"><span>或使用账号密码登录</span></div>

        <div class="form-grid">
          <div class="field">
            <label>账号</label>
            <input v-model="form.username" autocomplete="username" placeholder="请输入账号" />
          </div>
          <div class="field">
            <label>密码</label>
            <input v-model="form.password" type="password" autocomplete="current-password" placeholder="请输入密码" @keyup.enter="submit" />
          </div>
        </div>

        <div class="state-explain-card">
          <span>登录说明</span>
          <strong>请使用管理员分配的账号和密码登录。</strong>
        </div>

        <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>

        <div class="button-row">
          <button class="button" type="button" @click="submit" :disabled="auth.loading">
            {{ auth.loading ? "登录中..." : "登录" }}
          </button>
        </div>
      </div>
    </section>
  </div>
</template>

