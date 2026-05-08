const { expect } = require("chai");
const { ethers } = require("hardhat");

describe("TrustWorkProof", function () {
  async function deployFixture() {
    const [owner, developer, client, outsider] = await ethers.getSigners();
    const Factory = await ethers.getContractFactory("TrustWorkProof");
    const contract = await Factory.deploy();
    await contract.waitForDeployment();
    return { contract, owner, developer, client, outsider };
  }

  it("creates project and stores wallets", async function () {
    const { contract, developer, client } = await deployFixture();
    await contract.createProject(1, developer.address, client.address);
    const project = await contract.projects(1);
    expect(project.exists).to.equal(true);
    expect(project.developerWallet).to.equal(developer.address);
    expect(project.clientWallet).to.equal(client.address);
  });

  it("allows only developer to record worklog", async function () {
    const { contract, developer, client, outsider } = await deployFixture();
    await contract.createProject(1, developer.address, client.address);
    await expect(
      contract.connect(outsider).recordWorklog(1, 1001, ethers.id("worklog"), 1, 2)
    ).to.be.revertedWith("Only developer");
    await expect(
      contract.connect(developer).recordWorklog(1, 1001, ethers.id("worklog"), 1, 2)
    ).to.emit(contract, "WorklogRecorded");
  });

  it("rejects duplicate deliverable ids", async function () {
    const { contract, developer, client } = await deployFixture();
    await contract.createProject(1, developer.address, client.address);
    await contract.connect(developer).recordDeliverable(1, 5001, ethers.id("file"), "v1", ethers.id("name"));
    await expect(
      contract.connect(developer).recordDeliverable(1, 5001, ethers.id("file2"), "v2", ethers.id("name2"))
    ).to.be.revertedWith("Record already exists");
  });

  it("allows only client to confirm deliverable", async function () {
    const { contract, developer, client, outsider } = await deployFixture();
    await contract.createProject(1, developer.address, client.address);
    await contract.connect(developer).recordDeliverable(1, 7001, ethers.id("file"), "v1", ethers.id("name"));
    await expect(
      contract.connect(outsider).confirmDeliverable(1, 7001, true, ethers.id("ok"))
    ).to.be.revertedWith("Only client");
    await expect(
      contract.connect(client).confirmDeliverable(1, 7001, true, ethers.id("ok"))
    ).to.emit(contract, "DeliverableConfirmed");
  });
});
