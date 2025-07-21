// /src/storage/disk_manager.cpp

#include "storage/disk_manager.h"

namespace venus {
namespace storage {

	// Using OS file handling
	DiskManager::DiskManager(const std::string& db_file)
	    : db_file_name(db_file)
	    , next_page_id_(FIRST_USABLE_PAGE_ID)
	    , file_size_(0) {
		db_io_.open(db_file_name, std::ios::in | std::ios::out | std::ios::binary);
		if (!db_io_) {
			db_io_.clear();
			db_io_.open(db_file_name, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
		}
		
		if (!db_io_.is_open()) {
			throw std::runtime_error("Failed to open database file: " + db_file);
		}
		
		db_io_.seekg(0, std::ios::end);
		file_size_ = db_io_.tellg();
		db_io_.seekg(0, std::ios::beg);
		db_io_.seekp(0, std::ios::beg);  // put pointer
		
		// first 3 pages (0, 1, 2) are reserved for system tables
		if (file_size_ > 0) {
			page_id_t calculated_page_id = static_cast<page_id_t>(file_size_ / PAGE_SIZE);
			next_page_id_ = std::max(calculated_page_id, FIRST_USABLE_PAGE_ID);
		}
	}	DiskManager::~DiskManager() {
		if (db_io_.is_open()) {
			db_io_.close();
		}
	}

	void DiskManager::ReadPage(page_id_t page_id, char* page_data) {
		// allow reading system pages if not allocated
		if (page_id >= next_page_id_ && page_id >= FIRST_USABLE_PAGE_ID) {
			throw std::out_of_range("Page ID out of range");
		}

		long long offset = static_cast<long long>(page_id) * PAGE_SIZE;
		db_io_.seekg(offset);
		if (!db_io_.read(page_data, PAGE_SIZE)) {
			throw std::runtime_error("Failed to read page from disk");
		}
	}

	void DiskManager::WritePage(page_id_t page_id, const char* page_data) {
		if (page_id >= next_page_id_ && page_id >= FIRST_USABLE_PAGE_ID) {
			throw std::out_of_range("Page ID out of range");
		}

		long long offset = static_cast<long long>(page_id) * PAGE_SIZE;
		
		db_io_.clear();  
		db_io_.seekp(offset);
		
		if (!db_io_.good()) {
			throw std::runtime_error("Failed to seek to write position");
		}
		
		if (!db_io_.write(page_data, PAGE_SIZE)) {
			throw std::runtime_error("Failed to write page to disk");
		}

		// Update file size if page extends beyond current file size limit
		if (offset + PAGE_SIZE > file_size_) {
			file_size_ = offset + PAGE_SIZE;
		}

		LOG("Disk Manager: WritePage - Page ID: " + std::to_string(page_id) + ", Offset: " + std::to_string(offset) + ", Size: " + std::to_string(PAGE_SIZE) + ", File Size: " + std::to_string(file_size_));

		db_io_.flush(); // force policy
	}

	page_id_t DiskManager::AllocatePage() {
		page_id_t allocated_page_id = next_page_id_;
		next_page_id_++;
		file_size_ += PAGE_SIZE;
		return allocated_page_id;
	}

	void DiskManager::DeallocatePage(page_id_t page_id) {
		if (page_id >= next_page_id_) {
			throw std::out_of_range("Page ID out of range");
		}

		// mark a page as free (generally removing entire page from disk is done using a background thread)
	}

	uint32_t DiskManager::GetNumberOfPages() const {
		return next_page_id_;
	}

} // namespace storage
} // namespace venus