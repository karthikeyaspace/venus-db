// /src/database/database_manager.h

/**
 * DatabaseManager for Venus DB
 *
 * Central coordinator that manages the lifecycle and dependencies of all
 * database components. Resolves cyclic dependencies by owning both
 * BufferPoolManager and CatalogManager.
 *
 * Responsibilities:
 *   - Database initialization and cleanup
 *   - Component lifecycle management
 *   - Dependency injection for executors
 *   - Resource coordination
 *
 * DatabaseManager
 * ├── DiskManager (disk I/O)
 * ├── BufferPoolManager (memory management)
 * ├── BufferPoolManager (memory management)
 * ├── CatalogManager (metadata management)
 * └── ExecutionManager (query execution)
 */

#pragma once

#include "common/config.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace venus {
namespace storage {
	class DiskManager;
}
namespace buffer {
	class BufferPoolManager;
}
namespace catalog {
	class CatalogManager;
}
namespace engine {
	class ExecutionEngine;
}
}

namespace venus {
namespace database {
	class DatabaseManager {
	public:
		DatabaseManager();

		~DatabaseManager();

		buffer::BufferPoolManager* GetBufferPoolManager() const { return bpm_; }
		catalog::CatalogManager* GetCatalogManager() const { return catalog_; }
		storage::DiskManager* GetDiskManager() const { return disk_manager_; }
		engine::ExecutionEngine* GetExecutionEngine() const { return executor_; }

		bool IsOpen() const { return is_open_; }
		const std::string& GetDatabasePath() const { return db_path_; }

		void FlushAllPages();
		void Close();
		void Start();

	private:
		std::string db_path_;
		bool is_open_;

		storage::DiskManager* disk_manager_;
		buffer::BufferPoolManager* bpm_;
		catalog::CatalogManager* catalog_;
		engine::ExecutionEngine* executor_;

		void Initialize(const std::string& db_path);
		void Cleanup();

		DISALLOW_COPY_AND_MOVE(DatabaseManager);
	};
} // namespace database
} // namespace venus