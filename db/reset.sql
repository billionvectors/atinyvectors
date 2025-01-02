-- Drop all existing tables
DROP TABLE IF EXISTS BM25;
DROP TABLE IF EXISTS RbacToken;
DROP TABLE IF EXISTS Snapshot;
DROP TABLE IF EXISTS Space;
DROP TABLE IF EXISTS Version;
DROP TABLE IF EXISTS Vector;
DROP TABLE IF EXISTS VectorValue;
DROP TABLE IF EXISTS VectorIndex;
DROP TABLE IF EXISTS VectorMetadata;
DROP TABLE IF EXISTS info;

-- Recreate all tables and indexes
CREATE TABLE info (
    version TEXT NOT NULL,
    dbversion INTEGER NOT NULL,
    created_time_utc INTEGER NOT NULL,
    updated_time_utc INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS BM25 (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    vectorId INTEGER NOT NULL,
    doc TEXT NOT NULL,
    docLength INTEGER NOT NULL,
    tokens TEXT NOT NULL
);

CREATE TABLE RbacToken (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    token TEXT NOT NULL,
    space_id INTEGER NOT NULL,
    system_permission INTEGER NOT NULL,
    space_permission INTEGER NOT NULL,
    version_permission INTEGER NOT NULL,
    vector_permission INTEGER NOT NULL,
    search_permission INTEGER NOT NULL,
    snapshot_permission INTEGER NOT NULL,
    security_permission INTEGER NOT NULL,
    keyvalue_permission INTEGER NOT NULL,
    expire_time_utc INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_rbac_expire_time_utc ON RbacToken(expire_time_utc);
CREATE INDEX IF NOT EXISTS idx_rbac_token_expire ON RbacToken(token, expire_time_utc);

CREATE TABLE Snapshot (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    request_json TEXT NOT NULL,
    file_name TEXT NOT NULL,
    created_time_utc INTEGER
);

CREATE TABLE Space (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    created_time_utc INTEGER,
    updated_time_utc INTEGER
);

CREATE INDEX IF NOT EXISTS idx_space_name ON Space(name);

CREATE TABLE Version (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    spaceId INTEGER NOT NULL,
    unique_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    description TEXT,
    tag TEXT,
    created_time_utc INTEGER,
    updated_time_utc INTEGER,
    is_default BOOLEAN DEFAULT 0,
    FOREIGN KEY(spaceId) REFERENCES Space(id)
);

CREATE INDEX IF NOT EXISTS idx_version_unique_id ON Version(unique_id);
CREATE INDEX IF NOT EXISTS idx_version_space_id ON Version(spaceId);

CREATE TABLE Vector (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    versionId INTEGER NOT NULL,
    unique_id INTEGER NOT NULL,
    type INTEGER,
    deleted BOOLEAN DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_vector_unique_id ON Vector(unique_id);

CREATE TABLE VectorValue (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    vectorId INTEGER NOT NULL,
    vectorIndexId INTEGER NOT NULL,
    type INTEGER,
    data BLOB,
    FOREIGN KEY(vectorId) REFERENCES Vector(id),
    FOREIGN KEY(vectorIndexId) REFERENCES VectorIndex(id)
);

CREATE TABLE VectorIndex (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    versionId INTEGER NOT NULL,
    vectorValueType INTEGER,
    name TEXT NOT NULL,
    metricType INTEGER,
    dimension INTEGER,
    hnswConfigJson TEXT,
    quantizationConfigJson TEXT,
    create_date_utc INTEGER,
    updated_time_utc INTEGER,
    is_default BOOLEAN DEFAULT 0,
    FOREIGN KEY(versionId) REFERENCES Version(id)
);

CREATE INDEX IF NOT EXISTS idx_vectorindex_version_id ON VectorIndex(versionId);

CREATE TABLE VectorMetadata (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    vectorId INTEGER NOT NULL,
    key TEXT NOT NULL,
    value TEXT
);
