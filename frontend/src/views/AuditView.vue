<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref, Teleport } from "vue";
import AppShell from "../components/AppShell.vue";
import PageHero from "../components/PageHero.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useAuthStore } from "../stores/auth";
import { confirmEvidenceTransaction, getPendingEvidence, retryEvidenceVerification, submitFinalAudit as sendFinalAudit, submitMigrationBatch, submitMigrationCheckpoint, submitStageAudit as sendStageAudit } from "../web3/evidence";

const auth = useAuthStore();

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
const migrationProjects = ref([]);
const selectedMigrationProjectId = ref("");
const migrationPlan = ref(null);
const migrationBusy = ref(false);
const migrationMessage = ref("");
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

async function loadMigrationProjects() {
  const payload = await api.getProjects();
  migrationProjects.value = payload.items.filter((project) => project.chainStatus !== "confirmed");
  if (!selectedMigrationProjectId.value && migrationProjects.value[0]) {
    selectedMigrationProjectId.value = String(migrationProjects.value[0].id);
  }
  if (selectedMigrationProjectId.value) {
    await loadMigration(Number(selectedMigrationProjectId.value));
  }
}

async function loadMigration(projectId = Number(selectedMigrationProjectId.value)) {
  if (!projectId) {
    migrationPlan.value = null;
    return;
  }
  migrationPlan.value = await api.getMigration(projectId);
}

function formatEpoch(value) {
  return value ? new Date(Number(value) * 1000).toLocaleString() : "无时间记录";
}

async function previewMigration() {
  migrationMessage.value = "";
  try {
    migrationPlan.value = await api.previewMigration(Number(selectedMigrationProjectId.value));
  } catch (error) {
    errorMessage.value = error.message;
  }
}

async function processMigration() {
  const projectId = Number(selectedMigrationProjectId.value);
  if (!projectId || migrationBusy.value) return;
  migrationBusy.value = true;
  errorMessage.value = "";
  migrationMessage.value = "迁移交易按检查点和历史摘要批次逐笔核验。";
  try {
    let plan = await api.startMigration(projectId);
    migrationPlan.value = plan;
    if (!plan.checkpoint.txHash) {
      const pending = getPendingEvidence("migration_checkpoint", projectId);
      if (plan.checkpoint.candidateTxHash || pending) {
        await retryEvidenceVerification(pending || {
          businessType: "migration_checkpoint",
          businessId: projectId,
          chainId: Number(plan.checkpoint.chainId),
          txHash: plan.checkpoint.candidateTxHash,
        }, auth.user?.walletAddress);
      } else {
        const transaction = await submitMigrationCheckpoint(projectId, plan.checkpoint, auth.user?.walletAddress);
        await confirmEvidenceTransaction("migration_checkpoint", projectId, transaction);
      }
    }
    plan = await api.getMigration(projectId);
    migrationPlan.value = plan;
    for (const batch of plan.batches) {
      if (batch.txHash) continue;
      const pending = getPendingEvidence("migration_batch", batch.id);
      if (batch.candidateTxHash || pending) {
        await retryEvidenceVerification(pending || {
          businessType: "migration_batch",
          businessId: batch.id,
          chainId: Number(batch.chainId),
          txHash: batch.candidateTxHash,
        }, auth.user?.walletAddress);
      } else {
        const transaction = await submitMigrationBatch(projectId, batch, auth.user?.walletAddress);
        await confirmEvidenceTransaction("migration_batch", batch.id, transaction);
      }
      plan = await api.getMigration(projectId);
      migrationPlan.value = plan;
      migrationMessage.value = `已核验历史批次 ${batch.batchNo + 1}/${plan.batches.length}：${batch.category}`;
    }
    plan = await api.getMigration(projectId);
    migrationPlan.value = plan;
    migrationMessage.value = plan.status === "completed" ? "历史摘要迁移全部完成，项目现在可继续由链上流程推进。" : "迁移检查点和批次已处理，可刷新查看最新状态。";
    await Promise.all([loadAudit(), loadMigrationProjects()]);
  } catch (error) {
    errorMessage.value = error.message;
    await loadMigration(projectId).catch(() => {});
  } finally {
    migrationBusy.value = false;
  }
}

async function submitStageAudit(decision) {
  if (!selectedStage.value) {
    return;
  }
  submitting.value = true;
  errorMessage.value = "";
  try {
    const request = await api.submitStageAudit({
      projectId: selectedStage.value.projectId,
      stageNo: selectedStage.value.stageNo,
      decision,
      comment: stageAuditForm.comment.trim(),
    });
    const pending = getPendingEvidence("stage_audit", request.businessId);
    if (request.candidateTxHash || pending) {
      await retryEvidenceVerification(pending || {
        businessType: "stage_audit",
        businessId: request.businessId,
        chainId: Number(request.chainId),
        txHash: request.candidateTxHash,
        comment: stageAuditForm.comment.trim(),
      }, auth.user?.walletAddress);
    } else {
      const transaction = await sendStageAudit(
        request.projectId,
        request.stageNo,
        request.businessId,
        decision === "pass",
        request.digest,
        auth.user?.walletAddress,
      );
      await confirmEvidenceTransaction("stage_audit", request.businessId, transaction, stageAuditForm.comment.trim());
    }
    actionMessage.value = `阶段审计交易已核验：${request.businessId}`;
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
    const request = await api.submitFinalAudit({
      projectId: selectedFinalProject.value.projectId,
      decision,
      comment: finalAuditForm.comment.trim(),
    });
    const pending = getPendingEvidence("final_audit", request.businessId);
    if (request.candidateTxHash || pending) {
      await retryEvidenceVerification(pending || {
        businessType: "final_audit",
        businessId: request.businessId,
        chainId: Number(request.chainId),
        txHash: request.candidateTxHash,
        comment: finalAuditForm.comment.trim(),
      }, auth.user?.walletAddress);
    } else {
      const transaction = await sendFinalAudit(
        request.projectId,
        request.businessId,
        decision === "pass",
        request.digest,
        auth.user?.walletAddress,
      );
      await confirmEvidenceTransaction("final_audit", request.businessId, transaction, finalAuditForm.comment.trim());
    }
    actionMessage.value = `最终审计交易已核验：${request.businessId}`;
    closeFinalAudit();
    await loadAudit();
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    submitting.value = false;
  }
}

onMounted(() => {
  Promise.all([loadAudit(), loadMigrationProjects()]).catch((error) => {
    errorMessage.value = error.message;
  });
});

</script>

<template>
  <AppShell>
    <PageHero
      eyebrow="管理员工作区"
      title="审计阶段结果，给出最终结论。"
      description="集中处理待阶段审计、待总审计项目，以及对应留痕。"
      tone="admin"
      variant="minimal"
      :stats="heroStats"
    >
      <template #actions>
        <button class="button" type="button" @click="loadAudit">刷新审计态势</button>
      </template>
    </PageHero>

    <section class="panel reveal-card stride-section migration-panel">
      <div class="section-heading">
        <div>
          <div class="eyebrow">历史摘要迁移</div>
          <h3>把旧记录摘要锚定到真实合约</h3>
        </div>
        <span class="pill subtle">管理员钱包签名</span>
      </div>
      <p class="meta-line">迁移记录会分别保存原始创建时间和实际上链时间。迁移只证明摘要在当前区块被锚定，不表示原业务发生时已经上链。</p>
      <div class="migration-controls">
        <div class="field">
          <label>选择项目</label>
          <select v-model="selectedMigrationProjectId" @change="loadMigration(Number(selectedMigrationProjectId))">
            <option value="" disabled>请选择需要迁移的项目</option>
            <option v-for="project in migrationProjects" :key="project.id" :value="String(project.id)">
              {{ project.name }} / {{ project.chainStatus === "legacy" ? "待迁移" : project.chainStatus }}
            </option>
          </select>
        </div>
        <div class="button-row">
          <button class="secondary-button" type="button" :disabled="!selectedMigrationProjectId || migrationBusy" @click="previewMigration">预览确定性摘要</button>
          <button class="button" type="button" :disabled="!selectedMigrationProjectId || migrationBusy || migrationPlan?.status === 'completed'" @click="processMigration">
            {{ migrationBusy ? "等待钱包确认与核验..." : migrationPlan?.status === "pending" || migrationPlan?.status === "confirmed" ? "启动 / 续跑迁移" : "开始迁移" }}
          </button>
        </div>
      </div>
      <template v-if="migrationPlan">
        <div class="migration-checkpoint">
          <strong>{{ migrationPlan.projectName }} / 检查点</strong>
          <div class="meta-line">状态：{{ migrationPlan.status }} ｜ 原项目创建：{{ migrationPlan.checkpoint.sourceCreatedAtText }} ｜ 恢复阶段：{{ migrationPlan.checkpoint.currentStage }} ｜ 已通过阶段：{{ migrationPlan.checkpoint.passedStageCount }}</div>
          <div class="mono">确定性摘要：{{ migrationPlan.checkpoint.digest }}</div>
          <div v-if="migrationPlan.checkpoint.txHash" class="mono">迁移交易：{{ migrationPlan.checkpoint.txHash }} / 区块 {{ migrationPlan.checkpoint.blockNumber }} / 实际上链时间 {{ migrationPlan.checkpoint.anchoredAt }}</div>
          <div v-else-if="migrationPlan.checkpoint.candidateTxHash" class="meta-line">待核验的同一笔交易：{{ migrationPlan.checkpoint.candidateTxHash }}</div>
        </div>
        <div class="migration-batch-list">
          <article v-for="batch in migrationPlan.batches" :key="batch.batchNo" class="migration-batch-card">
            <div class="project-card-top">
              <strong>{{ batch.category }} / {{ batch.recordCount }} 条源记录</strong>
              <StatusTag :value="batch.status === 'confirmed' ? 'migrated' : 'pending'" />
            </div>
            <div class="meta-line">源时间：{{ formatEpoch(batch.sourceFrom) }} — {{ formatEpoch(batch.sourceTo) }}</div>
            <div class="mono">摘要：{{ batch.digest }}</div>
            <div v-if="batch.txHash" class="mono">实际上链交易：{{ batch.txHash }} / 区块 {{ batch.blockNumber }} / {{ batch.anchoredAt }}</div>
            <div v-else-if="batch.candidateTxHash" class="meta-line">待核验交易：{{ batch.candidateTxHash }}</div>
          </article>
        </div>
      </template>
      <p v-if="migrationMessage" class="feedback success-text">{{ migrationMessage }}</p>
    </section>

    <section class="showcase-grid showcase-grid-audit">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">阶段决策</div>
            <h3>待阶段审计</h3>
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
            <div class="eyebrow">最终决策</div>
            <h3>待总审计项目</h3>
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
    </section>

    <section class="panel reveal-card stride-section">
      <div class="section-heading">
        <div>
          <div class="eyebrow">审计总览</div>
          <h3>项目审计态势</h3>
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

    <section class="showcase-grid showcase-grid-audit-bottom">
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
              <div class="meta-line">网络 {{ tx.chainId }} ｜ 区块 {{ tx.blockNumber }} ｜ 实际上链 {{ tx.anchoredAt || tx.createdAt }}</div>
              <div v-if="tx.recordOrigin === 'migration'" class="meta-line">迁移存证。源创建时间：{{ tx.sourceCreatedAt || '无时间记录' }}；该交易证明摘要在迁移时上链。</div>
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
            <h3>后台日志</h3>
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
    </section>

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

