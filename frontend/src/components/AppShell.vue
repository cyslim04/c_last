<script setup>
import { computed, nextTick, onBeforeUnmount, onMounted, ref, Teleport, watch } from "vue";
import { RouterLink, useRoute, useRouter } from "vue-router";
import { useAuthStore } from "../stores/auth";
import { useExperienceStore } from "../stores/experience";
import { useWalletStore } from "../stores/wallet";

const router = useRouter();
const route = useRoute();
const auth = useAuthStore();
const experience = useExperienceStore();
const wallet = useWalletStore();

const walletOpen = ref(false);
const userOpen = ref(false);
const mobileNavOpen = ref(false);
const compactLayout = ref(false);
const syncingWallet = ref(false);
const walletMessage = ref("");
const walletTrigger = ref(null);
const userTrigger = ref(null);
const topbarRef = ref(null);
const walletStyle = ref({});
const userStyle = ref({});
const topbarHeight = ref(96);
const contentStyle = computed(() => ({
  "--content-top-offset": `${topbarHeight.value}px`,
}));

let topbarResizeObserver;

const roleLabelMap = {
  admin: "系统管理员",
  developer: "开发者",
  client: "客户",
  guest: "访客",
};

const roleToneMap = {
  admin: "admin",
  developer: "developer",
  client: "client",
  guest: "neutral",
};

function buildUserBadgeLabel(value) {
  const source = String(value || "访客").trim();
  if (!source) {
    return "访";
  }
  if (/[\u4e00-\u9fa5]/.test(source)) {
    return source.slice(0, 1);
  }
  return source.slice(0, 2).toUpperCase();
}

const navItems = computed(() => {
  if (auth.role === "admin") {
    return [
      { to: "/home", label: "首页" },
      { to: "/members", label: "成员管理" },
      { to: "/projects", label: "新增项目" },
      { to: "/project-management", label: "项目管理" },
      { to: "/process-flow", label: "流程总览" },
      { to: "/audit", label: "审计管理" },
      { to: "/profile", label: "个人中心" },
    ];
  }
  if (auth.role === "developer") {
    return [
      { to: "/home", label: "首页" },
      { to: "/worklogs", label: "工时记录" },
      { to: "/deliverables", label: "交付物" },
      { to: "/process-flow", label: "流程总览" },
      { to: "/profile", label: "个人中心" },
    ];
  }
  if (auth.role === "client") {
    return [
      { to: "/home", label: "首页" },
      { to: "/confirmations", label: "客户确认" },
      { to: "/process-flow", label: "流程总览" },
      { to: "/profile", label: "个人中心" },
    ];
  }
  return [
    { to: "/home", label: "首页" },
    { to: "/process-flow", label: "流程总览" },
    { to: "/profile", label: "个人中心" },
  ];
});

const supportedNetworks = computed(() => wallet.availableNetworks);
const roleLabel = computed(() => roleLabelMap[auth.user?.role || "guest"] || auth.user?.role || "访客");
const displayName = computed(() => auth.user?.name || auth.user?.username || "访客");
const userBadgeLabel = computed(() => buildUserBadgeLabel(auth.user?.name || auth.user?.username || "访客"));
const userMetaLabel = computed(() => auth.user?.company || auth.user?.username || roleLabel.value);
const userCompany = computed(() => auth.user?.company || "演示环境");
const shellTone = computed(() => experience.pageContext.tone || roleToneMap[auth.role] || "neutral");
const mobileMenuLabel = computed(() => (mobileNavOpen.value ? "收起导航" : "展开导航"));
const footerGroups = computed(() => {
  if (auth.role === "admin") {
    return [
      {
        title: "管理入口",
        links: [
          { to: "/members", label: "成员管理" },
          { to: "/projects", label: "新增项目" },
          { to: "/project-management", label: "项目管理" },
        ],
      },
      {
        title: "闭环控制",
        links: [
          { to: "/audit", label: "审计管理" },
          { to: "/process-flow", label: "流程总览" },
          { to: "/profile", label: "个人中心" },
        ],
      },
      {
        title: "当前身份",
        links: [
          { to: "/home", label: roleLabel.value },
          { to: "/home", label: userCompany.value },
          { to: "/home", label: wallet.connected ? wallet.connectionLabel : "钱包未连接" },
        ],
      },
    ];
  }
  if (auth.role === "developer") {
    return [
      {
        title: "开发工作区",
        links: [
          { to: "/worklogs", label: "工时记录" },
          { to: "/deliverables", label: "交付物" },
          { to: "/process-flow", label: "流程总览" },
        ],
      },
      {
        title: "协作状态",
        links: [
          { to: "/home", label: roleLabel.value },
          { to: "/home", label: wallet.connected ? wallet.connectionLabel : "等待连接钱包" },
          { to: "/profile", label: "个人中心" },
        ],
      },
      {
        title: "系统入口",
        links: [
          { to: "/home", label: "首页" },
          { to: "/profile", label: "身份资料" },
        ],
      },
    ];
  }
  if (auth.role === "client") {
    return [
      {
        title: "确认工作区",
        links: [
          { to: "/confirmations", label: "客户确认" },
          { to: "/process-flow", label: "流程总览" },
          { to: "/profile", label: "个人中心" },
        ],
      },
      {
        title: "当前身份",
        links: [
          { to: "/home", label: roleLabel.value },
          { to: "/home", label: wallet.connected ? wallet.connectionLabel : "等待连接钱包" },
        ],
      },
      {
        title: "系统入口",
        links: [
          { to: "/home", label: "首页" },
          { to: "/profile", label: "身份资料" },
        ],
      },
    ];
  }
  return [
    {
      title: "导航",
      links: [
        { to: "/home", label: "首页" },
        { to: "/process-flow", label: "流程总览" },
        { to: "/profile", label: "个人中心" },
      ],
    },
  ];
});

function buildFloatingStyle(triggerRef, preferredWidth = 360) {
  if (window.innerWidth <= 900) {
    return {
      left: "12px",
      right: "12px",
      bottom: "12px",
      top: "auto",
      width: "auto",
      maxHeight: `${Math.max(220, window.innerHeight - 24)}px`,
    };
  }

  const rect = triggerRef?.value?.getBoundingClientRect();
  if (!rect) {
    return {};
  }

  const panelWidth = Math.min(preferredWidth, window.innerWidth - 24);
  let left = rect.right - panelWidth;
  left = Math.max(12, Math.min(left, window.innerWidth - panelWidth - 12));
  let top = rect.bottom + 12;
  const estimatedHeight = 320;
  if (top + estimatedHeight > window.innerHeight - 12 && rect.top > estimatedHeight) {
    top = Math.max(12, rect.top - estimatedHeight - 12);
  }

  return {
    left: `${left}px`,
    top: `${top}px`,
    width: `${panelWidth}px`,
    maxHeight: `${Math.max(220, window.innerHeight - top - 12)}px`,
  };
}

function refreshOverlayPositions() {
  if (walletOpen.value) {
    walletStyle.value = buildFloatingStyle(walletTrigger);
  }
  if (userOpen.value) {
    userStyle.value = buildFloatingStyle(userTrigger, 320);
  }
}

function closeMenus() {
  walletOpen.value = false;
  userOpen.value = false;
}

function closeMobileNav() {
  mobileNavOpen.value = false;
}

function toggleWalletMenu() {
  walletOpen.value = !walletOpen.value;
  userOpen.value = false;
  mobileNavOpen.value = false;
}

function toggleUserMenu() {
  userOpen.value = !userOpen.value;
  walletOpen.value = false;
  mobileNavOpen.value = false;
}

function toggleMobileNav() {
  mobileNavOpen.value = !mobileNavOpen.value;
  closeMenus();
}

function updateViewportState() {
  compactLayout.value = window.innerWidth <= 1080;
  if (!compactLayout.value) {
    mobileNavOpen.value = false;
  }
  refreshOverlayPositions();
  syncTopbarHeight();
}

function syncTopbarHeight() {
  const nextHeight = Math.ceil(topbarRef.value?.getBoundingClientRect().height || 0);
  if (nextHeight > 0) {
    topbarHeight.value = nextHeight;
  }
}

function handleWindowKeydown(event) {
  if (event.key === "Escape") {
    closeMenus();
    closeMobileNav();
  }
}

async function logout() {
  await auth.logout();
  await wallet.disconnect();
  closeMenus();
  closeMobileNav();
  router.push("/login");
}

async function connectWallet() {
  try {
    walletMessage.value = "";
    syncingWallet.value = true;
    const result = await wallet.connect();
    if (auth.isAuthenticated && result?.address) {
      await auth.bindWallet({
        walletAddress: result.address,
        chainId: result.chainId,
        chainName: result.chainName,
      });
    }
  } catch (error) {
    walletMessage.value = error.message;
  } finally {
    syncingWallet.value = false;
    refreshOverlayPositions();
  }
}

async function switchAccount() {
  try {
    walletMessage.value = "";
    syncingWallet.value = true;
    const result = await wallet.switchAccount();
    if (auth.isAuthenticated && result?.address) {
      await auth.bindWallet({
        walletAddress: result.address,
        chainId: result.chainId,
        chainName: result.chainName,
      });
    }
  } catch (error) {
    walletMessage.value = error.message;
  } finally {
    syncingWallet.value = false;
    refreshOverlayPositions();
  }
}

async function switchNetwork(chainId) {
  try {
    walletMessage.value = "";
    syncingWallet.value = true;
    const result = await wallet.switchNetwork(chainId);
    if (auth.isAuthenticated && result?.address) {
      await auth.bindWallet({
        walletAddress: result.address,
        chainId: result.chainId,
        chainName: result.chainName,
      });
    }
  } catch (error) {
    walletMessage.value = error.message;
  } finally {
    syncingWallet.value = false;
    refreshOverlayPositions();
  }
}

async function disconnectWallet() {
  walletMessage.value = "";
  await wallet.disconnect();
  refreshOverlayPositions();
}

watch(
  () => router.currentRoute.value.fullPath,
  () => {
    closeMenus();
    closeMobileNav();
  },
);

watch(
  () => walletOpen.value,
  async (open) => {
    if (open) {
      await nextTick();
      walletStyle.value = buildFloatingStyle(walletTrigger);
    }
  },
);

watch(
  () => userOpen.value,
  async (open) => {
    if (open) {
      await nextTick();
      userStyle.value = buildFloatingStyle(userTrigger, 320);
    }
  },
);

watch(
  () => [wallet.address, wallet.chainId].join("|"),
  async () => {
    if (!auth.isAuthenticated || !wallet.connected || !wallet.address) {
      return;
    }
    const current = auth.user?.walletAddress?.toLowerCase();
    if (current === wallet.address.toLowerCase()) {
      return;
    }
    try {
      await auth.bindWallet({
        walletAddress: wallet.address,
        chainId: wallet.chainId,
        chainName: wallet.chainName,
      });
    } catch (error) {
      walletMessage.value = error.message;
    }
  },
);

onMounted(async () => {
  auth.refreshMe().catch(() => {});
  updateViewportState();
  window.addEventListener("keydown", handleWindowKeydown);
  window.addEventListener("resize", updateViewportState);
  await nextTick();
  syncTopbarHeight();
  if (typeof ResizeObserver !== "undefined" && topbarRef.value) {
    topbarResizeObserver = new ResizeObserver(() => {
      syncTopbarHeight();
    });
    topbarResizeObserver.observe(topbarRef.value);
  }
});

onBeforeUnmount(() => {
  window.removeEventListener("keydown", handleWindowKeydown);
  window.removeEventListener("resize", updateViewportState);
  topbarResizeObserver?.disconnect();
});
</script>

<template>
  <div class="shell" :class="[`shell-tone-${shellTone}`]">
    <div v-if="compactLayout && mobileNavOpen" class="shell-backdrop" @click="closeMobileNav" />

    <header ref="topbarRef" class="topbar topbar-global">
      <div class="w-100">
        <div class="topbar-main topbar-main-global d-flex justify-content-between align-items-center gap-3">
          <div class="d-flex align-items-center gap-3 min-w-0">
            <RouterLink class="brand-panel brand-panel-topbar" to="/home">
              <div class="brand-mark brand-mark-topbar">TW</div>
              <div class="brand-copy brand-copy-topbar">
                <span class="brand-overline">可信协作平台</span>
                <h1>Trust Work</h1>
              </div>
            </RouterLink>
          </div>

          <button
            v-if="compactLayout"
            class="topbar-icon-button topbar-menu-button"
            type="button"
            :aria-expanded="mobileNavOpen"
            :aria-label="mobileMenuLabel"
            @click="toggleMobileNav"
          >
            <span class="menu-bar" />
            <span class="menu-bar" />
            <span class="menu-bar short" />
          </button>

          <div v-if="!compactLayout" class="topbar-actions topbar-actions-global d-flex align-items-center gap-3">
            <nav class="nav-list nav-list-topbar" aria-label="主导航">
              <RouterLink
                v-for="item in navItems"
                :key="item.to"
                class="nav-link nav-link-topbar"
                :to="item.to"
              >
                <strong>{{ item.label }}</strong>
              </RouterLink>
            </nav>

            <div class="topbar-group topbar-group-wallet">
              <button ref="walletTrigger" class="wallet-pill" type="button" @click="toggleWalletMenu">
                <span class="wallet-indicator" :class="{ active: wallet.connected && wallet.supportedNetwork }" />
                <span class="wallet-label">{{ wallet.connectionLabel }}</span>
              </button>
            </div>

            <button
              ref="userTrigger"
              class="session-chip session-chip-topbar"
              type="button"
              @click="toggleUserMenu"
              :title="`${auth.user?.name || '访客'} · ${roleLabel} · ${userCompany}`"
              aria-label="打开会话菜单"
            >
              <img v-if="auth.user?.avatarUrl" :src="auth.user.avatarUrl" alt="avatar" class="session-chip-avatar avatar-image" />
              <span v-else class="session-chip-avatar avatar-fallback">{{ userBadgeLabel }}</span>
              <span class="session-chip-copy">
                <strong>{{ displayName }}</strong>
                <span>{{ userMetaLabel }}</span>
              </span>
              <span class="session-chip-caret menu-caret" :class="{ open: userOpen }">⌄</span>
            </button>
          </div>
        </div>

        <div v-if="compactLayout && mobileNavOpen" class="mobile-nav-panel">
          <nav class="nav-list nav-list-mobile" aria-label="移动导航">
            <RouterLink
              v-for="item in navItems"
              :key="item.to"
              class="nav-link nav-link-mobile"
              :to="item.to"
              @click="closeMobileNav"
            >
              <strong>{{ item.label }}</strong>
            </RouterLink>
          </nav>

          <div class="topbar-group topbar-group-wallet">
            <button ref="walletTrigger" class="wallet-pill" type="button" @click="toggleWalletMenu">
              <span class="wallet-indicator" :class="{ active: wallet.connected && wallet.supportedNetwork }" />
              <span class="wallet-label">{{ wallet.connectionLabel }}</span>
            </button>
          </div>

          <button
            ref="userTrigger"
            class="session-chip session-chip-topbar"
            type="button"
            @click="toggleUserMenu"
            :title="`${auth.user?.name || '访客'} · ${roleLabel} · ${userCompany}`"
            aria-label="打开会话菜单"
          >
            <img v-if="auth.user?.avatarUrl" :src="auth.user.avatarUrl" alt="avatar" class="session-chip-avatar avatar-image" />
            <span v-else class="session-chip-avatar avatar-fallback">{{ userBadgeLabel }}</span>
            <span class="session-chip-copy">
              <strong>{{ displayName }}</strong>
              <span>{{ userMetaLabel }}</span>
            </span>
            <span class="session-chip-caret menu-caret" :class="{ open: userOpen }">⌄</span>
          </button>
        </div>
      </div>
    </header>

    <main class="content content-topbar" :style="contentStyle">
      <section class="page-slot">
        <slot />
      </section>
    </main>

    <footer class="stride-footer">
      <div class="stride-footer-inner">
        <div class="stride-footer-grid">
          <div class="stride-footer-card">
            <h3>Trust Work</h3>
            <p>把工时、交付确认、阶段审计与链上留痕组织成可展示、可审计、可验证的协作前端。</p>
          </div>

          <div v-for="group in footerGroups" :key="group.title" class="stride-footer-nav">
            <h4>{{ group.title }}</h4>
            <RouterLink v-for="link in group.links" :key="`${group.title}-${link.label}`" :to="link.to">
              {{ link.label }}
            </RouterLink>
          </div>
        </div>

        <div class="stride-footer-copy">
          当前身份：{{ roleLabel }} · {{ displayName }} · {{ wallet.connected ? wallet.connectionLabel : "钱包未连接" }}
        </div>
      </div>
    </footer>

    <Teleport to="body">
      <template v-if="walletOpen">
        <div class="overlay-backdrop" @click="closeMenus" />
        <div class="dropdown-panel floating-panel wallet-dropdown" :style="walletStyle">
          <div class="dropdown-title">钱包状态</div>
          <p class="dropdown-copy">
            {{
              wallet.connected
                ? `当前地址 ${wallet.shortAddress}，网络 ${wallet.chainName || "未知网络"}`
                : "连接 MetaMask 后，可快速切换账户、网络，并推进演示存证。"
            }}
          </p>
          <p v-if="walletMessage" class="feedback error-text">{{ walletMessage }}</p>
          <div class="dropdown-actions">
            <button class="button" type="button" @click="connectWallet" :disabled="syncingWallet">
              {{ wallet.connected ? "重新连接" : "连接 MetaMask" }}
            </button>
            <button class="secondary-button" type="button" @click="switchAccount" :disabled="!wallet.connected || syncingWallet">
              切换账户
            </button>
            <button class="ghost-button" type="button" @click="disconnectWallet" :disabled="!wallet.connected || syncingWallet">
              退出连接
            </button>
          </div>
          <div class="network-list">
            <button
              v-for="network in supportedNetworks"
              :key="network.chainId"
              class="network-button"
              :class="{ active: wallet.chainId === network.chainId }"
              type="button"
              @click="switchNetwork(network.chainId)"
              :disabled="syncingWallet"
            >
              {{ network.name }}
            </button>
          </div>
          <p v-if="wallet.connected && !wallet.supportedNetwork" class="feedback error-text">
            当前网络不在支持列表，请切换到 Sepolia 或本地 Hardhat。
          </p>
        </div>
      </template>
    </Teleport>

    <Teleport to="body">
      <template v-if="userOpen">
        <div class="overlay-backdrop" @click="closeMenus" />
        <div class="dropdown-panel floating-panel user-dropdown" :style="userStyle">
          <div class="dropdown-title">会话操作</div>
          <p class="dropdown-copy">头像可直接进入个人中心，此处保留会话操作。</p>
          <RouterLink class="menu-link" to="/profile" @click="closeMenus">查看资料</RouterLink>
          <button class="ghost-button full-width" type="button" @click="logout">退出登录</button>
        </div>
      </template>
    </Teleport>
  </div>
</template>
