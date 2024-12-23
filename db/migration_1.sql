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
