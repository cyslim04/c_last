// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

contract TrustWorkProof {
    struct Project {
        uint256 projectId;
        address developerWallet;
        address clientWallet;
        bool exists;
    }

    struct EvidenceRecord {
        uint256 projectId;
        uint256 recordId;
        bytes32 contentHash;
        uint64 timestamp;
        address submitter;
        string recordType;
        bool confirmed;
    }

    mapping(uint256 => Project) public projects;
    mapping(uint256 => mapping(uint256 => EvidenceRecord)) private evidenceRecords;
    mapping(uint256 => mapping(uint256 => bool)) public usedRecordIds;

    event ProjectCreated(uint256 indexed projectId, address indexed developerWallet, address indexed clientWallet);
    event WorklogRecorded(uint256 indexed projectId, uint256 indexed worklogId, bytes32 worklogHash, uint64 periodStart, uint64 periodEnd, address submitter);
    event DeliverableRecorded(uint256 indexed projectId, uint256 indexed deliverableId, bytes32 fileHash, string version, bytes32 fileNameHash, address submitter);
    event DeliverableConfirmed(uint256 indexed projectId, uint256 indexed deliverableId, bool approved, bytes32 commentHash, address submitter);

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

    function createProject(uint256 projectId, address developerWallet, address clientWallet) external {
        require(!projects[projectId].exists, "Project already exists");
        require(developerWallet != address(0) && clientWallet != address(0), "Invalid wallet");

        projects[projectId] = Project({
            projectId: projectId,
            developerWallet: developerWallet,
            clientWallet: clientWallet,
            exists: true
        });

        emit ProjectCreated(projectId, developerWallet, clientWallet);
    }

    function recordWorklog(
        uint256 projectId,
        uint256 worklogId,
        bytes32 worklogHash,
        uint64 periodStart,
        uint64 periodEnd
    ) external onlyDeveloper(projectId) {
        require(!usedRecordIds[projectId][worklogId], "Record already exists");
        require(periodEnd >= periodStart, "Invalid period");

        usedRecordIds[projectId][worklogId] = true;
        evidenceRecords[projectId][worklogId] = EvidenceRecord({
            projectId: projectId,
            recordId: worklogId,
            contentHash: worklogHash,
            timestamp: uint64(block.timestamp),
            submitter: msg.sender,
            recordType: "worklog",
            confirmed: true
        });

        emit WorklogRecorded(projectId, worklogId, worklogHash, periodStart, periodEnd, msg.sender);
    }

    function recordDeliverable(
        uint256 projectId,
        uint256 deliverableId,
        bytes32 fileHash,
        string calldata version,
        bytes32 fileNameHash
    ) external onlyDeveloper(projectId) {
        require(!usedRecordIds[projectId][deliverableId], "Record already exists");

        usedRecordIds[projectId][deliverableId] = true;
        evidenceRecords[projectId][deliverableId] = EvidenceRecord({
            projectId: projectId,
            recordId: deliverableId,
            contentHash: fileHash,
            timestamp: uint64(block.timestamp),
            submitter: msg.sender,
            recordType: version,
            confirmed: false
        });

        emit DeliverableRecorded(projectId, deliverableId, fileHash, version, fileNameHash, msg.sender);
    }

    function confirmDeliverable(
        uint256 projectId,
        uint256 deliverableId,
        bool approved,
        bytes32 commentHash
    ) external onlyClient(projectId) {
        require(usedRecordIds[projectId][deliverableId], "Deliverable not found");
        evidenceRecords[projectId][deliverableId].confirmed = approved;
        emit DeliverableConfirmed(projectId, deliverableId, approved, commentHash, msg.sender);
    }

    function getEvidence(uint256 projectId, uint256 recordId) external view returns (EvidenceRecord memory) {
        require(projects[projectId].exists, "Project not found");
        require(usedRecordIds[projectId][recordId], "Record not found");
        return evidenceRecords[projectId][recordId];
    }
}
