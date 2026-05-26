#pragma once

// Convenience header that includes all knob system components
#include "config.h"
#include "knob_base.h"
#include "knob_list.h"

// Example usage:
/*
#include "knob_system.h"

class MyModule {
public:
    MyModule(const std::string& instance_name)
        : knob_list(csim::config::get_or_create(instance_name)),
          clock_period(knob_list.add_knob("clock_period", "Clock period in ns", 1.0)),
          buffer_size(knob_list.add_knob("buffer_size", "Size of internal buffer", 1024)),
          enable_debug(knob_list.add_knob("enable_debug", "Enable debug output", false)),
          module_name(knob_list.add_knob("name", "Name of the module", std::string("default"))) {
        // Additional initialization if needed
    }

    void run() {
        std::cout << "Module " << knob_list.get_module_name() << ":" << std::endl;
        std::cout << "  Clock period: " << clock_period.get() << " ns" << std::endl;
        std::cout << "  Buffer size: " << buffer_size.get() << std::endl;
        std::cout << "  Debug enabled: " << (enable_debug.get() ? "yes" : "no") << std::endl;
        std::cout << "  Module name: " << module_name.get() << std::endl;
    }

private:
    KnobList& knob_list;
    Knob<double>& clock_period;
    Knob<int>& buffer_size;
    Knob<bool>& enable_debug;
    Knob<std::string>& module_name;
};

// Alternative approach using pointers instead of references:
class MyModuleAlt {
public:
    MyModuleAlt(const std::string& instance_name)
        : knob_list(csim::config::get_or_create(instance_name)) {
        // Initialize knobs in constructor body
        clock_period = &knob_list.add_knob("clock_period", "Clock period in ns", 1.0);
        buffer_size = &knob_list.add_knob("buffer_size", "Size of internal buffer", 1024);
        enable_debug = &knob_list.add_knob("enable_debug", "Enable debug output", false);
        module_name = &knob_list.add_knob("name", "Name of the module", std::string("default"));
    }

    void run() {
        std::cout << "Module " << knob_list.get_module_name() << ":" << std::endl;
        std::cout << "  Clock period: " << clock_period->get() << " ns" << std::endl;
        std::cout << "  Buffer size: " << buffer_size->get() << std::endl;
        std::cout << "  Debug enabled: " << (enable_debug->get() ? "yes" : "no") << std::endl;
        std::cout << "  Module name: " << module_name->get() << std::endl;
    }

private:
    KnobList& knob_list;
    Knob<double>* clock_period;
    Knob<int>* buffer_size;
    Knob<bool>* enable_debug;
    Knob<std::string>* module_name;
};

int main(int argc, char* argv[]) {
    // Create modules - they auto-register their knobs
    MyModule module1("module1");
    MyModule module2("module2");

    // Parse command line ONCE - handles --config automatically
    // The --config file is loaded first, then command line args override
    // Example: ./program --config config.txt --module1.clock_period 3.0
    //   - Loads all values from config.txt
    //   - Then overrides module1.clock_period to 3.0
    csim::config::parse_command_line(argc, argv);

    // Run modules with their configured values
    module1.run();
    module2.run();

    return 0;
}

// Command line usage examples:
// ./program --help
// ./program --module1.clock_period 2.0 --module2.buffer_size 4096
// ./program --config myconfig.txt
// ./program --config base.txt --module1.enable_debug  (config + override)
// ./program --dump-config current_settings.txt

// Config file format (config.txt):
// # Module configuration
// module1.clock_period = 2.0
// module1.buffer_size = 2048
// module1.enable_debug = true
// module2.clock_period = 1.0
// module2.buffer_size = 512
// module2.enable_debug = false
*/