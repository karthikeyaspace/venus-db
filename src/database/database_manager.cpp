// /src/database/database_manager.cpp

#include "database/database_manager.h"
#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "common/config.h"
#include "engine/execution_engine.h"
#include "storage/disk_manager.h"

using namespace venus::database;
using namespace venus::storage;
using namespace venus::buffer;
using namespace venus::catalog;
using namespace venus::executor;

DatabaseManager::DatabaseManager()
    : db_path_("")
    , is_open_(false)
    , disk_manager_(nullptr)
    , bpm_(nullptr)
    , catalog_(nullptr) {
	executor_ = new ExecutionEngine();
	if (!executor_) {
		throw std::runtime_error("Database Error: Failed to create ExecutionEngine");
	}
	executor_->InitializeCallback(
	    [this](const std::string& db_name) {
		    this->Initialize(db_name);
	    });

	executor_->SetStopDBCallback(
	    [this]() {
		    if (is_open_) {
			    this->Close();
		    }
	    });
}

DatabaseManager::~DatabaseManager() {
	if (is_open_) {
		Close();
	}
}

void DatabaseManager::Initialize(const std::string& db_name) {
	if (db_name.empty()) {
		throw std::invalid_argument("Database name cannot be empty");
	}

	if (is_open_) {
		Close();
	}

	// Build full path: db_dir + "/" + db_name + ".db"
	db_path_ = std::string(venus::db_dir) + "/" + db_name + ".db";

	std::cout << "DatabaseManager: Opening database file: " << db_path_ << std::endl;

	disk_manager_ = new DiskManager(db_path_);
	if (!disk_manager_) {
		throw std::runtime_error("Failed to create DiskManager");
	}

	bpm_ = new BufferPoolManager(disk_manager_);
	if (!bpm_) {
		throw std::runtime_error("Failed to create BufferPoolManager");
	}

	catalog_ = new CatalogManager(bpm_);
	if (!catalog_) {
		throw std::runtime_error("Failed to create CatalogManager");
	}

	if (!executor_) {
		throw std::runtime_error("ExecutionEngine is not initialized");
	}

	executor_->SetLocalContext(bpm_, catalog_);

	is_open_ = true;
	std::cout << "DatabaseManager: Database '" << db_name << "' opened successfully" << std::endl;
}

void create_database(const std::string& db_name) {
	std::string db_path = std::string(venus::db_dir) + "/" + db_name + ".db";

	std::filesystem::create_directories(venus::db_dir);

	std::ofstream db_file(db_path);
	if (!db_file) {
		throw std::runtime_error("Failed to create database file: " + db_path);
	}

	std::cout << "Created database: " << db_path << std::endl;

	db_file.close();
}

void show_databases() {
	for (const auto& entry : std::filesystem::directory_iterator(venus::db_dir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".db") {
			std::cout << "- " << entry.path().stem().string() << std::endl;
		}
	}
}

void drop_database(const std::string& db_name) {
	std::string db_path = std::string(venus::db_dir) + "/" + db_name + ".db";

	if (std::filesystem::exists(db_path)) {
		std::filesystem::remove(db_path);
		std::cout << "Dropped database: " << db_name << std::endl;
	} else {
		std::cerr << "Database does not exist: " << db_name << std::endl;
	}
}

void DatabaseManager::Start() {
	executor_->StartRepl();
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

	std::cout << "DatabaseManager: Flushed all pages to disk" << std::endl;
}

void DatabaseManager::Close() {
	if (!is_open_) {
		return;
	}

	try {
		FlushAllPages();

		Cleanup();

		std::cout << "DatabaseManager: Database closed successfully" << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Error during database close: " << e.what() << std::endl;
		Cleanup();
	}
}
