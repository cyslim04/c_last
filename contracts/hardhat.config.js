require("@nomicfoundation/hardhat-toolbox");
require("dotenv").config();

const { SEPOLIA_RPC_URL, PRIVATE_KEY } = process.env;

module.exports = {
  solidity: {
    version: "0.8.24",
    settings: {
      optimizer: { enabled: true, runs: 200 },
      viaIR: true,
    },
  },
  networks: {
    hardhat: {},
    localhost: {
      url: process.env.HARDHAT_RPC_URL || "http://127.0.0.1:8545",
      accounts: process.env.HARDHAT_PRIVATE_KEY ? [process.env.HARDHAT_PRIVATE_KEY] : undefined,
    },
    sepolia: {
      url: SEPOLIA_RPC_URL || "",
      accounts: PRIVATE_KEY ? [PRIVATE_KEY] : [],
    },
  },
  paths: {
    sources: "./contracts",
    tests: "./test",
    cache: process.env.HARDHAT_CACHE_PATH || "./cache",
    artifacts: process.env.HARDHAT_ARTIFACTS_PATH || "./artifacts",
  },
};
