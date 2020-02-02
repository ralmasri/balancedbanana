#pragma once


#include "CommandLineProcessor.h"

namespace balancedbanana
{
namespace commandLineInterface
{
    
class SchedulerCommandLineProcessor : public balancedbanana::commandLineInterface::CommandLineProcessor
{
    public:

        virtual std::shared_ptr<Task> process(int argc, char** argv);

    private:


};

} // namespace commandLineInterface
} // namespace balancedbanana