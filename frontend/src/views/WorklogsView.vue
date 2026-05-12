<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref, watchEffect } from "vue";
import { RouterLink } from "vue-router";
import AppShell from "../components/AppShell.vue";
import MemberPicker from "../components/MemberPicker.vue";
import PageHero from "../components/PageHero.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useExperienceStore } from "../stores/experience";
import { useWalletStore } from "../stores/wallet";

const experience = useExperienceStore();
const wallet = useWalletStore();

const items = ref([]);
const projects = ref([]);
const resultMessage = ref("");
const errorMessage = ref("");
const form = reactive({
  projectId: "",
  workDate: new Date().toISOString().slice(0, 10),
  hours: 4,
  taskDescription: "",
});

const canWriteEvidence = computed(() => wallet.connected && wallet.supportedNetwork && wallet.address);
const selectedProject = computed(() => projects.value.find((item) => String(item.id) === String(form.projectId)) || null);
const heroStats = computed(() => [
  {
    label: "工时记录",
    value: items.value.length,
    helper: "当前角色可见的工时历史总数。",
  },
  {
    label: "当前项目",
    value: selectedProject.value?.name || "未选择",
    helper: "新的工时摘要会进入这个项目当前阶段。",
  },
  {
    label: "钱包状态",
    value: canWriteEvidence.value ? "可存证" : "待连接",
    helper: canWriteEvidence.value ? wallet.chainName : "连接钱包后可发起演示存证。",
  },
]);

function getProjectLabel(project) {
  return project.name;
}

async function loadData() {
  const [worklogPayload, projectPayload] = await Promise.all([api.getWorklogs(), api.getProjects()]);
  items.value = worklogPayload.items;
  projects.value = projectPayload.items;
  if (!form.projectId && projects.value[0]) {
    form.projectId = projects.value[0].id;
  }
}

async function createWorklog() {
  resultMessage.value = "";
  errorMessage.value = "";
  try {
    const payload = await api.createWorklog({
      ...form,
      projectId: Number(form.projectId),
      hours: Number(form.hours),
    });
    resultMessage.value = `工时已生成，阶段 ${payload.item.stageNo}，摘要：${payload.item.digest}`;
    form.taskDescription = "";
    await loadData();
  } catch (error) {
    errorMessage.value = error.message;
  }
}

async function writeEvidence(item) {
  if (!canWriteEvidence.value) {
    errorMessage.value = "请先连接 MetaMask 并切换到受支持网络。";
    return;
  }
  errorMessage.value = "";
  resultMessage.value = "";
  try {
    await api.saveEvidence({
      businessType: "worklog",
      businessId: item.id,
      txHash: `0xwl${Date.now().toString(16)}${item.id.toString(16)}`,
      blockNumber: Date.now(),
      status: "confirmed",
      comment: `${item.projectName} 的工时摘要已完成演示存证`,
    });
    resultMessage.value = `工时 ${item.id} 已写入演示存证。`;
    await loadData();
  } catch (error) {
    errorMessage.value = error.message;
  }
}

watchEffect(() => {
  experience.setPageContext({
    projectName: selectedProject.value?.name || "开发工作台",
    stageLabel: canWriteEvidence.value ? "钱包已就绪" : "等待链上连接",
    cue: "工时页既是操作台，也是解释开发过程为什么可信的展示页。",
    tone: "developer",
  });
});

onMounted(() => {
  loadData().catch((error) => {
    errorMessage.value = error.message;
  });
});

onBeforeUnmount(() => experience.resetPageContext());
</script>

<template>
  <AppShell>
    <PageHero
      eyebrow="开发工作区"
      title="把开发过程沉淀成可核验的工时摘要。"
      description="每次开发、联调或修复都不是一次匿名提交，而是项目阶段中的一条可信说明，可以继续进入交付、确认和审计链路。"
      tone="developer"
      :stats="heroStats"
    >
      <template #actions>
        <button class="button" type="button" @click="createWorklog">生成工时摘要</button>
        <RouterLink class="ghost-button" to="/deliverables">查看交付物</RouterLink>
      </template>

      <article class="hero-side-card">
        <div class="hero-kicker">本次录入</div>
        <strong>{{ form.taskDescription || "描述本次开发、修复或联调内容" }}</strong>
        <p>
          {{
            selectedProject
              ? `当前会记录到 ${selectedProject.name}，日期 ${form.workDate}，工时 ${form.hours}h。`
              : "先选择项目，再补充工时日期、时长与任务说明。"
          }}
        </p>
      </article>
    </PageHero>

    <section class="showcase-grid showcase-grid-secondary">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">录入工时</div>
            <h3>新增工时记录</h3>
          </div>
          <span class="pill subtle">
            {{ canWriteEvidence ? `网络可用：${wallet.chainName}` : "需要连接钱包后才能写入存证" }}
          </span>
        </div>

        <div class="form-grid">
          <div class="field">
            <label>所属项目</label>
            <MemberPicker
              v-model="form.projectId"
              :items="projects"
              placeholder="请选择所属项目"
              :show-meta="false"
              :get-label="getProjectLabel"
            />
          </div>
          <div class="field">
            <label>日期</label>
            <input v-model="form.workDate" type="date" />
          </div>
          <div class="field">
            <label>工时</label>
            <input v-model.number="form.hours" type="number" min="1" max="24" step="0.5" />
          </div>
          <div class="field">
            <label>任务说明</label>
            <textarea v-model="form.taskDescription" placeholder="说明本次开发或联调的实际内容" />
          </div>
        </div>

        <div class="state-explain-card">
          <span>提交后会发生什么</span>
          <strong>提交后生成工时摘要。</strong>
        </div>

        <p v-if="resultMessage" class="feedback success-text mono">{{ resultMessage }}</p>
        <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
      </section>

      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">历史记录</div>
            <h3>工时摘要</h3>
          </div>
        </div>

        <div class="brand-project-grid">
          <article v-for="item in items" :key="item.id" class="brand-project-card">
            <div class="project-card-top">
              <div>
                <strong>{{ item.projectName }} / {{ item.workDate }}</strong>
                <div class="meta-line">阶段：{{ item.stageNo }} ｜ 提交人：{{ item.userName }} ｜ 工时：{{ item.hours }}</div>
              </div>
              <StatusTag :value="item.evidenceStatus" />
            </div>
            <p>{{ item.taskDescription }}</p>
            <div class="mono">{{ item.digest }}</div>
            <div class="button-row">
              <button
                v-if="item.evidenceStatus !== 'confirmed'"
                class="secondary-button"
                type="button"
                @click="writeEvidence(item)"
              >
                写入演示存证
              </button>
            </div>
          </article>
        </div>
      </section>
    </section>
  </AppShell>
</template>
