// src/storage/table_heap.cpp

#include "table/table_heap.h"

namespace venus {
namespace table {

	bool TableHeap::InsertTuple(const Tuple& tuple, RID* rid) {
		if (tuple.GetSize() == 0 || rid == nullptr) {
			return false;
		}

		Page* page = nullptr;
		page_id_t curr_page_id = first_page_id_;
		page_id_t prev_page_id = INVALID_PAGE_ID;

		while (true) {
			if (curr_page_id == INVALID_PAGE_ID) {
				// End of pages/no pages in heap -> allocate new page

				// BPM Newpage() allocates a new page in memory, disk, and lru
				page = bpm_->NewPage();
				page_id_t new_page_id = page->GetPageId();
				if (page == nullptr) {
					return false;
				}

				// if heap has no page yet
				if (first_page_id_ == INVALID_PAGE_ID) {
					first_page_id_ = new_page_id;
				}
				// heap is at end of pages
				else {
					Page* prev_page = bpm_->FetchPage(prev_page_id);
					if (prev_page == nullptr) {
						bpm_->UnpinPage(prev_page->GetPageId(), true);
						return false;
					}

					prev_page->GetHeader()->next_page_id = new_page_id;
					page->GetHeader()->prev_page_id = prev_page_id;
					bpm_->UnpinPage(prev_page->GetPageId(), true);
				}
				curr_page_id = new_page_id;
				break; // got the page, now allocate tuple
			} else {
				page = bpm_->FetchPage(curr_page_id);
				if (page == nullptr) {
					return false;
				}

				uint32_t required_space = tuple.GetSize() + sizeof(SlotDirectory);
				uint32_t avail_space = page->GetHeader()->tuple_start_ptr - page->GetHeader()->free_space_ptr;

				if (required_space > avail_space) {
					// go to next page
					prev_page_id = curr_page_id;
					curr_page_id = page->GetHeader()->next_page_id;
					bpm_->UnpinPage(page->GetPageId(), false);
				} else {
					// Enough space found, insert the tuple
					break;
				}
			}
		}

		// Here we have the page

		slot_id_t slot_id = page->GetHeader()->num_slots;

		// create a new entry
		SlotDirectory* slot = reinterpret_cast<SlotDirectory*>(page->GetData() + sizeof(PageHeader) + (slot_id * sizeof(SlotDirectory)));
		slot->is_live = true;
		slot->tuple_length = tuple.GetSize();
		
		// offset from start of page
		slot->tuple_offset = page->GetHeader()->tuple_start_ptr + tuple.GetSize();

		// copy tuple data to page 
		memcpy(page->GetData() + slot->tuple_offset, tuple.GetData(), tuple.GetSize());

		// update page
		page->GetHeader()->num_slots++;
		page->GetHeader()->free_space_ptr += sizeof(SlotDirectory);
		page->GetHeader()->tuple_start_ptr -= tuple.GetSize();

		rid->page_id = page->GetPageId();
		rid->slot_id = slot_id;
		tuple.SetRID(*rid);

		// unpin and mark as dirty
		bpm_->UnpinPage(page->GetPageId(), true);

		return true;
	}

	bool TableHeap::DeleteTuple(const RID& rid) {
		// Implementation for deleting a tuple from the table heap
		return false;
	}

	bool TableHeap::UpdateTuple(const Tuple& new_tuple, const RID& rid) {
		// Implementation for updating a tuple in the table heap
		// This will involve checking if the new tuple size matches the old one,
		// and either replacing it or deleting and inserting a new one.
		return false;
	}

	bool TableHeap::GetTuple(const RID& rid, Tuple* tuple) const {
		// Implementation for retrieving a tuple from the table heap
		return false;
	}

	TableHeap::Iterator TableHeap::begin() {
		// Implementation for returning an iterator pointing to the first tuple
		return Iterator(this, RID());
	}

	TableHeap::Iterator TableHeap::end() {
		// Implementation for returning an iterator pointing to the end of the table
		return Iterator(this, RID(INVALID_PAGE_ID, 0));
	}

}
}