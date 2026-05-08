<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from "vue";

const props = defineProps({
  modelValue: {
    type: [String, Number],
    default: "",
  },
  items: {
    type: Array,
    default: () => [],
  },
  placeholder: {
    type: String,
    default: "请选择成员",
  },
  showMeta: {
    type: Boolean,
    default: true,
  },
  getLabel: {
    type: Function,
    default: (item) => item?.name || item?.label || "",
  },
  getMeta: {
    type: Function,
    default: (item) => item?.company || item?.username || "",
  },
});

const emit = defineEmits(["update:modelValue"]);

const rootRef = ref(null);
const open = ref(false);

const selectedItem = computed(() => props.items.find((item) => String(item.id) === String(props.modelValue)) || null);

function getItemLabel(item) {
  return props.getLabel(item);
}

function getItemMeta(item) {
  return props.getMeta(item);
}

const selectedSummary = computed(() => {
  if (!selectedItem.value) {
    return props.placeholder;
  }

  const label = getItemLabel(selectedItem.value);
  const meta = props.showMeta ? getItemMeta(selectedItem.value) : "";
  return meta ? `${label} / ${meta}` : label;
});

function toggle() {
  if (!props.items.length) {
    return;
  }
  open.value = !open.value;
}

function selectItem(item) {
  emit("update:modelValue", item.id);
  open.value = false;
}

function handleDocumentClick(event) {
  if (rootRef.value && !rootRef.value.contains(event.target)) {
    open.value = false;
  }
}

function handleWindowKeydown(event) {
  if (event.key === "Escape") {
    open.value = false;
  }
}

onMounted(() => {
  document.addEventListener("mousedown", handleDocumentClick);
  window.addEventListener("keydown", handleWindowKeydown);
});

onBeforeUnmount(() => {
  document.removeEventListener("mousedown", handleDocumentClick);
  window.removeEventListener("keydown", handleWindowKeydown);
});
</script>

<template>
  <div ref="rootRef" class="member-picker" :class="{ open }">
    <button
      class="member-picker-trigger"
      :class="{ open }"
      type="button"
      @click="toggle"
      :disabled="!items.length"
    >
      <div class="member-picker-copy">
        <strong>{{ selectedSummary }}</strong>
      </div>
      <span class="menu-caret" :class="{ open }">⌄</span>
    </button>

    <div v-if="open" class="member-picker-menu">
      <button
        v-for="item in items"
        :key="item.id"
        class="member-picker-option"
        :class="{ active: String(item.id) === String(modelValue) }"
        type="button"
        @click="selectItem(item)"
      >
        <strong>{{ getItemLabel(item) }}</strong>
        <span v-if="showMeta && getItemMeta(item)">{{ getItemMeta(item) }}</span>
      </button>
    </div>
  </div>
</template>
