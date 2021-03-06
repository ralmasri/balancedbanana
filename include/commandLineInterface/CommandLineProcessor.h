#pragma once


#include <memory>
#include <string>
#include <cstring>
#include <vector>

#include <communication/Task.h>
#include "CLI11/CLI11.hpp"


#define ERROR_UNCLEAR_TASK_TYPE -1

namespace balancedbanana {
	namespace commandLineInterface {
		
		class CommandLineProcessor
		{
		public:
			/**
			 * Process an array of command line arguments (argv : &char[argc])
			 * Write processed arguments into the provided Task instance
			 * Since a set of global standard values is provided by the scheduler,
			 * the task instance is not tested to posess all necessary arguments.
			 * 
			 * Returns 0 on success and an error code on failure.
			 */
			virtual int process(int argc, const char* const * argv, const std::shared_ptr<balancedbanana::communication::Task>& task) { return 0; }

			static std::string readPassword();
		};

	}
}