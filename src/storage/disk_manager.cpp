// /src/storage/disk_manager.cpp

#include "storage/disk_manager.h"

namespace minidb {
namespace storage {

	// Using OS file handling
	DiskManager::DiskManager(const std::string& db_file)
	    : db_file_name(db_file)
	    , next_page_id_(0)
	    , file_size_(0) {
		db_io_.open(db_file_name, std::ios::in | std::ios::out | std::ios::binary);
		if (!db_io_) {
			db_io_.clear();
			db_io_.open(db_file_name, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
		}
		db_io_.seekg(0, std::ios::end);
		file_size_ = db_io_.tellg();
		db_io_.seekg(0, std::ios::beg);
	}

	void DiskManager::ReadPage(page_id_t page_id, char* page_data) {
		if (page_id >= next_page_id_) {
			throw std::out_of_range("Page ID out of range");
		}

		long long offset = static_cast<long long>(page_id) * PAGE_SIZE;
		db_io_.seekg(offset);
		if (!db_io_.read(page_data, PAGE_SIZE)) {
			throw std::runtime_error("Failed to read page from disk");
		}

		db_io_.close();
	}

	void DiskManager::WritePage(page_id_t page_id, const char* page_data) {
		if (page_id >= next_page_id_) {
			throw std::out_of_range("Page ID out of range");
		}

		long long offset = static_cast<long long>(page_id) * PAGE_SIZE;
		db_io_.seekp(offset);
		if (!db_io_.write(page_data, PAGE_SIZE)) {
			throw std::runtime_error("Failed to write page to disk");
		}

		// Update file size if page extends beyond current file size limit
		if (offset + PAGE_SIZE > file_size_) {
			file_size_ = offset + PAGE_SIZE;
		}

		db_io_.flush(); // force policy

		db_io_.close();
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
} // namespace minidb