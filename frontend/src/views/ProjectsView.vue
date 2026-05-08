<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref, watchEffect } from "vue";
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
const message = ref("");
const errorMessage = ref("");
const form = reactive({
  name: "",
  description: "",
  developerId: "",
  clientId: "",
});

const recentProjects = computed(() => projects.value.slice(0, 4));
const heroStats = computed(() => [
  {
    label: "在线项目",
    value: projects.value.length,
    helper: "当前系统内可被展示和推进的项目总数",
  },
  {
    label: "开发者资源",
    value: developers.value.length,
    helper: "可分配到新项目的开发者账号",
  },
  {
    label: "客户资源",
    value: clients.value.length,
    helper: "可绑定到项目协作链路中的客户账号",
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
    message.value = payload.message;
    form.name = "";
    form.description = "";
    await loadData();
  } catch (error) {
    errorMessage.value = error.message;
  }
}

watchEffect(() => {
  experience.setPageContext({
    projectName: "新增项目",
    stageLabel: `${projects.value.length} 个项目在线`,
    cue: "这个页面只负责创建新项目，不再承担项目状态查看和分类筛选。",
    tone: "admin",
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
      title="新增项目"
      tone="admin"
      variant="minimal"
    >
      <template #actions>
        <button class="button" type="button" @click="createProject">保存项目</button>
      </template>

      <article class="hero-side-card">
        <div class="hero-kicker">创建提示</div>
        <strong>{{ form.name || "先为新项目定义清晰目标" }}</strong>
        <p>项目创建成功后，会自动进入“项目管理”页面可筛选的项目池，并在首页、流程总览、审计页中同步可见。</p>
      </article>
    </PageHero>

    <div class="showcase-grid showcase-grid-secondary">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">新建项目</div>
            <h3>新建项目</h3>
          </div>
        </div>

        <div class="form-grid">
          <div class="field">
            <label>项目名称</label>
            <input v-model="form.name" placeholder="例如：企业官网 3.0 升级" />
          </div>
          <div class="field">
            <label>项目说明</label>
            <textarea v-model="form.description" placeholder="说明项目目标、验收范围与答辩重点" />
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
          <strong>创建后进入项目管理。</strong>
        </div>

        <p v-if="message" class="feedback success-text">{{ message }}</p>
        <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
      </section>

      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">最近项目</div>
            <h3>最近项目</h3>
          </div>
        </div>

        <div class="brand-project-grid">
          <article v-for="project in recentProjects" :key="project.id" class="brand-project-card">
            <div class="project-card-top">
              <div>
                <strong>{{ project.name }}</strong>
                <div class="meta-line">开发者：{{ project.developerName }} ｜ 客户：{{ project.clientName }}</div>
              </div>
              <StatusTag :value="project.status" />
            </div>
            <p>{{ project.description }}</p>
          </article>
        </div>
      </section>
    </div>
  </AppShell>
</template>
