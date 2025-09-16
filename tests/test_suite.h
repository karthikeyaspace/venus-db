// /tests/test_suite.h

#pragma once

#include <string>
#include <vector>

namespace venus {
namespace engine {
	class ExecutionEngine;
}
namespace database {
	class DatabaseManager;
}
}

namespace venus {
namespace tests {
	
	class TestSuite {
	public:
		TestSuite();
		~TestSuite();
		
		void Start();
		
	private:
		// Test categories
		void RunBasicTests();
		void RunDMLTests();
		void RunBulkInsertTests();
		void RunErrorTests();
		void RunPerformanceTests();
		
		// Test methods
		void TestDatabaseSetup();
		void TestTableCreation();
		void TestSingleInsert();
		void TestBulkInsert();
		void TestSelectQuery();
		void TestInvalidQueries();
		void TestLargeDataset();
		
		// Simple utilities
		void RunTest(const std::string& name, void (TestSuite::*test)());
		void Assert(bool condition, const std::string& message);
		
		// Single engine instance
		venus::database::DatabaseManager* db_manager_;
		venus::engine::ExecutionEngine* engine_;
		
		// Simple counters
		int total_tests_;
		int passed_tests_;
	};
	
} // namespace tests
} // namespace venus