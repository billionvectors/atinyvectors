ALTER TABLE VectorMetadata
ADD COLUMN versionId INTEGER NOT NULL DEFAULT 0;

CREATE INDEX IF NOT EXISTS idx_vectormetadata_versionId_key_value ON VectorMetadata(versionId, key, value);
CREATE INDEX IF NOT EXISTS idx_vectormetadata_vectorId_key_value ON VectorMetadata(vectorId, key, value);
