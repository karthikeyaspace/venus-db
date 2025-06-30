// /src/buffer/buffer_pool.cpp

#include "buffer/buffer_pool.h"

namespace minidb {
namespace buffer {

	Page* BufferPoolManager::FetchPage(page_id_t page_id) {
		if (page_id == INVALID_PAGE_ID) {
			return nullptr; // Invalid page ID
		}

		auto it = pages_.find(page_id);
		if (it != pages_.end()) {
			lru_list_.remove(it->second);
			lru_list_.push_front(it->second);
			return it->second;
		}

		// evict if lru is full
		if (lru_list_.size() >= pool_size_) {
			Page* victim = lru_list_.back();
			lru_list_.pop_back();
			pages_.erase(victim->GetPageId());

			// if victim is dirty, write to disk
			if (victim->IsDirty()) {
				disk_manager_->WritePage(victim->GetPageId(), victim->GetData());
			}
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

	bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
		if (pages_.find(page_id) == pages_.end()) {
			return false;
		}

		Page* page = pages_[page_id];
		if (is_dirty) {
			page->GetHeader()->is_dirty = true;
		}
		return true;
	}

	// Flush page to disk if dirty
	bool BufferPoolManager::FlushPage(page_id_t page_id) {
		if (pages_.find(page_id) == pages_.end()) {
			return false;
		}

		Page* page = pages_[page_id];
		if (page->IsDirty()) {
			disk_manager_->WritePage(page_id, page->GetData());
			page->SetDirty(false);
			return true;
		}
		return true; // page persisted
	}

	page_id_t BufferPoolManager::NewPage() {
		page_id_t new_page_id = disk_manager_->AllocatePage();
		Page* new_page = new Page(new char[PAGE_SIZE]);
		new_page->NewPage(new_page_id, PageType::TABLE_PAGE);

		disk_manager_->WritePage(new_page_id, new_page->GetData());
		pages_[new_page_id] = new_page;

		return new_page_id;
	}

	bool BufferPoolManager::DeletePage(page_id_t page_id) {
		if (pages_.find(page_id) == pages_.end()) {
			return false;
		}

		Page* page = pages_[page_id];
		pages_.erase(page_id);
		lru_list_.remove(page);
		delete[] page->GetData();
		delete page;

		return true;
	}

	BufferPoolManager::~BufferPoolManager() {
		for (auto& pair : pages_) {
			delete[] pair.second->GetData();
			delete pair.second;
		}
		pages_.clear();
		lru_list_.clear();
	}
}
}