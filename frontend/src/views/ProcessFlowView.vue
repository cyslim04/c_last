<script setup>
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch, watchEffect } from "vue";
import { useRoute, useRouter } from "vue-router";
import AppShell from "../components/AppShell.vue";
import PageHero from "../components/PageHero.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useAuthStore } from "../stores/auth";
import { useExperienceStore } from "../stores/experience";

const auth = useAuthStore();
const experience = useExperienceStore();
const route = useRoute();
const router = useRouter();

const flow = ref({
  globalHint: null,
  globalOverview: null,
  globalSteps: [],
  projectOptions: [],
  selectedProject: null,
  roleProgress: [],
  stageSummaries: [],
  currentStage: null,
  finalAuditStatus: null,
});
const selectedProjectId = ref("");
const selectedGlobalKey = ref("");
const expandedHistoryStageNo = ref(null);
const loading = ref(false);
const errorMessage = ref("");
const projectMenuOpen = ref(false);
const projectMenuRef = ref(null);
const projectMenuPanelRef = ref(null);
const projectTriggerRef = ref(null);
const projectMenuStyle = ref({});
const viewMode = ref("global");

const laneOrder = ["admin", "developer", "client"];

const roleLabelMap = {
  admin: "管理员",
  developer: "开发者",
  client: "客户",
  completed: "阶段闭环",
  none: "未开始",
};

const laneCueMap = {
  admin: "项目建立、阶段审计与总审计",
  developer: "登记工时、提交交付与补充修订",
  client: "确认、反馈与回退判断",
};

const overviewNodeOrder = [
  "project_setup",
  "wallet_ready",
  "developer_log_submit",
  "client_review",
  "developer_revise",
  "admin_stage_audit",
  "final_audit",
];

const viewModes = [
  { key: "global", label: "高层流程" },
  { key: "loop", label: "阶段循环" },
  { key: "snapshot", label: "当前快照" },
];

const routeProjectId = computed(() => {
  const value = route.query.projectId;
  if (Array.isArray(value)) {
    return value[0] || "";
  }
  return value ? String(value) : "";
});

const hasProject = computed(() => Boolean(flow.value.selectedProject));
const selectedProjectLabel = computed(() => flow.value.selectedProject?.name || "选择演示项目");
const selectedProjectMeta = computed(() => {
  if (!flow.value.selectedProject) {
    return "当前角色暂无可见项目";
  }
  const stageMeta = currentStage.value?.stageNo ? `当前阶段 ${currentStage.value.stageNo}` : "尚未进入阶段";
  return `${flow.value.selectedProject.developerName || "未分配开发者"} / ${flow.value.selectedProject.clientName || "未分配客户"} / ${stageMeta}`;
});

const globalOverviewNodes = computed(() =>
  (flow.value.globalOverview?.nodes || [])
    .map((node, index) => ({
      ...node,
      orderIndex: (overviewNodeOrder.indexOf(node.key) >= 0 ? overviewNodeOrder.indexOf(node.key) : index) + 1,
      laneIndex: laneOrder.indexOf(node.actor) >= 0 ? laneOrder.indexOf(node.actor) : 0,
    }))
    .sort((a, b) => a.orderIndex - b.orderIndex),
);

const selectedGlobalStep = computed(() => {
  if (!globalOverviewNodes.value.length) {
    return null;
  }
  return (
    globalOverviewNodes.value.find((step) => step.key === selectedGlobalKey.value) ||
    globalOverviewNodes.value.find((step) => step.isCurrentFocus) ||
    globalOverviewNodes.value[0]
  );
});

const currentStage = computed(() => {
  if (flow.value.stageSummaries?.length) {
    return (
      flow.value.stageSummaries.find((stage) => stage.isCurrent) ||
      [...flow.value.stageSummaries].sort((a, b) => b.stageNo - a.stageNo)[0]
    );
  }
  return flow.value.currentStage;
});

const orderedStages = computed(() =>
  [...(flow.value.stageSummaries?.length ? flow.value.stageSummaries : currentStage.value ? [currentStage.value] : [])].sort(
    (a, b) => (a.stageNo || 0) - (b.stageNo || 0),
  ),
);

const historicalStages = computed(() =>
  orderedStages.value
    .filter((stage) => !currentStage.value || stage.stageNo !== currentStage.value.stageNo)
    .sort((a, b) => (b.stageNo || 0) - (a.stageNo || 0)),
);

const finalAuditRequirements = computed(() => flow.value.finalAuditStatus?.requirements || []);

const globalStepCount = computed(() => Math.max(globalOverviewNodes.value.length, 1));
const globalLaneTemplate = computed(() => `repeat(${globalStepCount.value}, minmax(0, 1fr))`);
const roleProgressByRole = computed(() =>
  Object.fromEntries((flow.value.roleProgress || []).map((item) => [item.role, item])),
);
const orderedRoleProgress = computed(() => laneOrder.map((role) => roleProgressByRole.value[role]).filter(Boolean));

const globalLaneRows = computed(() =>
  (flow.value.globalOverview?.lanes || laneOrder.map((role) => ({ role, title: roleLabel(role), summary: laneCueMap[role] }))).map((lane) => ({
    role: lane.role,
    label: lane.title || roleLabel(lane.role),
    cue: lane.summary || laneCueMap[lane.role],
    steps: globalOverviewNodes.value.filter((step) => step.actor === lane.role),
  })),
);

const globalNextTitles = computed(() => {
  const edges = flow.value.globalOverview?.edges || [];
  const nodesByKey = Object.fromEntries(globalOverviewNodes.value.map((node) => [node.key, node]));
  if (!selectedGlobalStep.value) {
    return [];
  }
  return edges
    .filter((edge) => edge.from === selectedGlobalStep.value.key)
    .map((edge) => nodesByKey[edge.to]?.title)
    .filter(Boolean);
});

const globalLanePaths = computed(() => {
  const stepsByKey = Object.fromEntries(globalOverviewNodes.value.map((step) => [step.key, step]));
  const left = 76;
  const right = 924;
  const top = 68;
  const laneGap = 140;
  const xFor = (orderIndex) => {
    if (globalStepCount.value <= 1) {
      return (left + right) / 2;
    }
    return left + ((orderIndex - 1) / (globalStepCount.value - 1)) * (right - left);
  };
  const yFor = (laneIndex) => top + laneIndex * laneGap;

  return (flow.value.globalOverview?.edges || [])
    .map((link) => {
      const fromStep = stepsByKey[link.from];
      const toStep = stepsByKey[link.to];
      if (!fromStep || !toStep) {
        return null;
      }
      const x1 = xFor(fromStep.orderIndex);
      const y1 = yFor(fromStep.laneIndex);
      const x2 = xFor(toStep.orderIndex);
      const y2 = yFor(toStep.laneIndex);
      let d = "";

      if (link.loop) {
        d = `M ${x1} ${y1} C ${x1 - 72} ${y1 + 92}, ${x2 + 118} ${y2 + 102}, ${x2} ${y2}`;
      } else {
        const controlX = x1 + (x2 - x1) * 0.48;
        d = `M ${x1} ${y1} C ${controlX} ${y1}, ${controlX} ${y2}, ${x2} ${y2}`;
      }

      const stateClass = link.tone === "focus" ? "link-active" : "link-muted";

      return {
        ...link,
        d,
        stateClass,
      };
    })
    .filter(Boolean);
});

function buildStageLaneRows(stage, includeProgress = false) {
  return laneOrder.map((role) => {
    const roleState = stage?.roleStates?.find((item) => item.role === role);
    const progress = includeProgress ? roleProgressByRole.value[role] : null;
    return {
      role,
      title: roleLabel(role),
      status: roleState?.status || progress?.status || "neutral",
      summary: roleState?.summary || progress?.summary || "当前暂无该角色的推进信息。",
      nextAction: progress?.nextAction || (stage?.currentOwner === role ? stage?.nextAction : ""),
      blockedBy: progress?.blockedBy || (roleState?.status === "locked" ? stage?.blockingReason : ""),
      stageNo: progress?.stageNo || stage?.stageNo || "-",
      isOwner: stage?.currentOwner === role,
    };
  });
}

const currentStageLaneRows = computed(() => buildStageLaneRows(currentStage.value, true));
const snapshotRoleRows = computed(() =>
  laneOrder.map((role) => {
    const progress = roleProgressByRole.value[role];
    return {
      role,
      title: roleLabel(role),
      status: progress?.status || "neutral",
      stageNo: progress?.stageNo || currentStage.value?.stageNo || "-",
      summary: progress?.summary || "当前暂无该角色推进信息。",
      nextAction: progress?.nextAction || "",
      blockedBy: progress?.blockedBy || "",
    };
  }),
);

const snapshotGateRows = computed(() => {
  const unmetRequirement = finalAuditRequirements.value.find((item) => !item.met)?.label || "";
  const blockerSummary =
    flow.value.finalAuditStatus?.blockingReason ||
    currentStage.value?.blockingReason ||
    "";

  return [
    {
      key: "current-stage",
      label: "当前阶段",
      title: currentStage.value
        ? `阶段 ${currentStage.value.stageNo} · ${currentStage.value.phase}`
        : "尚未进入阶段循环",
      status: currentStage.value?.status || "neutral",
      owner: currentOwnerLabel(currentStage.value),
      summary: currentStage.value?.summary || "项目还未形成可推进阶段。",
      detail: currentStage.value?.nextAction || currentStage.value?.blockingReason || "当前暂无额外说明。",
    },
    {
      key: "final-audit",
      label: "最终总审计",
      title: flow.value.finalAuditStatus?.eligible ? "管理员可执行最终总审计" : "最终总审计仍未解锁",
      status: flow.value.finalAuditStatus?.status || "neutral",
      owner: "管理员",
      summary: flow.value.finalAuditStatus?.summary || "当前没有总审计状态。",
      detail: flow.value.finalAuditStatus?.blockingReason || unmetRequirement || "当前没有额外门禁说明。",
    },
    {
      key: "blocking",
      label: "当前阻塞",
      title: flow.value.finalAuditStatus?.blockingStageNo
        ? `阶段 ${flow.value.finalAuditStatus.blockingStageNo}`
        : blockerSummary
          ? "全局门禁"
          : "当前无阻塞",
      status: blockerSummary ? "blocked" : "completed",
      owner: currentOwnerLabel(currentStage.value),
      summary: blockerSummary || "当前没有额外阻塞，流程可以继续正常推进。",
      detail: currentStage.value?.nextAction || "当前无需额外人工干预。",
    },
  ];
});

const heroStats = computed(() => [
  {
    label: "高层节点",
    value: globalOverviewNodes.value.length,
    helper: "用于说明整条协作链路结构的关键节点数量。",
  },
  {
    label: "阶段总数",
    value: orderedStages.value.length,
    helper: "当前项目已经形成或仍在推进的阶段数量。",
  },
  {
    label: "当前视图",
    value: viewModes.find((item) => item.key === viewMode.value)?.label || "高层流程",
    helper: "高层流程、阶段循环与快照三种视图共用同一项目上下文。",
  },
  {
    label: "总审计门禁",
    value: flow.value.finalAuditStatus?.eligible ? "已满足" : "未满足",
    helper: flow.value.finalAuditStatus?.blockingReason || "当前项目已满足最终总审计条件。",
  },
]);

function compactText(value, max = 28) {
  if (!value) {
    return "";
  }
  return value.length > max ? `${value.slice(0, max)}...` : value;
}

function statusTagValue(value) {
  if (value === "locked") {
    return "locked";
  }
  if (value === "ready") {
    return "ready";
  }
  if (value === "blocked") {
    return "blocked";
  }
  return value || "neutral";
}

function flowNodeStateClass(value) {
  if (value === "completed") {
    return "state-completed";
  }
  if (value === "running" || value === "ready") {
    return "state-running";
  }
  if (value === "failed" || value === "blocked") {
    return "state-blocked";
  }
  return "state-locked";
}

function overviewToneClass(value) {
  if (value === "focus") {
    return "overview-focus";
  }
  if (value === "available") {
    return "overview-available";
  }
  return "overview-neutral";
}

function overviewToneLabel(step) {
  if (!step) {
    return "";
  }
  if (step.isCurrentFocus) {
    return "当前焦点";
  }
  if (step.tone === "available") {
    return "结构节点";
  }
  return "流程节点";
}

function roleLabel(value) {
  return roleLabelMap[value] || value || "未知角色";
}

function currentOwnerLabel(stage) {
  return roleLabel(stage?.currentOwner);
}

function historicalStageLaneRows(stage) {
  return buildStageLaneRows(stage, false);
}

function isGlobalLaneFocused(lane) {
  return lane.steps.some((step) => step.key === selectedGlobalStep.value?.key);
}

function hasStageContent(stage) {
  return Boolean(stage?.requirement || stage?.nextAction || stage?.blockingReason || stage?.roleStates?.length);
}

function isStageExpanded(stage) {
  return Boolean(stage?.isCurrent || expandedHistoryStageNo.value === stage?.stageNo);
}

function toggleStage(stage) {
  if (!stage || stage.isCurrent) {
    return;
  }
  expandedHistoryStageNo.value = expandedHistoryStageNo.value === stage.stageNo ? null : stage.stageNo;
}

function syncRouteProjectId(projectId, navigationMode = "replace") {
  const nextProjectId = projectId ? String(projectId) : "";
  const currentProjectId = routeProjectId.value ? String(routeProjectId.value) : "";
  if (nextProjectId === currentProjectId) {
    return Promise.resolve();
  }
  const nextQuery = { ...route.query };
  if (nextProjectId) {
    nextQuery.projectId = nextProjectId;
  } else {
    delete nextQuery.projectId;
  }
  return router[navigationMode]({
    path: route.path,
    query: nextQuery,
  });
}

async function loadFlow(projectId) {
  loading.value = true;
  errorMessage.value = "";
  try {
    const requestedProjectId = projectId ? String(projectId) : "";
    flow.value = await api.getProcessFlow(requestedProjectId);
    selectedProjectId.value = String(flow.value.selectedProject?.id || "");
    projectMenuOpen.value = false;
    selectedGlobalKey.value =
      flow.value.globalOverview?.focusKey ||
      flow.value.globalSteps?.find((step) => step.isCurrent)?.key ||
      flow.value.globalSteps?.[0]?.key ||
      "";
    expandedHistoryStageNo.value = null;
    if (requestedProjectId && selectedProjectId.value && requestedProjectId !== selectedProjectId.value) {
      await syncRouteProjectId(selectedProjectId.value, "replace");
    }
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    loading.value = false;
  }
}

function toggleProjectMenu() {
  if (!flow.value.projectOptions.length) {
    return;
  }
  const nextOpen = !projectMenuOpen.value;
  if (nextOpen) {
    projectMenuStyle.value = buildProjectMenuStyle();
  }
  projectMenuOpen.value = nextOpen;
}

function selectProject(projectId) {
  if (String(projectId) === String(selectedProjectId.value)) {
    projectMenuOpen.value = false;
    return;
  }
  projectMenuOpen.value = false;
  syncRouteProjectId(projectId, "push");
}

function selectGlobalStep(stepKey) {
  selectedGlobalKey.value = stepKey;
}

function handleDocumentClick(event) {
  const clickedInsideTrigger = projectMenuRef.value?.contains(event.target);
  const clickedInsideMenu = projectMenuPanelRef.value?.contains(event.target);
  if (!clickedInsideTrigger && !clickedInsideMenu) {
    projectMenuOpen.value = false;
  }
}

function handleWindowKeydown(event) {
  if (event.key === "Escape") {
    projectMenuOpen.value = false;
  }
}

function buildProjectMenuStyle() {
  const rect = projectTriggerRef.value?.getBoundingClientRect();
  if (!rect) {
    return {};
  }

  const viewportPadding = 12;
  const desiredWidth = Math.max(rect.width, 320);
  const width = Math.min(desiredWidth, window.innerWidth - viewportPadding * 2);
  const left = Math.max(
    viewportPadding,
    Math.min(rect.left, window.innerWidth - width - viewportPadding),
  );
  const spaceBelow = window.innerHeight - rect.bottom - viewportPadding;
  const spaceAbove = rect.top - viewportPadding;
  const openUpward = spaceBelow < 240 && spaceAbove > spaceBelow;
  const baseStyle = {
    left: `${left}px`,
    width: `${width}px`,
    maxHeight: `${Math.max(220, openUpward ? spaceAbove - 10 : spaceBelow - 10)}px`,
  };

  if (openUpward) {
    return {
      ...baseStyle,
      top: "auto",
      bottom: `${Math.max(viewportPadding, window.innerHeight - rect.top + 10)}px`,
    };
  }

  return {
    ...baseStyle,
    top: `${Math.min(rect.bottom + 10, window.innerHeight - viewportPadding)}px`,
    bottom: "auto",
  };
}

function syncProjectMenuPosition() {
  if (!projectMenuOpen.value) {
    return;
  }
  projectMenuStyle.value = buildProjectMenuStyle();
}

watchEffect(() => {
  experience.setPageContext({
    projectName: flow.value.selectedProject?.name || "品牌级链路地图",
    stageLabel: currentStage.value?.stageNo ? `阶段 ${currentStage.value.stageNo}` : "全局链路视图",
    cue: "把高层流程图、阶段循环与总审计门禁放在同一张流程总览里讲清楚。",
    tone: auth.role || "neutral",
  });
});

watch(
  () => projectMenuOpen.value,
  async (open) => {
    if (!open) {
      projectMenuStyle.value = {};
      return;
    }
    await nextTick();
    syncProjectMenuPosition();
  },
);

onMounted(() => {
  document.addEventListener("mousedown", handleDocumentClick);
  window.addEventListener("keydown", handleWindowKeydown);
  window.addEventListener("resize", syncProjectMenuPosition);
  window.addEventListener("scroll", syncProjectMenuPosition, true);
});

watch(
  routeProjectId,
  (projectId) => {
    loadFlow(projectId || "");
  },
  { immediate: true },
);

onBeforeUnmount(() => {
  document.removeEventListener("mousedown", handleDocumentClick);
  window.removeEventListener("keydown", handleWindowKeydown);
  window.removeEventListener("resize", syncProjectMenuPosition);
  window.removeEventListener("scroll", syncProjectMenuPosition, true);
  experience.resetPageContext();
});
</script>

<template>
  <AppShell>
    <PageHero
      eyebrow="全局流程"
      title="把高层结构、阶段循环与最终门禁收敛到一张流程总览里。"
      description="流程总览页不是装饰性流程图，而是把项目切换、角色责任、阶段推进和总审计条件放在同一张工作地图里解释清楚。"
      :tone="auth.role"
      variant="minimal"
      :stats="heroStats"
      class="process-flow-hero"
    >
      <template #actions>
        <div class="process-flow-toolbar">
          <div ref="projectMenuRef" class="project-switcher project-switcher-hero" :class="{ open: projectMenuOpen }">
            <button
              ref="projectTriggerRef"
              class="project-switcher-trigger"
              :class="{ open: projectMenuOpen }"
              type="button"
              @click="toggleProjectMenu"
              :disabled="loading || !flow.projectOptions.length"
              :aria-expanded="projectMenuOpen"
              aria-haspopup="listbox"
            >
              <div class="project-switcher-copy">
                <strong>{{ selectedProjectLabel }}</strong>
                <span>{{ selectedProjectMeta }}</span>
              </div>
              <span class="menu-caret" :class="{ open: projectMenuOpen }">⌄</span>
            </button>
          </div>
          <div class="segmented-control process-flow-view-modes">
            <button
              v-for="item in viewModes"
              :key="item.key"
              class="segment-button"
              :class="{ active: viewMode === item.key }"
              type="button"
              @click="viewMode = item.key"
            >
              {{ item.label }}
            </button>
          </div>
          <button
            class="ghost-button process-flow-refresh"
            type="button"
            @click="loadFlow(routeProjectId || selectedProjectId || '')"
            :disabled="loading"
          >
            {{ loading ? "刷新中..." : "刷新状态" }}
          </button>
        </div>

        <Teleport to="body">
          <div
            v-if="projectMenuOpen"
            ref="projectMenuPanelRef"
            class="project-switcher-menu project-switcher-floating-menu"
            :style="projectMenuStyle"
            role="listbox"
            aria-label="项目切换菜单"
          >
            <button
              v-for="project in flow.projectOptions"
              :key="project.id"
              class="project-option"
              :class="{ active: String(project.id) === String(selectedProjectId) }"
              type="button"
              @click="selectProject(project.id)"
            >
              <div class="project-option-copy">
                <strong>{{ project.name }}</strong>
                <span>{{ project.developerName }} / {{ project.clientName }}</span>
              </div>
              <StatusTag :value="project.status" />
            </button>
          </div>
        </Teleport>
      </template>

      <article class="hero-side-card">
        <div class="hero-kicker">当前项目</div>
        <strong>{{ selectedProjectLabel }}</strong>
        <p>{{ selectedProjectMeta }}</p>
      </article>
    </PageHero>

    <section v-if="viewMode === 'global'" class="panel reveal-card process-map-panel">
      <div class="section-heading process-map-heading">
        <div>
          <div class="eyebrow">高层流程</div>
          <h3>泳道流程图</h3>
        </div>
        <p class="process-map-summary">
          按三角色泳道查看流程推进。
        </p>
      </div>

      <div class="process-lane-layout">
        <div class="process-lane-board">
          <svg class="process-lane-links" viewBox="0 0 1000 420" aria-hidden="true">
            <defs>
              <linearGradient id="process-lane-line" x1="0%" x2="100%" y1="0%" y2="0%">
                <stop offset="0%" stop-color="#7f8791" stop-opacity="0.18"></stop>
                <stop offset="50%" stop-color="#d4d8de" stop-opacity="0.92"></stop>
                <stop offset="100%" stop-color="#7f8791" stop-opacity="0.18"></stop>
              </linearGradient>
              <marker id="process-lane-arrow" markerWidth="10" markerHeight="10" refX="6" refY="3" orient="auto">
                <path d="M0,0 L6,3 L0,6" fill="#d4d8de"></path>
              </marker>
            </defs>
            <path
              v-for="link in globalLanePaths"
              :key="link.key"
              :class="['process-lane-link', link.stateClass, { loop: link.loop }]"
              :d="link.d"
            />
          </svg>

          <div class="process-lane-rows">
            <div
              v-for="lane in globalLaneRows"
              :key="lane.role"
              class="process-lane-row"
              :class="{ focused: isGlobalLaneFocused(lane) }"
            >
              <article
                class="process-lane-label-card"
                :class="{ active: isGlobalLaneFocused(lane) }"
              >
                <span class="process-lane-label-kicker">{{ lane.label }}</span>
                <strong>{{ lane.label }}泳道</strong>
              </article>
              <div
                class="process-lane-track"
                :class="{ focused: isGlobalLaneFocused(lane) }"
                :style="{ gridTemplateColumns: globalLaneTemplate }"
              >
                <button
                  v-for="step in lane.steps"
                  :key="step.key"
                  type="button"
                  class="process-lane-node"
                  :class="[
                    overviewToneClass(step.tone),
                    { current: step.isCurrentFocus, selected: selectedGlobalStep?.key === step.key },
                  ]"
                  :style="{ gridColumn: step.orderIndex }"
                  @click="selectGlobalStep(step.key)"
                >
                  <span class="process-lane-node-index">0{{ step.orderIndex }}</span>
                  <strong>{{ step.title }}</strong>
                  <p class="process-lane-node-summary">{{ compactText(step.summary, 14) }}</p>
                  <span class="process-overview-pill" :class="overviewToneClass(step.tone)">
                    {{ step.isCurrentFocus ? "当前" : step.kind === "loop" ? "循环" : step.kind === "audit" ? "审计" : step.key === "project_setup" ? "立项" : "准备" }}
                  </span>
                </button>
              </div>
            </div>
          </div>
        </div>

        <aside v-if="selectedGlobalStep" class="process-map-detail-card">
          <div class="eyebrow">当前聚焦</div>
          <span class="process-overview-pill detail-pill" :class="overviewToneClass(selectedGlobalStep.tone)">
            {{ overviewToneLabel(selectedGlobalStep) }}
          </span>
          <h3>{{ selectedGlobalStep.title }}</h3>
          <p class="process-map-detail-copy">{{ compactText(selectedGlobalStep.detail, 40) }}</p>

          <div class="process-map-detail-block">
            <span>责任角色</span>
            <strong>{{ roleLabel(selectedGlobalStep.actor) }}</strong>
          </div>
          <div class="process-map-detail-block">
            <span>结构说明</span>
            <strong>{{ selectedGlobalStep.summary }}</strong>
          </div>
          <div class="process-map-detail-block">
            <span>下一步流转</span>
            <strong>{{ globalNextTitles.length ? globalNextTitles.join(" / ") : "当前是流程收束点" }}</strong>
          </div>
        </aside>
      </div>
    </section>

    <div v-else-if="viewMode === 'loop'" class="process-loop-layout">
      <section class="panel reveal-card process-role-strip-panel">
        <div class="section-heading">
          <div>
            <div class="eyebrow">角色推进</div>
            <h3>泳道摘要条</h3>
          </div>
          <p class="process-role-summary">先看三角色当前状态。</p>
        </div>

        <div class="process-role-strip process-role-strip-lanes">
          <article
            v-for="item in orderedRoleProgress"
            :key="item.role"
            class="process-role-chip process-role-lane-chip"
            :class="flowNodeStateClass(item.status)"
          >
            <div class="process-role-chip-top">
              <span>{{ roleLabel(item.role) }}</span>
              <StatusTag :value="statusTagValue(item.status)" />
            </div>
            <strong>{{ item.title }}</strong>
            <div class="process-role-chip-stage">阶段 {{ item.stageNo || "-" }}</div>
            <small>{{ compactText(item.summary, 18) }}</small>
            <div v-if="item.blockedBy" class="process-role-chip-blocked">{{ compactText(item.blockedBy, 16) }}</div>
          </article>
        </div>
      </section>

      <section v-if="hasProject && currentStage" class="panel reveal-card process-current-stage-panel process-stage-lane-panel">
        <div class="section-heading">
          <div>
            <div class="eyebrow">当前阶段</div>
            <h3>阶段 {{ currentStage.stageNo }}</h3>
          </div>
          <StatusTag :value="statusTagValue(currentStage.status)" />
        </div>

        <div class="process-current-stage-copy">
          <div class="process-current-meta-strip">
            <span class="process-owner-pill">当前责任方：{{ currentOwnerLabel(currentStage) }}</span>
            <span class="pill subtle">工时 {{ currentStage.worklogCount || 0 }}</span>
            <span class="pill subtle">交付 {{ currentStage.deliverableCount || 0 }}</span>
            <span class="pill subtle">存证 {{ currentStage.evidenceCount || 0 }}</span>
          </div>
        </div>

        <div class="process-stage-lane-board">
          <article
            v-for="row in currentStageLaneRows"
            :key="row.role"
            class="process-stage-lane-row"
            :class="[flowNodeStateClass(row.status), { owner: row.isOwner }]"
          >
            <div class="process-stage-lane-label">
              <strong>{{ row.title }}</strong>
              <span>阶段 {{ row.stageNo }}</span>
            </div>
            <div class="process-stage-lane-content">
              <div class="process-stage-lane-head">
                <StatusTag :value="statusTagValue(row.status)" />
                <span v-if="row.isOwner" class="pill subtle">当前责任方</span>
              </div>
              <strong>{{ compactText(row.summary, 24) }}</strong>
              <p v-if="row.nextAction">{{ compactText(row.nextAction, 20) }}</p>
              <p v-if="row.blockedBy"><strong>阻塞：</strong>{{ compactText(row.blockedBy, 18) }}</p>
            </div>
          </article>
        </div>

        <div class="process-stage-footnotes">
          <div v-if="currentStage.latestDeliverableVersion" class="process-footnote-card">
            <span>最新交付</span>
            <strong>{{ currentStage.latestDeliverableVersion }} / {{ currentStage.latestDeliverableName }}</strong>
          </div>
          <div v-if="currentStage.approvalComment" class="process-footnote-card">
            <span>客户意见</span>
            <strong>{{ currentStage.approvalComment }}</strong>
          </div>
          <div v-if="currentStage.stageAuditComment" class="process-footnote-card">
            <span>阶段审计</span>
            <strong>{{ currentStage.stageAuditComment }}</strong>
          </div>
          <div class="process-footnote-card">
            <span>最终总审计</span>
            <strong>
              {{
                flow.finalAuditStatus?.eligible
                  ? "当前项目已满足最终总审计条件"
                  : flow.finalAuditStatus?.blockingReason || "仍未满足最终总审计条件"
              }}
            </strong>
          </div>
        </div>
      </section>

      <section v-if="hasProject" class="panel reveal-card process-history-panel">
          <div class="section-heading">
            <div>
              <div class="eyebrow">历史阶段</div>
            <h3>历史阶段</h3>
          </div>
          <p class="process-role-summary">按阶段号倒序查看。</p>
        </div>

        <div v-if="historicalStages.length" class="process-history-lane-list">
          <article
            v-for="stage in historicalStages"
            :key="stage.stageNo"
            class="process-history-lane-card"
            :class="{ open: isStageExpanded(stage) }"
          >
            <button type="button" class="process-history-lane-trigger" @click="toggleStage(stage)">
              <div class="process-history-lane-title">
                <span class="process-history-stage">阶段 {{ stage.stageNo }}</span>
                <strong>{{ stage.phase }}</strong>
                <p>{{ compactText(stage.summary, 22) }}</p>
              </div>
              <div class="process-history-lane-trigger-meta">
                <span class="pill subtle">责任方：{{ currentOwnerLabel(stage) }}</span>
                <StatusTag :value="statusTagValue(stage.status)" />
              </div>
            </button>

            <Transition name="process-expand">
              <div v-if="isStageExpanded(stage) && hasStageContent(stage)" class="process-history-lane-body">
                <div class="process-history-lane-meta">
                  <span class="pill subtle">工时 {{ stage.worklogCount || 0 }}</span>
                  <span class="pill subtle">交付 {{ stage.deliverableCount || 0 }}</span>
                  <span class="pill subtle">存证 {{ stage.evidenceCount || 0 }}</span>
                </div>
                <div class="process-history-lane-rows">
                  <article
                    v-for="row in historicalStageLaneRows(stage)"
                    :key="`${stage.stageNo}-${row.role}`"
                    class="process-history-lane-row"
                    :class="[flowNodeStateClass(row.status), { owner: row.isOwner }]"
                  >
                    <div class="process-history-lane-role">
                      <strong>{{ row.title }}</strong>
                    </div>
                    <div class="process-history-lane-copy">
                      <StatusTag :value="statusTagValue(row.status)" />
                      <p>{{ compactText(row.summary, 18) }}</p>
                    </div>
                  </article>
                </div>
                <p><strong>门禁：</strong>{{ compactText(stage.requirement || "暂无门禁说明", 26) }}</p>
                <p><strong>下一步：</strong>{{ compactText(stage.nextAction || "暂无下一步说明", 22) }}</p>
                <p v-if="stage.blockingReason"><strong>阻塞：</strong>{{ compactText(stage.blockingReason, 22) }}</p>
                <p v-if="stage.latestDeliverableVersion"><strong>最新交付：</strong>{{ stage.latestDeliverableVersion }} / {{ stage.latestDeliverableName }}</p>
                <p v-if="stage.approvalComment"><strong>客户意见：</strong>{{ compactText(stage.approvalComment, 20) }}</p>
                <p v-if="stage.stageAuditComment"><strong>阶段审计：</strong>{{ compactText(stage.stageAuditComment, 20) }}</p>
              </div>
            </Transition>
          </article>
        </div>
        <div v-else class="empty-state brand-empty-state">
          <strong>当前没有历史阶段</strong>
          <p>项目仍停留在当前阶段，暂无可回溯的历史闭环。</p>
        </div>
      </section>

      <div v-if="!hasProject" class="empty-state brand-empty-state">
        <strong>当前角色没有可展示的项目流程</strong>
        <p>请先选择可见项目后再查看阶段循环详情。</p>
      </div>
    </div>

    <section v-else class="panel reveal-card process-snapshot-panel">
      <div class="section-heading">
        <div>
          <div class="eyebrow">当前快照</div>
          <h3>门禁总览板</h3>
        </div>
        <p class="process-map-summary">先看门禁，再看责任。</p>
      </div>

      <div class="process-gate-band">
        <article
          v-for="gate in snapshotGateRows"
          :key="gate.key"
          class="process-gate-segment"
          :class="flowNodeStateClass(gate.status)"
        >
          <div class="process-gate-segment-head">
            <div>
              <div class="eyebrow">{{ gate.label }}</div>
              <strong>{{ gate.title }}</strong>
            </div>
            <StatusTag :value="statusTagValue(gate.status)" />
          </div>
          <p>{{ compactText(gate.summary, 24) }}</p>
          <div class="process-gate-meta">
            <span class="process-requirement-chip">责任方：{{ gate.owner }}</span>
            <span class="process-requirement-chip">{{ compactText(gate.detail, 18) }}</span>
          </div>
        </article>
      </div>

      <div class="process-snapshot-matrix">
        <div class="section-heading process-snapshot-matrix-heading">
          <div>
            <div class="eyebrow">责任状态矩阵</div>
            <h3>三角色当前动作</h3>
          </div>
          <p class="process-role-summary">只保留当前动作与阻塞。</p>
        </div>

        <div class="process-snapshot-role-list">
          <article
            v-for="row in snapshotRoleRows"
            :key="row.role"
            class="process-snapshot-role-row"
            :class="flowNodeStateClass(row.status)"
          >
            <div class="process-snapshot-role-head">
              <div>
                <strong>{{ row.title }}</strong>
                <span>阶段 {{ row.stageNo }}</span>
              </div>
              <StatusTag :value="statusTagValue(row.status)" />
            </div>
            <p>{{ compactText(row.summary, 24) }}</p>
            <div class="process-snapshot-role-meta">
              <span v-if="row.nextAction" class="pill subtle">{{ compactText(row.nextAction, 18) }}</span>
              <span v-if="row.blockedBy" class="pill subtle">阻塞：{{ compactText(row.blockedBy, 16) }}</span>
            </div>
          </article>
        </div>
      </div>
    </section>

    <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
  </AppShell>
</template>
