/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/simix/smx_private.hpp"
#include "src/include/simgrid/sg_config.h"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "simgrid/kernel/routing/ClusterZone.hpp"
#include "simgrid/kernel/routing/DijkstraZone.hpp"
#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/kernel/routing/EmptyZone.hpp"
#include "simgrid/kernel/routing/FatTreeZone.hpp"
#include "simgrid/kernel/routing/FloydZone.hpp"
#include "simgrid/kernel/routing/FullZone.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/kernel/routing/TorusZone.hpp"
#include "simgrid/kernel/routing/VivaldiZone.hpp"

#include <string>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);

XBT_PRIVATE std::map<std::string, simgrid::surf::StorageImpl*> mount_list;
XBT_PRIVATE std::vector<std::string> known_storages;

namespace simgrid {
namespace surf {

simgrid::xbt::signal<void(kernel::routing::ClusterCreationArgs*)> on_cluster;
}
}

static int surf_parse_models_setup_already_called = 0;
std::map<std::string, simgrid::surf::StorageType*> storage_types;

/** The current AS in the parsing */
static simgrid::kernel::routing::NetZoneImpl* current_routing = nullptr;
static simgrid::kernel::routing::NetZoneImpl* routing_get_current()
{
  return current_routing;
}

/** Module management function: creates all internal data structures */
void sg_platf_init()
{ /* Do nothing: just for symmetry of user code */
}

/** Module management function: frees all internal data structures */
void sg_platf_exit() {
  simgrid::surf::on_cluster.disconnectSlots();
  simgrid::s4u::onPlatformCreated.disconnectSlots();

  /* make sure that we will reinit the models while loading the platf once reinited */
  surf_parse_models_setup_already_called = 0;
  surf_parse_lex_destroy();
}

/** @brief Add an host to the current AS */
void sg_platf_new_host(simgrid::kernel::routing::HostCreationArgs* args)
{
  std::map<std::string, std::string> props;
  if (args->properties) {
    for (auto const& elm : *args->properties)
      props.insert({elm.first, elm.second});
    delete args->properties;
  }

  simgrid::s4u::Host* host =
      routing_get_current()->createHost(args->id, &args->speed_per_pstate, args->core_amount, &props);

  host->pimpl_->storage_ = mount_list;
  mount_list.clear();

  /* Change from the defaults */
  if (args->state_trace)
    host->pimpl_cpu->setStateTrace(args->state_trace);
  if (args->speed_trace)
    host->pimpl_cpu->setSpeedTrace(args->speed_trace);
  if (args->pstate != 0)
    host->pimpl_cpu->setPState(args->pstate);
  if (args->coord && strcmp(args->coord, ""))
    new simgrid::kernel::routing::vivaldi::Coords(host->pimpl_netpoint, args->coord);
}

/** @brief Add a "router" to the network element list */
simgrid::kernel::routing::NetPoint* sg_platf_new_router(std::string name, const char* coords)
{
  simgrid::kernel::routing::NetZoneImpl* current_routing = routing_get_current();

  if (current_routing->hierarchy_ == simgrid::kernel::routing::NetZoneImpl::RoutingMode::unset)
    current_routing->hierarchy_ = simgrid::kernel::routing::NetZoneImpl::RoutingMode::base;
  xbt_assert(nullptr == simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name),
             "Refusing to create a router named '%s': this name already describes a node.", name.c_str());

  simgrid::kernel::routing::NetPoint* netpoint =
      new simgrid::kernel::routing::NetPoint(name, simgrid::kernel::routing::NetPoint::Type::Router, current_routing);
  XBT_DEBUG("Router '%s' has the id %u", name.c_str(), netpoint->id());

  if (coords && strcmp(coords, ""))
    new simgrid::kernel::routing::vivaldi::Coords(netpoint, coords);


  return netpoint;
}

void sg_platf_new_link(simgrid::kernel::routing::LinkCreationArgs* link)
{
  std::vector<std::string> names;

  if (link->policy == SURF_LINK_SPLITDUPLEX) {
    names.push_back(link->id+ "_UP");
    names.push_back(link->id+ "_DOWN");
  } else {
    names.push_back(link->id);
  }
  for (auto const& link_name : names) {
    simgrid::surf::LinkImpl* l =
        surf_network_model->createLink(link_name, link->bandwidth, link->latency, link->policy);

    if (link->properties) {
      for (auto const& elm : *link->properties)
        l->setProperty(elm.first, elm.second);
    }

    if (link->latency_trace)
      l->setLatencyTrace(link->latency_trace);
    if (link->bandwidth_trace)
      l->setBandwidthTrace(link->bandwidth_trace);
    if (link->state_trace)
      l->setStateTrace(link->state_trace);
  }
  delete link->properties;
}

void sg_platf_new_cluster(simgrid::kernel::routing::ClusterCreationArgs* cluster)
{
  using simgrid::kernel::routing::ClusterZone;
  using simgrid::kernel::routing::DragonflyZone;
  using simgrid::kernel::routing::FatTreeZone;
  using simgrid::kernel::routing::TorusZone;

  int rankId=0;

  // What an inventive way of initializing the AS that I have as ancestor :-(
  simgrid::kernel::routing::ZoneCreationArgs zone;
  zone.id = cluster->id;
  switch (cluster->topology) {
    case simgrid::kernel::routing::ClusterTopology::TORUS:
      zone.routing = A_surfxml_AS_routing_ClusterTorus;
      break;
    case simgrid::kernel::routing::ClusterTopology::DRAGONFLY:
      zone.routing = A_surfxml_AS_routing_ClusterDragonfly;
      break;
    case simgrid::kernel::routing::ClusterTopology::FAT_TREE:
      zone.routing = A_surfxml_AS_routing_ClusterFatTree;
      break;
    default:
      zone.routing = A_surfxml_AS_routing_Cluster;
      break;
  }
  sg_platf_new_Zone_begin(&zone);
  simgrid::kernel::routing::ClusterZone* current_as = static_cast<ClusterZone*>(routing_get_current());
  current_as->parse_specific_arguments(cluster);

  if(cluster->loopback_bw > 0 || cluster->loopback_lat > 0){
    current_as->linkCountPerNode_++;
    current_as->hasLoopback_ = true;
  }

  if(cluster->limiter_link > 0){
    current_as->linkCountPerNode_++;
    current_as->hasLimiter_ = true;
  }

  for (int const& i : *cluster->radicals) {
    std::string host_id = std::string(cluster->prefix) + std::to_string(i) + cluster->suffix;
    std::string link_id = std::string(cluster->id) + "_link_" + std::to_string(i);

    XBT_DEBUG("<host\tid=\"%s\"\tpower=\"%f\">", host_id.c_str(), cluster->speeds.front());

    simgrid::kernel::routing::HostCreationArgs host;
    host.id = host_id.c_str();
    if ((cluster->properties != nullptr) && (not cluster->properties->empty())) {
      host.properties = new std::map<std::string, std::string>;

      for (auto const& elm : *cluster->properties)
        host.properties->insert({elm.first, elm.second});
    }

    host.speed_per_pstate = cluster->speeds;
    host.pstate = 0;
    host.core_amount = cluster->core_amount;
    host.coord = "";
    sg_platf_new_host(&host);
    XBT_DEBUG("</host>");

    XBT_DEBUG("<link\tid=\"%s\"\tbw=\"%f\"\tlat=\"%f\"/>", link_id.c_str(), cluster->bw, cluster->lat);

    // All links are saved in a matrix;
    // every row describes a single node; every node may have multiple links.
    // the first column may store a link from x to x if p_has_loopback is set
    // the second column may store a limiter link if p_has_limiter is set
    // other columns are to store one or more link for the node

    //add a loopback link
    simgrid::surf::LinkImpl* linkUp   = nullptr;
    simgrid::surf::LinkImpl* linkDown = nullptr;
    if(cluster->loopback_bw > 0 || cluster->loopback_lat > 0){
      std::string tmp_link = link_id + "_loopback";
      XBT_DEBUG("<loopback\tid=\"%s\"\tbw=\"%f\"/>", tmp_link.c_str(), cluster->loopback_bw);

      simgrid::kernel::routing::LinkCreationArgs link;
      link.id        = tmp_link;
      link.bandwidth = cluster->loopback_bw;
      link.latency   = cluster->loopback_lat;
      link.policy    = SURF_LINK_FATPIPE;
      sg_platf_new_link(&link);
      linkUp   = simgrid::surf::LinkImpl::byName(tmp_link);
      linkDown = simgrid::surf::LinkImpl::byName(tmp_link);

      auto* as_cluster = static_cast<ClusterZone*>(current_as);
      as_cluster->privateLinks_.insert({as_cluster->nodePosition(rankId), {linkUp, linkDown}});
    }

    //add a limiter link (shared link to account for maximal bandwidth of the node)
    linkUp   = nullptr;
    linkDown = nullptr;
    if(cluster->limiter_link > 0){
      std::string tmp_link = std::string(link_id) + "_limiter";
      XBT_DEBUG("<limiter\tid=\"%s\"\tbw=\"%f\"/>", tmp_link.c_str(), cluster->limiter_link);

      simgrid::kernel::routing::LinkCreationArgs link;
      link.id        = tmp_link;
      link.bandwidth = cluster->limiter_link;
      link.latency = 0;
      link.policy = SURF_LINK_SHARED;
      sg_platf_new_link(&link);
      linkDown = simgrid::surf::LinkImpl::byName(tmp_link);
      linkUp   = linkDown;
      current_as->privateLinks_.insert({current_as->nodePositionWithLoopback(rankId), {linkUp, linkDown}});
    }

    //call the cluster function that adds the others links
    if (cluster->topology == simgrid::kernel::routing::ClusterTopology::FAT_TREE) {
      static_cast<FatTreeZone*>(current_as)->addProcessingNode(i);
    } else {
      current_as->create_links_for_node(cluster, i, rankId, current_as->nodePositionWithLimiter(rankId));
    }
    rankId++;
  }
  delete cluster->properties;

  // Add a router.
  XBT_DEBUG(" ");
  XBT_DEBUG("<router id=\"%s\"/>", cluster->router_id.c_str());
  if (cluster->router_id.empty()) {
    std::string newid   = std::string(cluster->prefix) + cluster->id + "_router" + cluster->suffix;
    current_as->router_ = sg_platf_new_router(newid, NULL);
  } else {
    current_as->router_ = sg_platf_new_router(cluster->router_id, NULL);
  }

  //Make the backbone
  if ((cluster->bb_bw > 0) || (cluster->bb_lat > 0)) {

    simgrid::kernel::routing::LinkCreationArgs link;
    link.id        = std::string(cluster->id)+ "_backbone";
    link.bandwidth = cluster->bb_bw;
    link.latency   = cluster->bb_lat;
    link.policy    = cluster->bb_sharing_policy;

    XBT_DEBUG("<link\tid=\"%s\" bw=\"%f\" lat=\"%f\"/>", link.id.c_str(), cluster->bb_bw, cluster->bb_lat);
    sg_platf_new_link(&link);

    routing_cluster_add_backbone(simgrid::surf::LinkImpl::byName(link.id));
  }

  XBT_DEBUG("</AS>");
  sg_platf_new_Zone_seal();

  simgrid::surf::on_cluster(cluster);
  delete cluster->radicals;
}

void routing_cluster_add_backbone(simgrid::surf::LinkImpl* bb)
{
  simgrid::kernel::routing::ClusterZone* cluster =
      dynamic_cast<simgrid::kernel::routing::ClusterZone*>(current_routing);

  xbt_assert(cluster, "Only hosts from Cluster can get a backbone.");
  xbt_assert(nullptr == cluster->backbone_, "Cluster %s already has a backbone link!", cluster->getCname());

  cluster->backbone_ = bb;
  XBT_DEBUG("Add a backbone to AS '%s'", current_routing->getCname());
}

void sg_platf_new_cabinet(simgrid::kernel::routing::CabinetCreationArgs* cabinet)
{
  for (int const& radical : *cabinet->radicals) {
    std::string hostname = cabinet->prefix + std::to_string(radical) + cabinet->suffix;
    simgrid::kernel::routing::HostCreationArgs host;
    host.pstate           = 0;
    host.core_amount      = 1;
    host.id               = hostname.c_str();
    host.speed_per_pstate.push_back(cabinet->speed);
    sg_platf_new_host(&host);

    simgrid::kernel::routing::LinkCreationArgs link;
    link.policy    = SURF_LINK_SPLITDUPLEX;
    link.latency   = cabinet->lat;
    link.bandwidth = cabinet->bw;
    link.id        = "link_" + hostname;
    sg_platf_new_link(&link);

    simgrid::kernel::routing::HostLinkCreationArgs host_link;
    host_link.id        = hostname;
    host_link.link_up   = std::string("link_") + hostname + "_UP";
    host_link.link_down = std::string("link_") + hostname + "_DOWN";
    sg_platf_new_hostlink(&host_link);
  }
  delete cabinet->radicals;
}

void sg_platf_new_storage(simgrid::kernel::routing::StorageCreationArgs* storage)
{
  xbt_assert(std::find(known_storages.begin(), known_storages.end(), storage->id) == known_storages.end(),
             "Refusing to add a second storage named \"%s\"", storage->id.c_str());

  simgrid::surf::StorageType* stype;
  auto st = storage_types.find(storage->type_id);
  if (st != storage_types.end()) {
    stype = st->second;
  } else {
    xbt_die("No storage type '%s'", storage->type_id.c_str());
  }

  XBT_DEBUG("ROUTING Create a storage name '%s' with type_id '%s' and content '%s'", storage->id.c_str(),
            storage->type_id.c_str(), storage->content.c_str());

  known_storages.push_back(storage->id);

  // if storage content is not specified use the content of storage_type if any
  if (storage->content.empty() && not stype->content.empty()) {
    storage->content = stype->content;
    XBT_DEBUG("For disk '%s' content is empty, inherit the content (of type %s)", storage->id.c_str(),
              stype->id.c_str());
  }

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' "
            "\n\t\tmodel '%s' \n\t\tcontent '%s' "
            "\n\t\tproperties '%p''\n",
            storage->id.c_str(), stype->model.c_str(), stype->id.c_str(), storage->content.c_str(),
            storage->properties);

  auto s = surf_storage_model->createStorage(storage->id, stype->id, storage->content, storage->attach);

  if (storage->properties) {
    for (auto const& elm : *storage->properties)
      s->setProperty(elm.first, elm.second);
    delete storage->properties;
  }
}

void sg_platf_new_storage_type(simgrid::kernel::routing::StorageTypeCreationArgs* storage_type)
{
  xbt_assert(storage_types.find(storage_type->id) == storage_types.end(),
             "Reading a storage type, processing unit \"%s\" already exists", storage_type->id.c_str());

  simgrid::surf::StorageType* stype =
      new simgrid::surf::StorageType(storage_type->id, storage_type->model, storage_type->content,
                                     storage_type->properties, storage_type->model_properties, storage_type->size);

  XBT_DEBUG("Create a storage type id '%s' with model '%s', content '%s'", storage_type->id.c_str(),
            storage_type->model.c_str(), storage_type->content.c_str());

  storage_types[storage_type->id] = stype;
}

void sg_platf_new_mount(simgrid::kernel::routing::MountCreationArgs* mount)
{
  xbt_assert(std::find(known_storages.begin(), known_storages.end(), mount->storageId) != known_storages.end(),
             "Cannot mount non-existent disk \"%s\"", mount->storageId.c_str());

  XBT_DEBUG("Mount '%s' on '%s'", mount->storageId.c_str(), mount->name.c_str());

  if (mount_list.empty())
    XBT_DEBUG("Create a Mount list for %s", A_surfxml_host_id);
  mount_list.insert({mount->name, simgrid::s4u::Engine::getInstance()->storageByName(mount->storageId)->getImpl()});
}

void sg_platf_new_route(simgrid::kernel::routing::RouteCreationArgs* route)
{
  routing_get_current()->addRoute(route->src, route->dst, route->gw_src, route->gw_dst, route->link_list,
                                  route->symmetrical);
}

void sg_platf_new_bypassRoute(simgrid::kernel::routing::RouteCreationArgs* bypassRoute)
{
  routing_get_current()->addBypassRoute(bypassRoute->src, bypassRoute->dst, bypassRoute->gw_src, bypassRoute->gw_dst,
                                        bypassRoute->link_list, bypassRoute->symmetrical);
}

void sg_platf_new_actor(simgrid::kernel::routing::ActorCreationArgs* actor)
{
  sg_host_t host = sg_host_by_name(actor->host);
  if (not host) {
    // The requested host does not exist. Do a nice message to the user
    std::string msg = std::string("Cannot create actor '") + actor->function + "': host '" + actor->host +
                      "' does not exist\nExisting hosts: '";

    std::vector<simgrid::s4u::Host*> list = simgrid::s4u::Engine::getInstance()->getAllHosts();

    for (auto const& host : list) {
      msg += host->getName();
      msg += "', '";
      if (msg.length() > 1024) {
        msg.pop_back(); // remove trailing quote
        msg += "...(list truncated)......";
        break;
      }
    }
    xbt_die("%s", msg.c_str());
  }
  simgrid::simix::ActorCodeFactory& factory = SIMIX_get_actor_code_factory(actor->function);
  xbt_assert(factory, "Function '%s' unknown", actor->function);

  double start_time = actor->start_time;
  double kill_time  = actor->kill_time;
  bool auto_restart = actor->on_failure != simgrid::kernel::routing::ActorOnFailure::DIE;

  std::string actor_name     = actor->args[0];
  std::function<void()> code = factory(std::move(actor->args));
  std::shared_ptr<std::map<std::string, std::string>> properties(actor->properties);

  simgrid::simix::ProcessArg* arg =
      new simgrid::simix::ProcessArg(actor_name, code, nullptr, host, kill_time, properties, auto_restart);

  host->extension<simgrid::simix::Host>()->boot_processes.push_back(arg);

  if (start_time > SIMIX_get_clock()) {

    arg = new simgrid::simix::ProcessArg(actor_name, code, nullptr, host, kill_time, properties, auto_restart);

    XBT_DEBUG("Process %s@%s will be started at time %f", arg->name.c_str(), arg->host->getCname(), start_time);
    SIMIX_timer_set(start_time, [arg, auto_restart]() {
      smx_actor_t actor = simix_global->create_process_function(arg->name.c_str(), std::move(arg->code), arg->data,
                                                                arg->host, arg->properties.get(), nullptr);
      if (arg->kill_time >= 0)
        simcall_process_set_kill_time(actor, arg->kill_time);
      if (auto_restart)
        SIMIX_process_auto_restart_set(actor, auto_restart);
      delete arg;
    });
  } else {                      // start_time <= SIMIX_get_clock()
    XBT_DEBUG("Starting Process %s(%s) right now", arg->name.c_str(), host->getCname());

    smx_actor_t actor = simix_global->create_process_function(arg->name.c_str(), std::move(code), nullptr, host,
                                                              arg->properties.get(), nullptr);

    /* The actor creation will fail if the host is currently dead, but that's fine */
    if (actor != nullptr) {
      if (arg->kill_time >= 0)
        simcall_process_set_kill_time(actor, arg->kill_time);
      if (auto_restart)
        SIMIX_process_auto_restart_set(actor, auto_restart);
    }
  }
}

void sg_platf_new_peer(simgrid::kernel::routing::PeerCreationArgs* peer)
{
  simgrid::kernel::routing::VivaldiZone* as = dynamic_cast<simgrid::kernel::routing::VivaldiZone*>(current_routing);
  xbt_assert(as, "<peer> tag can only be used in Vivaldi netzones.");

  std::vector<double> speedPerPstate;
  speedPerPstate.push_back(peer->speed);
  simgrid::s4u::Host* host = as->createHost(peer->id.c_str(), &speedPerPstate, 1, nullptr);

  as->setPeerLink(host->pimpl_netpoint, peer->bw_in, peer->bw_out, peer->coord);

  /* Change from the defaults */
  if (peer->state_trace)
    host->pimpl_cpu->setStateTrace(peer->state_trace);
  if (peer->speed_trace)
    host->pimpl_cpu->setSpeedTrace(peer->speed_trace);
}

void sg_platf_begin() { /* Do nothing: just for symmetry of user code */ }

void sg_platf_end() {
  simgrid::s4u::onPlatformCreated();
}

/* Pick the right models for CPU, net and host, and call their model_init_preparse */
static void surf_config_models_setup()
{
  std::string host_model_name    = xbt_cfg_get_string("host/model");
  std::string network_model_name = xbt_cfg_get_string("network/model");
  std::string cpu_model_name     = xbt_cfg_get_string("cpu/model");
  std::string storage_model_name = xbt_cfg_get_string("storage/model");

  /* The compound host model is needed when using non-default net/cpu models */
  if ((not xbt_cfg_is_default_value("network/model") || not xbt_cfg_is_default_value("cpu/model")) &&
      xbt_cfg_is_default_value("host/model")) {
    host_model_name = "compound";
    xbt_cfg_set_string("host/model", host_model_name.c_str());
  }

  XBT_DEBUG("host model: %s", host_model_name.c_str());
  if (host_model_name == "compound") {
    xbt_assert(not cpu_model_name.empty(), "Set a cpu model to use with the 'compound' host model");
    xbt_assert(not network_model_name.empty(), "Set a network model to use with the 'compound' host model");

    int cpu_id = find_model_description(surf_cpu_model_description, cpu_model_name);
    surf_cpu_model_description[cpu_id].model_init_preparse();

    int network_id = find_model_description(surf_network_model_description, network_model_name);
    surf_network_model_description[network_id].model_init_preparse();
  }

  XBT_DEBUG("Call host_model_init");
  int host_id = find_model_description(surf_host_model_description, host_model_name);
  surf_host_model_description[host_id].model_init_preparse();

  XBT_DEBUG("Call vm_model_init");
  surf_vm_model_init_HL13();

  XBT_DEBUG("Call storage_model_init");
  int storage_id = find_model_description(surf_storage_model_description, storage_model_name);
  surf_storage_model_description[storage_id].model_init_preparse();
}

/**
 * \brief Add a Zone to the platform
 *
 * Add a new autonomous system to the platform. Any elements (such as host, router or sub-Zone) added after this call
 * and before the corresponding call to sg_platf_new_Zone_seal() will be added to this Zone.
 *
 * Once this function was called, the configuration concerning the used models cannot be changed anymore.
 *
 * @param zone the parameters defining the Zone to build.
 */
simgrid::s4u::NetZone* sg_platf_new_Zone_begin(simgrid::kernel::routing::ZoneCreationArgs* zone)
{
  if (not surf_parse_models_setup_already_called) {
    /* Initialize the surf models. That must be done after we got all config, and before we need the models.
     * That is, after the last <config> tag, if any, and before the first of cluster|peer|AS|trace|trace_connect
     *
     * I'm not sure for <trace> and <trace_connect>, there may be a bug here
     * (FIXME: check it out by creating a file beginning with one of these tags)
     * but cluster and peer create ASes internally, so putting the code in there is ok.
     */
    surf_parse_models_setup_already_called = 1;
    surf_config_models_setup();
  }

  _sg_cfg_init_status = 2; /* HACK: direct access to the global controlling the level of configuration to prevent
                            * any further config now that we created some real content */

  /* search the routing model */
  simgrid::kernel::routing::NetZoneImpl* new_zone = nullptr;
  switch (zone->routing) {
    case A_surfxml_AS_routing_Cluster:
      new_zone = new simgrid::kernel::routing::ClusterZone(current_routing, zone->id);
      break;
    case A_surfxml_AS_routing_ClusterDragonfly:
      new_zone = new simgrid::kernel::routing::DragonflyZone(current_routing, zone->id);
      break;
    case A_surfxml_AS_routing_ClusterTorus:
      new_zone = new simgrid::kernel::routing::TorusZone(current_routing, zone->id);
      break;
    case A_surfxml_AS_routing_ClusterFatTree:
      new_zone = new simgrid::kernel::routing::FatTreeZone(current_routing, zone->id);
      break;
    case A_surfxml_AS_routing_Dijkstra:
      new_zone = new simgrid::kernel::routing::DijkstraZone(current_routing, zone->id, false);
      break;
    case A_surfxml_AS_routing_DijkstraCache:
      new_zone = new simgrid::kernel::routing::DijkstraZone(current_routing, zone->id, true);
      break;
    case A_surfxml_AS_routing_Floyd:
      new_zone = new simgrid::kernel::routing::FloydZone(current_routing, zone->id);
      break;
    case A_surfxml_AS_routing_Full:
      new_zone = new simgrid::kernel::routing::FullZone(current_routing, zone->id);
      break;
    case A_surfxml_AS_routing_None:
      new_zone = new simgrid::kernel::routing::EmptyZone(current_routing, zone->id);
      break;
    case A_surfxml_AS_routing_Vivaldi:
      new_zone = new simgrid::kernel::routing::VivaldiZone(current_routing, zone->id);
      break;
    default:
      xbt_die("Not a valid model!");
      break;
  }

  if (current_routing == nullptr) { /* it is the first one */
    xbt_assert(simgrid::s4u::Engine::getInstance()->pimpl->netRoot_ == nullptr,
               "All defined components must belong to a networking zone.");
    simgrid::s4u::Engine::getInstance()->pimpl->netRoot_ = new_zone;

  } else {
    /* set the father behavior */
    if (current_routing->hierarchy_ == simgrid::kernel::routing::NetZoneImpl::RoutingMode::unset)
      current_routing->hierarchy_ = simgrid::kernel::routing::NetZoneImpl::RoutingMode::recursive;
    /* add to the sons dictionary */
    current_routing->getChildren()->push_back(static_cast<simgrid::s4u::NetZone*>(new_zone));
  }

  /* set the new current component of the tree */
  current_routing = new_zone;

  simgrid::s4u::NetZone::onCreation(*new_zone); // notify the signal

  return new_zone;
}

/**
 * \brief Specify that the description of the current AS is finished
 *
 * Once you've declared all the content of your AS, you have to seal
 * it with this call. Your AS is not usable until you call this function.
 */
void sg_platf_new_Zone_seal()
{
  xbt_assert(current_routing, "Cannot seal the current AS: none under construction");
  current_routing->seal();
  simgrid::s4u::NetZone::onSeal(*current_routing);
  current_routing = static_cast<simgrid::kernel::routing::NetZoneImpl*>(current_routing->getFather());
}

/** @brief Add a link connecting an host to the rest of its AS (which must be cluster or vivaldi) */
void sg_platf_new_hostlink(simgrid::kernel::routing::HostLinkCreationArgs* hostlink)
{
  simgrid::kernel::routing::NetPoint* netpoint = sg_host_by_name(hostlink->id.c_str())->pimpl_netpoint;
  xbt_assert(netpoint, "Host '%s' not found!", hostlink->id.c_str());
  xbt_assert(dynamic_cast<simgrid::kernel::routing::ClusterZone*>(current_routing),
             "Only hosts from Cluster and Vivaldi ASes can get an host_link.");

  simgrid::surf::LinkImpl* linkUp   = simgrid::surf::LinkImpl::byName(hostlink->link_up);
  simgrid::surf::LinkImpl* linkDown = simgrid::surf::LinkImpl::byName(hostlink->link_down);

  xbt_assert(linkUp, "Link '%s' not found!", hostlink->link_up.c_str());
  xbt_assert(linkDown, "Link '%s' not found!", hostlink->link_down.c_str());

  auto* as_cluster = static_cast<simgrid::kernel::routing::ClusterZone*>(current_routing);

  if (as_cluster->privateLinks_.find(netpoint->id()) != as_cluster->privateLinks_.end())
    surf_parse_error(std::string("Host_link for '") + hostlink->id.c_str() + "' is already defined!");

  XBT_DEBUG("Push Host_link for host '%s' to position %u", netpoint->getCname(), netpoint->id());
  as_cluster->privateLinks_.insert({netpoint->id(), {linkUp, linkDown}});
}

void sg_platf_new_trace(simgrid::kernel::routing::TraceCreationArgs* trace)
{
  tmgr_trace_t tmgr_trace;
  if (not trace->file.empty()) {
    tmgr_trace = tmgr_trace_new_from_file(trace->file);
  } else {
    xbt_assert(not trace->pc_data.empty(), "Trace '%s' must have either a content, or point to a file on disk.",
               trace->id.c_str());
    tmgr_trace = tmgr_trace_new_from_string(trace->id, trace->pc_data, trace->periodicity);
  }
  traces_set_list.insert({trace->id, tmgr_trace});
}
