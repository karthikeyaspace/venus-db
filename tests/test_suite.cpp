// /tests/test_suite.cpp

#include "test_suite.h"
#include "database/database_manager.h"
#include "engine/execution_engine.h"

#include <chrono>
#include <iostream>

using namespace venus::database;
using namespace venus::engine;

namespace venus {
namespace tests {

	TestSuite::TestSuite()
	    : db_manager_(nullptr)
	    , engine_(nullptr)
	    , total_tests_(0)
	    , passed_tests_(0) {
		db_manager_ = new DatabaseManager();
		engine_ = db_manager_->GetExecutionEngine();

		if (!engine_) {
			throw std::runtime_error("Failed to initialize execution engine");
		}
	}

	TestSuite::~TestSuite() {
		if (db_manager_) {
			delete db_manager_;
		}
	}

	void TestSuite::Start() {
		std::cout << "Venus DB Test Suite" << std::endl;
		std::cout << "===================" << std::endl;

		auto start_time = std::chrono::high_resolution_clock::now();

		try {
			RunBasicTests();
			RunDMLTests();
			RunBulkInsertTests();
			RunErrorTests();
			RunPerformanceTests();

		} catch (const std::exception& e) {
			std::cout << "FAILED: Test suite failed: " << e.what() << std::endl;
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

		std::cout << "\nResults: " << passed_tests_ << "/" << total_tests_ << " passed";
		std::cout << " (" << duration.count() << "ms)" << std::endl;

		if (passed_tests_ == total_tests_) {
			std::cout << "SUCCESS: All tests passed!" << std::endl;
		} else {
			std::cout << (total_tests_ - passed_tests_) << " tests failed" << std::endl;
		}
	}

	void TestSuite::RunBasicTests() {
		std::cout << "\nBasic Tests" << std::endl;
		RunTest("Database Setup", &TestSuite::TestDatabaseSetup);
		RunTest("Table Creation", &TestSuite::TestTableCreation);
	}

	void TestSuite::RunDMLTests() {
		std::cout << "\nDML Tests" << std::endl;
		RunTest("Single INSERT", &TestSuite::TestSingleInsert);
		RunTest("SELECT Query", &TestSuite::TestSelectQuery);
	}

	void TestSuite::RunBulkInsertTests() {
		std::cout << "\nBulk Insert Tests" << std::endl;
		RunTest("Bulk INSERT", &TestSuite::TestBulkInsert);
	}

	void TestSuite::RunErrorTests() {
		std::cout << "\nError Tests" << std::endl;
		RunTest("Invalid Queries", &TestSuite::TestInvalidQueries);
	}

	void TestSuite::RunPerformanceTests() {
		std::cout << "\nPerformance Tests" << std::endl;
		RunTest("Large Dataset", &TestSuite::TestLargeDataset);
	}

	void TestSuite::TestDatabaseSetup() {
		auto result = engine_->Execute("CREATE DATABASE test_db");
		Assert(result.success_, "Failed to create database");

		result = engine_->Execute("USE test_db");
		Assert(result.success_, "Failed to use database");
	}

	void TestSuite::TestTableCreation() {
		auto result = engine_->Execute("CREATE TABLE users (id INT, name CHAR, score FLOAT)");
		Assert(result.success_, "Failed to create users table");

		result = engine_->Execute("CREATE TABLE products (id INT, title CHAR, price FLOAT)");
		Assert(result.success_, "Failed to create products table");
	}

	void TestSuite::TestSingleInsert() {
		auto result = engine_->Execute("INSERT INTO users VALUES (1, 'Alice', 95.5)");
		Assert(result.success_, "Failed to insert single record");

		result = engine_->Execute("INSERT INTO users VALUES (2, 'Bob', 87.2)");
		Assert(result.success_, "Failed to insert second record");
	}

	void TestSuite::TestBulkInsert() {
		std::string query = "INSERT INTO products VALUES "
		                    "(1, 'Laptop', 999.99), "
		                    "(2, 'Mouse', 25.50), "
		                    "(3, 'Keyboard', 75.00)";

		auto result = engine_->Execute(query);
		Assert(result.success_, "Failed to execute bulk insert");
	}

	void TestSuite::TestSelectQuery() {
		auto result = engine_->Execute("SELECT * FROM users");
		Assert(result.success_, "Failed to SELECT from users");

		result = engine_->Execute("SELECT * FROM products");
		Assert(result.success_, "Failed to SELECT from products");
	}

	void TestSuite::TestInvalidQueries() {
		auto result = engine_->Execute("SELEKT * FROM users"); 
		Assert(!result.success_, "Should fail on syntax error");

		result = engine_->Execute("INSERT INTO nonexistent VALUES (1, 'test')");
		Assert(!result.success_, "Should fail on nonexistent table");
	}

	void TestSuite::TestLargeDataset() {
		auto result = engine_->Execute("CREATE TABLE large_test (id INT, data CHAR, value FLOAT)");
		Assert(result.success_, "Failed to create large test table");

		std::string query = "INSERT INTO large_test VALUES ";
		for (int i = 1; i <= 500; i++) {
			if (i > 1)
				query += ", ";
			query += "(" + std::to_string(i) + ", 'data" + std::to_string(i) + "', " + std::to_string(i * 1.5) + ")";
		}

		auto start_time = std::chrono::high_resolution_clock::now();
		result = engine_->Execute(query);
		auto end_time = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
		std::cout << "    TIME: Inserted 50 records in " << duration.count() << "μs" << std::endl;

		Assert(result.success_, "Failed to insert large dataset");

		start_time = std::chrono::high_resolution_clock::now();
		result = engine_->Execute("SELECT * FROM large_test");
		end_time = std::chrono::high_resolution_clock::now();

		duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
		std::cout << "    TIME: Selected 50 records in " << duration.count() << "μs" << std::endl;

		Assert(result.success_, "Failed to select large dataset");
	}

	void TestSuite::RunTest(const std::string& name, void (TestSuite::*test)()) {
		total_tests_++;

		try {
			(this->*test)();
			std::cout << "  PASS: " << name << std::endl;
			passed_tests_++;
		} catch (const std::exception& e) {
			std::cout << "  FAIL: " << name << " - " << e.what() << std::endl;
		}
	}

	void TestSuite::Assert(bool condition, const std::string& message) {
		if (!condition) {
			throw std::runtime_error(message);
		}
	}

} // namespace tests
} // namespace venus