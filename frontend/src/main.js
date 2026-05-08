import { createApp } from "vue";
import { createPinia } from "pinia";
import App from "./App.vue";
import router from "./router";
import { useWalletStore } from "./stores/wallet";
import "./styles/main.css";

const app = createApp(App);
const pinia = createPinia();

app.use(pinia);

const wallet = useWalletStore(pinia);
document.documentElement.setAttribute("data-theme", "dark");
document.documentElement.style.colorScheme = "dark";
wallet.init();
wallet.sync().catch(() => wallet.reset());

app.use(router).mount("#app");
