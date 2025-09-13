// tests/load_insert_test.cpp
// High load insertion test on a database
// Steps - Create a db, use a db, create a table, insert n random values to the table

#include "database/database_manager.h"
#include "engine/execution_engine.h"
#include <chrono>
#include <iostream>
#include <random>
#include <string>

using namespace venus;
using namespace venus::database;
using namespace venus::engine;

int main() {
	try {
		DatabaseManager db_manager;

		ExecutionEngine* engine = db_manager.GetExecutionEngine();

		if (!engine) {
			throw std::runtime_error("ExecutionEngine is not initialized");
		}

		executor::ResultSet result = engine->Execute("CREATE DATABASE test");
		if (!result.success_) {
			throw std::runtime_error("Failed to create database test");
		}

		result = engine->Execute("USE test");
		if (!result.success_) {
			throw std::runtime_error("Failed to use database");
		}

		result = engine->Execute("CREATE TABLE test_table (id INT, name CHAR, score FLOAT)");
		if (!result.success_) {
			throw std::runtime_error("Failed to create table");
		}

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<> score_dist(0.0, 100.0);

		std::vector<std::string> names = { "Alice", "Bob", "Charlie", "David", "Eve", "Frank", "Grace", "Henry" };
		std::uniform_int_distribution<> name_dist(0, names.size() - 1);

		const int NUM_INSERTS = 500;
		int successful_inserts = 0;

		auto start_time = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < NUM_INSERTS; i++) {
			std::string name = names[name_dist(gen)];
			float score = score_dist(gen);

			std::string insert_query = "INSERT INTO test_table VALUES (" + std::to_string(i) + ", '" + name + "', " + std::to_string(score) + ")";

			result = engine->Execute(insert_query);
			if (result.success_) {
				successful_inserts++;
			}
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

		auto select_start_time = std::chrono::high_resolution_clock::now();
		result = engine->Execute("SELECT * FROM test_table");
		if (!result.success_) {
			throw std::runtime_error("Failed to execute select");
		}

		auto select_end_time = std::chrono::high_resolution_clock::now();
		auto select_duration = std::chrono::duration_cast<std::chrono::milliseconds>(select_end_time - select_start_time);

		result = engine->Execute("SHOW TABLES");
		if (!result.success_) {
			std::cerr << "Failed to show tables!" << std::endl;
		}

		std::cout << "\n=== Test Summary ===" << std::endl;
		std::cout << "Records attempted: " << NUM_INSERTS << std::endl;
		std::cout << "Records successful: " << successful_inserts << std::endl;
		std::cout << "Success rate: " << (double)successful_inserts / NUM_INSERTS * 100.0 << "%" << std::endl;
		std::cout << "Time taken for insertion: " << duration.count() << " ms" << std::endl;
		std::cout << "Time taken for selection (seq scan): " << select_duration.count() << " ms" << std::endl;
		std::cout << "Average time per insert: " << (double)duration.count() / NUM_INSERTS << " ms" << std::endl;
		std::cout << "Test completed!" << std::endl;

	} catch (const std::exception& e) {
		std::cerr << "Test error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}