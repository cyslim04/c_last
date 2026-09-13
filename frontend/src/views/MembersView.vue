<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref } from "vue";
import AppShell from "../components/AppShell.vue";
import PageHero from "../components/PageHero.vue";
import StatusTag from "../components/StatusTag.vue";
import { api } from "../api/http";


const filterMode = ref("all");
const members = ref([]);
const selectedMemberId = ref(null);
const loading = ref(false);
const saving = ref(false);
const message = ref("");
const errorMessage = ref("");

const createForm = reactive({
  name: "",
  username: "",
  password: "",
  role: "developer",
  company: "",
  email: "",
  phone: "",
  bio: "",
});

const editForm = reactive({
  id: 0,
  name: "",
  company: "",
  email: "",
  phone: "",
  bio: "",
});

const passwordForm = reactive({
  newPassword: "",
});

const filterTabs = [
  { key: "all", label: "全部" },
  { key: "developer", label: "开发者" },
  { key: "client", label: "客户" },
  { key: "inactive", label: "停用" },
];

const selectedMember = computed(() => members.value.find((item) => item.id === selectedMemberId.value) || null);
const developerCount = computed(() => members.value.filter((item) => item.role === "developer").length);
const clientCount = computed(() => members.value.filter((item) => item.role === "client").length);
const activeCount = computed(() => members.value.filter((item) => item.status === "active").length);

const heroStats = computed(() => [
  { label: "成员总数", value: members.value.length, helper: "当前资源台中的开发者与客户账号总数" },
  { label: "开发者", value: developerCount.value, helper: "可参与工时与交付的角色数量" },
  { label: "客户", value: clientCount.value, helper: "可参与确认与审计闭环的客户数量" },
  { label: "可用成员", value: activeCount.value, helper: "仍处于启用状态、可继续参与项目的成员" },
]);

function syncEditForm(member) {
  editForm.id = member.id;
  editForm.name = member.name || "";
  editForm.company = member.company || "";
  editForm.email = member.email || "";
  editForm.phone = member.phone || "";
  editForm.bio = member.bio || "";
}

function chooseMember(member) {
  selectedMemberId.value = member.id;
  syncEditForm(member);
  passwordForm.newPassword = "";
}

function currentFilters() {
  if (filterMode.value === "developer") {
    return { role: "developer", status: "all" };
  }
  if (filterMode.value === "client") {
    return { role: "client", status: "all" };
  }
  if (filterMode.value === "inactive") {
    return { role: "", status: "inactive" };
  }
  return { role: "", status: "all" };
}

async function loadMembers() {
  loading.value = true;
  errorMessage.value = "";
  try {
    const { role, status } = currentFilters();
    const payload = await api.getUsers(role || undefined, status);
    members.value = payload.items;
    if (selectedMemberId.value && !members.value.some((item) => item.id === selectedMemberId.value)) {
      selectedMemberId.value = null;
    }
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    loading.value = false;
  }
}

async function createMember() {
  saving.value = true;
  errorMessage.value = "";
  message.value = "";
  try {
    await api.createUser({ ...createForm });
    message.value = "成员创建成功。";
    createForm.name = "";
    createForm.username = "";
    createForm.password = "";
    createForm.company = "";
    createForm.email = "";
    createForm.phone = "";
    createForm.bio = "";
    await loadMembers();
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    saving.value = false;
  }
}

async function updateMember() {
  if (!selectedMember.value) {
    return;
  }
  saving.value = true;
  errorMessage.value = "";
  message.value = "";
  try {
    await api.updateUser({ ...editForm });
    message.value = "成员资料已更新。";
    await loadMembers();
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    saving.value = false;
  }
}

async function resetPassword() {
  if (!selectedMember.value) {
    return;
  }
  saving.value = true;
  errorMessage.value = "";
  message.value = "";
  try {
    await api.resetUserPassword({
      id: selectedMember.value.id,
      newPassword: passwordForm.newPassword,
    });
    message.value = "成员密码已重置。";
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    saving.value = false;
  }
}

async function toggleStatus(member) {
  saving.value = true;
  errorMessage.value = "";
  message.value = "";
  try {
    await api.updateUserStatus({
      id: member.id,
      status: member.status === "active" ? "inactive" : "active",
    });
    message.value = member.status === "active" ? "成员已停用。" : "成员已启用。";
    await loadMembers();
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    saving.value = false;
  }
}

async function removeMember(member) {
  saving.value = true;
  errorMessage.value = "";
  message.value = "";
  try {
    await api.deleteUser(member.id);
    message.value = "成员已删除。";
    if (selectedMemberId.value === member.id) {
      selectedMemberId.value = null;
    }
    await loadMembers();
  } catch (error) {
    errorMessage.value = error.message;
  } finally {
    saving.value = false;
  }
}


onMounted(loadMembers);
</script>

<template>
  <AppShell>
    <PageHero
      eyebrow="管理员工作区"
      title="成员管理"
      description="管理成员账号、角色、资料和启用状态。"
      tone="admin"
      variant="minimal"
      :stats="heroStats"
    >
      <template #actions>
        <div class="filter-chip-row">
          <button
            v-for="tab in filterTabs"
            :key="tab.key"
            class="pill filter-chip"
            :class="{ subtle: filterMode !== tab.key }"
            type="button"
            @click="filterMode = tab.key; loadMembers()"
          >
            {{ tab.label }}
          </button>
        </div>
      </template>
    </PageHero>

    <div class="showcase-grid showcase-grid-secondary">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">新增成员</div>
            <h3>新增成员</h3>
          </div>
        </div>

        <div class="member-role-switch">
          <button
            class="secondary-button"
            :class="{ active: createForm.role === 'developer' }"
            type="button"
            @click="createForm.role = 'developer'"
          >
            开发者
          </button>
          <button
            class="secondary-button"
            :class="{ active: createForm.role === 'client' }"
            type="button"
            @click="createForm.role = 'client'"
          >
            客户
          </button>
        </div>

        <div class="form-grid">
          <div class="field">
            <label>用户名</label>
            <input v-model="createForm.name" placeholder="请输入用户名" />
          </div>
          <div class="field">
            <label>账号</label>
            <input v-model="createForm.username" placeholder="例如：dev3" />
          </div>
          <div class="field">
            <label>密码</label>
            <input v-model="createForm.password" type="password" placeholder="请输入初始密码" />
          </div>
          <div class="field">
            <label>公司 / 团队</label>
            <input v-model="createForm.company" placeholder="请输入公司或团队" />
          </div>
          <div class="field">
            <label>邮箱</label>
            <input v-model="createForm.email" placeholder="请输入邮箱" />
          </div>
          <div class="field">
            <label>手机号</label>
            <input v-model="createForm.phone" placeholder="请输入手机号" />
          </div>
          <div class="field">
            <label>个人简介</label>
            <textarea v-model="createForm.bio" placeholder="说明职责或工作范围" />
          </div>
        </div>

        <div class="state-explain-card">
          <span>创建后会发生什么</span>
          <strong>创建后即可分配项目。</strong>
        </div>

        <div class="button-row">
          <button class="button" type="button" @click="createMember" :disabled="saving">
            {{ saving ? "保存中..." : "创建成员" }}
          </button>
        </div>
      </section>

      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">成员名片</div>
            <h3>成员列表</h3>
          </div>
          <button class="ghost-button" type="button" @click="loadMembers" :disabled="loading">
            {{ loading ? "刷新中..." : "刷新列表" }}
          </button>
        </div>

        <div class="resource-card-grid">
          <article
            v-for="member in members"
            :key="member.id"
            class="resource-card"
            :class="{ active: member.id === selectedMemberId }"
            @click="chooseMember(member)"
          >
            <div class="resource-card-head">
              <div>
                <strong>{{ member.name }}</strong>
                <div class="meta-line">{{ member.username }}</div>
              </div>
              <StatusTag :value="member.status === 'active' ? 'running' : 'inactive'" />
            </div>
            <p>{{ member.company || "未填写单位" }}</p>
            <div class="metric-strip">
              <span>{{ member.role === "developer" ? "开发者" : "客户" }}</span>
              <span>项目引用 {{ member.referencedProjectCount }}</span>
            </div>
            <div class="button-row">
              <button class="secondary-button" type="button" @click.stop="toggleStatus(member)">
                {{ member.status === "active" ? "停用" : "启用" }}
              </button>
              <button class="ghost-button" type="button" @click.stop="removeMember(member)" :disabled="!member.canDelete">
                删除
              </button>
            </div>
          </article>
        </div>
      </section>
    </div>

    <section v-if="selectedMember" class="panel reveal-card">
      <div class="section-heading">
        <div>
          <div class="eyebrow">正在维护成员</div>
          <h3>{{ selectedMember.name }}</h3>
        </div>
        <div class="member-context-meta">
          <span class="pill subtle">{{ selectedMember.role }} / {{ selectedMember.username }}</span>
          <span class="meta-line">当前登录：系统管理员</span>
        </div>
      </div>

      <div class="showcase-grid showcase-grid-secondary">
        <section class="member-edit-panel">
          <div class="section-heading">
            <div>
              <div class="eyebrow">资料编辑</div>
              <h3>资料</h3>
            </div>
          </div>

          <div class="form-grid">
            <div class="field">
              <label>姓名</label>
              <input v-model="editForm.name" />
            </div>
            <div class="field">
              <label>公司 / 团队</label>
              <input v-model="editForm.company" />
            </div>
            <div class="field">
              <label>邮箱</label>
              <input v-model="editForm.email" />
            </div>
            <div class="field">
              <label>手机号</label>
              <input v-model="editForm.phone" />
            </div>
            <div class="field">
              <label>个人简介</label>
              <textarea v-model="editForm.bio" />
            </div>
          </div>

          <div class="button-row">
            <button class="button" type="button" @click="updateMember" :disabled="saving">
              保存资料
            </button>
          </div>
        </section>

        <section class="member-edit-panel">
          <div class="section-heading">
            <div>
              <div class="eyebrow">账号安全</div>
              <h3>密码</h3>
            </div>
          </div>

          <div class="field">
            <label>新密码</label>
            <input v-model="passwordForm.newPassword" type="password" placeholder="请输入新密码" />
          </div>

          <div class="state-explain-card">
            <span>重置后会发生什么</span>
            <strong>重置后使用新密码登录。</strong>
          </div>

          <div class="button-row">
            <button class="button" type="button" @click="resetPassword" :disabled="saving">
              重置密码
            </button>
          </div>
        </section>
      </div>
    </section>

    <p v-if="message" class="feedback success-text">{{ message }}</p>
    <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
  </AppShell>
</template>

