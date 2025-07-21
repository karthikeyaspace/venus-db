// /src/database/database_manager.cpp

#include "database/database_manager.h"
#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "common/config.h"
#include "storage/disk_manager.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace venus::database;
using namespace venus::storage;
using namespace venus::buffer;
using namespace venus::catalog;

DatabaseManager::DatabaseManager()
    : db_path_("")
    , is_open_(false)
    , disk_manager_(nullptr)
    , bpm_(nullptr)
    , catalog_(nullptr) { }

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
	// Simulate REPL
	while (true) {
		int option;
		std::cout << std::endl;
		std::cout << "1: Create Database\n"
		          << "2: Use Database\n"
		          << "3: Show Databases\n"
		          << "4: Drop Database\n"
		          << "5: Show master tables\n" // both master_tables and master_columns
		          << "6: CREATE\n"
		          << "7: INSERT\n"
		          << "8: SELECT\n"
		          << "Choose an option: ";
		std::cin >> option;

		switch (option) {
		case 1: {
			std::string db_name;
			std::cout << "Enter database name: ";
			std::cin >> db_name;
			std::cout << "\n\n";

			create_database(db_name);
		} break;
		case 2: {
			std::string db_name;
			std::cout << "Enter database name: ";
			std::cin >> db_name;
			std::cout << "\n\n";

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
			std::cout << "Enter database name:";
			std::cin >> db_name;
			std::cout << "\n\n";

			drop_database(db_name);
		} break;

		case 5: {
			std::cout << "\n\n";
			if (!is_open_) {
				std::cerr << "Database is not initialized" << std::endl;
				break;
			}
			catalog_->ShowMasterTables();
		} break;
		}

		
	}
}

void DatabaseManager::Cleanup() {
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
