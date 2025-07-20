// /tests/test_phase_1.cpp

/** Tests for the Phase 1 functionality of the project
 * 1. Disk manager
 * 2. Buffer manager
 * 3. Table heap
 */

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "buffer/buffer_pool.h"
#include "catalog/schema.h"
#include "common/config.h"
#include "storage/disk_manager.h"
#include "table/table_heap.h"

using namespace venus;
using namespace venus::buffer;
using namespace venus::storage;
using namespace venus::table;

void create_database(const std::string& db_name);
void use_database(const std::string& db_name,
    DiskManager*& disk_manager,
    BufferPoolManager*& buffer_pool_manager);
void show_databases();

void insert_tuple_interactive(BufferPoolManager* bpm, Schema* schema);
void table_scan_memory(BufferPoolManager* bpm, Schema* schema);
void table_scan_from_disk(BufferPoolManager* bpm, Schema* schema);
Schema* create_test_schema();

void table_scan(BufferPoolManager* bpm, TableHeap* table_heap);
void insert_tuple(TableHeap* table_heap, const Schema* schema, const std::vector<std::string>& values);

int main() {

	const std::string db = "test";

	DiskManager* disk_manager;
	BufferPoolManager* buffer_pool_manager;
	Schema* schema = create_test_schema();

	while (true) {
		int option;
		std::cout << std::endl;
		std::cout << "1: Create Database\n"
		          << "2: Use Database\n"
		          << "3: Show Databases\n"
		          << "4: Insert Tuple\n"
		          << "5: Table Scan (Memory)\n"
		          << "6: Table Scan (From Disk)\n"
		          << "7: Exit\n"
		          << "Choose an option: ";
		std::cin >> option;

		std::cout << std::endl;
		switch (option) {
		case 1:
			create_database(db);
			break;
		case 2:
			use_database(db, disk_manager, buffer_pool_manager);
			break;
		case 3:
			show_databases();
			break;
		case 4:
			insert_tuple_interactive(buffer_pool_manager, schema);
			break;
		case 5:
			table_scan_memory(buffer_pool_manager, schema);
			break; 
		case 6:
			table_scan_from_disk(buffer_pool_manager, schema);
			break;
		case 7:
			std::cout << "Exiting...\n";
			delete buffer_pool_manager;
			delete disk_manager;
			delete schema;
			return 0;
		default:
			std::cout << "Invalid option. Please try again.\n";
		}
	}
	return 0;
}

void create_database(const std::string& db_name) {
	std::string db_path = std::string(venus::db_dir) + "/" + db_name + ".db";

	std::filesystem::create_directories(venus::db_dir);

	std::ofstream db_file(db_path);
	if (!db_file) {
		throw std::runtime_error("Failed to create database file: " + db_path);
	}

	std::cout << "Created database: " << db_path << std::endl;

	db_file.close();
}

void use_database(const std::string& db_name,
    DiskManager*& disk_manager,
    BufferPoolManager*& buffer_pool_manager) {

	// Construct the full path to the database file
	std::string db_path = std::string(venus::db_dir) + "/" + db_name + ".db";

	disk_manager = new DiskManager(db_path);
	buffer_pool_manager = new BufferPoolManager(disk_manager);

	if (!disk_manager || !buffer_pool_manager) {
		throw std::runtime_error("Failed to initialize disk or buffer manager.");
	}

	std::cout << "Using database: " << db_path << std::endl;
}

void show_databases() {
	for (const auto& entry : std::filesystem::directory_iterator(venus::db_dir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".db") {
			std::cout << "- " << entry.path().stem().string() << std::endl;
		}
	}
}

Schema* create_test_schema() {
	Schema* schema = new Schema();

	schema->AddColumn("id", ColumnType::INT, true, 0);
	schema->AddColumn("name", ColumnType::CHAR, false, 1);
	schema->AddColumn("score", ColumnType::FLOAT, false, 2);

	std::cout << "Schema generated" << std::endl;
	return schema;
}

void table_scan(BufferPoolManager* bpm, TableHeap* table_heap) {
	std::cout << "\n--- Scanning table in memory ---" << std::endl;

	auto iterator = table_heap->begin();
	auto end = table_heap->end();

	const Schema* schema = table_heap->GetSchema();
	size_t column_count = schema->GetColumnCount();

	int count = 0;
	while (iterator != end) {
		count++;
		const Tuple& current_tuple = *iterator;
		RID current_rid = current_tuple.GetRID();

		std::cout << "\nTuple " << count << " at RID(" << current_rid.page_id
		          << ", " << current_rid.slot_id << "): \n";

		// Iterate columns in schema
		for (size_t i = 0; i < column_count; i++) {
			const Column& column = schema->GetColumn(i);
			const char* value = current_tuple.GetValue(i, schema);
			if (value != nullptr) {
				switch (column.GetType()) {
				case ColumnType::INT:
					std::cout << column.GetName() << "=" << *reinterpret_cast<const int*>(value) << " ";
					break;
				case ColumnType::CHAR:
					std::cout << column.GetName() << "='" << std::string(value, venus::MAX_CHAR_LENGTH) << "' ";
					break;
				case ColumnType::FLOAT:
					std::cout << column.GetName() << "=" << *reinterpret_cast<const float*>(value) << " ";
					break;
				default:
					std::cout << column.GetName() << "=UNKNOWN ";
				}
			} else {
				std::cout << column.GetName() << "=NULL ";
			}

			std::cout << std::endl;
		}

		++iterator;
	}

	std::cout << "Total tuples found: " << count << std::endl;
}

void insert_tuple(TableHeap* table_heap, const Schema* schema, const std::vector<std::string>& values) {
	if (values.size() != schema->GetColumnCount()) {
		std::cerr << "Error: Number of values (" << values.size()
		          << ") doesn't match schema column count (" << schema->GetColumnCount() << ")" << std::endl;
		return;
	}

	std::vector<const char*> tuple_values;
	std::vector<std::unique_ptr<char[]>> allocated_data; // To keep track of allocated memory

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

	// Create and insert the tuple
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

void insert_tuple_interactive(BufferPoolManager* bpm, Schema* schema) {
	if (bpm == nullptr || schema == nullptr) {
		std::cerr << "BufferPoolManager or Schema is null!" << std::endl;
		return;
	}

	static page_id_t table_page_id = INVALID_PAGE_ID;

	// If we don't have a table page yet, create one
	if (table_page_id == INVALID_PAGE_ID) {
		Page* first_page = bpm->NewPage();
		if (first_page == nullptr) {
			std::cerr << "Failed to allocate first page for table" << std::endl;
			return;
		}
		table_page_id = first_page->GetPageId();
		std::cout << "Created new table with first page ID: " << table_page_id << std::endl;
	}

	// Create table heap
	TableHeap table_heap(bpm, schema, table_page_id);

	std::cout << "\n--- Insert Tuple ---" << std::endl;

	std::vector<std::string> values;
	for (size_t i = 0; i < schema->GetColumnCount(); ++i) {
		const Column& column = schema->GetColumn(i);
		std::string value;
		std::cout << "Enter " << column.GetName() << ": ";
		std::cin >> value;
		values.push_back(value);
	}

	std::cout << std::endl;

	insert_tuple(&table_heap, schema, values);

	bpm->FlushPage(table_page_id);
}

void table_scan_memory(BufferPoolManager* bpm, Schema* schema) {
	if (bpm == nullptr || schema == nullptr) {
		std::cerr << "BufferPoolManager or Schema is null!" << std::endl;
		return;
	}

	// Try to get the existing table page (should be page 0 if it exists)
	Page* table_page = bpm->FetchPage(0);
	if (table_page == nullptr) {
		std::cout << "No table found. Please insert some tuples first." << std::endl;
		return;
	}

	// Create table heap with the existing page
	TableHeap table_heap(bpm, schema, 0);

	std::cout << "\n=== Table Scan (Memory) ===" << std::endl;
	table_scan(bpm, &table_heap);
}

void table_scan_from_disk(BufferPoolManager* bpm, Schema* schema) {
	if (bpm == nullptr || schema == nullptr) {
		std::cerr << "BufferPoolManager or Schema is null!" << std::endl;
		return;
	}

	std::cout << "\n=== Table Scan (From Disk) ===" << std::endl;
	std::cout << "Simulating disk read by creating fresh buffer pool..." << std::endl;

	// Create a fresh disk manager and buffer pool to simulate reading from disk
	DiskManager* fresh_disk_manager = new DiskManager(std::string(venus::db_dir) + "/test.db");
	BufferPoolManager* fresh_bpm = new BufferPoolManager(fresh_disk_manager);

	// Try to fetch page 0 from disk
	Page* table_page = fresh_bpm->FetchPage(0);
	if (table_page == nullptr) {
		std::cout << "No table found on disk. Please insert some tuples first." << std::endl;
		delete fresh_bpm;
		delete fresh_disk_manager;
		return;
	}

	// Create table heap with the page read from disk
	TableHeap table_heap(fresh_bpm, schema, 0);

	std::cout << "Successfully loaded table from disk." << std::endl;
	table_scan(fresh_bpm, &table_heap);

	// Cleanup the fresh instances
	delete fresh_bpm;
	delete fresh_disk_manager;
}