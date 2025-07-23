#include <iostream>
#include <string>
#include <stdexcept>
#include <list>
#include <vector>
#include <sstream>
#include <memory>
#include <utility>
#include <functional>
#include <thread>
#include <chrono>

#include <cstring>

#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/IOGroup.hpp"
#include "LevelZeroIOGroup.hpp"
//#include "PlatformIOImp.hpp"

#include "geopm_pio.h"

std::vector<geopm_request_s> parse_signals(std::istream &in);
std::list<std::shared_ptr<geopm::IOGroup> > create_iogroups(
    std::vector<std::pair<std::string, std::function<std::unique_ptr<geopm::IOGroup>()> > > &iogroups);
std::vector<int> push_signals(geopm::PlatformIO &pio, const std::vector<geopm_request_s> &signal_config);
void run_loop(int num_loops, int period, geopm::PlatformIO &pio, const std::vector<int> &signal_handle);

int main(int argc, char **argv)
{
    static constexpr int NUM_LOOPS = 3;
    static constexpr int PERIOD_MS = 20;

    std::vector<std::pair<std::string, std::function<std::unique_ptr<geopm::IOGroup>()> > > iogroups = {
        {geopm::LevelZeroIOGroup::plugin_name(), geopm::LevelZeroIOGroup::make_plugin},
        {geopm::TimeIOGroup::plugin_name(), geopm::TimeIOGroup::make_plugin}
    };

    std::vector<geopm_request_s> signal_config;
    try {
        signal_config = parse_signals(std::cin);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        return -1;
    }

    std::list<std::shared_ptr<geopm::IOGroup> > iogroup_list = create_iogroups(iogroups);
    //static std::unique_ptr<geopm::PlatformIOImp> pio =
    //    geopm::make_unique<geopm::PlatformIOImp>(iogroup_list, geopm::platform_topo());
    geopm::PlatformIO &pio = geopm::platform_io(iogroup_list, geopm::platform_topo());
    std::vector<int> signal_handle = push_signals(pio, signal_config);
    run_loop(NUM_LOOPS, PERIOD_MS, pio, signal_handle);

    return 0;
}

std::vector<geopm_request_s> parse_signals(std::istream &in)
{
    std::string input_line;
    std::vector<geopm_request_s> signal_config;
    std::ostringstream msg;

    while (getline(in, input_line)) {
        if (input_line.empty()) {
            continue;
        }

        std::vector<std::string> split_line = geopm::string_split(input_line, " ");
        if (split_line.size() != 3) {
            msg << "Unexpected input: " << input_line;
            throw std::runtime_error(msg.str());
        }

        geopm_request_s request;
        try {
            request.domain_type = std::stoi(split_line[1]);
            request.domain_idx = std::stoi(split_line[2]);
        } catch(const std::exception &ex) {
            msg << "Unable to parse line: " << input_line << ": "
                      << ex.what();
            throw std::runtime_error(msg.str());
        }

	    size_t name_max = sizeof(request.name);
        request.name[name_max - 1] = '\0';
        std::strncpy(request.name, split_line[0].c_str(), name_max - 1);

        signal_config.push_back(std::move(request));
    }

    if (in.bad()) {
        msg << "Unable to read input";
        throw std::runtime_error(msg.str());
    }

    return signal_config;
}

std::list<std::shared_ptr<geopm::IOGroup> > create_iogroups(
    std::vector<std::pair<std::string, std::function<std::unique_ptr<geopm::IOGroup>()>>> &iogroups)
{
    std::list<std::shared_ptr<geopm::IOGroup> > iogroup_list;
    for (const auto &plugin : iogroups) {
        std::cout << "Instantiating IOGroup: " << plugin.first << std::endl;
        iogroup_list.push_back(plugin.second());
    }
    return iogroup_list;
}

std::vector<int> push_signals(geopm::PlatformIO &pio, const std::vector<geopm_request_s> &signal_config)
{
    std::vector<int> signal_handle;
    for (const auto &req : signal_config) {
        signal_handle.push_back(pio.push_signal(req.name, req.domain_type, req.domain_idx));
    }
    return signal_handle;
}

void run_loop(int num_loops, int period, geopm::PlatformIO &pio, const std::vector<int> &signal_handle)
{
    for (int i = 0; i < num_loops; ++i) {
        pio.read_batch();
        for (const auto &handle : signal_handle) {
            std::cout << pio.sample(handle) << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(period));
    }
}
