/**
Copyright 2009-2022 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2022, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <sstmac/software/libraries/unblock_event.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/hardware/node/node.h>
#include <sstmac/hardware/nic/nic.h>
#include <sstmac/hardware/memory/memory_model.h>
#include <sstmac/hardware/processor/processor.h>
#include <sstmac/hardware/interconnect/interconnect.h>
#include <sstmac/hardware/topology/topology.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/app.h>
#include <sstmac/software/launch/app_launcher.h>
#include <sstmac/software/launch/launch_event.h>
#include <sstmac/software/launch/job_launcher.h>
#include <sstmac/common/event_callback.h>
#include <sstmac/common/runtime.h>
#include <sprockit/keyword_registration.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/util.h>
#include <sprockit/output.h>
#include <thread>

#include <sstmac/sst_core/integrated_component.h>
#include <sstmac/sst_core/connectable_wrapper.h>

RegisterDebugSlot(node)
RegisterNamespaces("os", "memory", "proc", "node");
RegisterKeywords(
{ "nsockets", "the number of sockets/processors in a node" },
{ "node_name", "DEPRECATED: the type of node on each endpoint" },
{ "node_memory_model", "DEPRECATED: the type of memory model on each node" },
{ "node_sockets", "DEPRECATED: the number of sockets/processors in a node" },
{ "job_launcher", "the type of launcher for scheduling jobs on the system - equivalent to MOAB or SLURM" },
);

namespace sstmac {
namespace hw {

using namespace sstmac::sw;

Node::Node(uint32_t id, SST::Params& params)
  : ConnectableComponent(id, params),
  app_refcount_(0),
  job_launcher_(nullptr)
{
  //printf("Test 0 in Node of node/node.cc\n");
  fflush(stdout);
#if SSTMAC_INTEGRATED_SST_CORE
  //printf("Test 1 in Node of node/node.cc\n");
  fflush(stdout);
  static bool init_debug = false;
  if (!init_debug){
    std::vector<std::string> debug_params;
    if (params.contains("debug")){
      params.find_array("debug", debug_params);
    }
    for (auto& str : debug_params){
      sprockit::Debug::turnOn(str);
    }
    init_debug = true;
  }
#endif
  //printf("Test 2 in Node of node/node.cc\n");
  fflush(stdout);
  my_addr_ = params.find<int>("id");
  next_outgoing_id_.setSrcNode(my_addr_);

  //std::cout << my_addr_ << " ";
  //printf("Test 3 in Node of node/node.cc\n");
  fflush(stdout);
  SST::Params nic_params = params.get_scoped_params("nic");
  auto nic_name = nic_params.find<std::string>("name");

  //std::cout << nic_name << " ";
  //printf("Test 4 in Node of node/node.cc\n");
  fflush(stdout);
  SST::Params mem_params = params.get_scoped_params("memory");
  auto mem_name = mem_params.find<std::string>("name");
  mem_model_ = loadSub<MemoryModel>(mem_name, "memory", MEMORY_SLOT, mem_params, this);

  //std::cout << mem_name << " ";
  //printf("Test 5 in Node of node/node.cc\n");
  fflush(stdout);
  SST::Params proc_params = params.get_scoped_params("proc");
  auto proc_name = proc_params.find<std::string>("processor", "instruction");
  if (proc_name.empty()){
    spkt_abort_printf("Missing node.processor parameter");
  }
  proc_ = sprockit::create<Processor>("macro", proc_name, proc_params, mem_model_, this);

  //std::cout << proc_name << " ";
  //printf("Test 6 in Node of node/node.cc\n");
  fflush(stdout);
  nsocket_ = params.find<int>("nsockets", 1);

  //std::cout << nsocket_ << " ";
  //printf("Test 7 in Node of node/node.cc\n");
  fflush(stdout);
  SST::Params os_params = params.get_scoped_params("os");
  os_ = newSub<sw::OperatingSystem>("os", OS_SLOT, os_params, this);

  //printf("Test 8 in Node of node/node.cc\n");
  fflush(stdout);
  app_launcher_ = new AppLauncher(os_);

  //printf("Test 9 in Node of node/node.cc\n");
  fflush(stdout);
  launchRoot_ = params.find<int>("launchRoot", 0);
  if (my_addr_ == launchRoot_){
    job_launcher_ = sprockit::create<JobLauncher>(
      "macro", params.find<std::string>("job_launcher", "default"), params, os_);
  }

  //std::cout << launchRoot_ << " ";
  //printf("Test 10 in Node of node/node.cc\n");
  fflush(stdout);
#if SSTMAC_INTEGRATED_SST_CORE
  // we need all the params to make sure static topology can get constructed on all simulation ranks
  params.insert(nic_params);
  nic_ = loadSub<NIC>(nic_name, "nic", NIC_SLOT, params, this);
#else
  nic_ = loadSub<NIC>(nic_name, "nic", NIC_SLOT, nic_params, this);
#endif

  //printf("Test 11 in Node of node/node.cc\n");
  fflush(stdout);
  //nic_ = sprockit::create<NIC>("macro", nic_name, nic_params, this);
  //sstmac::loadSubComponent<NIC>(nic_name, this, nic_params);
}

LinkHandler*
Node::creditHandler(int port)
{
  return nic_->creditHandler(port);
}

std::string
Node::hostname() const
{
  return nic_-> topology()->nodeIdToName(addr());
}

LinkHandler*
Node::payloadHandler(int port)
{
  return nic_->payloadHandler(port);
}

void
Node::setup()
{
  //printf("Test 0 in Node::setup of hardware/node/node.cc\n");
  fflush(stdout);
  Component::setup();
  mem_model_->setup();
  os_->setup();
  nic_->setup();
  if (job_launcher_){
    job_launcher_->scheduleLaunchRequests();
  }
  //printf("Test 1 in Node::setup of hardware/node/node.cc\n");
  fflush(stdout);
}

void
Node::init(unsigned int phase)
{
  //printf("Test 0 in Node::init of hardware/node/node.cc\n");
  fflush(stdout);
#if SSTMAC_INTEGRATED_SST_CORE
  Component::init(phase);
#endif
  //printf("Test 1 in Node::init of hardware/node/node.cc\n");
  fflush(stdout);
  nic_->init(phase);
  //printf("Test 2 in Node::init of hardware/node/node.cc\n");
  fflush(stdout);
  os_->init(phase);
  //printf("Test 3 in Node::init of hardware/node/node.cc\n");
  fflush(stdout);
  mem_model_->init(phase);
  //printf("Test 4 in Node::init of hardware/node/node.cc\n");
  fflush(stdout);
}

Node::~Node()
{
  if (job_launcher_) delete job_launcher_;
  if (app_launcher_) delete app_launcher_;
  if (mem_model_) delete mem_model_;
  if (proc_) delete proc_;
  if (os_) delete os_;
  if (nic_) delete nic_;
  //if (JobLauncher_) delete JobLauncher_;
}

void
Node::connectOutput(int src_outport, int dst_inport, EventLink::ptr&& link)
{
  //forward connection to nic
  nic_->connectOutput(src_outport, dst_inport, std::move(link));
}

void
Node::connectInput(int src_outport, int dst_inport, EventLink::ptr&& link)
{
  //forward connection to nic
  nic_->connectInput(src_outport, dst_inport, std::move(link));
}

void
Node::execute(ami::SERVICE_FUNC  /*func*/, Event*  /*data*/)
{
  sprockit::abort("node does not implement asynchronous services - choose new node model");
}

std::string
Node::toString() const
{
  return sprockit::sprintf("node(%d)", int(my_addr_));
}

void
Node::incrementAppRefcount()
{
#if SSTMAC_INTEGRATED_SST_CORE
  if (app_refcount_ == 0){
    primaryComponentDoNotEndSim();
  }
#endif
  ++app_refcount_;
}

void
Node::decrementAppRefcount()
{
  app_refcount_--;

#if SSTMAC_INTEGRATED_SST_CORE
  if (app_refcount_ == 0){
    primaryComponentOKToEndSim();
  }
#endif
}

void
Node::handle(Request* req)
{
    //std::cout << "Test 0 in handle with id: " << my_addr_ << " of node/node.cc" << std::endl;
    fflush(stdout);
  os_->handleRequest(req);
    //std::cout << "Test 1 in handle of node/node.cc" << std::endl;
    fflush(stdout);
}

}
} // end of namespace sstmac
