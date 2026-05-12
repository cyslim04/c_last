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
const fileHash = ref("");
const message = ref("");
const errorMessage = ref("");
const form = reactive({
  projectId: "",
  version: "v1.0.0",
  fileName: "",
  summary: "",
  fileHash: "",
});

const canWriteEvidence = computed(() => wallet.connected && wallet.supportedNetwork && wallet.address);
const selectedProject = computed(() => projects.value.find((item) => String(item.id) === String(form.projectId)) || null);
const heroStats = computed(() => [
  {
    label: "交付记录",
    value: items.value.length,
    helper: "当前角色可见的交付历史总数",
  },
  {
    label: "当前项目",
    value: selectedProject.value?.name || "未选择",
    helper: "新交付会进入该项目当前活跃阶段",
  },
  {
    label: "链上状态",
    value: canWriteEvidence.value ? "可推进" : "待连接",
    helper: canWriteEvidence.value ? `当前网络 ${wallet.chainName}` : "连接钱包后可推进交付存证",
  },
]);

function getProjectLabel(project) {
  return project.name;
}

async function sha256(file) {
  const buffer = await file.arrayBuffer();
  const digest = await crypto.subtle.digest("SHA-256", buffer);
  return Array.from(new Uint8Array(digest)).map((value) => value.toString(16).padStart(2, "0")).join("");
}

async function onFileChange(event) {
  const [file] = event.target.files;
  if (!file) {
    return;
  }
  form.fileName = file.name;
  form.fileHash = await sha256(file);
  fileHash.value = form.fileHash;
}

async function loadData() {
  const [deliverablePayload, projectPayload] = await Promise.all([api.getDeliverables(), api.getProjects()]);
  items.value = deliverablePayload.items;
  projects.value = projectPayload.items;
  if (!form.projectId && projects.value[0]) {
    form.projectId = projects.value[0].id;
  }
}

async function saveDeliverable() {
  message.value = "";
  errorMessage.value = "";
  try {
    const payload = await api.createDeliverable({
      ...form,
      projectId: Number(form.projectId),
    });
    message.value = `交付物已登记到阶段 ${payload.item.stageNo}，记录 ID：${payload.item.id}`;
    form.summary = "";
    form.fileName = "";
    form.fileHash = "";
    fileHash.value = "";
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
  try {
    await api.saveEvidence({
      businessType: "deliverable",
      businessId: item.id,
      txHash: `0xdl${Date.now().toString(16)}${item.id.toString(16)}`,
      blockNumber: Date.now(),
      status: "confirmed",
      comment: `${item.fileName} 已完成演示链上存证`,
    });
    message.value = `交付物 ${item.id} 已完成演示存证。`;
    await loadData();
  } catch (error) {
    errorMessage.value = error.message;
  }
}

watchEffect(() => {
  experience.setPageContext({
    projectName: selectedProject.value?.name || "交付工作台",
    stageLabel: fileHash.value ? "文件哈希已生成" : "准备交付材料",
    cue: "把每次交付变成可确认、可审计、可上链说明的阶段性成果。",
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
      title="把版本、文件与阶段说明组织成可确认的交付材料。"
      description="交付页负责把文件版本、摘要、哈希和后续存证动作放在一起，避免交付说明与确认链路断开。"
      tone="developer"
      variant="minimal"
      :stats="heroStats"
    >
      <template #actions>
        <button class="button" type="button" @click="saveDeliverable">登记交付物</button>
        <RouterLink class="ghost-button" to="/worklogs">工时记录</RouterLink>
      </template>

      <article class="hero-side-card">
        <div class="hero-kicker">当前文件指纹</div>
        <strong>{{ form.fileName || "尚未选择交付文件" }}</strong>
        <p>{{ fileHash ? `SHA-256：${fileHash}` : "选择文件后会自动生成 SHA-256 哈希，用于后续确认、审计与演示存证。" }}</p>
      </article>
    </PageHero>

    <div class="showcase-grid showcase-grid-secondary">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">登记交付</div>
            <h3>新增交付</h3>
          </div>
          <span class="pill subtle">
            {{ canWriteEvidence ? `网络可用：${wallet.chainName}` : "连接钱包后可推进存证" }}
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
            <label>版本号</label>
            <input v-model="form.version" placeholder="例如：v1.0.0-beta" />
          </div>
          <div class="field">
            <label>选择文件</label>
            <input type="file" @change="onFileChange" />
          </div>
          <div class="field">
            <label>交付说明</label>
            <textarea v-model="form.summary" placeholder="说明本次交付范围、修复内容与确认重点" />
          </div>
        </div>

        <div class="state-explain-card">
          <span>提交后会发生什么</span>
          <strong>提交后进入当前阶段的客户确认链路。</strong>
        </div>

        <p class="feedback mono">{{ fileHash ? `文件哈希：${fileHash}` : "等待生成 SHA-256 哈希。" }}</p>
        <p v-if="message" class="feedback success-text">{{ message }}</p>
        <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
      </section>

      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">交付历史</div>
            <h3>交付记录</h3>
          </div>
          <span class="pill subtle">{{ items.length }} 条已登记交付</span>
        </div>

        <div class="brand-project-grid">
          <article v-for="item in items" :key="item.id" class="brand-project-card">
            <div class="project-card-top">
              <div>
                <strong>{{ item.projectName }} / {{ item.version }}</strong>
                <div class="meta-line">阶段：{{ item.stageNo }} ｜ 提交人：{{ item.userName }}</div>
              </div>
              <StatusTag :value="item.status" />
            </div>
            <p>{{ item.summary }}</p>
            <div class="meta-line">文件：{{ item.fileName }}</div>
            <div class="mono">{{ item.fileHash }}</div>
            <div v-if="item.confirmationComment" class="meta-line">确认意见：{{ item.confirmationComment }}</div>
            <div class="button-row">
              <button
                v-if="item.status === 'uploaded'"
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
    </div>
  </AppShell>
</template>
