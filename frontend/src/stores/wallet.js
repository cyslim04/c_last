import { defineStore } from "pinia";

const NETWORKS = [
  {
    chainId: "0xaa36a7",
    decimalChainId: "11155111",
    name: "Sepolia",
    switchLabel: "切换到 Sepolia",
    addParams: null,
  },
  {
    chainId: "0x7a69",
    decimalChainId: "31337",
    name: "Hardhat",
    switchLabel: "切换到本地链",
    addParams: {
      chainId: "0x7a69",
      chainName: "Hardhat Local",
      nativeCurrency: { name: "ETH", symbol: "ETH", decimals: 18 },
      rpcUrls: ["http://127.0.0.1:8545"],
      blockExplorerUrls: [],
    },
  },
];

function shortenAddress(address) {
  if (!address) {
    return "";
  }
  return `${address.slice(0, 6)}...${address.slice(-4)}`;
}

function getNetworkMeta(chainId) {
  return NETWORKS.find((item) => item.chainId.toLowerCase() === (chainId || "").toLowerCase()) || null;
}

export const useWalletStore = defineStore("wallet", {
  state: () => ({
    address: "",
    shortAddress: "",
    chainId: "",
    chainName: "",
    connected: false,
    supportedNetwork: false,
    availableNetworks: NETWORKS,
    initialized: false,
  }),
  getters: {
    connectionLabel(state) {
      if (!state.connected) {
        return "连接 MetaMask";
      }
      return `${state.shortAddress} · ${state.chainName || "未知网络"}`;
    },
  },
  actions: {
    updateState(address, chainId) {
      const network = getNetworkMeta(chainId);
      this.address = address || "";
      this.shortAddress = shortenAddress(address);
      this.chainId = chainId || "";
      this.chainName = network?.name || (chainId ? `未知网络 ${chainId}` : "");
      this.connected = Boolean(address);
      this.supportedNetwork = Boolean(network);
    },
    reset() {
      this.updateState("", "");
    },
    async sync() {
      if (!window.ethereum) {
        this.reset();
        return null;
      }
      const [accounts, chainId] = await Promise.all([
        window.ethereum.request({ method: "eth_accounts" }),
        window.ethereum.request({ method: "eth_chainId" }),
      ]);
      this.updateState(accounts?.[0] || "", chainId || "");
      return this.connected
        ? { address: this.address, chainId: this.chainId, chainName: this.chainName }
        : null;
    },
    init() {
      if (this.initialized || !window.ethereum) {
        this.initialized = true;
        return;
      }
      window.ethereum.on("accountsChanged", (accounts) => {
        this.updateState(accounts?.[0] || "", this.chainId);
      });
      window.ethereum.on("chainChanged", (chainId) => {
        this.updateState(this.address, chainId);
      });
      this.initialized = true;
    },
    async connect() {
      if (!window.ethereum) {
        throw new Error("未检测到 MetaMask，请先安装扩展。");
      }
      const [accounts, chainId] = await Promise.all([
        window.ethereum.request({ method: "eth_requestAccounts" }),
        window.ethereum.request({ method: "eth_chainId" }),
      ]);
      this.updateState(accounts?.[0] || "", chainId || "");
      return { address: this.address, chainId: this.chainId, chainName: this.chainName };
    },
    async switchAccount() {
      if (!window.ethereum) {
        throw new Error("未检测到 MetaMask。");
      }
      try {
        await window.ethereum.request({
          method: "wallet_requestPermissions",
          params: [{ eth_accounts: {} }],
        });
      } catch (error) {
        if (error?.code !== 4001) {
          throw error;
        }
      }
      return this.connect();
    },
    async switchNetwork(targetChainId) {
      if (!window.ethereum) {
        throw new Error("未检测到 MetaMask。");
      }
      const target = getNetworkMeta(targetChainId);
      if (!target) {
        throw new Error("不支持的网络。");
      }
      try {
        await window.ethereum.request({
          method: "wallet_switchEthereumChain",
          params: [{ chainId: target.chainId }],
        });
      } catch (error) {
        if (error?.code === 4902 && target.addParams) {
          await window.ethereum.request({
            method: "wallet_addEthereumChain",
            params: [target.addParams],
          });
        } else {
          throw error;
        }
      }
      return this.sync();
    },
    async disconnect() {
      if (window.ethereum) {
        try {
          await window.ethereum.request({
            method: "wallet_revokePermissions",
            params: [{ eth_accounts: {} }],
          });
        } catch (error) {
          if (error?.code !== 4100) {
            // Ignore best-effort revoke failures.
          }
        }
      }
      this.reset();
    },
  },
});
