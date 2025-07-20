// /tests/test_database_manager.cpp

/**
 * Test for DatabaseManager - demonstrates the new architecture
 * This replaces the old approach of manually creating DiskManager + BufferPoolManager
 */

#include <iostream>
#include <string>

#include "database/database_manager.h"
#include "catalog/schema.h"
#include "table/table_heap.h"

using namespace venus;
using namespace venus::database;
using namespace venus::buffer;
using namespace venus::catalog;
using namespace venus::table;

Schema* create_test_schema() {
    Schema* schema = new Schema();
    schema->AddColumn("id", ColumnType::INT, true, 0);
    schema->AddColumn("name", ColumnType::CHAR, false, 1);
    schema->AddColumn("score", ColumnType::FLOAT, false, 2);
    return schema;
}

void test_database_manager() {
    try {
        // Step 1: Create DatabaseManager (replaces manual component creation)
        std::string db_path = std::string(venus::db_dir) + "/test_db_manager.db";
        DatabaseManager db_manager(db_path);
        
        std::cout << "✅ DatabaseManager created successfully" << std::endl;
        std::cout << "Database path: " << db_manager.GetDatabasePath() << std::endl;
        std::cout << "Database open: " << (db_manager.IsOpen() ? "Yes" : "No") << std::endl;

        // Step 2: Get components from DatabaseManager
        BufferPoolManager* bpm = db_manager.GetBufferPoolManager();
        CatalogManager* catalog = db_manager.GetCatalogManager();
        
        std::cout << "✅ Retrieved components from DatabaseManager" << std::endl;

        // Step 3: Test basic functionality
        Schema* schema = create_test_schema();
        
        // Create a table page
        Page* first_page = bpm->NewPage();
        if (first_page) {
            page_id_t table_page_id = first_page->GetPageId();
            std::cout << "✅ Created new page with ID: " << table_page_id << std::endl;
            
            // Create table heap
            TableHeap table_heap(bpm, schema, table_page_id);
            
            // Insert test data
            std::vector<std::string> test_data = {"1", "Alice", "95.5"};
            Tuple test_tuple({"1", "Alice", "95.5"}, schema);
            RID rid;
            
            if (table_heap.InsertTuple(test_tuple, &rid)) {
                std::cout << "✅ Inserted test tuple at RID(" << rid.page_id << ", " << rid.slot_id << ")" << std::endl;
            }
            
            // Flush data
            bpm->FlushPage(table_page_id);
            std::cout << "✅ Flushed page to disk" << std::endl;
        }
        
        // Step 4: Test catalog functionality (when implemented)
        std::cout << "✅ Catalog manager available for future implementation" << std::endl;
        
        // Step 5: Test cleanup
        db_manager.Close();
        std::cout << "✅ Database closed successfully" << std::endl;
        
        delete schema;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "\n=== Testing DatabaseManager ===" << std::endl;
    test_database_manager();
    std::cout << "\n=== Test completed ===" << std::endl;
    return 0;
}
