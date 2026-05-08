import { BrowserProvider, Contract } from "ethers";

export const evidenceAbi = [
  "function createProject(uint256 projectId, address developerWallet, address clientWallet)",
  "function recordWorklog(uint256 projectId, uint256 worklogId, bytes32 worklogHash, uint64 periodStart, uint64 periodEnd)",
  "function recordDeliverable(uint256 projectId, uint256 deliverableId, bytes32 fileHash, string version, bytes32 fileNameHash)",
  "function confirmDeliverable(uint256 projectId, uint256 deliverableId, bool approved, bytes32 commentHash)"
];

async function getContract(address) {
  if (!window.ethereum) {
    throw new Error("未检测到 MetaMask");
  }

  const provider = new BrowserProvider(window.ethereum);
  const signer = await provider.getSigner();
  return new Contract(address, evidenceAbi, signer);
}

export async function submitWorklogEvidence(contractAddress, payload) {
  const contract = await getContract(contractAddress);
  const tx = await contract.recordWorklog(
    payload.projectId,
    payload.worklogId,
    payload.worklogHash,
    payload.periodStart,
    payload.periodEnd,
  );
  return tx.wait();
}

export async function submitDeliverableEvidence(contractAddress, payload) {
  const contract = await getContract(contractAddress);
  const tx = await contract.recordDeliverable(
    payload.projectId,
    payload.deliverableId,
    payload.fileHash,
    payload.version,
    payload.fileNameHash,
  );
  return tx.wait();
}

export async function confirmDeliverableEvidence(contractAddress, payload) {
  const contract = await getContract(contractAddress);
  const tx = await contract.confirmDeliverable(
    payload.projectId,
    payload.deliverableId,
    payload.approved,
    payload.commentHash,
  );
  return tx.wait();
}
