import { defineStore } from "pinia";

const defaultPageContext = () => ({
  projectName: "",
  stageLabel: "",
  cue: "",
  tone: "neutral",
});

export const useExperienceStore = defineStore("experience", {
  state: () => ({
    pageContext: defaultPageContext(),
  }),
  actions: {
    setPageContext(payload = {}) {
      this.pageContext = {
        ...defaultPageContext(),
        ...payload,
      };
    },
    resetPageContext() {
      this.pageContext = defaultPageContext();
    },
  },
});
