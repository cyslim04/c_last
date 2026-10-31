import { BrowserProvider, Contract } from "ethers";
import { api } from "../api/http";

export const evidenceAbi = [
  "function createProject(uint256 projectId, address developerWallet, address clientWallet, bytes32 digest)",
  "function recordWorklog(uint256 projectId, uint32 stageNo, uint256 worklogId, bytes32 digest)",
  "function recordDeliverable(uint256 projectId, uint32 stageNo, uint256 deliverableId, bytes32 digest)",
  "function confirmDeliverable(uint256 projectId, uint32 stageNo, uint256 deliverableId, bool approved, bytes32 digest)",
  "function recordStageAudit(uint256 projectId, uint32 stageNo, uint256 auditId, bool approved, bytes32 digest)",
  "function recordFinalAudit(uint256 projectId, uint256 auditId, bool approved, bytes32 digest)",
  "function recordMigrationCheckpoint(uint256 projectId, address developerWallet, address clientWallet, bytes32 checkpointHash, uint32 batchCount, uint64 sourceCreatedAt, uint32 currentStage, uint32 passedStageCount, uint32 currentStageWorklogCount, uint32 currentStageDeliverableCount, uint32 totalWorklogCount, uint32 totalDeliverableCount, uint32 totalConfirmationCount, uint32 totalStageAuditCount, uint32 totalFinalAuditCount, uint256 latestDeliverableId, uint32 latestDeliverableStage, bool currentStageClientApproved, bool finalized, bool finalApproved)",
  "function recordMigrationBatch(uint256 projectId, uint32 batchNo, uint8 category, bytes32 digest, uint32 recordCount, uint64 sourceFrom, uint64 sourceTo)",
];

async function getChainContext(expectedWallet) {
  if (!window.ethereum) {
    throw new Error("未检测到 MetaMask，请先安装并连接钱包。");
  }

  const config = await api.getChainConfig();
  const provider = new BrowserProvider(window.ethereum);
  const signer = await provider.getSigner();
  const [network, address] = await Promise.all([provider.getNetwork(), signer.getAddress()]);
  const networkConfig = config.networks.find((item) => String(item.chainId) === network.chainId.toString());

  if (!networkConfig?.contractAddress) {
    throw new Error("当前钱包网络尚未配置存证合约，请切换到已配置的 Hardhat 或 Sepolia 网络。");
  }
  if (expectedWallet && address.toLowerCase() !== expectedWallet.toLowerCase()) {
    throw new Error(`当前钱包 ${address} 与该业务角色绑定的钱包不一致。`);
  }

  return {
    contract: new Contract(networkConfig.contractAddress, evidenceAbi, signer),
    provider,
    address,
    chainId: Number(network.chainId),
    confirmations: Number(networkConfig.confirmations || 1),
  };
}

async function send(expectedWallet, sendTransaction) {
  const context = await getChainContext(expectedWallet);
  const tx = await sendTransaction(context.contract);
  return { tx, chainId: context.chainId, confirmations: context.confirmations };
}

export function submitProjectCreation(projectId, developerWallet, clientWallet, digest, adminWallet) {
  return send(adminWallet, (contract) => contract.createProject(projectId, developerWallet, clientWallet, `0x${digest}`));
}

export function submitWorklogEvidence(project, worklog) {
  return send(project.developerWallet, (contract) => contract.recordWorklog(
    project.id,
    worklog.stageNo,
    worklog.id,
    `0x${worklog.digest}`,
  ));
}

export function submitDeliverableEvidence(project, deliverable) {
  return send(project.developerWallet, (contract) => contract.recordDeliverable(
    project.id,
    deliverable.stageNo,
    deliverable.id,
    `0x${deliverable.fileHash}`,
  ));
}

export function submitDeliverableConfirmation(project, deliverable, approved) {
  return send(project.clientWallet, (contract) => contract.confirmDeliverable(
    project.id,
    deliverable.stageNo,
    deliverable.id,
    approved,
    `0x${deliverable.fileHash}`,
  ));
}

export function submitStageAudit(projectId, stageNo, auditId, approved, digest, adminWallet) {
  return send(adminWallet, (contract) => contract.recordStageAudit(
    projectId,
    stageNo,
    auditId,
    approved,
    `0x${digest}`,
  ));
}

export function submitFinalAudit(projectId, auditId, approved, digest, adminWallet) {
  return send(adminWallet, (contract) => contract.recordFinalAudit(
    projectId,
    auditId,
    approved,
    `0x${digest}`,
  ));
}

export function submitMigrationCheckpoint(projectId, checkpoint, adminWallet) {
  return send(adminWallet, (contract) => contract.recordMigrationCheckpoint(
    projectId,
    checkpoint.developerWallet,
    checkpoint.clientWallet,
    `0x${checkpoint.digest}`,
    checkpoint.batchCount,
    checkpoint.sourceCreatedAt,
    checkpoint.currentStage,
    checkpoint.passedStageCount,
    checkpoint.currentStageWorklogCount,
    checkpoint.currentStageDeliverableCount,
    checkpoint.totalWorklogCount,
    checkpoint.totalDeliverableCount,
    checkpoint.totalConfirmationCount,
    checkpoint.totalStageAuditCount,
    checkpoint.totalFinalAuditCount,
    checkpoint.latestDeliverableId,
    checkpoint.latestDeliverableStage,
    checkpoint.currentStageClientApproved,
    checkpoint.finalized,
    checkpoint.finalApproved,
  ));
}

export function submitMigrationBatch(projectId, batch, adminWallet) {
  return send(adminWallet, (contract) => contract.recordMigrationBatch(
    projectId,
    batch.batchNo,
    batch.categoryId,
    `0x${batch.digest}`,
    batch.recordCount,
    batch.sourceFrom,
    batch.sourceTo,
  ));
}

export async function waitForEvidenceTransaction(transaction, confirmations) {
  const receipt = await transaction.wait(confirmations);
  if (!receipt || receipt.status !== 1) {
    throw new Error("链上交易未成功，请检查钱包记录后重试。");
  }
  return receipt;
}

function pendingKey(businessType, businessId) {
  return `trust-work-pending-evidence:${businessType}:${businessId}`;
}

export async function confirmEvidenceTransaction(businessType, businessId, transactionContext, comment = "") {
  const payload = {
    businessType,
    businessId: Number(businessId),
    chainId: transactionContext.chainId,
    txHash: transactionContext.tx.hash,
    ...(comment ? { comment } : {}),
  };
  localStorage.setItem(pendingKey(businessType, businessId), JSON.stringify(payload));

  // The first request stores the candidate hash, even if the node has not mined it yet.
  // A temporary RPC failure is retried after the wallet transaction reaches its required depth.
  try {
    await api.verifyEvidence(payload);
  } catch {
    // The local retry payload and the transaction response remain available below.
  }
  const receipt = await waitForEvidenceTransaction(transactionContext.tx, transactionContext.confirmations);
  const result = await api.verifyEvidence(payload);
  localStorage.removeItem(pendingKey(businessType, businessId));
  return { ...result, receipt, payload };
}

export async function retryEvidenceVerification(payload, expectedWallet, confirmations) {
  const context = await getChainContext(expectedWallet);
  if (Number(payload.chainId) !== context.chainId) {
    throw new Error("请先切换到该待核验交易提交时使用的网络。");
  }
  if (payload.txHash) {
    const receipt = await context.provider.waitForTransaction(payload.txHash, confirmations || context.confirmations, 120000);
    if (!receipt || receipt.status !== 1) {
      throw new Error("交易仍未成功确认，原业务记录保持待核验状态。");
    }
  }
  const result = await api.verifyEvidence(payload);
  localStorage.removeItem(pendingKey(payload.businessType, payload.businessId));
  return result;
}

export function getPendingEvidence(businessType, businessId) {
  try {
    return JSON.parse(localStorage.getItem(pendingKey(businessType, businessId)) || "null");
  } catch {
    return null;
  }
}
