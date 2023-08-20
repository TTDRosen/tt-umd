#include "soc_descriptor.h"
#include "l1_address_map.h"
#include <fstream>
#include <assert.h>
#include "yaml-cpp/yaml.h"

const char* ws = " \t\n\r\f\v";

// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}

void load_core_descriptors_from_device_descriptor(
    YAML::Node device_descriptor_yaml, tt_SocDescriptor &soc_descriptor) {
  auto worker_l1_size = device_descriptor_yaml["worker_l1_size"].as<int>();
  auto eth_l1_size = device_descriptor_yaml["eth_l1_size"].as<int>();

  auto arc_cores = device_descriptor_yaml["arc"].as<std::vector<std::string>>();
  for (const auto &core_string : arc_cores) {
    CoreDescriptor core_descriptor;
    core_descriptor.coord = format_node(core_string);
    core_descriptor.type = CoreType::ARC;
    soc_descriptor.cores.insert({core_descriptor.coord, core_descriptor});
    soc_descriptor.arc_cores.push_back(core_descriptor.coord);
  }
  auto pcie_cores = device_descriptor_yaml["pcie"].as<std::vector<std::string>>();
  for (const auto &core_string : pcie_cores) {
    CoreDescriptor core_descriptor;
    core_descriptor.coord = format_node(core_string);
    core_descriptor.type = CoreType::PCIE;
    soc_descriptor.cores.insert({core_descriptor.coord, core_descriptor});
    soc_descriptor.pcie_cores.push_back(core_descriptor.coord);
  }

  int current_dram_channel = 0;
  for (auto channel_it = device_descriptor_yaml["dram"].begin(); channel_it != device_descriptor_yaml["dram"].end(); ++channel_it) {
    soc_descriptor.dram_cores.push_back({});
    auto &soc_dram_cores = soc_descriptor.dram_cores.at(soc_descriptor.dram_cores.size() - 1);
    const auto &dram_cores = (*channel_it).as<std::vector<std::string>>();
    for (unsigned int i = 0; i < dram_cores.size(); i++) {
      const auto &dram_core = dram_cores.at(i);
      CoreDescriptor core_descriptor;
      core_descriptor.coord = format_node(dram_core);
      core_descriptor.type = CoreType::DRAM;
      soc_descriptor.cores.insert({core_descriptor.coord, core_descriptor});
      soc_dram_cores.push_back(core_descriptor.coord);
      soc_descriptor.dram_core_channel_map[core_descriptor.coord] = {current_dram_channel, i};
    }
    current_dram_channel++;
  }
  auto eth_cores = device_descriptor_yaml["eth"].as<std::vector<std::string>>();
  int current_ethernet_channel = 0;
  for (const auto &core_string : eth_cores) {
    CoreDescriptor core_descriptor;
    core_descriptor.coord = format_node(core_string);
    core_descriptor.type = CoreType::ETH;
    core_descriptor.l1_size = eth_l1_size;
    soc_descriptor.cores.insert({core_descriptor.coord, core_descriptor});
    soc_descriptor.ethernet_cores.push_back(core_descriptor.coord);

    soc_descriptor.ethernet_core_channel_map[core_descriptor.coord] = current_ethernet_channel;
    current_ethernet_channel++;
  }
  std::vector<std::string> worker_cores = device_descriptor_yaml["functional_workers"].as<std::vector<std::string>>();
  std::set<int> worker_routing_coords_x;
  std::set<int> worker_routing_coords_y;
  std::unordered_map<int,int> routing_coord_worker_x;
  std::unordered_map<int,int> routing_coord_worker_y;
  for (const auto &core_string : worker_cores) {
    CoreDescriptor core_descriptor;
    core_descriptor.coord = format_node(core_string);
    core_descriptor.type = CoreType::WORKER;
    core_descriptor.l1_size = worker_l1_size;
    core_descriptor.dram_size_per_core = DEFAULT_DRAM_SIZE_PER_CORE;
    soc_descriptor.cores.insert({core_descriptor.coord, core_descriptor});
    soc_descriptor.workers.push_back(core_descriptor.coord);
    worker_routing_coords_x.insert(core_descriptor.coord.x);
    worker_routing_coords_y.insert(core_descriptor.coord.y);
  }

  int func_x_start = 0;
  int func_y_start = 0;
  std::set<int>::iterator it;
  for (it=worker_routing_coords_x.begin(); it!=worker_routing_coords_x.end(); ++it) {
    soc_descriptor.worker_log_to_routing_x[func_x_start] = *it;
    soc_descriptor.routing_x_to_worker_x[*it] = func_x_start;
    func_x_start++;
  }
  for (it=worker_routing_coords_y.begin(); it!=worker_routing_coords_y.end(); ++it) {
    soc_descriptor.worker_log_to_routing_y[func_y_start] = *it;
    soc_descriptor.routing_y_to_worker_y[*it] = func_y_start;
    func_y_start++;
  }

  soc_descriptor.worker_grid_size = tt_xy_pair(func_x_start, func_y_start);

  auto harvested_cores = device_descriptor_yaml["harvested_workers"].as<std::vector<std::string>>();
  for (const auto &core_string : harvested_cores) {
    CoreDescriptor core_descriptor;
    core_descriptor.coord = format_node(core_string);
    core_descriptor.type = CoreType::HARVESTED;
    soc_descriptor.cores.insert({core_descriptor.coord, core_descriptor});
  }
  auto router_only_cores = device_descriptor_yaml["router_only"].as<std::vector<std::string>>();
  for (const auto &core_string : router_only_cores) {
    CoreDescriptor core_descriptor;
    core_descriptor.coord = format_node(core_string);
    core_descriptor.type = CoreType::ROUTER_ONLY;
    soc_descriptor.cores.insert({core_descriptor.coord, core_descriptor});
  }
}

void load_soc_features_from_device_descriptor(YAML::Node &device_descriptor_yaml, tt_SocDescriptor *soc_descriptor) {
  soc_descriptor->overlay_version = device_descriptor_yaml["features"]["overlay"]["version"].as<int>();
  soc_descriptor->noc_translation_id_enabled = device_descriptor_yaml["features"]["noc"] && device_descriptor_yaml["features"]["noc"]["translation_id_enabled"] ? device_descriptor_yaml["features"]["noc"]["translation_id_enabled"].as<bool>() : false;
  soc_descriptor->packer_version = device_descriptor_yaml["features"]["packer"]["version"].as<int>();
  soc_descriptor->unpacker_version = device_descriptor_yaml["features"]["unpacker"]["version"].as<int>();
  soc_descriptor->dst_size_alignment = device_descriptor_yaml["features"]["math"]["dst_size_alignment"].as<int>();
  soc_descriptor->worker_l1_size = device_descriptor_yaml["worker_l1_size"].as<int>();
  soc_descriptor->eth_l1_size = device_descriptor_yaml["eth_l1_size"].as<int>();
  soc_descriptor->dram_bank_size = device_descriptor_yaml["dram_bank_size"].as<uint32_t>();
}

// Determines which core will write perf-events on which dram-bank.
// Creates a map of dram cores to worker cores, in the order that they will get dumped.
void map_workers_to_dram_banks(tt_SocDescriptor *soc_descriptor) {
  for (tt_xy_pair worker:soc_descriptor->workers) {
    assert(soc_descriptor->dram_cores.size() > 0);
    // Initialize target dram core to the first dram.
    tt_xy_pair target_dram_bank = soc_descriptor->dram_cores.at(0).at(0);
    std::vector<std::vector<tt_xy_pair>> dram_cores_per_channel;
    if (soc_descriptor->arch == tt::ARCH::WORMHOLE || soc_descriptor->arch == tt::ARCH::WORMHOLE_B0) {
      dram_cores_per_channel = {{tt_xy_pair(0, 0)}, {tt_xy_pair(0, 5)}, {tt_xy_pair(5, 0)}, {tt_xy_pair(5, 2)}, {tt_xy_pair(5, 3)}, {tt_xy_pair(5, 5)}};
    } else {
      dram_cores_per_channel = soc_descriptor->dram_cores;
    }
    for (const auto &dram_cores : dram_cores_per_channel) {
      for (tt_xy_pair dram: dram_cores) {
        int diff_x = worker.x - dram.x;
        int diff_y = worker.y - dram.y;
        // Represents a dram core that comes "before" this worker.
        if (diff_x >= 0 && diff_y >= 0) {
          int diff_dram_x = worker.x - target_dram_bank.x;
          int diff_dram_y = worker.y - target_dram_bank.y;
          // If initial dram core comes after the worker, swap it with this dram.
          if (diff_dram_x < 0 || diff_dram_y < 0) {
            target_dram_bank = dram;
            // If both target dram core and current dram core come before the worker, choose the one that's closer.
          } else if (diff_x + diff_y < diff_dram_x + diff_dram_y) {
            target_dram_bank = dram;
          }
        }
      }
    }
    if (soc_descriptor->perf_dram_bank_to_workers.find(target_dram_bank) == soc_descriptor->perf_dram_bank_to_workers.end()) {
      soc_descriptor->perf_dram_bank_to_workers.insert(std::pair<tt_xy_pair, std::vector<tt_xy_pair>>(target_dram_bank, {worker}));
    } else {
      soc_descriptor->perf_dram_bank_to_workers[target_dram_bank].push_back(worker);
    }
  }
}

std::unique_ptr<tt_SocDescriptor> load_soc_descriptor_from_yaml(std::string device_descriptor_file_path) {
    std::unique_ptr<tt_SocDescriptor> soc_descriptor = std::unique_ptr<tt_SocDescriptor>(new tt_SocDescriptor());

    std::ifstream fdesc(device_descriptor_file_path);
    if (fdesc.fail()) {
        throw std::runtime_error("Error: device descriptor file " + device_descriptor_file_path + " does not exist!");
    }
    fdesc.close();

    YAML::Node device_descriptor_yaml = YAML::LoadFile(device_descriptor_file_path);
    std::vector<std::size_t> trisc_sizes = {l1_mem::address_map::TRISC0_SIZE,
                                            l1_mem::address_map::TRISC1_SIZE,
                                            l1_mem::address_map::TRISC2_SIZE};  // TODO: Read trisc size from yaml

    auto grid_size_x = device_descriptor_yaml["grid"]["x_size"].as<int>();
    auto grid_size_y = device_descriptor_yaml["grid"]["y_size"].as<int>();
    int physical_grid_size_x = device_descriptor_yaml["physical"] && device_descriptor_yaml["physical"]["x_size"] ? 
                                device_descriptor_yaml["physical"]["x_size"].as<int>() : grid_size_x;
    int physical_grid_size_y = device_descriptor_yaml["physical"] && device_descriptor_yaml["physical"]["y_size"] ? 
                                device_descriptor_yaml["physical"]["y_size"].as<int>() : grid_size_y;

    load_core_descriptors_from_device_descriptor(device_descriptor_yaml, *soc_descriptor);
    soc_descriptor->grid_size = tt_xy_pair(grid_size_x, grid_size_y);
    soc_descriptor->physical_grid_size = tt_xy_pair(physical_grid_size_x, physical_grid_size_y);
    soc_descriptor->device_descriptor_file_path = device_descriptor_file_path;
    soc_descriptor->trisc_sizes = trisc_sizes;

    std::string arch_name_value = device_descriptor_yaml["arch_name"].as<std::string>();
    arch_name_value = trim(arch_name_value);

    soc_descriptor->arch = get_arch_name(arch_name_value);

    load_soc_features_from_device_descriptor(device_descriptor_yaml, soc_descriptor.get());

    map_workers_to_dram_banks(soc_descriptor.get());
    return soc_descriptor;
}