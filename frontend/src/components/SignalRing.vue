<script setup>
import { computed } from "vue";

const props = defineProps({
  title: {
    type: String,
    default: "",
  },
  value: {
    type: Number,
    default: 0,
  },
  max: {
    type: Number,
    default: 100,
  },
  subtitle: {
    type: String,
    default: "",
  },
  tone: {
    type: String,
    default: "neutral",
  },
});

const percentage = computed(() => {
  if (!props.max) {
    return 0;
  }
  return Math.max(0, Math.min(100, Math.round((props.value / props.max) * 100)));
});

const ringStyle = computed(() => ({
  "--ring-progress": `${percentage.value}%`,
}));
</script>

<template>
  <article class="signal-ring-card" :class="`tone-${tone}`">
    <div class="signal-ring" :style="ringStyle">
      <div class="signal-ring-core">
        <span>闭环度</span>
        <strong>{{ percentage }}%</strong>
      </div>
    </div>
    <div class="signal-ring-copy">
      <div class="hero-kicker">{{ title }}</div>
      <p>{{ subtitle }}</p>
    </div>
  </article>
</template>
