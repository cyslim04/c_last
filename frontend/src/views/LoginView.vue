<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref } from "vue";
import { useRouter } from "vue-router";
import { api } from "../api/http";
import { useAuthStore } from "../stores/auth";

const router = useRouter();
const auth = useAuthStore();

const errorMessage = ref("");
const quickLoginError = ref("");
const openGroup = ref("");
const quickLoginRef = ref(null);
const quickAccounts = ref([]);
const form = reactive({
  username: "admin",
  password: "admin123",
});

const quickAccountGroups = [
  { key: "admin", title: "管理员", placeholder: "选择管理员账号" },
  { key: "developer", title: "开发者", placeholder: "选择开发者账号" },
  { key: "client", title: "客户", placeholder: "选择客户账号" },
];

const selectedQuick = reactive({
  admin: "",
  developer: "",
  client: "",
});

const groupedQuickAccounts = computed(() => {
  const groups = {
    admin: [],
    developer: [],
    client: [],
  };

  quickAccounts.value.forEach((account) => {
    if (groups[account.role]) {
      groups[account.role].push({
        ...account,
        id: String(account.id),
      });
    }
  });

  return groups;
});

function getSelectedAccount(groupKey) {
  return groupedQuickAccounts.value[groupKey].find((account) => account.id === selectedQuick[groupKey]) || null;
}

function useDemoAccount(account) {
  form.username = account.username;
  form.password = account.password;
}

function toggleGroup(groupKey) {
  openGroup.value = openGroup.value === groupKey ? "" : groupKey;
}

function selectAccount(groupKey, account) {
  Object.keys(selectedQuick).forEach((key) => {
    selectedQuick[key] = key === groupKey ? String(account.id) : "";
  });
  useDemoAccount(account);
  openGroup.value = "";
}

async function loadQuickAccounts() {
  quickLoginError.value = "";
  try {
    const payload = await api.getQuickAccounts();
    quickAccounts.value = Array.isArray(payload.items) ? payload.items : [];
    Object.keys(selectedQuick).forEach((key) => {
      if (!groupedQuickAccounts.value[key].some((account) => account.id === selectedQuick[key])) {
        selectedQuick[key] = "";
      }
    });
  } catch (error) {
    quickAccounts.value = [];
    quickLoginError.value = error.message || "快捷登录加载失败，请手动输入账号密码。";
  }
}

function handleDocumentClick(event) {
  if (quickLoginRef.value && !quickLoginRef.value.contains(event.target)) {
    openGroup.value = "";
  }
}

function handleWindowKeydown(event) {
  if (event.key === "Escape") {
    openGroup.value = "";
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

onMounted(() => {
  loadQuickAccounts();
  document.addEventListener("mousedown", handleDocumentClick);
  window.addEventListener("keydown", handleWindowKeydown);
});

onBeforeUnmount(() => {
  document.removeEventListener("mousedown", handleDocumentClick);
  window.removeEventListener("keydown", handleWindowKeydown);
});
</script>

<template>
  <div class="auth-layout auth-layout-stage auth-layout-compact">
    <section class="auth-hero auth-hero-stage auth-hero-simple">
      <h1>Trust Work</h1>
      <p>选择快捷账号或手动输入账号密码后进入系统。</p>
    </section>

    <section class="auth-panel auth-panel-stage">
      <div class="panel auth-card auth-card-stage">
        <div class="page-header compact">
          <div class="auth-card-head">
            <div>
              <h2>登录</h2>
            </div>
          </div>
        </div>

        <div ref="quickLoginRef" class="quick-login-compact-panel">
          <div class="quick-login-compact-head">
            <div class="eyebrow">快捷角色</div>
          </div>

          <div class="quick-login-compact-grid">
            <section
              v-for="group in quickAccountGroups"
              :key="group.key"
              class="quick-login-group quick-login-group-compact"
              :class="{ open: openGroup === group.key }"
            >
              <button class="quick-login-trigger quick-login-trigger-compact" type="button" @click="toggleGroup(group.key)">
                <div class="quick-login-trigger-copy">
                  <span class="quick-login-role">{{ group.title }}</span>
                  <strong>{{ getSelectedAccount(group.key)?.displayName || group.placeholder }}</strong>
                </div>
                <span class="menu-caret" :class="{ open: openGroup === group.key }">⌄</span>
              </button>

              <div v-if="openGroup === group.key" class="quick-login-menu quick-login-menu-compact">
                <p v-if="!groupedQuickAccounts[group.key].length" class="empty-state">当前暂无可用快捷账号</p>
                <button
                  v-for="account in groupedQuickAccounts[group.key]"
                  :key="account.id"
                  class="quick-login-option quick-login-option-compact"
                  :class="{ active: selectedQuick[group.key] === account.id }"
                  type="button"
                  @click="selectAccount(group.key, account)"
                >
                  <strong>{{ account.displayName }}</strong>
                  <span>账号：{{ account.username }}</span>
                  <span>密码：{{ account.password }}</span>
                </button>
              </div>
            </section>
          </div>
        </div>

        <p v-if="quickLoginError" class="feedback error-text">{{ quickLoginError }}</p>

        <div class="form-grid">
          <div class="field">
            <label>账号</label>
            <input v-model="form.username" placeholder="请输入账号" />
          </div>
          <div class="field">
            <label>密码</label>
            <input v-model="form.password" type="password" placeholder="请输入密码" />
          </div>
        </div>

        <div class="login-tip-card">
          <div class="hero-kicker">登录提示</div>
          <strong>快捷账号只会填充表单，不会自动登录。</strong>
        </div>

        <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>

        <div class="button-row">
          <button class="button" type="button" @click="submit" :disabled="auth.loading">
            {{ auth.loading ? "登录中..." : "进入系统" }}
          </button>
        </div>
      </div>
    </section>
  </div>
</template>
