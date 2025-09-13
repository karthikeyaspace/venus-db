// /src/network/network.cpp

#include "network/network.h"
#include "common/config.h"
#include "common/utils.h"
#include <cstdlib>

namespace venus {
namespace network {
	void NetworkManager::Start() {
		std::cout << "===== Venus DB =====" << std::endl;
		std::string accumulated_input;

		while (open_) {
			if (accumulated_input.empty()) {
				std::cout << "venus> ";
			} else {
				std::cout << "     > ";
			}

			std::string current_line;
			if (!std::getline(std::cin, current_line)) {
				break;
			}
			HandleInput(current_line, accumulated_input);
		}
	}

	void NetworkManager::HandleInput(const std::string& input, std::string& accumulated_input) {
		if (input.empty() && accumulated_input.empty()) {
			return;
		}

		if (!accumulated_input.empty()) {
			accumulated_input += " ";
		}
		accumulated_input += input;

		if (!accumulated_input.empty() && accumulated_input.back() == ';') {
			std::string statement = accumulated_input.substr(0, accumulated_input.length() - 1);
			auto result = execute(statement);
			utils::PrintResultSet(result);
			accumulated_input.clear();
		}
	}
} // namespace network
} // namespace venus