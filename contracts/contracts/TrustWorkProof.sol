// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

/// @title TrustWorkProof
/// @notice Anchors business digests and enforces the project approval sequence.
/// @dev Business text, file contents and comments stay off-chain. Every business
///      record namespace is keyed by its record type so unrelated IDs cannot collide.
contract TrustWorkProof {
    uint8 public constant WORKLOG = 1;
    uint8 public constant DELIVERABLE = 2;
    uint8 public constant CONFIRMATION = 3;
    uint8 public constant STAGE_AUDIT = 4;
    uint8 public constant FINAL_AUDIT = 5;

    struct Project {
        address developerWallet;
        address clientWallet;
        uint32 currentStage;
        uint32 passedStageCount;
        bool exists;
        bool migrating;
        bool finalized;
        bool finalApproved;
        bool finalAuditRecorded;
    }

    struct Stage {
        uint32 worklogCount;
        uint32 deliverableCount;
        uint256 latestDeliverableId;
        bool clientApproved;
        bool audited;
    }

    address public immutable owner;
    mapping(uint256 => Project) public projects;
    mapping(uint256 => mapping(uint32 => Stage)) public stages;
    mapping(uint256 => mapping(uint8 => mapping(uint256 => bool))) public usedBusinessIds;
    mapping(uint256 => mapping(uint32 => mapping(uint256 => bool))) public deliverableExists;
    mapping(uint256 => mapping(uint32 => mapping(uint256 => bool))) public deliverableApproved;
    mapping(uint256 => bool) public migrationStarted;
    mapping(uint256 => uint32) public migrationBatchCount;
    mapping(uint256 => uint32) public migrationBatchesCompleted;
    mapping(uint256 => mapping(uint32 => bool)) public migrationBatchRecorded;
    mapping(uint256 => bool) private migrationWillFinalize;
    mapping(uint256 => bool) private migrationFinalApproved;
    mapping(uint256 => uint32) public migrationTotalWorklogs;
    mapping(uint256 => uint32) public migrationTotalDeliverables;
    mapping(uint256 => uint32) public migrationTotalConfirmations;
    mapping(uint256 => uint32) public migrationTotalStageAudits;
    mapping(uint256 => uint32) public migrationTotalFinalAudits;

    event ProjectCreated(
        uint256 indexed projectId,
        address indexed developerWallet,
        address indexed clientWallet,
        bytes32 digest,
        address submitter
    );
    event EvidenceAnchored(
        uint8 indexed recordType,
        uint256 indexed projectId,
        uint256 indexed businessId,
        uint256 stageNo,
        bytes32 digest,
        uint8 decision,
        address submitter
    );
    event MigrationCheckpointRecorded(
        uint256 indexed projectId,
        bytes32 checkpointHash,
        uint32 batchCount,
        uint64 sourceCreatedAt,
        uint32 currentStage,
        uint32 passedStageCount,
        uint32 currentStageWorklogCount,
        uint32 currentStageDeliverableCount,
        uint32 totalWorklogCount,
        uint32 totalDeliverableCount,
        uint32 totalConfirmationCount,
        uint32 totalStageAuditCount,
        uint32 totalFinalAuditCount,
        uint256 latestDeliverableId,
        uint32 latestDeliverableStage,
        bool currentStageClientApproved,
        bool finalized,
        bool finalApproved,
        address submitter
    );
    event MigrationBatchRecorded(
        uint256 indexed projectId,
        uint32 indexed batchNo,
        uint8 category,
        bytes32 digest,
        uint32 recordCount,
        uint64 sourceFrom,
        uint64 sourceTo,
        address submitter
    );

    modifier onlyOwner() {
        require(msg.sender == owner, "Only admin");
        _;
    }

    modifier existingProject(uint256 projectId) {
        require(projects[projectId].exists, "Project not found");
        _;
    }

    modifier onlyDeveloper(uint256 projectId) {
        require(projects[projectId].exists, "Project not found");
        require(msg.sender == projects[projectId].developerWallet, "Only developer");
        _;
    }

    modifier onlyClient(uint256 projectId) {
        require(projects[projectId].exists, "Project not found");
        require(msg.sender == projects[projectId].clientWallet, "Only client");
        _;
    }

    constructor() {
        owner = msg.sender;
    }

    function createProject(uint256 projectId, address developerWallet, address clientWallet, bytes32 digest) external onlyOwner {
        require(!projects[projectId].exists, "Project already exists");
        require(developerWallet != address(0) && clientWallet != address(0), "Invalid wallet");
        require(developerWallet != clientWallet, "Wallets must differ");
        require(digest != bytes32(0), "Empty digest");

        projects[projectId] = Project({
            developerWallet: developerWallet,
            clientWallet: clientWallet,
            currentStage: 1,
            passedStageCount: 0,
            exists: true,
            migrating: false,
            finalized: false,
            finalApproved: false,
            finalAuditRecorded: false
        });
        emit ProjectCreated(projectId, developerWallet, clientWallet, digest, msg.sender);
    }

    function recordWorklog(uint256 projectId, uint32 stageNo, uint256 worklogId, bytes32 digest)
        external
        onlyDeveloper(projectId)
    {
        Project storage project = projects[projectId];
        require(!project.migrating, "Migration in progress");
        require(!project.finalAuditRecorded, "Final audit already recorded");
        require(stageNo == project.currentStage, "Not current stage");
        require(digest != bytes32(0), "Empty digest");
        require(!usedBusinessIds[projectId][WORKLOG][worklogId], "Worklog already exists");
        usedBusinessIds[projectId][WORKLOG][worklogId] = true;
        stages[projectId][stageNo].worklogCount++;
        emit EvidenceAnchored(WORKLOG, projectId, worklogId, stageNo, digest, 0, msg.sender);
    }

    function recordDeliverable(uint256 projectId, uint32 stageNo, uint256 deliverableId, bytes32 digest)
        external
        onlyDeveloper(projectId)
    {
        Project storage project = projects[projectId];
        require(!project.migrating, "Migration in progress");
        require(!project.finalAuditRecorded, "Final audit already recorded");
        require(stageNo == project.currentStage, "Not current stage");
        require(digest != bytes32(0), "Empty digest");
        require(!usedBusinessIds[projectId][DELIVERABLE][deliverableId], "Deliverable already exists");
        usedBusinessIds[projectId][DELIVERABLE][deliverableId] = true;
        deliverableExists[projectId][stageNo][deliverableId] = true;
        Stage storage stage = stages[projectId][stageNo];
        stage.deliverableCount++;
        stage.latestDeliverableId = deliverableId;
        stage.clientApproved = false;
        emit EvidenceAnchored(DELIVERABLE, projectId, deliverableId, stageNo, digest, 0, msg.sender);
    }

    function confirmDeliverable(
        uint256 projectId,
        uint32 stageNo,
        uint256 deliverableId,
        bool approved,
        bytes32 digest
    ) external onlyClient(projectId) {
        Project storage project = projects[projectId];
        require(!project.migrating, "Migration in progress");
        require(!project.finalAuditRecorded, "Final audit already recorded");
        require(stageNo == project.currentStage, "Not current stage");
        require(deliverableExists[projectId][stageNo][deliverableId], "Deliverable not found");
        require(stages[projectId][stageNo].latestDeliverableId == deliverableId, "Not latest deliverable");
        require(digest != bytes32(0), "Empty digest");
        stages[projectId][stageNo].clientApproved = approved;
        deliverableApproved[projectId][stageNo][deliverableId] = approved;
        emit EvidenceAnchored(CONFIRMATION, projectId, deliverableId, stageNo, digest, approved ? 1 : 2, msg.sender);
    }

    function recordStageAudit(
        uint256 projectId,
        uint32 stageNo,
        uint256 auditId,
        bool approved,
        bytes32 digest
    ) external onlyOwner existingProject(projectId) {
        Project storage project = projects[projectId];
        Stage storage stage = stages[projectId][stageNo];
        require(!project.migrating, "Migration in progress");
        require(!project.finalAuditRecorded, "Final audit already recorded");
        require(stageNo == project.currentStage, "Not current stage");
        require(stage.worklogCount > 0 && stage.deliverableCount > 0, "Stage has no evidence");
        require(stage.clientApproved, "Client approval required");
        require(!stage.audited, "Stage already audited");
        require(deliverableApproved[projectId][stageNo][stage.latestDeliverableId], "Latest delivery not approved");
        require(!usedBusinessIds[projectId][STAGE_AUDIT][auditId], "Audit already exists");
        require(digest != bytes32(0), "Empty digest");
        usedBusinessIds[projectId][STAGE_AUDIT][auditId] = true;
        if (approved) {
            stage.audited = true;
            project.passedStageCount++;
            project.currentStage++;
        } else {
            stage.clientApproved = false;
        }
        emit EvidenceAnchored(STAGE_AUDIT, projectId, auditId, stageNo, digest, approved ? 1 : 2, msg.sender);
    }

    function recordFinalAudit(uint256 projectId, uint256 auditId, bool approved, bytes32 digest)
        external
        onlyOwner
        existingProject(projectId)
    {
        Project storage project = projects[projectId];
        Stage storage openStage = stages[projectId][project.currentStage];
        require(!project.migrating, "Migration in progress");
        require(!project.finalAuditRecorded, "Final audit already recorded");
        require(project.passedStageCount > 0, "No completed stage");
        require(openStage.worklogCount == 0 && openStage.deliverableCount == 0, "Current stage is not empty");
        require(!openStage.clientApproved, "Stage approval pending");
        require(!usedBusinessIds[projectId][FINAL_AUDIT][auditId], "Audit already exists");
        require(digest != bytes32(0), "Empty digest");
        usedBusinessIds[projectId][FINAL_AUDIT][auditId] = true;
        project.finalized = true;
        project.finalApproved = approved;
        project.finalAuditRecorded = true;
        emit EvidenceAnchored(FINAL_AUDIT, projectId, auditId, project.currentStage - 1, digest, approved ? 1 : 2, msg.sender);
    }

    function recordMigrationCheckpoint(
        uint256 projectId,
        address developerWallet,
        address clientWallet,
        bytes32 checkpointHash,
        uint32 batchCount,
        uint64 sourceCreatedAt,
        uint32 currentStage,
        uint32 passedStageCount,
        uint32 currentStageWorklogCount,
        uint32 currentStageDeliverableCount,
        uint32 totalWorklogCount,
        uint32 totalDeliverableCount,
        uint32 totalConfirmationCount,
        uint32 totalStageAuditCount,
        uint32 totalFinalAuditCount,
        uint256 latestDeliverableId,
        uint32 latestDeliverableStage,
        bool currentStageClientApproved,
        bool finalized,
        bool finalApproved
    ) external onlyOwner {
        require(!migrationStarted[projectId], "Migration already started");
        require(checkpointHash != bytes32(0), "Empty digest");
        require(developerWallet != address(0) && clientWallet != address(0), "Invalid wallet");
        require(developerWallet != clientWallet, "Wallets must differ");
        require(currentStage > 0 && passedStageCount < currentStage, "Invalid project checkpoint");
        require(
            (latestDeliverableId == 0 && latestDeliverableStage == 0) ||
                (latestDeliverableId > 0 && latestDeliverableStage == currentStage),
            "Invalid latest deliverable"
        );
        require(
            (currentStageDeliverableCount == 0 && latestDeliverableId == 0) ||
                (currentStageDeliverableCount > 0 && latestDeliverableId > 0),
            "Invalid deliverable checkpoint"
        );
        require(!currentStageClientApproved || latestDeliverableId > 0, "Approval without deliverable");
        if (!projects[projectId].exists) {
            projects[projectId] = Project({
                developerWallet: developerWallet,
                clientWallet: clientWallet,
                currentStage: currentStage,
                passedStageCount: passedStageCount,
                exists: true,
                migrating: true,
                finalized: false,
                finalApproved: false,
                finalAuditRecorded: false
            });
            stages[projectId][currentStage] = Stage({
                worklogCount: currentStageWorklogCount,
                deliverableCount: currentStageDeliverableCount,
                latestDeliverableId: latestDeliverableId,
                clientApproved: currentStageClientApproved,
                audited: false
            });
            if (latestDeliverableId > 0 && latestDeliverableStage > 0) {
                deliverableExists[projectId][latestDeliverableStage][latestDeliverableId] = true;
                deliverableApproved[projectId][latestDeliverableStage][latestDeliverableId] = currentStageClientApproved;
            }
            emit ProjectCreated(projectId, developerWallet, clientWallet, checkpointHash, msg.sender);
        } else {
            require(projects[projectId].developerWallet == developerWallet, "Developer wallet mismatch");
            require(projects[projectId].clientWallet == clientWallet, "Client wallet mismatch");
            projects[projectId].currentStage = currentStage;
            projects[projectId].passedStageCount = passedStageCount;
            projects[projectId].migrating = true;
            stages[projectId][currentStage] = Stage({
                worklogCount: currentStageWorklogCount,
                deliverableCount: currentStageDeliverableCount,
                latestDeliverableId: latestDeliverableId,
                clientApproved: currentStageClientApproved,
                audited: false
            });
            if (latestDeliverableId > 0 && latestDeliverableStage > 0) {
                deliverableExists[projectId][latestDeliverableStage][latestDeliverableId] = true;
                deliverableApproved[projectId][latestDeliverableStage][latestDeliverableId] = currentStageClientApproved;
            }
        }
        migrationStarted[projectId] = true;
        migrationBatchCount[projectId] = batchCount;
        migrationTotalWorklogs[projectId] = totalWorklogCount;
        migrationTotalDeliverables[projectId] = totalDeliverableCount;
        migrationTotalConfirmations[projectId] = totalConfirmationCount;
        migrationTotalStageAudits[projectId] = totalStageAuditCount;
        migrationTotalFinalAudits[projectId] = totalFinalAuditCount;
        migrationWillFinalize[projectId] = finalized;
        migrationFinalApproved[projectId] = finalApproved;
        if (batchCount == 0) {
            Project storage project = projects[projectId];
            project.migrating = false;
            project.finalized = finalized;
            project.finalApproved = finalApproved;
            project.finalAuditRecorded = finalized;
        }
        emit MigrationCheckpointRecorded(
            projectId,
            checkpointHash,
            batchCount,
            sourceCreatedAt,
            currentStage,
            passedStageCount,
            currentStageWorklogCount,
            currentStageDeliverableCount,
            totalWorklogCount,
            totalDeliverableCount,
            totalConfirmationCount,
            totalStageAuditCount,
            totalFinalAuditCount,
            latestDeliverableId,
            latestDeliverableStage,
            currentStageClientApproved,
            finalized,
            finalApproved,
            msg.sender
        );
    }

    function recordMigrationBatch(
        uint256 projectId,
        uint32 batchNo,
        uint8 category,
        bytes32 digest,
        uint32 recordCount,
        uint64 sourceFrom,
        uint64 sourceTo
    ) external onlyOwner existingProject(projectId) {
        Project storage project = projects[projectId];
        require(migrationStarted[projectId], "Migration not started");
        require(batchNo < migrationBatchCount[projectId], "Batch out of range");
        require(!migrationBatchRecorded[projectId][batchNo], "Batch already recorded");
        require(category >= 1 && category <= 6, "Invalid category");
        require(digest != bytes32(0) && recordCount > 0, "Empty batch");
        require(sourceTo >= sourceFrom, "Invalid source range");
        migrationBatchRecorded[projectId][batchNo] = true;
        migrationBatchesCompleted[projectId]++;
        if (migrationBatchesCompleted[projectId] == migrationBatchCount[projectId]) {
            project.migrating = false;
            project.finalized = migrationWillFinalize[projectId];
            project.finalApproved = migrationFinalApproved[projectId];
            project.finalAuditRecorded = migrationWillFinalize[projectId];
        }
        emit MigrationBatchRecorded(projectId, batchNo, category, digest, recordCount, sourceFrom, sourceTo, msg.sender);
    }
}
