<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref } from "vue";
import AppShell from "../components/AppShell.vue";
import MemberPicker from "../components/MemberPicker.vue";
import PageHero from "../components/PageHero.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";
import { useAuthStore } from "../stores/auth";
import { useWalletStore } from "../stores/wallet";
import { confirmEvidenceTransaction, getPendingEvidence, retryEvidenceVerification, submitProjectCreation } from "../web3/evidence";

const auth = useAuthStore();
const wallet = useWalletStore();

const projects = ref([]);
const developers = ref([]);
const clients = ref([]);
const message = ref("");
const errorMessage = ref("");
const chainBusy = ref({});
const form = reactive({
  name: "",
  description: "",
  developerId: "",
  clientId: "",
});

const recentProjects = computed(() => projects.value.slice(0, 4));
function chainStatusLabel(status) {
  if (status === "confirmed") return "链上已核验";
  if (status === "migrating") return "历史迁移中";
  if (status === "pending") return "待链上创建";
  return "待历史摘要迁移";
}
const heroStats = computed(() => [
  {
    label: "在线项目",
    value: projects.value.length,
    helper: "当前系统内可被展示和推进的项目总数。",
  },
  {
    label: "开发者资源",
    value: developers.value.length,
    helper: "当前可被分配到新项目中的开发者账号数量。",
  },
  {
    label: "客户资源",
    value: clients.value.length,
    helper: "可绑定到项目协作链路中的客户身份数量。",
  },
]);

async function loadData() {
  const [projectPayload, developerPayload, clientPayload] = await Promise.all([
    api.getProjects(),
    api.getUsers("developer"),
    api.getUsers("client"),
  ]);
  projects.value = projectPayload.items;
  developers.value = developerPayload.items;
  clients.value = clientPayload.items;
  if (!form.developerId && developers.value[0]) {
    form.developerId = developers.value[0].id;
  }
  if (!form.clientId && clients.value[0]) {
    form.clientId = clients.value[0].id;
  }
}

async function createProject() {
  message.value = "";
  errorMessage.value = "";
  try {
    const payload = await api.createProject({
      ...form,
      developerId: Number(form.developerId),
      clientId: Number(form.clientId),
    });
    form.name = "";
    form.description = "";
    await loadData();
    const project = projects.value.find((item) => item.id === payload.projectId);
    message.value = payload.message;
    if (project) {
      try {
        await writeProjectCreation(project);
      } catch (error) {
        errorMessage.value = `项目已保存，链上创建仍待处理：${error.message}`;
      }
    }
  } catch (error) {
    errorMessage.value = error.message;
  }
}

async function writeProjectCreation(project) {
  errorMessage.value = "";
  chainBusy.value[project.id] = true;
  try {
    const pending = getPendingEvidence("project", project.id);
    if (project.candidateTxHash || pending) {
      await retryEvidenceVerification(pending || {
        businessType: "project",
        businessId: project.id,
        chainId: Number(project.chainId),
        txHash: project.candidateTxHash,
      }, auth.user?.walletAddress);
    } else {
      const transaction = await submitProjectCreation(
        project.id,
        project.developerWallet,
        project.clientWallet,
        project.digest,
        auth.user?.walletAddress,
      );
      await confirmEvidenceTransaction("project", project.id, transaction);
    }
    message.value = `项目 ${project.name} 的链上创建已核验。`;
    await loadData();
  } finally {
    chainBusy.value[project.id] = false;
  }
}


onMounted(() => {
  loadData().catch((error) => {
    errorMessage.value = error.message;
  });
});

</script>

<template>
  <AppShell>
    <PageHero
      eyebrow="项目创建"
      title="创建项目，启动可信协作。"
      description="填写项目、开发者与客户后，系统会同步建立后续流程与审计入口。"
      tone="admin"
      variant="minimal"
      :stats="heroStats"
    >
      <template #actions>
        <button class="button" type="button" @click="createProject" :disabled="!form.name || !form.developerId || !form.clientId">保存并创建链上项目</button>
      </template>
    </PageHero>

    <section class="showcase-grid showcase-grid-secondary">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">录入区</div>
            <h3>填写新项目</h3>
          </div>
        </div>

        <div class="form-grid">
          <div class="field">
            <label>项目名称</label>
            <input v-model="form.name" placeholder="例如：企业官网 3.0 升级" />
          </div>
          <div class="field">
            <label>项目说明</label>
            <textarea v-model="form.description" placeholder="说明项目目标、验收范围与交付要求" />
          </div>
          <div class="field">
            <label>开发者</label>
            <MemberPicker v-model="form.developerId" :items="developers" placeholder="请选择开发者" />
          </div>
          <div class="field">
            <label>客户</label>
            <MemberPicker v-model="form.clientId" :items="clients" placeholder="请选择客户" />
          </div>
        </div>

        <div class="state-explain-card">
          <span>提交后会发生什么</span>
          <strong>创建后进入项目池。</strong>
        </div>

        <p v-if="message" class="feedback success-text">{{ message }}</p>
        <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
      </section>

      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">最近建立</div>
            <h3>最新项目</h3>
          </div>
        </div>

        <div class="brand-project-grid">
          <article v-for="project in recentProjects" :key="project.id" class="brand-project-card">
            <div class="project-card-top">
              <div>
                <strong>{{ project.name }}</strong>
                <div class="meta-line">开发者：{{ project.developerName }} ｜ 客户：{{ project.clientName }}</div>
              </div>
              <div class="project-status-stack">
                <StatusTag :value="project.status" />
                <span class="pill subtle">{{ chainStatusLabel(project.chainStatus) }}</span>
              </div>
            </div>
            <p>{{ project.description }}</p>
            <div v-if="project.txHash && project.recordOrigin !== 'migration'" class="mono">项目创建交易：{{ project.txHash }} / 网络 {{ project.chainId }} / 区块 {{ project.blockNumber }}</div>
            <div v-if="project.txHash && project.recordOrigin === 'migration'" class="meta-line">迁移存证｜源创建 {{ project.sourceCreatedAt || '时间未知' }}｜实际上链 {{ project.anchoredAt || '时间未知' }}｜检查点交易 {{ project.txHash }} / 区块 {{ project.blockNumber }}</div>
            <div v-if="project.legacyTxHash" class="meta-line">旧模拟哈希（不是链上交易）：{{ project.legacyTxHash }}</div>
            <div class="button-row">
              <button v-if="project.chainStatus === 'pending'" class="secondary-button" type="button" :disabled="chainBusy[project.id]" @click="writeProjectCreation(project)">
                {{ chainBusy[project.id] ? "等待链上确认..." : project.candidateTxHash ? "重试核验同一笔交易" : "完成链上创建" }}
              </button>
            </div>
          </article>
        </div>
      </section>
    </section>
  </AppShell>
</template>

