// /src/database/database_manager.cpp

#include "database/database_manager.h"

using namespace venus::database;
using namespace venus::storage;
using namespace venus::buffer;
using namespace venus::catalog;
using namespace venus::engine;
using namespace venus::network;

DatabaseManager::DatabaseManager()
    : db_path_("")
    , is_open_(false)
    , disk_manager_(nullptr)
    , bpm_(nullptr)
    , catalog_(nullptr)
    , network_(nullptr) {
	InitializeExecutor();
}

DatabaseManager::~DatabaseManager() {
	if (is_open_) {
		Close();
	}
}

void DatabaseManager::InitializeExecutor() {
	executor_ = new ExecutionEngine();
	network_ = new NetworkManager();

	executor_->InitializeCallback(
	    [this](const std::string& db_name) {
		    this->Initialize(db_name);
	    });

	executor_->SetStopDBCallback(
	    [this]() {
		    if (is_open_) {
			    this->Close();
		    }
		    network_->Stop();
	    });

	network_->SetExecuteCallback([this](const std::string& q) -> executor::ResultSet {
		return executor_->Execute(q);
	});
}

// Initialize() and InitializeExecutor() are cyclic
// I myself am embarrassed writing this part of code, it works though

void DatabaseManager::Initialize(const std::string& db_name) {
	if (db_name.empty()) {
		throw std::invalid_argument("Database name cannot be empty");
	}

	if (is_open_) {
		// situation where user is using a db, and greedily switches to another db (use command)
		// need to carefully reinitialize all the components especially executor
		Close();
		InitializeExecutor();
	}

	// full path: DATABASE_DIRECTORY + "/" + db_name + ".db"
	db_path_ = std::string(venus::DATABASE_DIRECTORY) + "/" + db_name + ".db";

	disk_manager_ = new DiskManager(db_path_);
	if (!disk_manager_) {
		throw std::runtime_error("DatabaseManager: Failed to create DiskManager");
	}

	bpm_ = new BufferPoolManager(disk_manager_);
	if (!bpm_) {
		throw std::runtime_error("DatabaseManager: Failed to create BufferPoolManager");
	}

	catalog_ = new CatalogManager(bpm_);
	if (!catalog_) {
		throw std::runtime_error("DatabaseManager: Failed to create CatalogManager");
	}

	if (!executor_) {
		throw std::runtime_error("DatabaseManager: ExecutionEngine is not initialized");
	}

	executor_->SetLocalContext(bpm_, catalog_);

	is_open_ = true;
}

void DatabaseManager::Start() {
	network_->Start();
}

void DatabaseManager::Cleanup() {
	if (executor_) {
		delete executor_;
		executor_ = nullptr;
	}

	if (catalog_) {
		delete catalog_;
		catalog_ = nullptr;
	}

	if (bpm_) {
		delete bpm_;
		bpm_ = nullptr;
	}

	if (disk_manager_) {
		delete disk_manager_;
		disk_manager_ = nullptr;
	}

	is_open_ = false;
}

void DatabaseManager::FlushAllPages() {
	if (!is_open_ || !bpm_) {
		throw std::runtime_error("Database is not open");
	}

	if (!bpm_->FlushAllPages()) {
		throw std::runtime_error("Failed to flush all pages to disk");
	}
}

void DatabaseManager::Close() {
	if (!is_open_) {
		return;
	}

	try {
		FlushAllPages();
		Cleanup();
	} catch (const std::exception& e) {
		std::cerr << "Error during database close: " << e.what() << std::endl;
	}
}
