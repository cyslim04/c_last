import { createApp } from "vue";
import { createPinia } from "pinia";
import App from "./App.vue";
import router from "./router";
import { useWalletStore } from "./stores/wallet";
import "bootstrap/dist/css/bootstrap-grid.min.css";
import "bootstrap/dist/css/bootstrap-utilities.min.css";
import "./styles/main.css";
import "./styles/stride.css";
import "./styles/light-theme.css";

const app = createApp(App);
const pinia = createPinia();

app.use(pinia);

const wallet = useWalletStore(pinia);
document.documentElement.setAttribute("data-theme", "light");
document.documentElement.style.colorScheme = "light";
wallet.init();
wallet.sync().catch(() => wallet.reset());

app.use(router).mount("#app");
