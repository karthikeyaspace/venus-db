// src/storage/tuple.h

/**
 * Tuple represents a row in a table.
 * Since VenusDB is a row-oriented database, a tuple stores entire column data contents of a record.
 * Currently VenusDB supports only INT, FLOAT, and CHAR(fixed length) declared at /src/common/config.h
 * Each tuple is identified by a Record ID (RID)
 * Tuple can be of any length based on the schema of the table
 * Assuming that all the data is not null based on schema, to avoid a null bitmap
 * We use a fixed length encoding and decoding based on the 'Schema' of the table.
 * Generally, A null bitmap is used to indicate which columns are null in a tuple.
 * No nullbitmap for VenusDB - strictly enforcing that all columns are not null.
 *
 * So we have raw data in the tuple body based on 'Schema' of the table.
 *
 *
 * General Tuple Format
 * +------------------------------------+
 * | Tuple Size (uint32_t)              | // Total size of this tuple (including header)
 * +------------------------------------+
 * | Null Bitmap (variable bytes)       | // Bitmap indicating NULL columns (ceil(num_cols/8) bytes)
 * +------------------------------------+
 * | Column Value 1 (variable/fixed)    |
 * +------------------------------------+ ...
 *
 * 
 * Generally size of tuple varies due to variable length columns.
 * But for fixed length columns, we can have a fixed size tuple.
 * For example, if we have a table with 3 columns: INT, FLOAT, CHAR(10)
 * The tuple size will be fixed as 4 + 4 + 10 = 18 bytes
 * 
 * p
 */

#pragma once

#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "common/config.h"
#include "storage/page.h"

namespace venus {

// Declared at /src/catalog/schema.h
class Schema;
class Column;

struct RID {
	page_id_t page_id;
	slot_id_t slot_id;

	RID()
	    : page_id(INVALID_PAGE_ID)
	    , slot_id(0) { }

	RID(page_id_t page_id, slot_id_t slot_id)
	    : page_id(page_id)
	    , slot_id(slot_id) { }

	// Equality operator
	bool operator==(const RID& other) const {
		return page_id == other.page_id && slot_id == other.slot_id;
	}
};

class Tuple {
public:
	Tuple() = default;

	// Initialize a tuple with values based on schema to serialize them to tuple byte buffer
	Tuple(const std::vector<const char*>& values, const Schema* schema)
	    : record_id_()
	    , data_() {
		Serialize(values, schema);
	}

	// Load existing tuple from a page
	Tuple(char* data, RID rid)
	    : record_id_(std::move(rid)) {
		if (data == nullptr) {
			throw std::invalid_argument("Tuple data cannot be null");
		}
		// First read the size from the data
		uint32_t tuple_size = *reinterpret_cast<const uint32_t*>(data);
		// Copy the data into the tuple's data vector
		data_.assign(data, data + tuple_size);
	}

	const char* GetData() const {
		return data_.data();
	}

	uint32_t GetSize() const {
		if (data_.empty()) {
			return 0;
		}
		// The first 4 bytes of data_ contain the size of the tuple, Since there is no null bitmap,
		// We can directly return the size
		return *reinterpret_cast<const uint32_t*>(data_.data());
	}

	RID GetRID() const {
		return record_id_;
	}

	void SetRID(const RID& rid) const {
		if (data_.empty()) {
			throw std::runtime_error("Cannot set RID on an empty tuple");
		}
		record_id_ = rid;
	}

	const char* GetValue(uint32_t idx, const Schema* schema) const;

private:
	mutable RID record_id_;
	std::vector<char> data_; // tuple header + body

	void Serialize(const std::vector<const char*>& values, const Schema* schema);
	void Deserialize(const char *data, const Schema* schema);
};

} // namespace venus
