// /src/buffer/buffer_pool.cpp

#include "buffer/buffer_pool.h"

namespace venus {
namespace buffer {

	Page* BufferPoolManager::FetchPage(page_id_t page_id) {
		if (page_id == INVALID_PAGE_ID) {
			return nullptr; // Invalid page ID
		}

		// If page in bufferpool
		auto it = pages_.find(page_id);
		if (it != pages_.end()) {
			lru_list_.remove(it->second);
			lru_list_.push_front(it->second);
			return it->second;
		}

		// Page not in buffer pool, and pool is full, evict a page from bp using lru
		if (lru_list_.size() >= pool_size_) {
			Page* victim = lru_list_.back();
			lru_list_.pop_back();
			pages_.erase(victim->GetPageId());

			// if victim page is dirty, write to disk
			if (victim->IsDirty()) {
				disk_manager_->WritePage(victim->GetPageId(), victim->GetData());
			}

			// reset victim page
			victim->NewPage(INVALID_PAGE_ID, PageType::INVALID_PAGE);
			delete[] victim->GetData();
			delete victim;
		}

		// create new page
		Page* new_page = new Page(new char[PAGE_SIZE]);
		new_page->NewPage(page_id, PageType::TABLE_PAGE);
		disk_manager_->ReadPage(page_id, new_page->GetData());
		new_page->GetHeader()->page_id = page_id;

		pages_[page_id] = new_page;
		lru_list_.push_front(new_page);

		return new_page;
	}

	// Generally, Pining and Unpinning involves a `pin_count` to track how many entities are using the page.
	// For Venus, we just set the page to dirty
	bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
		auto it = pages_.find(page_id);
		if (it == pages_.end()) {
			return false;
		}

		Page* page = it->second;
		if (is_dirty) {
			page->SetDirty(true);
		}
		return true;
	}

	// Flush page to disk if dirty
	bool BufferPoolManager::FlushPage(page_id_t page_id) {
		auto it = pages_.find(page_id);
		if (it == pages_.end()) {
			return false; // Page not found in buffer pool
		}

		Page* page = it->second;
		if (page->IsDirty()) {
			disk_manager_->WritePage(page_id, page->GetData());
			page->SetDirty(false); // Reset dirty flag after flushing
		}
		return true;
	}

	Page* BufferPoolManager::NewPage() {
		page_id_t new_page_id = disk_manager_->AllocatePage();
		Page* new_page = new Page(new char[PAGE_SIZE]);

		new_page->NewPage(new_page_id, PageType::TABLE_PAGE);

		disk_manager_->WritePage(new_page_id, new_page->GetData());

		pages_[new_page_id] = new_page;
		lru_list_.push_front(new_page);

		return new_page;
	}

	bool BufferPoolManager::DeletePage(page_id_t page_id) {
		auto it = pages_.find(page_id);
		if (it == pages_.end()) {
			return false;
		}

		Page* page = it->second;
		pages_.erase(page_id);
		lru_list_.remove(page);

		delete[] page->GetData();
		delete page;

		disk_manager_->DeallocatePage(page_id);

		return true;
	}

	BufferPoolManager::~BufferPoolManager() {
		for (auto& pair : pages_) {
			Page* page = pair.second;
			if (page->IsDirty()) {
				disk_manager_->WritePage(page->GetPageId(), page->GetData());
			}

			delete[] page->GetData();
			delete page;
		}
		pages_.clear();
		lru_list_.clear();
	}
}
}