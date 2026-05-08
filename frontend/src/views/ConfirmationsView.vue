<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref, watchEffect } from "vue";
import AppShell from "../components/AppShell.vue";
import PageHero from "../components/PageHero.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useExperienceStore } from "../stores/experience";
import { useWalletStore } from "../stores/wallet";

const experience = useExperienceStore();
const wallet = useWalletStore();

const stages = ref([]);
const message = ref("");
const errorMessage = ref("");
const comments = reactive({});

const canConfirm = computed(() => wallet.connected && wallet.supportedNetwork && wallet.address);
const pendingStages = computed(() => stages.value.filter((item) => ["uploaded", "notarized"].includes(item.latestDeliverable.status)));

const heroStats = computed(() => [
  {
    label: "待确认阶段",
    value: pendingStages.value.length,
    helper: "客户当前需要给出确认结论的阶段数量",
  },
  {
    label: "阶段池",
    value: stages.value.length,
    helper: "当前角色可查看到的阶段记录总数",
  },
  {
    label: "钱包状态",
    value: canConfirm.value ? "可确认" : "待连接",
    helper: canConfirm.value ? wallet.chainName : "连接钱包后可提交阶段确认结果",
  },
]);

function buildStageKey(item) {
  return `${item.projectId}-${item.stageNo}`;
}

async function loadStages() {
  const [deliverablePayload, worklogPayload] = await Promise.all([api.getDeliverables(), api.getWorklogs()]);
  const grouped = new Map();

  worklogPayload.items.forEach((worklog) => {
    const key = `${worklog.projectId}-${worklog.stageNo}`;
    if (!grouped.has(key)) {
      grouped.set(key, {
        projectId: worklog.projectId,
        projectName: worklog.projectName,
        stageNo: worklog.stageNo,
        latestDeliverable: null,
        worklogs: [],
      });
    }
    grouped.get(key).worklogs.push(worklog);
  });

  deliverablePayload.items.forEach((deliverable) => {
    const key = `${deliverable.projectId}-${deliverable.stageNo}`;
    if (!grouped.has(key)) {
      grouped.set(key, {
        projectId: deliverable.projectId,
        projectName: deliverable.projectName,
        stageNo: deliverable.stageNo,
        latestDeliverable: null,
        worklogs: [],
      });
    }
    const stage = grouped.get(key);
    if (!stage.latestDeliverable || deliverable.id > stage.latestDeliverable.id) {
      stage.latestDeliverable = deliverable;
    }
  });

  stages.value = Array.from(grouped.values())
    .filter((item) => item.latestDeliverable)
    .sort((left, right) => {
      if (left.projectId !== right.projectId) {
        return right.projectId - left.projectId;
      }
      return right.stageNo - left.stageNo;
    });

  stages.value.forEach((item) => {
    const key = buildStageKey(item);
    if (!(key in comments)) {
      comments[key] = item.latestDeliverable.status === "notarized" ? `阶段 ${item.stageNo} 客户确认通过，进入管理员阶段审计。` : "";
    }
  });
}

async function confirmStage(stage, approved) {
  if (!canConfirm.value) {
    errorMessage.value = "请先连接 MetaMask 并切换到 Sepolia 或 Hardhat。";
    return;
  }
  try {
    await api.saveEvidence({
      businessType: "deliverable_confirmation",
      businessId: stage.latestDeliverable.id,
      txHash: `0xcf${Date.now().toString(16)}${stage.latestDeliverable.id.toString(16)}`,
      blockNumber: Date.now(),
      status: approved ? "confirmed" : "failed",
      comment: comments[buildStageKey(stage)] || (approved ? "客户确认通过。" : "客户驳回，需要继续修改。"),
    });
    message.value = `阶段 ${stage.stageNo} 的确认结果已记录。`;
    await loadStages();
  } catch (error) {
    errorMessage.value = error.message;
  }
}

watchEffect(() => {
  experience.setPageContext({
    projectName: pendingStages.value[0]?.projectName || "确认看板",
    stageLabel: pendingStages.value.length ? `${pendingStages.value.length} 个阶段待确认` : "当前没有待确认阶段",
    cue: "客户看到的不只是交付物，还能看到阶段内的工时历史、文件哈希和确认后的流向。",
    tone: "client",
  });
});

onMounted(() => {
  loadStages().catch((error) => {
    errorMessage.value = error.message;
  });
});

onBeforeUnmount(() => experience.resetPageContext());
</script>

<template>
  <AppShell>
    <PageHero
      title="客户确认"
      tone="client"
      variant="minimal"
    >
      <template #actions>
        <button class="button" type="button" @click="loadStages">刷新确认状态</button>
      </template>

      <article class="hero-side-card">
        <div class="hero-kicker">确认说明</div>
        <strong>{{ pendingStages.length ? `你当前有 ${pendingStages.length} 个阶段待决定` : "当前没有新的阶段需要确认" }}</strong>
        <p>通过后会进入管理员阶段审计；驳回后会要求开发者继续补充和修正本阶段交付。</p>
      </article>
    </PageHero>

    <section class="panel reveal-card">
      <div class="section-heading">
        <div>
          <div class="eyebrow">阶段确认</div>
          <h3>确认任务</h3>
        </div>
        <span class="pill subtle">
          {{ canConfirm ? `网络可用：${wallet.chainName}` : "连接钱包后可完成确认" }}
        </span>
      </div>

      <div v-if="pendingStages.length" class="confirmation-board">
        <article v-for="stage in pendingStages" :key="buildStageKey(stage)" class="confirmation-scene-card">
          <div class="confirmation-scene-main">
            <div class="section-heading">
              <div>
                <div class="eyebrow">{{ stage.projectName }}</div>
                <h3>阶段 {{ stage.stageNo }}</h3>
              </div>
              <StatusTag :value="stage.latestDeliverable.status" />
            </div>

            <p>{{ stage.latestDeliverable.summary }}</p>
            <div class="metric-strip">
              <span>交付版本 {{ stage.latestDeliverable.version }}</span>
              <span>文件 {{ stage.latestDeliverable.fileName }}</span>
            </div>
            <div class="mono">{{ stage.latestDeliverable.fileHash }}</div>

            <div class="stage-history-panel">
              <div class="hero-kicker">阶段工时历史</div>
              <div v-if="stage.worklogs.length" class="stage-history-list">
                <article v-for="worklog in stage.worklogs" :key="worklog.id" class="stage-history-item">
                  <strong>{{ worklog.workDate }} ｜ {{ worklog.hours }}h</strong>
                  <p>{{ worklog.taskDescription }}</p>
                  <div class="mono">{{ worklog.digest }}</div>
                </article>
              </div>
              <div v-else class="empty-state">当前阶段还没有工时记录。</div>
            </div>
          </div>

          <div class="confirmation-actions confirmation-actions-panel">
            <div class="state-explain-card">
              <span>决策影响</span>
              <strong>通过进入阶段审计；驳回返回开发者。</strong>
            </div>
            <textarea v-model="comments[buildStageKey(stage)]" placeholder="填写本阶段确认意见或驳回原因" />
            <div class="button-row">
              <button class="button" type="button" @click="confirmStage(stage, true)">确认通过</button>
              <button class="secondary-button" type="button" @click="confirmStage(stage, false)">驳回阶段</button>
            </div>
          </div>
        </article>
      </div>
      <div v-else class="empty-state brand-empty-state">
        <strong>当前没有需要客户确认的阶段</strong>
        <p>暂无确认任务。</p>
      </div>

      <p v-if="message" class="feedback success-text">{{ message }}</p>
      <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
    </section>
  </AppShell>
</template>
