// src/common/utils.cpp

#include "common/utils.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace venus {
namespace utils {
	void InsertTuple(table::TableHeap* table_heap, const Schema* schema, const std::vector<std::string>& values) {
		if (table_heap == nullptr || schema == nullptr || values.empty()) {
			throw std::invalid_argument("Invalid arguments provided to InsertTuple");
		}

		if (values.size() != schema->GetColumnCount()) {
			std::cerr << "Error: Number of values (" << values.size()
			          << ") doesn't match schema column count (" << schema->GetColumnCount() << ")" << std::endl;
			return;
		}

		std::vector<const char*> tuple_values;
		std::vector<std::unique_ptr<char[]>> allocated_data;

		for (size_t i = 0; i < schema->GetColumnCount(); ++i) {
			const Column& column = schema->GetColumn(i);
			const std::string& value_str = values[i];

			switch (column.GetType()) {
			case ColumnType::INT: {
				int* int_value = new int(std::stoi(value_str));
				allocated_data.push_back(std::unique_ptr<char[]>(reinterpret_cast<char*>(int_value)));
				tuple_values.push_back(reinterpret_cast<const char*>(int_value));
				break;
			}
			case ColumnType::FLOAT: {
				float* float_value = new float(std::stof(value_str));
				allocated_data.push_back(std::unique_ptr<char[]>(reinterpret_cast<char*>(float_value)));
				tuple_values.push_back(reinterpret_cast<const char*>(float_value));
				break;
			}
			case ColumnType::CHAR: {
				char* char_value = new char[venus::MAX_CHAR_LENGTH];
				memset(char_value, 0, venus::MAX_CHAR_LENGTH);
				strncpy(char_value, value_str.c_str(), std::min(value_str.length(), static_cast<size_t>(venus::MAX_CHAR_LENGTH - 1)));
				allocated_data.push_back(std::unique_ptr<char[]>(char_value));
				tuple_values.push_back(char_value);
				break;
			}
			default:
				std::cerr << "Error: Unsupported column type for column " << column.GetName() << std::endl;
				return;
			}
		}

		Tuple tuple(tuple_values, schema);
		RID rid;
		bool success = table_heap->InsertTuple(tuple, &rid);

		if (success) {
			std::cout << "Inserted tuple id(" << values[0] << ")";
			std::cout << " at RID(" << rid.page_id << ", " << rid.slot_id << ")" << std::endl;
		} else {
			std::cout << "Failed to insert tuple id(" << values[0] << ")" << std::endl;
		}
	}
}
} // namespace venus