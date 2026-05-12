<script setup>
defineProps({
  eyebrow: {
    type: String,
    default: "",
  },
  title: {
    type: String,
    default: "",
  },
  description: {
    type: String,
    default: "",
  },
  tone: {
    type: String,
    default: "neutral",
  },
  variant: {
    type: String,
    default: "default",
  },
  size: {
    type: String,
    default: "default",
  },
  badges: {
    type: Array,
    default: () => [],
  },
  stats: {
    type: Array,
    default: () => [],
  },
});
</script>

<template>
  <section class="page-hero" :class="[`tone-${tone}`, `size-${size}`, `variant-${variant}`, { 'has-visual': !!$slots.default }]">
    <div class="page-hero-copy">
      <div v-if="eyebrow" class="hero-kicker">{{ eyebrow }}</div>
      <h2>{{ title }}</h2>
      <p v-if="description" class="page-hero-description">{{ description }}</p>

      <div v-if="badges.length" class="hero-badge-row">
        <span v-for="badge in badges" :key="badge" class="hero-inline-badge">{{ badge }}</span>
      </div>

      <div v-if="stats.length" class="hero-stat-row">
        <article v-for="item in stats" :key="item.label" class="hero-stat-card">
          <span>{{ item.label }}</span>
          <strong>{{ item.value }}</strong>
          <small>{{ item.helper }}</small>
        </article>
      </div>

      <div v-if="$slots.actions" class="hero-action-row">
        <slot name="actions" />
      </div>
    </div>

    <div v-if="$slots.default" class="page-hero-visual">
      <slot />
    </div>
  </section>
</template>
