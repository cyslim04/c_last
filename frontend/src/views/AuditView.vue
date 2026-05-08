<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref, Teleport, watchEffect } from "vue";
import AppShell from "../components/AppShell.vue";
import PageHero from "../components/PageHero.vue";
import SignalRing from "../components/SignalRing.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useExperienceStore } from "../stores/experience";

const experience = useExperienceStore();

const audit = ref({
  transactions: [],
  logs: [],
  stageQueue: [],
  finalQueue: [],
  projects: [],
});
const showTransactions = ref(false);
const showLogs = ref(false);
const errorMessage = ref("");
const actionMessage = ref("");
const selectedStageKey = ref("");
const selectedFinalProjectId = ref(null);
const stageAuditOpen = ref(false);
const finalAuditOpen = ref(false);
const submitting = ref(false);
const stageAuditForm = reactive({
  comment: "",
});
const finalAuditForm = reactive({
  comment: "",
});

const selectedStage = computed(
  () => audit.value.stageQueue.find((item) => `${item.projectId}-${item.stageNo}` === selectedStageKey.value) || null,
);
const selectedFinalProject = computed(
  () => audit.value.finalQueue.find((item) => item.projectId === selectedFinalProjectId.value) || null,
);

const logSummary = computed(() => {
  const total = audit.value.logs.length;
  const latest = audit.value.logs[0];
  if (!latest) {
    return "暂无后台日志";
  }
  return `共 ${total} 条 / 最新 ${latest.action}`;
});

const transactionSummary = computed(() => {
  const total = audit.value.transactions.length;
  const latest = audit.value.transactions[0];
  if (!latest) {
    return "暂无链上留痕";
  }
  return `共 ${total} 条 / 最新 ${latest.type} / ${latest.status}`;
});

const heroStats = computed(() => [
  {
    label: "待阶段审计",
    value: audit.value.stageQueue.length,
    helper: "等待管理员完成本阶段审计的项目数",
  },
  {
    label: "待总审计",
    value: audit.value.finalQueue.length,
    helper: "已满足总审计条件、等待最终决策的项目数",
  },
  {
    label: "历史留痕",
    value: audit.value.logs.length,
    helper: "后台日志与链上回写用于支撑审计解释层",
  },
]);

const auditReadiness = computed(() => {
  const openWork = audit.value.stageQueue.length + audit.value.finalQueue.length;
  return Math.max(18, Math.min(100, 100 - openWork * 14));
});

function selectStage(item) {
  selectedStageKey.value = `${item.projectId}-${item.stageNo}`;
}

function selectFinalProject(item) {
  selectedFinalProjectId.value = item.projectId;
}

function closeStageAudit() {
  stageAuditOpen.value = false;
  stageAuditForm.comment = "";
}

function closeFinalAudit() {
  finalAuditOpen.value = false;
  finalAuditForm.comment = "";
}

function openStageAudit() {
  if (!selectedStage.value) {
    errorMessage.value = "请选择待阶段审计记录。";
    return;
  }
  stageAuditForm.comment = "";
  stageAuditOpen.value = true;
}

function openFinalAudit() {
  if (!selectedFinalProject.value) {
    errorMessage.value = "请选择待最终审计项目。";
    return;
  }
  finalAuditForm.comment = "";
  finalAuditOpen.value = true;
}

async function loadAudit() {
  try {
    errorMessage.value = "";
    audit.value = await api.getAudit();
    if (selectedStageKey.value && !audit.value.stageQueue.some((item) => `${item.projectId}-${item.stageNo}` === selectedStageKey.value)) {
      selectedStageKey.value = "";
    }
    if (selectedFinalProjectId.value && !audit.value.finalQueue.some((item) => item.projectId === selectedFinalProjectId.value)) {
      selectedFinalProjectId.value = null;
    }
  } catch (error) {
    errorMessage.value = error.message;
  }
}

async function submitStageAudit(decision) {
  if (!selectedStage.value) {
    return;
  }
  submitting.value = true;
  errorMessage.value = "";
  try {
    const payload = await api.submitStageAudit({
      projectId: selectedStage.value.projectId,
      stageNo: selectedStage.value.stageNo,
      decision,
      comment: stageAuditForm.comment.trim(),
    });
    actionMessage.value = payload.message;
    closeStageAudit();
    await loadAudit();
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    submitting.value = false;
  }
}

async function submitFinalAudit(decision) {
  if (!selectedFinalProject.value) {
    return;
  }
  submitting.value = true;
  errorMessage.value = "";
  try {
    const payload = await api.submitFinalAudit({
      projectId: selectedFinalProject.value.projectId,
      decision,
      comment: finalAuditForm.comment.trim(),
    });
    actionMessage.value = payload.message;
    closeFinalAudit();
    await loadAudit();
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    submitting.value = false;
  }
}

watchEffect(() => {
  experience.setPageContext({
    projectName: selectedFinalProject.value?.projectName || selectedStage.value?.projectName || "审计剧场",
    stageLabel: audit.value.finalQueue.length ? `${audit.value.finalQueue.length} 个总审计待决` : "阶段与总审计态势",
    cue: "让审计动作拥有明确的证据依据、状态影响和链上留痕，而不是普通按钮提交。",
    tone: "admin",
  });
});

onMounted(loadAudit);
onBeforeUnmount(() => experience.resetPageContext());
</script>

<template>
  <AppShell>
    <PageHero
      title="审计管理"
      tone="admin"
      variant="minimal"
    >
      <template #actions>
        <button class="button" type="button" @click="loadAudit">刷新审计态势</button>
      </template>

      <div class="hero-visual-stack">
        <SignalRing
          title="审计准备度"
          :value="auditReadiness"
          :max="100"
          subtitle="审计准备度会随着待审动作减少而提升，最终项目会收敛到完整闭环。"
          tone="admin"
        />

        <article class="hero-side-card">
          <div class="hero-kicker">当前主动作</div>
          <strong>{{ selectedStage?.projectName || selectedFinalProject?.projectName || "请选择一条待审记录" }}</strong>
          <p>
            {{
              selectedStage
                ? `当前聚焦阶段 ${selectedStage.stageNo}，可以对客户已确认的证据发起阶段审计。`
                : selectedFinalProject
                  ? "当前聚焦总审计项目，可以对已完成阶段闭环的项目给出最终结论。"
                  : "在下方先选择阶段审计或最终总审计对象，再打开决策面板。"
            }}
          </p>
        </article>
      </div>
    </PageHero>

    <div class="showcase-grid showcase-grid-audit">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">阶段审计</div>
            <h3>阶段审计</h3>
          </div>
          <button class="button" type="button" @click="openStageAudit" :disabled="submitting || !selectedStage">
            阶段审计
          </button>
        </div>

        <div v-if="audit.stageQueue.length" class="decision-list">
          <button
            v-for="item in audit.stageQueue"
            :key="`${item.projectId}-${item.stageNo}`"
            class="decision-card"
            :class="{ active: `${item.projectId}-${item.stageNo}` === selectedStageKey }"
            type="button"
            @click="selectStage(item)"
          >
            <div class="decision-copy">
              <strong>{{ item.projectName }} / 阶段 {{ item.stageNo }}</strong>
              <div class="meta-line">工时：{{ item.worklogCount }} ｜ 交付物：{{ item.deliverableCount }}</div>
              <p>{{ item.latestDeliverableVersion }} ｜ {{ item.latestDeliverableName }}</p>
              <div class="meta-line">{{ item.approvalComment }}</div>
            </div>
            <StatusTag value="running" />
          </button>
        </div>
        <div v-else class="empty-state brand-empty-state">
          <strong>当前没有待阶段审计项目</strong>
          <p>暂无待审记录。</p>
        </div>
      </section>

      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">最终审计</div>
            <h3>总审计</h3>
          </div>
          <button class="button" type="button" @click="openFinalAudit" :disabled="submitting || !selectedFinalProject">
            最终总审计
          </button>
        </div>

        <div v-if="audit.finalQueue.length" class="decision-list">
          <button
            v-for="item in audit.finalQueue"
            :key="item.projectId"
            class="decision-card"
            :class="{ active: item.projectId === selectedFinalProjectId }"
            type="button"
            @click="selectFinalProject(item)"
          >
            <div class="decision-copy">
              <strong>{{ item.projectName }}</strong>
              <div class="meta-line">已通过阶段：{{ item.passedStageCount }} ｜ 最后阶段：{{ item.lastStageNo }}</div>
              <p>{{ item.summary }}</p>
            </div>
            <StatusTag value="pending" />
          </button>
        </div>
        <div v-else class="empty-state brand-empty-state">
          <strong>当前没有待最终审计项目</strong>
          <p>暂无待总审计项目。</p>
        </div>
      </section>
    </div>

    <section class="panel reveal-card">
      <div class="section-heading">
        <div>
          <div class="eyebrow">审计总览</div>
          <h3>项目</h3>
        </div>
      </div>

      <div class="brand-project-grid">
        <article v-for="project in audit.projects" :key="project.id" class="brand-project-card">
          <div class="project-card-top">
            <div>
              <strong>{{ project.name }}</strong>
              <div class="meta-line">阶段数：{{ project.stageCount }} ｜ 已通过阶段：{{ project.passedStageCount }}</div>
            </div>
            <StatusTag :value="project.status" />
          </div>
          <p>{{ project.finalAuditHint }}</p>
          <div class="metric-strip">
            <span>待客户确认 {{ project.pendingClientStageCount }}</span>
            <span>待阶段审计 {{ project.pendingStageAuditCount }}</span>
            <span>当前阶段 {{ project.currentStageNo || "-" }}</span>
          </div>
        </article>
      </div>
    </section>

    <div class="showcase-grid showcase-grid-audit-bottom">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">链上留痕</div>
            <h3>链上记录</h3>
          </div>
          <button class="secondary-button" type="button" @click="showTransactions = !showTransactions">
            {{ showTransactions ? "收起留痕" : "展开留痕" }}
          </button>
        </div>

        <div class="log-summary-card">{{ transactionSummary }}</div>

        <div v-if="showTransactions && audit.transactions.length" class="transaction-stream">
          <article v-for="tx in audit.transactions" :key="tx.id" class="transaction-card">
            <div class="transaction-beam" />
            <div class="transaction-copy">
              <strong>{{ tx.type }}</strong>
              <div class="mono">{{ tx.txHash }}</div>
              <p>{{ tx.comment }}</p>
              <div class="meta-line">{{ tx.createdAt }}</div>
            </div>
            <StatusTag :value="tx.status" />
          </article>
        </div>
        <div v-else-if="showTransactions && !audit.transactions.length" class="empty-state brand-empty-state">
          <strong>当前没有链上留痕</strong>
          <p>暂无记录。</p>
        </div>
        <div v-else-if="!audit.transactions.length" class="empty-state brand-empty-state">
          <strong>当前没有链上留痕</strong>
          <p>暂无记录。</p>
        </div>
      </section>

      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">后台留痕</div>
            <h3>日志</h3>
          </div>
          <button class="secondary-button" type="button" @click="showLogs = !showLogs">
            {{ showLogs ? "收起日志" : "展开日志" }}
          </button>
        </div>

        <div class="log-summary-card">{{ logSummary }}</div>

        <div v-if="showLogs" class="log-stage-list">
          <article v-for="log in audit.logs" :key="log.id" class="log-stage-item">
            <strong>{{ log.actorName }} / {{ log.action }}</strong>
            <p>{{ log.detail }}</p>
            <div class="meta-line">{{ log.entityType }} ｜ {{ log.createdAt }}</div>
          </article>
        </div>
      </section>
    </div>

    <p v-if="actionMessage" class="feedback success-text">{{ actionMessage }}</p>
    <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>

    <Teleport to="body">
      <template v-if="stageAuditOpen && selectedStage">
        <div class="modal-backdrop" @click="closeStageAudit" />
        <div class="modal-panel audit-modal">
          <div class="section-heading">
            <div>
              <div class="eyebrow">阶段审计决策面板</div>
              <h3>{{ selectedStage.projectName }} / 阶段 {{ selectedStage.stageNo }}</h3>
            </div>
            <button class="ghost-button" type="button" @click="closeStageAudit">取消</button>
          </div>

          <div class="audit-modal-grid">
            <div class="audit-modal-item">
              <span>阶段工时</span>
              <strong>{{ selectedStage.worklogCount }}</strong>
            </div>
            <div class="audit-modal-item">
              <span>阶段交付</span>
              <strong>{{ selectedStage.deliverableCount }}</strong>
            </div>
            <div class="audit-modal-item">
              <span>客户意见</span>
              <strong>{{ selectedStage.approvalComment || "已确认" }}</strong>
            </div>
          </div>

          <div class="field">
            <label>审计意见</label>
            <textarea v-model="stageAuditForm.comment" placeholder="填写阶段审计通过说明或驳回原因" />
          </div>

          <div class="button-row">
            <button class="button" type="button" @click="submitStageAudit('pass')" :disabled="submitting">
              {{ submitting ? "提交中..." : "通过" }}
            </button>
            <button class="secondary-button danger-button" type="button" @click="submitStageAudit('reject')" :disabled="submitting">
              驳回
            </button>
          </div>
        </div>
      </template>
    </Teleport>

    <Teleport to="body">
      <template v-if="finalAuditOpen && selectedFinalProject">
        <div class="modal-backdrop" @click="closeFinalAudit" />
        <div class="modal-panel audit-modal">
          <div class="section-heading">
            <div>
              <div class="eyebrow">最终总审计决策面板</div>
              <h3>{{ selectedFinalProject.projectName }}</h3>
            </div>
            <button class="ghost-button" type="button" @click="closeFinalAudit">取消</button>
          </div>

          <div class="log-summary-card">{{ selectedFinalProject.summary }}</div>

          <div class="field">
            <label>审计意见</label>
            <textarea v-model="finalAuditForm.comment" placeholder="填写项目总审计结论" />
          </div>

          <div class="button-row">
            <button class="button" type="button" @click="submitFinalAudit('pass')" :disabled="submitting">
              {{ submitting ? "提交中..." : "通过" }}
            </button>
            <button class="secondary-button danger-button" type="button" @click="submitFinalAudit('reject')" :disabled="submitting">
              驳回
            </button>
          </div>
        </div>
      </template>
    </Teleport>
  </AppShell>
</template>
