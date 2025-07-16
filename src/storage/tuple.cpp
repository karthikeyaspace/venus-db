// /src/storage/tuple.cpp

#include "storage/tuple.h"
#include "catalog/schema.h"
#include <cstring>

namespace venus {

void Tuple::Serialize(const std::vector<const char*>& values, const Schema* schema) {
    if (values.size() != schema->GetColumnCount()) {
        throw std::invalid_argument("Number of values does not match schema column count");
    }

    // Calculate total size needed
    uint32_t total_size = sizeof(uint32_t); // Size header
    for (size_t i = 0; i < schema->GetColumnCount(); ++i) {
        const Column& column = schema->GetColumn(i);
        total_size += column.GetLength();
    }

    // Resize data vector to hold the tuple
    data_.resize(total_size);

    // Write size header
    *reinterpret_cast<uint32_t*>(data_.data()) = total_size;

    // Write column values
    char* data_ptr = data_.data() + sizeof(uint32_t);
    for (size_t i = 0; i < schema->GetColumnCount(); ++i) {
        const Column& column = schema->GetColumn(i);
        size_t column_size = column.GetLength();
        
        if (column.GetType() == ColumnType::CHAR) {
            // For CHAR columns, initialize the entire buffer to zero and copy the string
            std::memset(data_ptr, 0, column_size);
            std::memcpy(data_ptr, values[i], std::strlen(values[i]));
        } else {
            // For fixed-size columns (INT, FLOAT), copy directly
            std::memcpy(data_ptr, values[i], column_size);
        }
        data_ptr += column_size;
    }
}

void Tuple::Deserialize(const char *data, const Schema* schema) {
    if (data == nullptr || schema == nullptr) {
        throw std::invalid_argument("Data or schema cannot be null");
    }

    // Read the size from the first 4 bytes
    uint32_t tuple_size = *reinterpret_cast<const uint32_t*>(data);
    
    // Copy the entire serialized data
    data_.assign(data, data + tuple_size);
}

const char* Tuple::GetValue(uint32_t idx, const Schema* schema) const {
    if (idx >= schema->GetColumnCount()) {
        throw std::out_of_range("Column index out of range");
    }

    if (data_.empty()) {
        throw std::runtime_error("Cannot get value from empty tuple");
    }

    // Skip size header
    const char* data_ptr = data_.data() + sizeof(uint32_t);
    
    // Skip to the desired column
    for (uint32_t i = 0; i < idx; ++i) {
        const Column& column = schema->GetColumn(i);
        data_ptr += column.GetLength();
    }

    return data_ptr;
}

} // namespace venus
