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
						bpm_->SetDirtyPage(prev_page->GetPageId(), true);
						return false;
					}

					prev_page->GetHeader()->next_page_id = new_page_id;
					page->GetHeader()->prev_page_id = prev_page_id;
					bpm_->SetDirtyPage(prev_page->GetPageId(), true);
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
					bpm_->SetDirtyPage(page->GetPageId(), false);
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
		slot->tuple_offset = page->GetHeader()->tuple_start_ptr - tuple.GetSize();

		// copy tuple data to page
		memcpy(page->GetData() + slot->tuple_offset, tuple.GetData(), tuple.GetSize());

		// update page
		page->GetHeader()->num_slots++;
		page->GetHeader()->free_space_ptr += sizeof(SlotDirectory);
		page->GetHeader()->tuple_start_ptr -= tuple.GetSize();

		rid->page_id = page->GetPageId();
		rid->slot_id = slot_id;
		tuple.SetRID(*rid);

		bpm_->SetDirtyPage(page->GetPageId(), true);

		return true;
	}

	bool TableHeap::DeleteTuple(const RID& rid) {
		page_id_t page_id = rid.page_id;
		slot_id_t slot_id = rid.slot_id;

		Page* page = bpm_->FetchPage(page_id);
		if (page == nullptr) {
			return false;
		}

		if (page->GetHeader()->page_type != PageType::TABLE_PAGE) {
			return false;
		}

		SlotDirectory* slot = page->GetSlotDirectory(slot_id);

		if (slot == nullptr || !slot->is_live) {
			return false;
		}

		slot->is_live = false;

		return true;
	}

	bool TableHeap::UpdateTuple(const Tuple& new_tuple, const RID& rid) {

		// flow
		// 1. Get the existing tuple using the RID
		// 2. If the sizes are the same, copy the new tuple data to the existing slot
		// 3. If the sizes are different, delete the existing tuple
		//    and insert the new tuple, ensuring the RID remains the same
		if (new_tuple.GetSize() == 0) {
			return false;
		}

		Tuple* existing_tuple = GetTuple(rid);

		if (existing_tuple == nullptr) {
			return false;
		}

		if (existing_tuple->GetSize() == new_tuple.GetSize()) {
			Page* page = bpm_->FetchPage(rid.page_id);
			if (page == nullptr) {
				delete existing_tuple;
				return false;
			}

			if (page->GetHeader()->page_type != PageType::TABLE_PAGE) {
				delete existing_tuple;
				return false;
			}

			SlotDirectory* slot = page->GetSlotDirectory(rid.slot_id);
			if (slot == nullptr || !slot->is_live) {
				delete existing_tuple;
				return false;
			}

			memcpy(page->GetData() + slot->tuple_offset, new_tuple.GetData(), new_tuple.GetSize());
			bpm_->SetDirtyPage(page->GetPageId(), true);
			existing_tuple->SetRID(rid);
			delete existing_tuple;
			return true;
		} else {
			RID new_rid;
			bool inserted = InsertTuple(new_tuple, &new_rid);
			if (!inserted) {
				delete existing_tuple;
				return false;
			}

			bool deleted = DeleteTuple(rid);
			if (!deleted) {
				delete existing_tuple;
				return false;
			}

			new_tuple.SetRID(new_rid);
			delete existing_tuple;
			return true;
		}
	}

	Tuple* TableHeap::GetTuple(const RID& rid) {
		page_id_t page_id = rid.page_id;
		slot_id_t slot_id = rid.slot_id;

		Page* page = bpm_->FetchPage(page_id);
		if (page == nullptr || slot_id >= page->GetHeader()->num_slots) {
			return nullptr;
		}

		if (page->GetHeader()->page_type != PageType::TABLE_PAGE) {
			return nullptr;
		}

		SlotDirectory* slot = page->GetSlotDirectory(slot_id);
		if (slot == nullptr || !slot->is_live) {
			return nullptr;
		}

		char* data = page->GetData() + slot->tuple_offset;
		if (data == nullptr) {
			return nullptr;
		}

		Tuple* tuple = new Tuple(data, rid);

		return tuple;
	}

	TableHeap::Iterator TableHeap::begin() {
		Page* page = bpm_->FetchPage(first_page_id_);
		if (page == nullptr) {
			return Iterator(this, RID());
		}

		// first slot in page
		slot_id_t first_slot_id = 0;
		RID first_rid(first_page_id_, first_slot_id);
		SlotDirectory* first_slot = page->GetSlotDirectory(first_slot_id);

		if (first_slot == nullptr || !first_slot->is_live) {
			// find next best slot
			while (first_slot_id < page->GetHeader()->num_slots) {
				first_slot = page->GetSlotDirectory(first_slot_id);
				if (first_slot != nullptr && first_slot->is_live) {
					break;
				}
				first_slot_id++;
			}
			if (first_slot_id >= page->GetHeader()->num_slots) {
				return Iterator(this, RID());
			}
			first_rid.slot_id = first_slot_id;
		}

		return Iterator(this, first_rid);
	}

	TableHeap::Iterator TableHeap::end() {
		return Iterator(this, RID());
	}

	// Iterator method implementations
	const Tuple& TableHeap::Iterator::operator*() {
		return current_tuple_;
	}

	Tuple* TableHeap::Iterator::operator->() {
		return &current_tuple_;
	}

	TableHeap::Iterator& TableHeap::Iterator::operator++() {
		// Move to next valid tuple
		slot_id_t next_slot_id = current_rid_.slot_id + 1;
		page_id_t current_page_id = current_rid_.page_id;
		
		Page* page = table_heap_->bpm_->FetchPage(current_page_id);
		if (page == nullptr) {
			current_rid_ = RID(); // Invalid RID to indicate end
			return *this;
		}
		
		// Look for next valid slot in current page
		while (next_slot_id < page->GetHeader()->num_slots) {
			SlotDirectory* slot = page->GetSlotDirectory(next_slot_id);
			if (slot != nullptr && slot->is_live) {
				current_rid_.slot_id = next_slot_id;
				
				// Update current_tuple_
				Tuple* tuple = table_heap_->GetTuple(current_rid_);
				if (tuple != nullptr) {
					current_tuple_ = *tuple;
					delete tuple;
				}
				return *this;
			}
			next_slot_id++;
		}
		
		// No more valid slots in current page, try next page
		page_id_t next_page_id = page->GetHeader()->next_page_id;
		if (next_page_id != INVALID_PAGE_ID) {
			Page* next_page = table_heap_->bpm_->FetchPage(next_page_id);
			if (next_page != nullptr) {
				// Look for first valid slot in next page
				for (slot_id_t slot_id = 0; slot_id < next_page->GetHeader()->num_slots; ++slot_id) {
					SlotDirectory* slot = next_page->GetSlotDirectory(slot_id);
					if (slot != nullptr && slot->is_live) {
						current_rid_.page_id = next_page_id;
						current_rid_.slot_id = slot_id;
						
						// Update current_tuple_
						Tuple* tuple = table_heap_->GetTuple(current_rid_);
						if (tuple != nullptr) {
							current_tuple_ = *tuple;
							delete tuple;
						}
						return *this;
					}
				}
			}
		}
		
		// No more valid tuples
		current_rid_ = RID(); // Invalid RID to indicate end
		return *this;
	}

	bool TableHeap::Iterator::operator==(const Iterator& other) const {
		return current_rid_.page_id == other.current_rid_.page_id && 
	       current_rid_.slot_id == other.current_rid_.slot_id;
	}

	bool TableHeap::Iterator::operator!=(const Iterator& other) const {
		return !(*this == other);
	}
}
}