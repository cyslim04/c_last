<script setup>
import { computed, onBeforeUnmount, onMounted, ref, watchEffect } from "vue";
import { RouterLink } from "vue-router";
import AppShell from "../components/AppShell.vue";
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

const primaryAction = computed(() => {
  if (auth.role === "admin") {
    return { to: "/audit", label: "进入审计管理" };
  }
  if (auth.role === "developer") {
    return { to: "/worklogs", label: "登记工时" };
  }
  if (auth.role === "client") {
    return { to: "/confirmations", label: "查看待确认" };
  }
  return { to: "/process-flow", label: "查看流程总览" };
});

const secondaryAction = computed(() => ({
  to: "/process-flow",
  label: "查看流程总览",
}));

const heroTitle = computed(() => {
  if (auth.role === "admin") {
    return "可信工时与交付协作。";
  }
  if (auth.role === "developer") {
    return "可信登记与交付协作。";
  }
  if (auth.role === "client") {
    return "可信确认与审计闭环。";
  }
  return "可信协作与链上存证。";
});

const heroSubtitle = computed(() => {
  if (auth.role === "admin") {
    return "面向系统管理员的协作入口，聚焦项目治理、阶段审计与链上留痕。";
  }
  if (auth.role === "developer") {
    return "面向开发者的协作入口，聚焦工时登记、交付回写与阶段闭环。";
  }
  if (auth.role === "client") {
    return "面向客户的协作入口，聚焦阶段确认、修订反馈与结果追踪。";
  }
  return "围绕工时、交付、确认与审计建立可验证的协作链路。";
});

const heroQuadrants = computed(() => [
  {
    index: "01",
    title: "工时登记",
    description: "记录开发动作与阶段投入，形成可核验起点。",
  },
  {
    index: "02",
    title: "交付确认",
    description: "围绕交付结果进行客户确认与修订反馈。",
  },
  {
    index: "03",
    title: "阶段审计",
    description: "管理员按阶段复核材料与流程动作。",
  },
  {
    index: "04",
    title: "链上存证",
    description: "关键节点回写链上，保留统一审计依据。",
  },
]);

const roleSections = computed(() => [
  {
    key: "admin",
    code: "A",
    title: "管理员",
    mission: "统筹项目、成员与阶段审计。",
    action: { to: "/audit", label: "进入审计管理" },
  },
  {
    key: "developer",
    code: "D",
    title: "开发者",
    mission: "登记工时、提交交付并响应修订。",
    action: { to: "/worklogs", label: "进入工时记录" },
  },
  {
    key: "client",
    code: "C",
    title: "客户",
    mission: "确认阶段结果并推动协作闭环。",
    action: { to: "/confirmations", label: "查看待确认" },
  },
]);

const flowSteps = computed(() => [
  {
    index: "01",
    title: "登记工时",
    description: "开发者提交阶段投入与交付依据。",
  },
  {
    index: "02",
    title: "提交交付",
    description: "交付物进入确认链路，准备客户核对。",
  },
  {
    index: "03",
    title: "客户确认",
    description: "客户确认或驳回，并给出修订反馈。",
  },
  {
    index: "04",
    title: "阶段审计",
    description: "管理员复核阶段结果并决定是否回写。",
  },
]);

const footerSummary = computed(() => {
  if (auth.role === "admin") {
    return "当前角色负责项目治理、阶段审计与最终闭环。";
  }
  if (auth.role === "developer") {
    return "当前角色负责工时登记、交付提交与修订响应。";
  }
  if (auth.role === "client") {
    return "当前角色负责阶段确认、驳回反馈与结果归档。";
  }
  return "当前页面用于说明可信工时与交付物协作结构。";
});

const footerStatus = computed(() => {
  return pendingCount.value ? `${pendingCount.value} 项待推进动作` : "当前无待推进动作";
});

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
      threshold: 0.16,
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
    projectName: "可信工时与交付物系统首页",
    stageLabel: pendingCount.value ? `${pendingCount.value} 项待推进` : "可信闭环稳定运行",
    cue: "首页保留结构说明、角色入口与可信协作闭环。",
    tone: auth.role || "neutral",
  });
});

let sectionObserver;

onMounted(async () => {
  await loadHomeData();
  sectionObserver = revealSections();
});

onBeforeUnmount(() => {
  sectionObserver?.disconnect();
  experience.resetPageContext();
});
</script>

<template>
  <AppShell>
    <div class="home-view home-view-editorial">
      <div class="home-showcase-board">
        <section :ref="collectHomeSection" class="home-showcase-section home-showcase-hero reveal-on-scroll is-visible">
          <div class="home-showcase-hero-inner">
            <div class="home-showcase-hero-copy">
              <div class="home-showcase-kicker home-showcase-kicker-dark">可信协作平台</div>
              <h2>{{ heroTitle }}</h2>
              <div class="home-showcase-accent-line" />
              <p class="home-showcase-hero-lead">
                {{ heroSubtitle }}
              </p>

              <div class="home-hero-action-row">
                <RouterLink class="button" :to="primaryAction.to">
                  {{ primaryAction.label }}
                </RouterLink>
                <RouterLink class="ghost-button" :to="secondaryAction.to">
                  {{ secondaryAction.label }}
                </RouterLink>
              </div>
            </div>

            <div class="home-hero-visual" aria-hidden="true">
              <div class="home-hero-structure-panel">
                <article v-for="item in heroQuadrants" :key="item.index" class="home-hero-structure-cell">
                  <span class="home-hero-structure-index">{{ item.index }}</span>
                  <strong>{{ item.title }}</strong>
                  <p>{{ item.description }}</p>
                </article>
              </div>
            </div>
          </div>
        </section>

        <section :ref="collectHomeSection" class="home-showcase-section reveal-on-scroll">
          <div class="home-showcase-heading">
            <div class="home-showcase-kicker">平台角色</div>
            <h3>三角色入口</h3>
          </div>

          <div class="home-role-grid">
            <article
              v-for="role in roleSections"
              :key="role.key"
              class="home-role-card"
              :class="{ active: role.key === auth.role }"
            >
              <div class="home-role-icon" aria-hidden="true">{{ role.code }}</div>
              <div class="home-role-title">
                <span>{{ role.title }}</span>
                <strong>{{ role.mission }}</strong>
              </div>

              <RouterLink class="ghost-button" :to="role.action.to">
                {{ role.action.label }}
              </RouterLink>
            </article>
          </div>
        </section>

        <section :ref="collectHomeSection" class="home-showcase-section reveal-on-scroll">
          <div class="home-showcase-heading">
            <div class="home-showcase-kicker">协作流程</div>
            <h3>四步协作流程</h3>
          </div>

          <div class="home-flow-grid">
            <article v-for="step in flowSteps" :key="step.index" class="home-flow-step">
              <span class="home-flow-index">{{ step.index }}</span>
              <strong>{{ step.title }}</strong>
              <p>{{ step.description }}</p>
            </article>
          </div>
        </section>

        <section :ref="collectHomeSection" class="home-showcase-section home-showcase-footer reveal-on-scroll">
          <div class="home-footer-bar">
            <div class="home-footer-copy">
              <div class="home-showcase-kicker home-showcase-kicker-dark">当前角色</div>
              <h3>{{ roleLabel }}</h3>
              <p>{{ footerSummary }}</p>
            </div>

            <div class="home-footer-actions">
              <RouterLink class="button" :to="primaryAction.to">{{ primaryAction.label }}</RouterLink>
              <RouterLink class="ghost-button" :to="secondaryAction.to">{{ secondaryAction.label }}</RouterLink>
            </div>
          </div>

          <div class="home-footer-meta">
            <span translate="no">Trust Work</span>
            <span>可信工时与交付物系统</span>
            <span>{{ footerStatus }}</span>
            <span>{{ loading ? "系统摘要刷新中" : "结构说明已加载" }}</span>
          </div>
        </section>
      </div>

      <p v-if="errorMessage" class="feedback error-text" aria-live="polite">{{ errorMessage }}</p>
    </div>
  </AppShell>
</template>
