async function main() {
  const TrustWorkProof = await ethers.getContractFactory("TrustWorkProof");
  const contract = await TrustWorkProof.deploy();
  await contract.waitForDeployment();
  console.log("TrustWorkProof deployed to:", await contract.getAddress());
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
