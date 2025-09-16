// /tests/main.cpp

#include "test_suite.h"
#include <iostream>

int main() {
	try {
		venus::tests::TestSuite test_suite;
		test_suite.Start();
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "Failed tests: " << e.what() << std::endl;
		return 1;
	}
}

