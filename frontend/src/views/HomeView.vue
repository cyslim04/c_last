<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from "vue";
import { RouterLink } from "vue-router";
import AppShell from "../components/AppShell.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useAuthStore } from "../stores/auth";

const auth = useAuthStore();

const overview = ref({
  headline: "",
  subline: "",
  metrics: [],
  pendingEvidence: [],
  recentTransactions: [],
});
const loading = ref(false);
const errorMessage = ref("");
const homeSections = ref([]);

const roleLabelMap = {
  admin: "系统管理员",
  developer: "开发者",
  client: "客户",
};

const roleLabel = computed(() => roleLabelMap[auth.role] || "平台成员");
const pendingCount = computed(() => overview.value.pendingEvidence.length);
const transactionCount = computed(() => overview.value.recentTransactions.length);

const primaryAction = computed(() => {
  if (auth.role === "admin") {
    return { to: "/audit", label: "进入审计管理" };
  }
  if (auth.role === "developer") {
    return { to: "/worklogs", label: "进入工时工作区" };
  }
  if (auth.role === "client") {
    return { to: "/confirmations", label: "查看待确认阶段" };
  }
  return { to: "/process-flow", label: "查看流程总览" };
});

const secondaryAction = computed(() => ({
  to: "/process-flow",
  label: "查看流程总览",
}));

const structurePanels = [
  {
    code: "01",
    title: "工时登记",
    description: "登记阶段投入与摘要。",
  },
  {
    code: "02",
    title: "交付确认",
    description: "围绕交付结果给出确认。",
  },
  {
    code: "03",
    title: "阶段审计",
    description: "按阶段审阅关键材料。",
  },
  {
    code: "04",
    title: "链上存证",
    description: "关键动作统一留痕。",
  },
];

const roleSections = computed(() => [
  {
    key: "admin",
    code: "A",
    title: "管理员工作区",
    description: "创建项目并推进审计。",
    action: { to: "/audit", label: "查看审计管理" },
  },
  {
    key: "developer",
    code: "D",
    title: "开发者工作区",
    description: "提交工时、交付与修订。",
    action: { to: "/worklogs", label: "进入工时记录" },
  },
  {
    key: "client",
    code: "C",
    title: "客户工作区",
    description: "确认阶段结果并给出反馈。",
    action: { to: "/confirmations", label: "查看客户确认" },
  },
]);

const flowSections = [
  {
    code: "01",
    title: "登记",
    description: "提交工时与交付。",
  },
  {
    code: "02",
    title: "确认",
    description: "客户确认或驳回。",
  },
  {
    code: "03",
    title: "审计",
    description: "管理员按阶段审阅。",
  },
  {
    code: "04",
    title: "存证",
    description: "关键结果统一留痕。",
  },
];

const summaryMetrics = computed(() => overview.value.metrics?.slice(0, 4) || []);
const visiblePendingEvidence = computed(() => overview.value.pendingEvidence?.slice(0, 4) || []);
const visibleTransactions = computed(() => overview.value.recentTransactions?.slice(0, 4) || []);

function shortHash(value) {
  if (!value) {
    return "";
  }
  if (value.length <= 18) {
    return value;
  }
  return `${value.slice(0, 10)}...${value.slice(-6)}`;
}

function collectHomeSection(element) {
  if (!element) {
    return;
  }
  if (!homeSections.value.includes(element)) {
    homeSections.value.push(element);
  }
}

function revealSections() {
  if (typeof window === "undefined") {
    return undefined;
  }

  const showAll = () => {
    homeSections.value.forEach((item) => item?.classList.add("is-visible"));
  };

  if (
    typeof IntersectionObserver === "undefined" ||
    window.matchMedia("(prefers-reduced-motion: reduce)").matches
  ) {
    showAll();
    return undefined;
  }

  const observer = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) {
          entry.target.classList.add("is-visible");
          observer.unobserve(entry.target);
        }
      });
    },
    {
      threshold: 0.1,
      rootMargin: "0px 0px -8% 0px",
    },
  );

  homeSections.value.forEach((item) => {
    if (item) {
      observer.observe(item);
    }
  });

  return observer;
}

async function loadHomeData() {
  loading.value = true;
  errorMessage.value = "";
  try {
    overview.value = await api.getDashboard();
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    loading.value = false;
  }
}


let observer;

onMounted(async () => {
  await loadHomeData();
  observer = revealSections();
});

onBeforeUnmount(() => {
  observer?.disconnect();
});
</script>

<template>
  <AppShell>
    <div class="home-view">
      <section class="home-showcase-board">
        <section class="home-showcase-section home-showcase-hero">
          <div class="home-showcase-hero-inner">
            <div class="home-showcase-hero-copy">
              <div class="home-showcase-kicker home-showcase-kicker-dark">业务工作台</div>
              <h2>工时、交付、确认、审计，一页进入。</h2>
              <div class="home-showcase-accent-line" />
              <p class="home-showcase-hero-lead">查看当前状态，直接处理待办事项。</p>
              <div class="home-showcase-hero-tags">
                <span class="home-hero-chip">{{ roleLabel }}</span>
                <span class="home-hero-chip">四步闭环</span>
              </div>
              <div class="button-row">
                <RouterLink class="button" :to="primaryAction.to">{{ primaryAction.label }}</RouterLink>
                <RouterLink class="ghost-button" :to="secondaryAction.to">{{ secondaryAction.label }}</RouterLink>
              </div>
            </div>

            <div class="home-hero-visual">
              <div class="home-hero-structure-panel">
                <article v-for="item in structurePanels" :key="item.code" class="home-hero-structure-cell">
                  <span class="home-hero-structure-index">{{ item.code }}</span>
                  <strong>{{ item.title }}</strong>
                  <p>{{ item.description }}</p>
                </article>
              </div>
            </div>
          </div>
        </section>

        <section :ref="collectHomeSection" class="home-showcase-section reveal-on-scroll">
          <div class="home-showcase-heading">
            <div class="home-showcase-kicker">角色入口</div>
            <h3>按角色进入对应入口。</h3>
          </div>

          <div class="home-role-grid">
            <article
              v-for="role in roleSections"
              :key="role.key"
              class="home-role-card"
              :class="{ active: role.key === auth.role }"
            >
              <div class="home-role-icon">{{ role.code }}</div>
              <div class="home-role-title">
                <span>{{ role.key === auth.role ? "当前角色" : "系统角色" }}</span>
                <strong>{{ role.title }}</strong>
              </div>
              <p>{{ role.description }}</p>
              <RouterLink class="home-inline-link" :to="role.action.to">{{ role.action.label }}</RouterLink>
            </article>
          </div>
        </section>

        <section :ref="collectHomeSection" class="home-showcase-section reveal-on-scroll">
          <div class="home-showcase-heading">
            <div class="home-showcase-kicker">四步协作</div>
            <h3>流程只保留四个核心动作。</h3>
          </div>

          <div class="home-flow-grid">
            <article v-for="item in flowSections" :key="item.code" class="home-flow-step">
              <span class="home-flow-index">{{ item.code }}</span>
              <strong>{{ item.title }}</strong>
              <p>{{ item.description }}</p>
            </article>
          </div>
        </section>

        <section :ref="collectHomeSection" class="home-showcase-section reveal-on-scroll">
          <div class="home-showcase-heading">
            <div class="home-showcase-kicker">运行信号</div>
            <h3>只看当前最关键的状态。</h3>
            <p class="home-showcase-copy">{{ overview.subline || "简要查看后，直接进入业务页。" }}</p>
          </div>

          <div class="home-archive-layout">
            <div class="home-archive-main">
              <div class="home-summary-grid">
                <article v-for="metric in summaryMetrics" :key="metric.label" class="home-summary-card">
                  <span>{{ metric.label }}</span>
                  <strong>{{ metric.value }}</strong>
                </article>
              </div>

              <div class="home-archive-grid">
                <article v-for="item in visiblePendingEvidence" :key="item.id" class="home-archive-card">
                  <span>{{ item.projectName }}</span>
                  <strong>{{ item.title }}</strong>
                  <StatusTag :value="item.status" />
                </article>
                <article v-if="!visiblePendingEvidence.length" class="home-archive-card">
                  <span>当前状态</span>
                  <strong>暂无待推进事项</strong>
                </article>
              </div>
            </div>

            <aside class="home-archive-aside">
              <article class="home-archive-panel">
                <div class="home-showcase-heading home-showcase-heading-tight">
                  <div class="home-showcase-kicker">最近回写</div>
                  <h3>链上存证</h3>
                </div>

                <div class="home-activity-list">
                  <article v-for="item in visibleTransactions" :key="item.id" class="home-activity-item">
                    <strong>{{ item.type }}</strong>
                    <div class="home-activity-meta">
                      <span>{{ item.createdAt }}</span>
                      <span class="mono">{{ shortHash(item.txHash) }}</span>
                    </div>
                    <StatusTag :value="item.status" />
                  </article>
                  <article v-if="!visibleTransactions.length" class="home-activity-item">
                    <strong>暂无链上回写</strong>
                    <div class="home-activity-meta">
                      <span>进入业务页后可继续推进。</span>
                    </div>
                  </article>
                </div>
              </article>

              <article class="home-pending-note">
                <span>当前身份</span>
                <strong>{{ roleLabel }}</strong>
                <div class="home-activity-meta">
                  <span>待推进 {{ pendingCount }}</span>
                  <span>回写 {{ transactionCount }}</span>
                </div>
              </article>
            </aside>
          </div>
        </section>

        <section :ref="collectHomeSection" class="home-showcase-section home-showcase-footer reveal-on-scroll">
          <div class="home-footer-bar">
            <div class="home-footer-copy">
              <div class="home-showcase-kicker">进入工作区</div>
              <h3>{{ roleLabel }} 已就位，继续下一步。</h3>
            </div>

            <div class="home-footer-actions">
              <RouterLink class="button" :to="primaryAction.to">{{ primaryAction.label }}</RouterLink>
              <RouterLink class="secondary-button" :to="secondaryAction.to">{{ secondaryAction.label }}</RouterLink>
            </div>
          </div>
        </section>
      </section>

      <p v-if="errorMessage" class="feedback error-text" aria-live="polite">{{ errorMessage }}</p>
    </div>
  </AppShell>
</template>

