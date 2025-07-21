
// src/main.cpp

#include "database/database_manager.h"
#include <iostream>

using namespace venus;
using namespace venus::database;

int main() {
	try {
		DatabaseManager db_manager;
		db_manager.Start();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}