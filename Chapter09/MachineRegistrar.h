#pragma once
#include "Machine.h"
#include <functional>
#include <map>
#include <stdexcept>

#define REGISTER_MACHINE(name, type)                                           \
  \
namespace {                                                                    \
    tiger::AutoRegistrar<type> registered_##name(#name);                       \
  }

namespace tiger {

using MachineRegistrar =
    std::map<std::string, std::function<std::unique_ptr<Machine>()>>;

MachineRegistrar &machineRegistrar() {
  static MachineRegistrar registrar;
  return registrar;
}

struct NoSuchArchError : std::logic_error {
  using std::logic_error::logic_error;
};

template <typename Creator>
inline void
registerMachine(const std::string &arch,
                Creator&& creator) {
  machineRegistrar()[arch] = std::forward<Creator>(creator);
}

template <typename ArchMachine> struct AutoRegistrar {
  AutoRegistrar(const std::string &arch) {
    registerMachine(arch, []() {
      return std::make_unique<ArchMachine>();
    });
  }
};

inline std::unique_ptr<Machine> createMachine(const std::string &arch) {
  auto it = machineRegistrar().find(arch);
  if (it == machineRegistrar().end()) {
    throw NoSuchArchError{"no machine named " + arch};
  }
  return it->second();
}

} // namespace tiger