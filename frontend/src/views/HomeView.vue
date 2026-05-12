<script setup>
import { computed, onBeforeUnmount, onMounted, ref, watchEffect } from "vue";
import { RouterLink } from "vue-router";
import AppShell from "../components/AppShell.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useAuthStore } from "../stores/auth";
import { useExperienceStore } from "../stores/experience";

const auth = useAuthStore();
const experience = useExperienceStore();

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
    description: "把任务说明、投入时长和阶段摘要固定下来，让开发过程具备可回看依据。",
  },
  {
    code: "02",
    title: "交付确认",
    description: "把文件版本、哈希和客户判断绑在一起，避免交付与反馈脱节。",
  },
  {
    code: "03",
    title: "阶段审计",
    description: "管理员按阶段审阅材料，而不是等到最后再对整项目一次性判断。",
  },
  {
    code: "04",
    title: "链上存证",
    description: "关键动作可回写演示链路，让过程解释与结果展示拥有统一留痕。",
  },
];

const roleSections = computed(() => [
  {
    key: "admin",
    code: "A",
    title: "管理员工作区",
    description: "建立项目、编排成员、观察流程，并在阶段审计与最终总审计中给出可追溯结论。",
    points: ["成员与项目一起编排", "阶段审计与总审计分层决策"],
    action: { to: "/audit", label: "查看审计管理" },
  },
  {
    key: "developer",
    code: "D",
    title: "开发者工作区",
    description: "登记工时、提交交付物、补充修订内容，把开发过程逐步组织成可核验的阶段材料。",
    points: ["工时与交付进入同一阶段", "客户反馈会回到修订回环"],
    action: { to: "/worklogs", label: "进入工时记录" },
  },
  {
    key: "client",
    code: "C",
    title: "客户工作区",
    description: "基于阶段材料给出确认或驳回意见，让协作流程具备明确反馈和再次推进的依据。",
    points: ["确认后进入管理员阶段审计", "驳回后回到开发者补充修订"],
    action: { to: "/confirmations", label: "查看客户确认" },
  },
]);

const flowSections = [
  {
    code: "01",
    title: "开发者登记阶段材料",
    description: "先形成工时摘要和交付版本，确保每一阶段都有可说明、可回看的输入。",
  },
  {
    code: "02",
    title: "客户给出确认或驳回",
    description: "客户围绕同阶段材料做判断，避免反馈只停留在口头或聊天窗口里。",
  },
  {
    code: "03",
    title: "管理员执行阶段审计",
    description: "客户通过后进入阶段审计，管理员按阶段沉淀逐步形成可解释的项目结论。",
  },
  {
    code: "04",
    title: "满足条件后进入总审计",
    description: "阶段逐步闭合后再进入最终审计，让最后结论建立在完整阶段链路之上。",
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

watchEffect(() => {
  experience.setPageContext({
    projectName: "Stride 化首页",
    stageLabel: pendingCount.value ? `${pendingCount.value} 项待推进` : "可信闭环稳定运行",
    cue: "首页需要同时承担品牌介绍、角色导流、流程说明和可信展示入口。",
    tone: auth.role || "neutral",
  });
});

let observer;

onMounted(async () => {
  await loadHomeData();
  observer = revealSections();
});

onBeforeUnmount(() => {
  observer?.disconnect();
  experience.resetPageContext();
});
</script>

<template>
  <AppShell>
    <div class="home-view home-view-editorial">
      <section class="home-showcase-board">
        <section class="home-showcase-section home-showcase-hero">
          <div class="home-showcase-hero-inner">
            <div class="home-showcase-hero-copy">
              <div class="home-showcase-kicker home-showcase-kicker-dark">可信协作首页</div>
              <h2>让工时、交付、确认与审计在一张暖石色工作纸上闭环推进。</h2>
              <div class="home-showcase-accent-line" />
              <p class="home-showcase-hero-lead">
                Trust Work 不是首屏塞满指标的仪表盘，而是一套真正可操作的多角色前端。首页先讲清结构，再把你送进对应工作区。
              </p>
              <p class="home-showcase-copy">
                {{ overview.headline || "管理员、开发者与客户围绕同一项目反复协作，每次提交、确认与审计都会留在同一条可信链路里。" }}
              </p>
              <div class="home-showcase-hero-tags">
                <span class="home-hero-chip">{{ roleLabel }}</span>
                <span class="home-hero-chip">4 个关键结构单元</span>
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
            <h3>每个角色都进入同一条协作链路，但承担不同的闭环动作。</h3>
            <p class="home-showcase-copy">首页负责导流，不把业务页的动态状态硬塞进首屏。</p>
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
              <ul class="home-role-list">
                <li v-for="point in role.points" :key="point">{{ point }}</li>
              </ul>
              <RouterLink class="home-inline-link" :to="role.action.to">{{ role.action.label }}</RouterLink>
            </article>
          </div>
        </section>

        <section :ref="collectHomeSection" class="home-showcase-section reveal-on-scroll">
          <div class="home-showcase-heading">
            <div class="home-showcase-kicker">四步协作</div>
            <h3>把多角色流程拆成清晰的四段节奏，而不是混成一张控制台。</h3>
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
            <h3>动态数据下沉到这里，只承担当前系统的说明作用。</h3>
            <p class="home-showcase-copy">{{ overview.subline || "你可以先看当前运行信号，再进入具体业务操作页面。" }}</p>
          </div>

          <div class="home-archive-layout">
            <div class="home-archive-main">
              <div class="home-summary-grid">
                <article v-for="metric in summaryMetrics" :key="metric.label" class="home-summary-card">
                  <span>{{ metric.label }}</span>
                  <strong>{{ metric.value }}</strong>
                  <p>{{ metric.description }}</p>
                </article>
              </div>

              <div class="home-archive-grid">
                <article v-for="item in visiblePendingEvidence" :key="item.id" class="home-archive-card">
                  <span>{{ item.projectName }}</span>
                  <strong>{{ item.title }}</strong>
                  <p>{{ item.description }}</p>
                  <StatusTag :value="item.status" />
                </article>
                <article v-if="!visiblePendingEvidence.length" class="home-archive-card">
                  <span>当前状态</span>
                  <strong>没有待解释的动作</strong>
                  <p>当前角色暂无待推进记录，可以直接进入工作区查看详情。</p>
                </article>
              </div>
            </div>

            <aside class="home-archive-aside">
              <article class="home-archive-panel">
                <div class="home-showcase-heading home-showcase-heading-tight">
                  <div class="home-showcase-kicker">最近回写</div>
                  <h3>链上与存证节奏</h3>
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
                    <strong>当前还没有链上回写</strong>
                    <div class="home-activity-meta">
                      <span>可在业务页推进演示存证后回到这里查看。</span>
                    </div>
                  </article>
                </div>
              </article>

              <article class="home-pending-note">
                <span>当前身份</span>
                <strong>{{ roleLabel }}</strong>
                <p>首屏不负责堆放业务数字，而是先把当前身份、平台结构和下一步入口讲清楚。</p>
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
              <h3>{{ roleLabel }} 已就位，继续进入你的下一步操作。</h3>
              <p>首页负责收口整体结构，真正的工时、交付、确认、审计和流程动作仍在对应业务页中完成。</p>
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
