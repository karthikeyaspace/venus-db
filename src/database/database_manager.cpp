// /src/database/database_manager.cpp

#include "database/database_manager.h"
#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "common/config.h"
#include "executor/executor.h"
#include "storage/disk_manager.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

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
    , catalog_(nullptr)
    , executor_(nullptr) { }

DatabaseManager::~DatabaseManager() {
	if (is_open_) {
		Close();
	}
}

void DatabaseManager::Initialize(const std::string& db_name) {
	if (db_name.empty()) {
		throw std::invalid_argument("Database name cannot be empty");
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

	executor_ = new ExecutionEngine(bpm_, catalog_);
	if (!executor_) {
		throw std::runtime_error("Failed to create ExecutionEngine");
	}

	std::cout << "DatabaseManager: Database initialized successfully" << std::endl;
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
	// Database Management REPL
	while (true) {
		int option;
		std::cout << std::endl;
		std::cout << "=== Venus DB Management Interface ===" << std::endl;
		std::cout << "1: Create Database" << std::endl;
		std::cout << "2: Use Database" << std::endl;
		std::cout << "3: Show Databases" << std::endl;
		std::cout << "4: Drop Database" << std::endl;
		std::cout << "5: Execute Query (requires open database)" << std::endl;
		std::cout << "6: Show Master Tables (requires open database)" << std::endl;
		std::cout << "0: Exit" << std::endl;
		std::cout << "Choose an option: ";

		std::cin >> option;
		std::cout << std::endl;

		switch (option) {
		case 0:
			std::cout << "Exiting Venus DB..." << std::endl;
			return;
		case 1: {
			std::string db_name;
			std::cout << "Enter database name: ";
			std::cin >> db_name;
			std::cout << std::endl;

			try {
				create_database(db_name);
			} catch (const std::exception& e) {
				std::cerr << "Error creating database: " << e.what() << std::endl;
			}
		} break;
		case 2: {
			std::string db_name;
			std::cout << "Enter database name: ";
			std::cin >> db_name;
			std::cout << std::endl;

			try {
				Initialize(db_name);
				is_open_ = true;
				std::cout << "Database " << db_name << " is now open." << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "Error opening database: " << e.what() << std::endl;
			}
		} break;
		case 3:
			show_databases();
			break;
		case 4: {
			std::string db_name;
			std::cout << "Enter database name: ";
			std::cin >> db_name;
			std::cout << std::endl;

			try {
				drop_database(db_name);
			} catch (const std::exception& e) {
				std::cerr << "Error dropping database: " << e.what() << std::endl;
			}
		} break;
		case 5: {
			if (!is_open_) {
				std::cerr << "No database is currently open. Please use option 2 to open a database first." << std::endl;
				break;
			}
			std::string query;
			std::cout << "> ";
			std::cin.ignore(); // Clear the newline left by previous cin >>
			std::getline(std::cin, query);
			executor_->ExecuteQuery(query);
		} break;
		case 6: {
			if (!is_open_) {
				std::cerr << "No database is currently open. Please use option 2 to open a database first." << std::endl;
				break;
			}
			catalog_->ShowMasterTables();
		} break;
		
	default:
		std::cout << "Invalid option. Please try again." << std::endl;
		break;
	}
}
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
