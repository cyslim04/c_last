async function main() {
  const [deployer] = await ethers.getSigners();
  const TrustWorkProof = await ethers.getContractFactory("TrustWorkProof");
  const contract = await TrustWorkProof.deploy();
  await contract.waitForDeployment();
  const network = await ethers.provider.getNetwork();
  console.log("Network chainId:", network.chainId.toString());
  console.log("Deployer wallet (contract administrator):", deployer.address);
  console.log("TrustWorkProof owner:", await contract.owner());
  console.log("TrustWorkProof deployed to:", await contract.getAddress());
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
