<script setup>
import { computed, reactive, ref, watch } from "vue";
import { useRouter } from "vue-router";
import AppShell from "../components/AppShell.vue";
import PageHero from "../components/PageHero.vue";
import StatusTag from "../components/StatusTag.vue";
import { useAuthStore } from "../stores/auth";
import { useWalletStore } from "../stores/wallet";

const router = useRouter();
const auth = useAuthStore();
const wallet = useWalletStore();

const profileForm = reactive({
  name: "",
  email: "",
  phone: "",
  company: "",
  bio: "",
});
const passwordForm = reactive({
  currentPassword: "",
  newPassword: "",
});
const message = ref("");
const errorMessage = ref("");
const previewUrl = ref("");

const avatarUrl = computed(() => previewUrl.value || auth.user?.avatarUrl || "");
const heroStats = computed(() => [
  { label: "当前角色", value: auth.user?.role || "guest", helper: "该身份决定你在系统中的可见页面与操作范围" },
  { label: "登录账号", value: auth.user?.username || "-", helper: "用于登录的账号标识" },
  { label: "钱包状态", value: wallet.connected ? "已连接" : "未连接", helper: wallet.connected ? wallet.connectionLabel : "连接钱包后可提交链上存证" },
]);

function syncForm() {
  profileForm.name = auth.user?.name || "";
  profileForm.email = auth.user?.email || "";
  profileForm.phone = auth.user?.phone || "";
  profileForm.company = auth.user?.company || "";
  profileForm.bio = auth.user?.bio || "";
}

async function saveProfile() {
  message.value = "";
  errorMessage.value = "";
  try {
    await auth.updateProfile({ ...profileForm });
    message.value = "个人资料已更新。";
  } catch (error) {
    errorMessage.value = error.message;
  }
}

async function updatePassword() {
  message.value = "";
  errorMessage.value = "";
  try {
    await auth.changePassword({ ...passwordForm });
    passwordForm.currentPassword = "";
    passwordForm.newPassword = "";
    message.value = "密码修改成功。";
  } catch (error) {
    errorMessage.value = error.message;
  }
}

async function onAvatarChange(event) {
  const [file] = event.target.files;
  if (!file) {
    return;
  }

  const fileReader = new FileReader();
  fileReader.onload = async () => {
    try {
      const result = String(fileReader.result || "");
      previewUrl.value = result;
      const [, contentBase64 = ""] = result.split(",");
      await auth.uploadAvatar({
        fileName: file.name,
        mimeType: file.type || "image/jpeg",
        contentBase64,
      });
      previewUrl.value = "";
      message.value = "头像已更新。";
    } catch (error) {
      errorMessage.value = error.message;
    }
  };
  fileReader.readAsDataURL(file);
}

async function logout() {
  await auth.logout();
  await wallet.disconnect();
  router.push("/login");
}


watch(
  () => auth.user,
  () => syncForm(),
  { immediate: true, deep: true },
);

</script>

<template>
  <AppShell>
    <PageHero
      eyebrow="身份资料"
      title="维护身份资料与账户安全。"
      description="在一处更新头像、账号信息、钱包状态与密码设置。"
      :tone="auth.role"
      variant="minimal"
      :stats="heroStats"
    >
      <template #actions>
        <button class="button" type="button" @click="saveProfile">保存资料</button>
      </template>
    </PageHero>

    <div class="showcase-grid showcase-grid-secondary">
      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">基础资料</div>
            <h3>基本资料</h3>
          </div>
        </div>

        <label class="ghost-button upload-button upload-button-stage">
          上传头像
          <input type="file" accept="image/*" @change="onAvatarChange" />
        </label>

        <div class="form-grid">
          <div class="field">
            <label>姓名</label>
            <input v-model="profileForm.name" />
          </div>
          <div class="field">
            <label>邮箱</label>
            <input v-model="profileForm.email" type="email" />
          </div>
          <div class="field">
            <label>手机号</label>
            <input v-model="profileForm.phone" />
          </div>
          <div class="field">
            <label>公司 / 团队</label>
            <input v-model="profileForm.company" />
          </div>
          <div class="field">
            <label>个人简介</label>
            <textarea v-model="profileForm.bio" />
          </div>
        </div>
      </section>

      <section class="panel reveal-card">
        <div class="section-heading">
          <div>
            <div class="eyebrow">账号安全</div>
            <h3>账号安全</h3>
          </div>
        </div>

        <div class="identity-status-grid">
          <article class="snapshot-card">
            <div class="hero-kicker">登录账号</div>
            <strong>{{ auth.user?.username }}</strong>
          </article>
          <article class="snapshot-card">
            <div class="hero-kicker">钱包状态</div>
            <strong>{{ wallet.connected ? wallet.connectionLabel : "未连接" }}</strong>
            <StatusTag :value="wallet.connected ? 'running' : 'inactive'" />
          </article>
        </div>

        <div class="form-grid">
          <div class="field">
            <label>当前密码</label>
            <input v-model="passwordForm.currentPassword" type="password" />
          </div>
          <div class="field">
            <label>新密码</label>
            <input v-model="passwordForm.newPassword" type="password" />
          </div>
        </div>

        <div class="state-explain-card">
          <span>修改后会发生什么</span>
          <strong>修改后使用新密码登录。</strong>
        </div>

        <div class="button-row">
          <button class="button" type="button" @click="updatePassword">修改密码</button>
          <button class="ghost-button" type="button" @click="logout">退出登录</button>
        </div>
      </section>
    </div>

    <p v-if="message" class="feedback success-text">{{ message }}</p>
    <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>
  </AppShell>
</template>

