<script setup>
import { onMounted } from "vue";
import { useRouter } from "vue-router";

const router = useRouter();

onMounted(() => {
  router.replace("/home");
});
</script>

<template>
  <div class="route-redirect-placeholder" aria-hidden="true" />
</template>
