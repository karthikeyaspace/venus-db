// /src/database/database_manager.cpp

#include "database/database_manager.h"
#include "buffer/buffer_pool.h"
#include "catalog/catalog.h"
#include "common/config.h"
#include "storage/disk_manager.h"

#include <iostream>
#include <stdexcept>

using namespace venus::database;
using namespace venus::storage;
using namespace venus::buffer;
using namespace venus::catalog;

DatabaseManager::DatabaseManager(const std::string& db_path)
    : db_path_(db_path)
    , is_open_(false)
    , disk_manager_(nullptr)
    , bpm_(nullptr)
    , catalog_(nullptr) {

	try {
		Initialize();
		is_open_ = true;
		std::cout << "DatabaseManager: Successfully opened database: " << db_path_ << std::endl;
	} catch (const std::exception& e) {
		Cleanup();
		throw std::runtime_error("Failed to open database '" + db_path_ + "': " + e.what());
	}
}

DatabaseManager::~DatabaseManager() {
	if (is_open_) {
		Close();
	}
}

void DatabaseManager::Initialize() {
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
