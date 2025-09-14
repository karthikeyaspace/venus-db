// tests/load_insert_test.cpp


#include <chrono>
#include <iostream>
#include <random>
#include <string>


#include "database/database_manager.h"
#include "engine/execution_engine.h"

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

		const int NUM_INSERTS = 10000;
		const int BATCHES = 1000;
		const int BATCH_SIZE = NUM_INSERTS / BATCHES;
		
		if (NUM_INSERTS % BATCHES != 0) {
			throw std::runtime_error("NUM_INSERTS must be evenly divisible by BATCHES");
		}
		
		int successful_inserts = 0;

		auto start_time = std::chrono::high_resolution_clock::now();

		for (int batch = 0; batch < BATCHES; batch++) {
			std::string insert_query = "INSERT INTO test_table VALUES ";
			
			for (int i = 0; i < BATCH_SIZE; i++) {
				int id = batch * BATCH_SIZE + i;
				std::string name = names[name_dist(gen)];
				float score = score_dist(gen);
				
				if (i > 0) {
					insert_query += ", ";
				}
				insert_query += "(" + std::to_string(id) + ", '" + name + "', " + std::to_string(score) + ")";
			}

			result = engine->Execute(insert_query);
			if (result.success_) {
				successful_inserts += BATCH_SIZE;
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

		std::cout << "\n=== Test Stats ===" << std::endl;
		std::cout << "Records attempted: " << NUM_INSERTS << std::endl;
		std::cout << "Batch size: " << BATCH_SIZE << std::endl;
		std::cout << "Number of batches: " << BATCHES << std::endl;
		std::cout << "Records successful: " << successful_inserts << std::endl;
		std::cout << "Success rate: " << (double)successful_inserts / NUM_INSERTS * 100.0 << "%" << std::endl;
		std::cout << "Time taken for insertion: " << duration.count() << " ms" << std::endl;
		std::cout << "Time taken for selection (seq scan): " << select_duration.count() << " ms" << std::endl;
		std::cout << "Average time per batch: " << (double)duration.count() / BATCHES << " ms" << std::endl;

	} catch (const std::exception& e) {
		std::cerr << "Test error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}