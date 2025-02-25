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

#ifndef COORDINATE_ALLOCATION_CC
#define COORDINATE_ALLOCATION_CC

#include <sstmac/hardware/interconnect/interconnect.h>
#include <sstmac/software/launch/coordinate_allocation.h>
#include <sstmac/backends/common/parallel_runtime.h>
#include <sstmac/hardware/topology/cartesian_topology.h>
#include <sprockit/fileio.h>
#include <sprockit/util.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/stl_string.h>
#include <sprockit/keyword_registration.h>

RegisterKeywords(
{ "launch_coordinate_file", "DEPRECATED: the file contining line-by-line coordinates for each node in an allocation" },
{ "coordinate_file", "the file contining line-by-line coordinates for each node in an allocation" },
);

namespace sstmac {
namespace sw {

CoordinateAllocation::CoordinateAllocation(SST::Params& params) :
  NodeAllocator(params)
{
  coord_file_ = params.find<std::string>("coordinate_file");
}

void
CoordinateAllocation::readCoordinateFile(
  ParallelRuntime* rt,
  const std::string &file,
  std::vector<hw::coordinates> &node_list)
{
  std::cout << "Test 0 in readCoordinateFile with " << file << std::endl;
  fflush(stdout);
  std::istream* instr = rt->bcastFileStream(file);
  std::cout << "Test 1 in readCoordinateFile with " << file << std::endl;
  fflush(stdout);
  std::istream& in = *instr;

  int num_nodes;
  int num_coords;

  in >> num_nodes;
  in >> num_coords;

  std::cout << "Test 1 in readCoordinateFile with " << num_nodes << ", " << num_coords << std::endl;
  fflush(stdout);
  node_list.resize(num_nodes);
  for (int nid=0; nid < num_nodes; ++nid){
    hw::coordinates coords(num_coords);
    for (int idx=0; idx < num_coords; ++idx){
        in >> coords[idx];
    }
    node_list[nid] = coords;
  }
  std::cout << "Test 2 in readCoordinateFile" << std::endl;
  fflush(stdout);

  delete instr;
}

bool
CoordinateAllocation::allocate(
  int nnode_requested,
  const ordered_node_set&  /*available*/,
  ordered_node_set& allocation) const
{
  std::vector<hw::coordinates> node_list;
  readCoordinateFile(rt_, coord_file_, node_list);

  hw::CartesianTopology* regtop = topology_->cartTopology();

  int num_coords = node_list[0].size();
  int top_ndim = regtop->ndimensions();
  int nps = regtop->concentration();
  if (nps > 1) ++top_ndim;
  if (top_ndim != num_coords){
    spkt_throw_printf(sprockit::ValueError,
        "coordinate_allocation::read_coordinate_file: mismatch between topology ndim=%d and file ncoords=%d, concentration=%d",
         top_ndim, num_coords, nps);
  }

  if (node_list.size() < nnode_requested){
    spkt_abort_printf("coordinate_allocation::allocation: requested %d, but only have %d nodes",
        int(node_list.size()), nnode_requested);
  }

  for (int i=0; i < nnode_requested; ++i){
    const hw::coordinates& coords = node_list[i];
    NodeId nid = regtop->node_addr(coords);
    debug_printf(sprockit::dbg::allocation,
        "adding node %d : %s to allocation",
        int(nid), stlString(coords).c_str());
    allocation.insert(nid);
  }

  return true;
}

}
}

#endif // COORDINATE_ALLOCATION_CC
