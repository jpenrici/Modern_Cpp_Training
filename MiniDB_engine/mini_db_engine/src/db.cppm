// Primary module interface unit -- re-exports every partition as the
// single public `db` module. Consumers only ever write `import db;`.
export module db;

export import :core;
export import :storage;
export import :index;
export import :wal;
export import :concurrency;
export import :exec;
export import :engine;
