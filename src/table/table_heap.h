// src/storage/table_heap.h

/**
 * TableHeap provides an abstraction for a table's data, which is stored as a collection of pages.
 * It handles tuple insertion, deletion, updates, and iteration over the entire table.
 *
 * It basically gives implementation over pages in the buffer pool.
 *
 * Table heap only communicates with buffer pool, not with disk manager directly.
 *
 * Table heap is a collection of pages that store tuples of a single table,
 * Every table has its own table heap, which is managed independently of other tables.
 * Each table heap is identified by a unique page ID, which is the first page of the table.
 *
 * An iterator is provided to iterate over the tuples in the table heap.
 * When we have an index, we can use the index to find the right page and use an Iterator to iterate over the tuples in that page.
 *
 * To insert a tuple in table heap, we need to find a page with enough space to store the tuple.
 * If no such page exists, we need to allocate a new page and insert the tuple there
 *
 * To Find a tuple in table heap, we need to use the RID (Record ID) of the tuple.
 * The RID is a unique identifier for a tuple in the table heap, which consists of the page ID and the slot ID.
 *
 */

#pragma once

#include "buffer/buffer_pool.h"
#include "storage/page.h"
#include "storage/tuple.h"

namespace venus {
namespace table {
	class TableHeap {
	public:
		// If this is a new table, then a page is initialized
		// TableHeap containes a pointer to the first page of the table used to access the entire table in a database

		TableHeap(buffer::BufferPoolManager* bpm, const Schema* schema, page_id_t first_page_id)
		    : bpm_(bpm)
		    , schema_(schema)
		    , first_page_id_(first_page_id) { }

		~TableHeap() = default;

		bool InsertTuple(const Tuple& tuple, RID* rid);

		// It marks the tuple as deleted -> basically unpins so lru can replace it later with needed and slot.is_live to false
		// Actual deletion happens in a seperate process on a background thread
		bool DeleteTuple(const RID& rid);

		// If the sizes are same, it just copies the data from new_tuple to the page
		// If the sizes are different, it deletes the old tuple and inserts the new one ensuring the RID is same
		bool UpdateTuple(const Tuple& new_tuple, const RID& rid);

		// tuple is passed by reference, so it is not owned by the caller
		Tuple* GetTuple(const RID& rid);

		// for sequential scans
		class Iterator {
		public:
			Iterator(TableHeap* table_heap, RID rid)
			    : table_heap_(table_heap)
			    , current_rid_(rid) {

				Tuple* tuple = table_heap_->GetTuple(current_rid_);
				if (tuple != nullptr) {
					current_rid_ = tuple->GetRID();
					current_tuple_ = *tuple;
					delete tuple;
				}
			};

			const Tuple& operator*();
			Tuple* operator->();
			Iterator& operator++();
			bool operator==(const Iterator& other) const;
			bool operator!=(const Iterator& other) const;

		private:
			TableHeap* table_heap_;
			RID current_rid_;
			Tuple current_tuple_;
		};

		Iterator begin(); // Returns an iterator pointing to the first tuple in the table
		Iterator end(); // Returns an iterator pointing to the end of the table (after the last tuple)

	private:
		buffer::BufferPoolManager* bpm_;
		const Schema* schema_;
		page_id_t first_page_id_; // to uniquely identify the table heap

		DISALLOW_COPY_AND_MOVE(TableHeap);
	};

} // namespace storage
} // namespace venus
