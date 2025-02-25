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

#include <sstmac/common/runtime.h>
#include <sstmac/hardware/interconnect/interconnect.h>
#include <sstmac/hardware/topology/cartesian_topology.h>
#include <sstmac/software/process/app.h>
#include <sstmac/software/launch/launch_request.h>
#include <sstmac/software/launch/job_launcher.h>
#include <sstmac/common/thread_lock.h>
#include <sstmac/dumpi_util/dumpi_meta.h>
#include <sprockit/output.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/keyword_registration.h>
#include <sprockit/util.h>
#include <sprockit/stl_string.h>
#include <sprockit/basic_string_tokenizer.h>
#include <unistd.h>
#include <getopt.h>


RegisterKeywords(
{ "launch_cmd", "a command specified via aprun or srun or mpirun to launch" },
{ "argv", "the list of command-line arguments" },
{ "allocation", "the strategy for allocating nodes to a job" },
{ "indexing", "the strategy for indexing ranks to nodes within a job" },
{ "start", "the time to issue a job start request" },
{ "size", "the number of tasks (ranks) within a job" },
);

namespace sstmac {
namespace sw {

SoftwareLaunchRequest::SoftwareLaunchRequest(SST::Params& params) :
  indexed_(false),
  num_finished_(0)
{
  printf("Test 0 in softwareLaunchRequest of launch/launch_request.cc\n");
  fflush(stdout);
  if (params.contains("core_affinities")) {
    params.find_array("core_affinities", core_affinities_);
  }

  time_ = Timestamp(params.find<SST::UnitAlgebra>("start", "0s").getValue().toDouble());

  if (params.contains("launch_cmd")){
    parseLaunchCmd(params);
  } else if (params.contains("dumpi_metaname")){
    std::string metafile = params.find<std::string>("dumpi_metaname");
    sw::DumpiMeta dm(metafile);
    nproc_ = dm.numProcs();
    procs_per_node_ = 1;
  } else {
    nproc_ = params.find<int>("size");
    procs_per_node_ = params.find<int>("tasks_per_node", 1);
  }

  allocator_ = sprockit::create<sw::NodeAllocator>(
     "macro", params.find<std::string>("allocation", "first_available"), params);

  indexer_ = sprockit::create<sw::TaskMapper>(
     "macro", params.find<std::string>("indexing", "block"), params);
  printf("Test 1 in softwareLaunchRequest of launch/launch_request.cc\n");
  fflush(stdout);
}

SoftwareLaunchRequest::~SoftwareLaunchRequest()
{
  delete allocator_;
  delete indexer_;
}

AppLaunchRequest::~AppLaunchRequest()
{
}

AppLaunchRequest::AppLaunchRequest(SST::Params& params,
                       AppId aid,
                       const std::string& app_namespace) :
  SoftwareLaunchRequest(params),
  aid_(aid),
  app_namespace_(app_namespace),
  app_params_(params)
{
}

void
SoftwareLaunchRequest::indexAllocation(
  hw::Topology* top,
  const ordered_node_set& allocation,
  std::vector<NodeId>& rank_to_node_indexing,
  std::vector<std::list<int>>& node_to_rank_indexing)
{
  indexer_->mapRanks(allocation,
               procs_per_node_,
               rank_to_node_indexing,
               nproc_);

  if (sprockit::Debug::slotActive(sprockit::dbg::indexing)){
    cout0 << sprockit::sprintf("Allocated and indexed %d nodes\n",
                rank_to_node_indexing.size());
    int num_nodes = rank_to_node_indexing.size();

    hw::CartesianTopology* regtop =
        test_cast(hw::CartesianTopology, top);

    for (int i=0; i < num_nodes; ++i){
      NodeId nid = rank_to_node_indexing[i];
      if (regtop){
        hw::coordinates coords = regtop->node_coords(nid);
        cout0 << sprockit::sprintf("Rank %d -> nid%d %s\n",
            i, int(nid), stlString(coords).c_str());
      } else {
         cout0 << sprockit::sprintf("Rank %d -> nid%d\n", i, int(nid));
      }
    }

  }

  int num_nodes = top->numNodes();
  node_to_rank_indexing.resize(num_nodes);
  int num_ranks = rank_to_node_indexing.size();
  for (int i=0; i < num_ranks; ++i){
    NodeId nid = rank_to_node_indexing[i];
    node_to_rank_indexing[nid].push_back(i);
  }

  indexed_ = true;
}

bool
SoftwareLaunchRequest::requestAllocation(
  const sw::ordered_node_set& available,
  sw::ordered_node_set& allocation)
{
  int num_nodes = nproc_ / procs_per_node_;
  int remainder = nproc_ % procs_per_node_;
  if (remainder) {
    ++num_nodes;
  }
  return allocator_->allocate(num_nodes, available, allocation);
}

void
SoftwareLaunchRequest::parseLaunchCmd(
  SST::Params& params,
  int& nproc,
  int& procs_per_node,
  std::vector<int>& affinities)
{
  printf("Test 0 in parseLaunchCmd of launch/launch_request.cc\n");
  fflush(stdout);
  if (params.contains("launch_cmd")) {
    /** Check for an aprun launch */
    std::string launch_cmd = params.find<std::string>("launch_cmd");
    size_t pos = launch_cmd.find_first_of(' ');
    std::string launcher;
    if (pos != std::string::npos) {
      launcher = launch_cmd.substr(0, pos);
    } else {
      launcher = launch_cmd;
    }

    if (launcher == "aprun") {
      parseAprun(launch_cmd, nproc, procs_per_node, affinities);
      if (procs_per_node == -1) { //nothing given
        int ncores = params.find<int>("node_cores", 1);
        procs_per_node = ncores > nproc ? nproc : ncores;
      }
    } else {
      spkt_throw_printf(sprockit::ValueError,
                        "invalid launcher %s given", launcher.c_str());
    }
  } else { //standard launch
    try {
      nproc = params.find<long>("size");
      procs_per_node = params.find<int>("concentration", 1);
    } catch (sprockit::InputError& e) {
      cerr0 << "Problem reading app size parameter in app_launch_request.\n"
               "If this is a DUMPI trace, set app name to dumpi.\n";
      throw e;
    }
  }
  printf("Test 1 in parseLaunchCmd of launch/launch_request.cc\n");
  fflush(stdout);
}

void
SoftwareLaunchRequest::parseLaunchCmd(SST::Params& params)
{
  parseLaunchCmd(params, nproc_, procs_per_node_, core_affinities_);
}

void
SoftwareLaunchRequest::parseAprun(
  const std::string &cmd,
  int &nproc, int &nproc_per_node,
  std::vector<int>& core_affinities)
{
  int aprun_numa_containment = 0;
  option aprun_gopt[] = {
    { "n", required_argument, NULL, 'n' },
    { "N", required_argument, NULL, 'N' },
    { "ss", no_argument, &aprun_numa_containment, 1 },
    { "cc", required_argument, NULL, 'C' },
    { "S", required_argument, NULL, 'S' },
    { "d", required_argument, NULL, 'd' },
    { NULL, 0, NULL, '\0' }
  };

  char cmdline_str[200];
  ::strcpy(cmdline_str, cmd.c_str());
  char *argv[50];
  int argc = 0;
  char* pch = strtok(cmdline_str, " ");
  while (pch != 0) {
    argv[argc] = pch;
    ++argc;
    pch = strtok(0, " ");
  }

  int _nproc = -1;
  int _nproc_per_node = -1;
  int ch;
  int option_index = 1;
  std::string core_aff_str;
  optind = 1;
  while ((ch = getopt_long_only(argc, argv, "n:N:S:d:", aprun_gopt,
                                &option_index)) != -1) {
    switch (ch) {
      case 0: //this set an input flag
        break;
      case 'n':
        _nproc = atoi(optarg);
        break;
      case 'N':
        _nproc_per_node = atoi(optarg);
        break;
      case 'S':
        break;
      case 'C':
        core_aff_str = optarg;
        break;
      default:
        spkt_abort_printf("got invalid option in launch command");
        break;
    }
  }
  if (_nproc <= 0)
    throw sprockit::InputError(
      "aprun allocator did not receive a valid -n specification");

  if (_nproc_per_node == 0 || _nproc_per_node > _nproc)
    throw sprockit::InputError(
      "aprun allocator did not receive a valid -N specification");



  if (core_aff_str.size() > 0) { //we got a core affinity spec
    if (!core_affinities.empty()) {
      spkt_throw_printf(sprockit::IllformedError,
                       "app_launch_request::parse_aprun: core affinities already assigned, cannot use -cc option");
    }

    std::string tosep = core_aff_str;
    std::deque<std::string> tok;
    std::string space = ",";
    pst::BasicStringTokenizer::tokenize(tosep, tok, space);
    for (auto& core : tok){
      core_affinities.push_back(atoi(core.c_str()));
    }
  }

  nproc = _nproc;
  nproc_per_node = _nproc_per_node;
}



}
}
