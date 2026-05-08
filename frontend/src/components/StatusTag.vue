<script setup>
import { computed } from "vue";

const props = defineProps({
  value: {
    type: String,
    required: true,
  },
});

const visualMap = {
  active: { label: "进行中", tone: "running" },
  completed: { label: "已完成", tone: "completed" },
  disputed: { label: "争议中", tone: "warning" },
  draft: { label: "草稿", tone: "neutral" },
  inactive: { label: "已停用", tone: "inactive" },
  neutral: { label: "无状态", tone: "neutral" },
  pending: { label: "待处理", tone: "pending" },
  locked: { label: "未解锁", tone: "pending" },
  ready: { label: "可执行", tone: "running" },
  running: { label: "推进中", tone: "running" },
  confirmed: { label: "已确认", tone: "completed" },
  uploaded: { label: "待存证", tone: "pending" },
  notarized: { label: "已存证", tone: "running" },
  approved: { label: "已通过", tone: "completed" },
  rejected: { label: "已驳回", tone: "failed" },
  failed: { label: "失败", tone: "failed" },
  blocked: { label: "受阻", tone: "warning" },
};

const visual = computed(() => visualMap[props.value] || { label: props.value, tone: "neutral" });
</script>

<template>
  <span class="status-badge" :class="`tone-${visual.tone}`">
    <span class="status-dot" />
    {{ visual.label }}
  </span>
</template>
