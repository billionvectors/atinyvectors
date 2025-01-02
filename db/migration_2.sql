-- Add index on spaceId in Version table
CREATE INDEX IF NOT EXISTS idx_version_space_id ON Version(spaceId);
