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
    cue: "项目创建页需要像一个业务落地页，而不是单纯的控制台弹窗替代品。",
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
      eyebrow="项目创建"
      title="为下一条可信协作链路建立项目入口。"
      description="项目一旦建立，就会连接开发者、客户、阶段流程与审计闭环。这个页面不只是录入表单，而是整条链路的起点。"
      tone="admin"
      :stats="heroStats"
    >
      <template #actions>
        <button class="button" type="button" @click="createProject">保存项目</button>
      </template>

      <article class="hero-side-card">
        <div class="hero-kicker">当前草稿</div>
        <strong>{{ form.name || "先定义项目名称" }}</strong>
        <p>项目创建成功后，会进入项目管理、流程总览与审计页面同步展示，成为后续协作和留痕的共同对象。</p>
      </article>
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
              <StatusTag :value="project.status" />
            </div>
            <p>{{ project.description }}</p>
          </article>
        </div>
      </section>
    </section>
  </AppShell>
</template>
