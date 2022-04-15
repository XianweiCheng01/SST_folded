// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "sst/core/event.h"
#include "sst/core/simulation.h"

#include "sst/core/link.h"

namespace SST {


std::atomic<uint64_t> SST::Event::id_counter(0);
const SST::Event::id_type SST::Event::NO_ID = std::make_pair(0, -1);

Event::~Event() {}


void Event::execute(void)
{
    // printf("Test 0 in Event::execute() of event.cc\n");
    fflush(stdout);
    delivery_link->deliverEvent(this);
    // printf("Test 1 in Event::execute() of event.cc\n");
    fflush(stdout);
}

Event* Event::clone() {
    Simulation::getSimulation()->getSimulationOutput().
        fatal(CALL_INFO,1,"Called clone() on an Event that doesn't"
              " implement it.");
    return nullptr;  // Never reached, but gets rid of compiler warning
}

Event::id_type Event::generateUniqueId()
{
    return std::make_pair(id_counter++, Simulation::getSimulation()->getRank().rank);
}


void NullEvent::execute(void)
{
    // printf("Test 0 in NullEvent::execute() of event.cc\n");
    fflush(stdout);
    delivery_link->deliverEvent(nullptr);
    // printf("Test 1 in NullEvent::execute() of event.cc\n");
    fflush(stdout);
    delete this;
}



#ifdef USE_MEMPOOL
std::mutex Activity::poolMutex;
std::vector<Activity::PoolInfo_t> Activity::memPools;
#endif


} // namespace SST
