// src/network/network.h

/**
 * Currently REPL is owned by executor, which seems bad
 * So swithcing REPL to a 'NetworkManager'
 *
 * This has 2 main advantages (for me atleast)
 * - Multi-thread execution using thread pool (queue)
 * - Network protocols (HTTP, gRPC)
 */

#pragma once

#include <functional>
#include <iostream>
#include <string>

#include "executor/executor.h"

namespace venus {
namespace network {
	class NetworkManager {
	public:
		NetworkManager() = default;
		~NetworkManager() = default;

		void Start();
		
		void SetExecuteCallback(std::function<executor::ResultSet(const std::string&)> cb) {
			execute = std::move(cb);
		}

		void Stop() {
			open_ = false;
		}

	private:
		void HandleInput(const std::string& input, std::string& accumulated_input);
		std::function<executor::ResultSet(const std::string&)> execute;

		bool open_ = true;
	};
} // namespace network
} // namespace venus