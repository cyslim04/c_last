const { expect } = require("chai");
const { ethers } = require("hardhat");

describe("TrustWorkProof", function () {
  async function deployFixture() {
    const [admin, developer, client, outsider] = await ethers.getSigners();
    const Factory = await ethers.getContractFactory("TrustWorkProof");
    const contract = await Factory.deploy();
    await contract.waitForDeployment();
    return { contract, admin, developer, client, outsider };
  }

  async function createProject(contract, developer, client, id = 1) {
    await contract.createProject(id, developer.address, client.address, ethers.id(`project-${id}`));
  }

  async function prepareApprovedStage(contract, developer, client, projectId = 1, stageNo = 1, id = 10) {
    await contract.connect(developer).recordWorklog(projectId, stageNo, id, ethers.id(`worklog-${id}`));
    await contract.connect(developer).recordDeliverable(projectId, stageNo, id, ethers.id(`deliverable-${id}`));
    await contract.connect(client).confirmDeliverable(
      projectId, stageNo, id, true, ethers.id(`deliverable-${id}`),
    );
  }

  it("limits project creation and audits to the administrator", async function () {
    const { contract, developer, client, outsider } = await deployFixture();
    await expect(contract.connect(outsider).createProject(1, developer.address, client.address, ethers.id("project")))
      .to.be.revertedWith("Only admin");
    await createProject(contract, developer, client);
    await expect(contract.createProject(2, developer.address, client.address, ethers.ZeroHash))
      .to.be.revertedWith("Empty digest");
    await expect(contract.connect(developer).recordStageAudit(1, 1, 90, true, ethers.id("audit")))
      .to.be.revertedWith("Only admin");
  });

  it("allows only the assigned developer and keeps worklog and deliverable IDs separate", async function () {
    const { contract, developer, client, outsider } = await deployFixture();
    await createProject(contract, developer, client);
    await expect(contract.connect(developer).recordWorklog(1, 1, 0, ethers.ZeroHash))
      .to.be.revertedWith("Empty digest");
    await expect(contract.connect(outsider).recordWorklog(1, 1, 5, ethers.id("work")))
      .to.be.revertedWith("Only developer");
    await expect(contract.connect(developer).recordWorklog(1, 1, 5, ethers.id("work")))
      .to.emit(contract, "EvidenceAnchored").withArgs(1, 1, 5, 1, ethers.id("work"), 0, developer.address);
    await expect(contract.connect(developer).recordDeliverable(1, 1, 5, ethers.id("file")))
      .to.emit(contract, "EvidenceAnchored").withArgs(2, 1, 5, 1, ethers.id("file"), 0, developer.address);
    await expect(contract.connect(developer).recordWorklog(1, 1, 5, ethers.id("work-duplicate")))
      .to.be.revertedWith("Worklog already exists");
    await expect(contract.connect(developer).recordDeliverable(1, 1, 5, ethers.id("file-duplicate")))
      .to.be.revertedWith("Deliverable already exists");
  });

  it("requires the assigned client to approve the latest delivery before stage audit", async function () {
    const { contract, admin, developer, client, outsider } = await deployFixture();
    await createProject(contract, developer, client);
    await contract.connect(developer).recordWorklog(1, 1, 1, ethers.id("work"));
    await contract.connect(developer).recordDeliverable(1, 1, 1, ethers.id("file"));
    await expect(contract.connect(outsider).confirmDeliverable(1, 1, 1, true, ethers.id("file")))
      .to.be.revertedWith("Only client");
    await expect(contract.recordStageAudit(1, 1, 100, true, ethers.id("audit")))
      .to.be.revertedWith("Client approval required");
    await contract.connect(client).confirmDeliverable(1, 1, 1, true, ethers.id("file"));
    await expect(contract.recordStageAudit(1, 1, 100, true, ethers.id("audit")))
      .to.emit(contract, "EvidenceAnchored").withArgs(4, 1, 100, 1, ethers.id("audit"), 1, admin.address);
    const project = await contract.projects(1);
    expect(project.currentStage).to.equal(2);
    expect(project.passedStageCount).to.equal(1);
  });

  it("does not advance on a rejected audit and only allows audit of the active stage", async function () {
    const { contract, developer, client } = await deployFixture();
    await createProject(contract, developer, client);
    await prepareApprovedStage(contract, developer, client);
    await contract.recordStageAudit(1, 1, 101, false, ethers.id("reject"));
    expect((await contract.projects(1)).currentStage).to.equal(1);
    await expect(contract.connect(client).confirmDeliverable(1, 1, 10, true, ethers.id("reconfirmed")))
      .not.to.be.reverted;
    await expect(contract.recordStageAudit(1, 2, 102, true, ethers.id("wrong-stage")))
      .to.be.revertedWith("Not current stage");
    await expect(contract.recordStageAudit(1, 1, 103, true, ethers.id("reconfirmed-audit")))
      .to.emit(contract, "EvidenceAnchored").withArgs(4, 1, 103, 1, ethers.id("reconfirmed-audit"), 1, (await ethers.getSigners())[0].address);
  });

  it("requires a closed completed stage before final audit", async function () {
    const { contract, developer, client } = await deployFixture();
    await createProject(contract, developer, client);
    await expect(contract.recordFinalAudit(1, 500, true, ethers.id("final")))
      .to.be.revertedWith("No completed stage");
    await prepareApprovedStage(contract, developer, client);
    await contract.recordStageAudit(1, 1, 501, true, ethers.id("stage"));
    await expect(contract.connect(developer).recordWorklog(1, 2, 20, ethers.id("next-stage-work")))
      .not.to.be.reverted;
    await expect(contract.recordFinalAudit(1, 502, true, ethers.id("premature-final")))
      .to.be.revertedWith("Current stage is not empty");
  });

  it("records an accepted final audit and prevents further activity", async function () {
    const { contract, developer, client } = await deployFixture();
    await createProject(contract, developer, client);
    await prepareApprovedStage(contract, developer, client);
    await contract.recordStageAudit(1, 1, 501, true, ethers.id("stage"));
    await contract.recordFinalAudit(1, 501, true, ethers.id("final"));
    expect((await contract.projects(1)).finalized).to.equal(true);
    expect((await contract.projects(1)).finalAuditRecorded).to.equal(true);
    await expect(contract.connect(developer).recordWorklog(1, 2, 20, ethers.id("late-work")))
      .to.be.revertedWith("Final audit already recorded");
  });

  it("runs the full on-chain workflow and exposes mined receipt metadata", async function () {
    const { contract, admin, developer, client } = await deployFixture();
    const submissions = [];
    const sendAndRecord = async (transaction) => {
      const receipt = await transaction.wait(1);
      expect(receipt.status).to.equal(1);
      expect(receipt.hash).to.equal(transaction.hash);
      expect(receipt.blockNumber).to.be.greaterThan(0);
      submissions.push({ hash: transaction.hash, blockNumber: receipt.blockNumber });
    };

    await sendAndRecord(await contract.createProject(77, developer.address, client.address, ethers.id("project-digest")));
    await sendAndRecord(await contract.connect(developer).recordWorklog(77, 1, 1001, ethers.id("worklog-digest")));
    await sendAndRecord(await contract.connect(developer).recordDeliverable(77, 1, 1001, ethers.id("delivery-digest")));
    await sendAndRecord(await contract.connect(client).confirmDeliverable(77, 1, 1001, true, ethers.id("delivery-digest")));
    await sendAndRecord(await contract.connect(admin).recordStageAudit(77, 1, 1001, true, ethers.id("stage-audit-digest")));
    await sendAndRecord(await contract.connect(admin).recordFinalAudit(77, 1001, true, ethers.id("final-audit-digest")));

    expect(submissions).to.have.length(6);
    expect((await contract.projects(77)).finalApproved).to.equal(true);
  });

  it("locks the project after a rejected final audit without accepting a second decision", async function () {
    const { contract, developer, client } = await deployFixture();
    await createProject(contract, developer, client);
    await prepareApprovedStage(contract, developer, client);
    await contract.recordStageAudit(1, 1, 501, true, ethers.id("stage"));
    await contract.recordFinalAudit(1, 502, false, ethers.id("final-reject"));
    const project = await contract.projects(1);
    expect(project.finalized).to.equal(true);
    expect(project.finalApproved).to.equal(false);
    await expect(contract.recordFinalAudit(1, 503, true, ethers.id("second-final")))
      .to.be.revertedWith("Final audit already recorded");
  });

  it("anchors resumable migration checkpoints and rejects duplicate batches", async function () {
    const { contract, developer, client, outsider } = await deployFixture();
    await createProject(contract, developer, client);
    const checkpoint = ethers.id("source-checkpoint");
    await expect(contract.connect(outsider).recordMigrationCheckpoint(
      1, developer.address, client.address, checkpoint, 2, 1000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, false, false,
    ))
      .to.be.revertedWith("Only admin");
    await expect(contract.recordMigrationCheckpoint(
      1, developer.address, client.address, checkpoint, 2, 1000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, false, false,
    ))
      .to.emit(contract, "MigrationCheckpointRecorded");
    await expect(contract.recordMigrationCheckpoint(
      1, developer.address, client.address, checkpoint, 2, 1000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, false, false,
    ))
      .to.be.revertedWith("Migration already started");
    const digest = ethers.id("historical-worklogs");
    await expect(contract.recordMigrationBatch(1, 0, 1, digest, 3, 10, 30))
      .to.emit(contract, "MigrationBatchRecorded");
    expect((await contract.projects(1)).migrating).to.equal(true);
    await expect(contract.recordMigrationBatch(1, 0, 1, digest, 3, 10, 30))
      .to.be.revertedWith("Batch already recorded");
    await expect(contract.recordMigrationBatch(1, 2, 1, digest, 1, 10, 30))
      .to.be.revertedWith("Batch out of range");
    await contract.recordMigrationBatch(1, 1, 2, ethers.id("historical-deliverables"), 2, 40, 60);
    expect((await contract.projects(1)).migrating).to.equal(false);
  });

  it("completes a checkpoint-only migration without leaving the project locked", async function () {
    const { contract, developer, client } = await deployFixture();
    await contract.recordMigrationCheckpoint(
      42, developer.address, client.address, ethers.id("empty-checkpoint"), 0, 1000,
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, false, false, false,
    );
    expect((await contract.projects(42)).migrating).to.equal(false);
    await expect(contract.connect(developer).recordWorklog(42, 1, 1, ethers.id("new-work")))
      .not.to.be.reverted;
  });

  it("restores the migrated latest delivery so the client can confirm and audit can advance", async function () {
    const { contract, developer, client } = await deployFixture();
    const latestDeliverableId = 9001;
    await contract.recordMigrationCheckpoint(
      42, developer.address, client.address, ethers.id("checkpoint-with-delivery"), 1, 1000,
      3, 2, 4, 1, 20, 9, 8, 2, 1, latestDeliverableId, 3, false, false, false,
    );
    await contract.recordMigrationBatch(42, 0, 1, ethers.id("historical-worklogs"), 20, 10, 900);

    const restoredStage = await contract.stages(42, 3);
    expect(restoredStage.worklogCount).to.equal(4);
    expect(restoredStage.deliverableCount).to.equal(1);
    expect(restoredStage.latestDeliverableId).to.equal(latestDeliverableId);
    expect(await contract.deliverableExists(42, 3, latestDeliverableId)).to.equal(true);

    await contract.connect(client).confirmDeliverable(42, 3, latestDeliverableId, true, ethers.id("migrated-delivery"));
    await contract.recordStageAudit(42, 3, 9901, true, ethers.id("stage-audit"));
    expect((await contract.projects(42)).currentStage).to.equal(4);
  });
});
