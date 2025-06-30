// /src/buffer/buffer_pool.h

/**
 * BufferPoolManager for mini-db.
 *
 * Coordinates between DiskManager, TableHeap, and query executors.
 * Owns and operates the DiskManager.
 *
 * Responsibilities:
 *   - Cache pages in memory
 *   - Allocate and deallocate pages
 *   - Pin and unpin pages
 *   - Flush dirty pages to disk
 *   - Manage page replacement policies
 *   - Handle page requests from Catalog, Executor, etc.
 *   - Supply pages for B+Tree index operations
 *
 * Flow of fetching a page:
 *   1. Calls FetchPage(page_id)
 * 		  - If page_id is INVALID_PAGE_ID, return nullptr
 *   2. Check in-memory page table
 *      - If resident, return buffer frame
 *      - Otherwise:
 *         a. Pick a victim frame
 *         b. Evict it (write back via DiskManager if dirty)
 *         c. Read new page via DiskManager::ReadPage(page_id, frame_data)
 *   3. Pin the frame and return it to TableHeap
 * 
 * Pinning - bumps the page's pin count so the buffer pool won’t evict it while in use.
 * Dirty - Marking a page “dirty” just flags that it’s been modified and must be written back to disk before eviction.
 * 
 */

#pragma once

#include "common/config.h"
#include "storage/disk_manager.h"
#include "storage/page.h"

#include <list>
#include <unordered_map>

namespace minidb {
namespace buffer {
	class BufferPoolManager {
	public:
		BufferPoolManager(storage::DiskManager* disk_manager)
		    : pool_size_(BUFFER_POOL_SIZE) // no. of pages
		    , disk_manager_(disk_manager) { };

		~BufferPoolManager();

		Page* FetchPage(page_id_t page_id);
		bool UnpinPage(page_id_t page_id, bool is_dirty);
		bool FlushPage(page_id_t page_id);
		page_id_t NewPage();
		bool DeletePage(page_id_t page_id);

	private:
		size_t pool_size_;
		storage::DiskManager* disk_manager_;
		std::unordered_map<page_id_t, Page*> pages_; // Pages which are allocated and deallocated in the buffer pool
		std::list<Page*> lru_list_; // For LRU replacement policy
	};
}
}