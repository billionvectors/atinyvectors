
CREATE TABLE IF NOT EXISTS billionvectors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    created_time_utc INTEGER NOT NULL,
    updated_time_utc INTEGER NOT NULL
);