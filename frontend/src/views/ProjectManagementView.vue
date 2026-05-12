<script setup>
import { computed, onBeforeUnmount, onMounted, ref, watchEffect } from "vue";
import { RouterLink } from "vue-router";
import AppShell from "../components/AppShell.vue";
import MemberPicker from "../components/MemberPicker.vue";
import PageHero from "../components/PageHero.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useExperienceStore } from "../stores/experience";

const experience = useExperienceStore();

const projects = ref([]);
const developers = ref([]);
const clients = ref([]);
const loading = ref(false);
const errorMessage = ref("");
const statusFilter = ref("all");
const developerFilter = ref("all");
const clientFilter = ref("all");

const statusTabs = [
  { key: "all", label: "全部" },
  { key: "active", label: "进行中" },
  { key: "completed", label: "已完成" },
  { key: "disputed", label: "争议中" },
  { key: "draft", label: "草稿" },
];

const developerOptions = computed(() => [{ id: "all", name: "全部开发者" }, ...developers.value]);
const clientOptions = computed(() => [{ id: "all", name: "全部客户" }, ...clients.value]);

const filteredProjects = computed(() =>
  projects.value.filter((project) => {
    const statusMatch = statusFilter.value === "all" ? true : project.status === statusFilter.value;
    const developerMatch = developerFilter.value === "all" ? true : String(project.developerId) === String(developerFilter.value);
    const clientMatch = clientFilter.value === "all" ? true : String(project.clientId) === String(clientFilter.value);
    return statusMatch && developerMatch && clientMatch;
  }),
);

const groupedProjects = computed(() => {
  const active = [];
  const completed = [];
  const abnormal = [];

  filteredProjects.value.forEach((project) => {
    if (project.status === "completed") {
      completed.push(project);
    } else if (project.status === "active") {
      active.push(project);
    } else {
      abnormal.push(project);
    }
  });

  return [
    { key: "active", title: "进行中项目", items: active },
    { key: "completed", title: "已完成项目", items: completed },
    { key: "abnormal", title: "异常 / 草稿项目", items: abnormal },
  ];
});

const summary = computed(() => ({
  total: filteredProjects.value.length,
  active: filteredProjects.value.filter((project) => project.status === "active").length,
  completed: filteredProjects.value.filter((project) => project.status === "completed").length,
  abnormal: filteredProjects.value.filter((project) => ["disputed", "draft"].includes(project.status)).length,
}));

const heroStats = computed(() => [
  {
    label: "当前结果",
    value: summary.value.total,
    helper: "在当前筛选条件下可见的项目数",
  },
  {
    label: "进行中",
    value: summary.value.active,
    helper: "仍处于活跃协作流程中的项目",
  },
  {
    label: "已完成",
    value: summary.value.completed,
    helper: "已通过最终总审计的项目",
  },
  {
    label: "异常项目",
    value: summary.value.abnormal,
    helper: "争议中或草稿状态的项目",
  },
]);

function canOpenProcessFlow(project) {
  return ["active", "completed"].includes(project.status);
}

function projectCardComponent(project) {
  return canOpenProcessFlow(project) ? RouterLink : "article";
}

function projectCardProps(project) {
  if (!canOpenProcessFlow(project)) {
    return {};
  }
  return {
    to: {
      path: "/process-flow",
      query: { projectId: String(project.id) },
    },
    "aria-label": `查看 ${project.name} 的流程总览`,
  };
}

async function loadData() {
  loading.value = true;
  errorMessage.value = "";
  try {
    const [projectPayload, developerPayload, clientPayload] = await Promise.all([
      api.getProjects(),
      api.getUsers("developer"),
      api.getUsers("client"),
    ]);
    projects.value = projectPayload.items;
    developers.value = developerPayload.items;
    clients.value = clientPayload.items;
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    loading.value = false;
  }
}

function clearFilters() {
  statusFilter.value = "all";
  developerFilter.value = "all";
  clientFilter.value = "all";
}

watchEffect(() => {
  experience.setPageContext({
    projectName: "项目管理",
    stageLabel: `${summary.value.completed} 个已完成项目`,
    cue: "项目管理页专门负责项目状态查看与筛选，方便管理员把已完成、进行中和异常项目清晰归类。",
    tone: "admin",
  });
});

onMounted(loadData);
onBeforeUnmount(() => experience.resetPageContext());
</script>

<template>
  <AppShell>
    <PageHero
      eyebrow="管理员工作区"
      title="在一个统一的浅色项目台里筛选、分组和追踪全部项目状态。"
      description="项目管理不再只是一个控制台列表，而是围绕状态分层、角色筛选和流程入口组织出来的全局项目台。"
      tone="admin"
      variant="minimal"
      :stats="heroStats"
    >
      <template #actions>
        <button class="button" type="button" @click="loadData" :disabled="loading">
          {{ loading ? "刷新中..." : "刷新项目" }}
        </button>
      </template>

      <article class="hero-side-card">
        <div class="hero-kicker">当前筛选</div>
        <strong>
          {{ statusTabs.find((item) => item.key === statusFilter)?.label || "全部" }}
          ｜ {{ developerOptions.find((item) => String(item.id) === String(developerFilter))?.name || "全部开发者" }}
          ｜ {{ clientOptions.find((item) => String(item.id) === String(clientFilter))?.name || "全部客户" }}
        </strong>
        <p>当切到“已完成”时，页面会只显示 completed 项目；在“全部”视图下，结果会按进行中、已完成、异常项目分段收纳。</p>
      </article>
    </PageHero>

    <section class="panel reveal-card project-filter-panel">
      <div class="section-heading">
        <div>
          <div class="eyebrow">筛选区</div>
          <h3>筛选</h3>
        </div>
        <button class="ghost-button" type="button" @click="clearFilters">清空筛选</button>
      </div>

      <div class="filter-chip-row">
        <button
          v-for="tab in statusTabs"
          :key="tab.key"
          class="pill filter-chip"
          :class="{ subtle: statusFilter !== tab.key }"
          type="button"
          @click="statusFilter = tab.key"
        >
          {{ tab.label }}
        </button>
      </div>

      <div class="showcase-grid showcase-grid-secondary">
        <div class="field">
          <label>开发者筛选</label>
          <MemberPicker
            v-model="developerFilter"
            :items="developerOptions"
            placeholder="全部开发者"
            :show-meta="false"
          />
        </div>
        <div class="field">
          <label>客户筛选</label>
          <MemberPicker
            v-model="clientFilter"
            :items="clientOptions"
            placeholder="全部客户"
            :show-meta="false"
          />
        </div>
      </div>
    </section>

    <template v-if="statusFilter === 'all'">
      <section
        v-for="group in groupedProjects"
        :key="group.key"
        class="panel reveal-card"
      >
        <div class="section-heading">
          <div>
            <div class="eyebrow">项目分组</div>
            <h3>{{ group.title }}</h3>
          </div>
          <span class="pill subtle">{{ group.items.length }} 个项目</span>
        </div>

        <div v-if="group.items.length" class="brand-project-grid">
          <component
            :is="projectCardComponent(project)"
            v-for="project in group.items"
            :key="project.id"
            v-bind="projectCardProps(project)"
            class="brand-project-card"
            :class="{
              'project-link-card': canOpenProcessFlow(project),
              'project-static-card': !canOpenProcessFlow(project),
            }"
          >
            <div class="project-card-top">
              <div>
                <strong>{{ project.name }}</strong>
                <div class="meta-line">开发者：{{ project.developerName }} ｜ 客户：{{ project.clientName }}</div>
              </div>
              <StatusTag :value="project.status" />
            </div>
            <p>{{ project.description }}</p>
            <div v-if="canOpenProcessFlow(project)" class="project-card-link-hint">
              进入流程总览
              <span aria-hidden="true">→</span>
            </div>
          </component>
        </div>
        <div v-else class="empty-state brand-empty-state">
          <strong>{{ group.title }}为空</strong>
          <p>暂无项目。</p>
        </div>
      </section>
    </template>

    <section v-else class="panel reveal-card">
      <div class="section-heading">
        <div>
          <div class="eyebrow">筛选结果</div>
          <h3>{{ statusTabs.find((item) => item.key === statusFilter)?.label || "项目结果" }}</h3>
        </div>
        <span class="pill subtle">{{ filteredProjects.length }} 个项目</span>
      </div>

      <div v-if="filteredProjects.length" class="brand-project-grid">
        <component
          :is="projectCardComponent(project)"
          v-for="project in filteredProjects"
          :key="project.id"
          v-bind="projectCardProps(project)"
          class="brand-project-card"
          :class="{
            'project-link-card': canOpenProcessFlow(project),
            'project-static-card': !canOpenProcessFlow(project),
          }"
        >
          <div class="project-card-top">
            <div>
              <strong>{{ project.name }}</strong>
              <div class="meta-line">开发者：{{ project.developerName }} ｜ 客户：{{ project.clientName }}</div>
            </div>
            <StatusTag :value="project.status" />
          </div>
          <p>{{ project.description }}</p>
          <div v-if="canOpenProcessFlow(project)" class="project-card-link-hint">
            进入流程总览
            <span aria-hidden="true">→</span>
          </div>
        </component>
      </div>
      <div v-else class="empty-state brand-empty-state">
        <strong>当前筛选没有命中项目</strong>
        <p>未找到项目。</p>
      </div>
    </section>

    <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
  </AppShell>
</template>
