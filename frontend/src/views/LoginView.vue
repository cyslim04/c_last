<script setup>
import { reactive, ref } from "vue";
import { useRouter } from "vue-router";
import { useAuthStore } from "../stores/auth";

const router = useRouter();
const auth = useAuthStore();
const errorMessage = ref("");
const form = reactive({ username: "", password: "" });

async function submit() {
  errorMessage.value = "";
  try {
    await auth.login(form);
    router.push("/home");
  } catch (error) {
    errorMessage.value = error.message;
  }
}
</script>

<template>
  <div class="auth-layout">
    <section class="auth-hero">
      <div>
        <div class="hero-kicker">Trust Work</div>
        <h1>让协作过程成为可信记录。</h1>
        <p>统一管理工时、交付物、客户确认、阶段审计与链上证据。</p>
      </div>
    </section>

    <section class="auth-panel">
      <div class="panel auth-card">
        <div class="auth-card-head">
          <div>
            <div class="eyebrow">账户登录</div>
            <h2>进入工作系统</h2>
          </div>
        </div>

        <div class="form-grid">
          <div class="field">
            <label>账号</label>
            <input v-model="form.username" autocomplete="username" placeholder="请输入账号" />
          </div>
          <div class="field">
            <label>密码</label>
            <input v-model="form.password" type="password" autocomplete="current-password" placeholder="请输入密码" @keyup.enter="submit" />
          </div>
        </div>

        <div class="state-explain-card">
          <span>登录说明</span>
          <strong>请使用管理员分配的账号和密码登录。</strong>
        </div>

        <p v-if="errorMessage" class="feedback error-text">{{ errorMessage }}</p>

        <div class="button-row">
          <button class="button" type="button" @click="submit" :disabled="auth.loading">
            {{ auth.loading ? "登录中..." : "登录" }}
          </button>
        </div>
      </div>
    </section>
  </div>
</template>

