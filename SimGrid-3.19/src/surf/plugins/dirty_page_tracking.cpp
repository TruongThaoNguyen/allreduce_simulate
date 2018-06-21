/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include <map>

namespace simgrid {
namespace vm {
class VmDirtyPageTrackingExt {
  bool dp_tracking = false;
  std::map<kernel::activity::ExecImplPtr, double> dp_objs;
  double dp_updated_by_deleted_tasks = 0.0;
  // Percentage of pages that get dirty compared to netspeed [0;1] bytes per 1 flop execution
  double dp_intensity          = 0.0;
  sg_size_t working_set_memory = 0.0;
  double max_downtime          = 0.03;
  double mig_speed             = 0.0;

public:
  void startTracking();
  void stopTracking() { dp_tracking = false; }
  bool isTracking() { return dp_tracking; }
  void track(kernel::activity::ExecImplPtr exec, double amount) { dp_objs.insert({exec, amount}); }
  void untrack(kernel::activity::ExecImplPtr exec) { dp_objs.erase(exec); }
  double getStoredRemains(kernel::activity::ExecImplPtr exec) { return dp_objs.at(exec); }
  void updateDirtyPageCount(double delta) { dp_updated_by_deleted_tasks += delta; }
  double computedFlopsLookup();
  double getIntensity() { return dp_intensity; }
  void setIntensity(double intensity) { dp_intensity = intensity; }
  double getWorkingSetMemory() { return working_set_memory; }
  void setWorkingSetMemory(sg_size_t size) { working_set_memory = size; }
  void setMigrationSpeed(double speed) { mig_speed = speed; }
  double getMigrationSpeed() { return mig_speed; }
  double getMaxDowntime() { return max_downtime; }

  static simgrid::xbt::Extension<VirtualMachineImpl, VmDirtyPageTrackingExt> EXTENSION_ID;
  virtual ~VmDirtyPageTrackingExt() = default;
  VmDirtyPageTrackingExt()          = default;
};

simgrid::xbt::Extension<VirtualMachineImpl, VmDirtyPageTrackingExt> VmDirtyPageTrackingExt::EXTENSION_ID;

void VmDirtyPageTrackingExt::startTracking()
{
  dp_tracking = true;
  for (auto const& elm : dp_objs)
    dp_objs[elm.first] = elm.first->remains();
}

double VmDirtyPageTrackingExt::computedFlopsLookup()
{
  double total = 0;

  for (auto const& elm : dp_objs) {
    total += elm.second - elm.first->remains();
    dp_objs[elm.first] = elm.first->remains();
  }
  total += dp_updated_by_deleted_tasks;

  dp_updated_by_deleted_tasks = 0;

  return total;
}
}
}

static void onVirtualMachineCreation(simgrid::vm::VirtualMachineImpl* vm)
{
  vm->extension_set<simgrid::vm::VmDirtyPageTrackingExt>(new simgrid::vm::VmDirtyPageTrackingExt());
}

static void onExecCreation(simgrid::kernel::activity::ExecImplPtr exec)
{
  simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(exec->host_);
  if (vm == nullptr)
    return;

  if (vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->isTracking()) {
    vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->track(exec, exec->remains());
  } else {
    vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->track(exec, 0.0);
  }
}

static void onExecCompletion(simgrid::kernel::activity::ExecImplPtr exec)
{
  simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(exec->host_);
  if (vm == nullptr)
    return;

  /* If we are in the middle of dirty page tracking, we record how much computation has been done until now, and keep
   * the information for the lookup_() function that will called soon. */
  if (vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->isTracking()) {
    double delta =
        vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->getStoredRemains(exec) - exec->remains();
    vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->updateDirtyPageCount(delta);
  }
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->untrack(exec);
}

SG_BEGIN_DECL()

void sg_vm_dirty_page_tracking_init()
{
  if (not simgrid::vm::VmDirtyPageTrackingExt::EXTENSION_ID.valid()) {
    simgrid::vm::VmDirtyPageTrackingExt::EXTENSION_ID =
        simgrid::vm::VirtualMachineImpl::extension_create<simgrid::vm::VmDirtyPageTrackingExt>();
    simgrid::vm::VirtualMachineImpl::onVmCreation.connect(&onVirtualMachineCreation);
    simgrid::kernel::activity::ExecImpl::onCreation.connect(&onExecCreation);
    simgrid::kernel::activity::ExecImpl::onCompletion.connect(&onExecCompletion);
  }
}

void sg_vm_start_dirty_page_tracking(sg_vm_t vm)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->startTracking();
}

void sg_vm_stop_dirty_page_tracking(sg_vm_t vm)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->stopTracking();
}

double sg_vm_lookup_computed_flops(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->computedFlopsLookup();
}

void sg_vm_set_dirty_page_intensity(sg_vm_t vm, double intensity)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->setIntensity(intensity);
}

double sg_vm_get_dirty_page_intensity(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->getIntensity();
}

void sg_vm_set_working_set_memory(sg_vm_t vm, sg_size_t size)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->setWorkingSetMemory(size);
}

sg_size_t sg_vm_get_working_set_memory(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->getWorkingSetMemory();
}

void sg_vm_set_migration_speed(sg_vm_t vm, double speed)
{
  vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->setMigrationSpeed(speed);
}

double sg_vm_get_migration_speed(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->getMigrationSpeed();
}

double sg_vm_get_max_downtime(sg_vm_t vm)
{
  return vm->getImpl()->extension<simgrid::vm::VmDirtyPageTrackingExt>()->getMaxDowntime();
}

SG_END_DECL()
