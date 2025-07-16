// /src/storage/page.h

/**
 * Page Layout
   +--------------------+  ← offset 0
   | Page Header        |
   +--------------------+
   | Slot Directory     |  (grows downward from header end)
   +--------------------+  ← free_space_ptr (start of free space)
   | Free Space         |     (grows downward)
   +--------------------+  ← tuple_start_ptr (start of tuples/records)
   | Tuples / Records   |     (grows upward)
   +--------------------+  ← offset 4096 (PAGE_SIZE)

  * Page Header
  * - Page Id: Unique identifier for the page.
  * - Page Type: Type of the page (e.g., table, index).
  * - Num Slots: Number of slots in the page.
  * - Free Space Pointer: Offset from start to begin of free space(grows downward).
  * - Tuple Start Pointer: Offset from start to the first tuple (grows upward) - end of free space.
  * - Is Dirty: If page is modified in mem by bpm
  *
  * Slot Directory Entry
  * - Slot Id: Unique identifier for the slot.
  * - Tuple Offset: Offset to the tuple in the page.
  * - Tuple Length: Length of the tuple in bytes.
  * - Status: Status of the slot (e.g., active, deleted).
*/

#pragma once

#include <cstdint>
#include <cstring>

#include "common/config.h"
#include "storage/tuple.h"

namespace venus {
struct SlotDirectory {
	uint32_t tuple_offset; // offset of tuple from the start of the page
	uint16_t tuple_length;
	bool is_live;
};

struct PageHeader {
	page_id_t page_id;
	page_id_t next_page_id = INVALID_PAGE_ID;
	page_id_t prev_page_id = INVALID_PAGE_ID;
	PageType page_type;
	uint16_t num_slots;
	uint32_t free_space_ptr;
	uint32_t tuple_start_ptr;
	bool is_dirty;

	uint32_t GetMetaSpace() const { // Page header + slot directory
		return sizeof(PageHeader) + (num_slots * sizeof(SlotDirectory));
	}
};

class Page {
public:
	// data is owned by bpm
	explicit Page(char* data)
	    : data_(data) { }

	void NewPage(page_id_t page_id, PageType page_type) {
		memset(data_, 0, PAGE_SIZE);

		PageHeader* header = GetHeader();
		header->page_id = page_id;
		header->page_type = page_type;
		header->num_slots = 0;
		header->free_space_ptr = sizeof(PageHeader); // end of page header(there are no slots during init)
		header->tuple_start_ptr = PAGE_SIZE; // start of tuples - end of page
		header->is_dirty = false;
		header->next_page_id = INVALID_PAGE_ID; //
		header->prev_page_id = INVALID_PAGE_ID;
	}

	PageHeader* GetHeader() {
		// A reinterpret_cast in C++ tells the compiler to treat a value (often a pointer or integer)
		// as if it were a different type, without changing the underlying bit‐pattern
		return reinterpret_cast<PageHeader*>(data_);
	}

	// const overload GetHeader - for use in functions like IsDirty
	const PageHeader* GetHeader() const {
		return reinterpret_cast<const PageHeader*>(data_);
	}

	char* GetData() {
		return data_; // Entire page
	}

	const char* GetData() const {
		return data_;
	}

	SlotDirectory* GetSlotDirectory(slot_id_t slot_id) {
		// slots are 0-indexed
		if (slot_id >= GetHeader()->num_slots) {
			return nullptr;
		}
		return reinterpret_cast<SlotDirectory*>(data_ + sizeof(PageHeader) + (slot_id * sizeof(SlotDirectory)));
	}

	void SetDirty(bool is_dirty) {
		GetHeader()->is_dirty = is_dirty;
	}

	bool IsDirty() const {
		return GetHeader()->is_dirty;
	}

	page_id_t GetPageId() const {
		return GetHeader()->page_id;
	}

	PageType GetPageType() const {
		return GetHeader()->page_type;
	}

private:
	// Pointer to the byte array representing the page
	// size => PAGE_SIZE
	char* data_;

	DISALLOW_COPY_AND_MOVE(Page);
};
} // namespace venus