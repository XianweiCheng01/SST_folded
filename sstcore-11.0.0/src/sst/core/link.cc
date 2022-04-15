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
#include "sst/core/link.h"

#include <utility>

#include "sst/core/event.h"
#include "sst/core/initQueue.h"
#include "sst/core/pollingLinkQueue.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeConverter.h"
#include "sst/core/timeLord.h"
#include "sst/core/timeVortex.h"
//#include "sst/core/syncQueue.h"
#include "sst/core/uninitializedQueue.h"
#include "sst/core/unitAlgebra.h"

namespace SST {

// ActivityQueue* Link::uninitQueue = nullptr;
ActivityQueue* Link::uninitQueue = new UninitializedQueue("ERROR: Trying to send or recv from link during initialization.  Send and Recv cannot be called before setup.");
ActivityQueue* Link::afterInitQueue = new UninitializedQueue("ERROR: Trying to call sendUntimedData/sendInitData or recvUntimedData/recvInitData during the run phase.");
ActivityQueue* Link::afterRunQueue = new UninitializedQueue("ERROR: Trying to call send or recv during complete phase.");

Link::Link(LinkId_t id) :
    rFunctor( nullptr ),
    defaultTimeBase( nullptr ),
    latency(1),
    type(HANDLER),
    id(id),
    configured(false)
{
    recvQueue = uninitQueue;
    untimedQueue = nullptr;
    configuredQueue = Simulation_impl::getSimulation()->getTimeVortex();
}

Link::Link() :
    rFunctor( nullptr ),
    defaultTimeBase( nullptr ),
    latency(1),
    type(HANDLER),
    id(-1),
    configured(false)
{
    recvQueue = uninitQueue;
    untimedQueue = nullptr;
    configuredQueue = Simulation_impl::getSimulation()->getTimeVortex();
}

Link::~Link() {
    if ( type == POLL && recvQueue != uninitQueue && recvQueue != afterInitQueue && recvQueue != afterRunQueue ) {
        delete recvQueue;
    }
    if ( rFunctor != nullptr ) delete rFunctor;
}

void Link::finalizeConfiguration() {
    recvQueue = configuredQueue;
    configuredQueue = untimedQueue;
    if ( untimedQueue != nullptr ) {
        if ( dynamic_cast<InitQueue*>(untimedQueue) != nullptr) {
            delete untimedQueue;
            configuredQueue = nullptr;
        }
    }
    // printf("Test 0 in finalizeConfiguration in link.cc\n");
    fflush(stdout);
    untimedQueue = afterInitQueue;
}

void Link::prepareForComplete() {
    if ( type == POLL && recvQueue != uninitQueue && recvQueue != afterInitQueue ) {
        delete recvQueue;
    }
    // printf("Test 0 in prepareForComplete in link.cc\n");
    fflush(stdout);
    recvQueue = afterRunQueue;
    untimedQueue = configuredQueue;
}

void Link::setPolling() {
    type = POLL;
    configuredQueue = new PollingLinkQueue();
}


void Link::setLatency(Cycle_t lat) {
    latency = lat;
}

void Link::addSendLatency(int cycles, const std::string& timebase) {
    SimTime_t tb = Simulation_impl::getSimulation()->getTimeLord()->getSimCycles(timebase,"addOutputLatency");
    latency += (cycles * tb);
}

void Link::addSendLatency(SimTime_t cycles, TimeConverter* timebase) {
    latency += timebase->convertToCoreTime(cycles);
}

void Link::addRecvLatency(int cycles, const std::string& timebase) {
    SimTime_t tb = Simulation::getSimulation()->getTimeLord()->getSimCycles(timebase,"addOutputLatency");
    pair_link->latency += (cycles * tb);
}

void Link::addRecvLatency(SimTime_t cycles, TimeConverter* timebase) {
    pair_link->latency += timebase->convertToCoreTime(cycles);
}

void Link::setFunctor(Event::HandlerBase* functor) {
    if ( UNLIKELY( type != HANDLER ) ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot call setFunctor on a Polling Link\n");
    }

    rFunctor = functor;
}

void Link::replaceFunctor(Event::HandlerBase* functor) {
    if ( UNLIKELY( type != HANDLER ) ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot call replaceFunctor on a Polling Link\n");
    }

    if ( rFunctor ) delete rFunctor;
    rFunctor = functor;
}

void Link::send( SimTime_t delay, TimeConverter* tc, Event* event ) {
    // printf("Test -1 in send of link.cc\n");
    fflush(stdout);
    if ( tc == nullptr ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot send an event on Link with nullptr TimeConverter\n");
    }

    Cycle_t cycle = Simulation::getSimulation()->getCurrentSimCycle() +
        tc->convertToCoreTime(delay) + latency;

    if ( event == nullptr ) {
        event = new NullEvent();
    }
    event->setDeliveryTime(cycle);
    event->setDeliveryLink(id,pair_link);

#if __SST_DEBUG_EVENT_TRACKING__
    event->addSendComponent(comp, ctype, port);
    event->addRecvComponent(pair_link->comp, pair_link->ctype, pair_link->port);
#endif

    // trace.getOutput().output(CALL_INFO, "%p\n",pair_link->recvQueue);
    // printf("Test 0 in send of link.cc\n");
    fflush(stdout);
    pair_link->recvQueue->insert( event );
    // printf("Test 1 in send of link.cc\n");
    fflush(stdout);
}


Event* Link::recv()
{
    // printf("Test 0 in recv of link.cc\n");
    fflush(stdout);
    // Check to make sure this is a polling link
    if ( UNLIKELY( type != POLL ) ) {
        Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot call recv on a Link with an event handler installed (non-polling link.\n");

    }

    Event* event = nullptr;
    Simulation *simulation = Simulation::getSimulation();

    if ( !recvQueue->empty() ) {
    Activity* activity = recvQueue->front();
    if ( activity->getDeliveryTime() <=  simulation->getCurrentSimCycle() ) {
        event = static_cast<Event*>(activity);
        recvQueue->pop();
    }
    }
    // printf("Test 1 in recv of link.cc\n");
    fflush(stdout);
    return event;
}

void Link::sendUntimedData(Event* data)
{
    // printf("Test 0 of sendUntimedData in link.cc\n");
    fflush(stdout);
    if ( pair_link->untimedQueue == nullptr ) {
        // printf("Test 0 1 of sendUntimedData in link.cc\n");
        fflush(stdout);
        pair_link->untimedQueue = new InitQueue();
    }
    Simulation_impl::getSimulation()->untimed_msg_count++;
    data->setDeliveryTime(Simulation_impl::getSimulation()->untimed_phase + 1);
    data->setDeliveryLink(id,pair_link);

    pair_link->untimedQueue->insert(data);
    // printf("Test 1 of sendUntimedData in link.cc\n");
    fflush(stdout);
#if __SST_DEBUG_EVENT_TRACKING__
    data->addSendComponent(comp,ctype,port);
    data->addRecvComponent(pair_link->comp, pair_link->ctype, pair_link->port);
#endif
}

void Link::sendUntimedData_sync(Event* data)
{
    if ( pair_link->untimedQueue == nullptr ) {
        pair_link->untimedQueue = new InitQueue();
    }
    // data->setDeliveryLink(id,pair_link);

    pair_link->untimedQueue->insert(data);
}

Event* Link::recvUntimedData()
{
    // printf("Test 0 of recvUntimedData in link.cc\n");
    fflush(stdout);
    if ( untimedQueue == nullptr ) {
        // printf("Test 0-1 of nullptr recvUntimedData in link.cc\n");
        fflush(stdout);
        return nullptr;
    }
    // printf("Test 0-1 of recvUntimedData in link.cc\n");
    fflush(stdout);

    Event* event = nullptr;
    // printf("Test 1 of recvUntimedData in link.cc\n");
    fflush(stdout);
    if ( !untimedQueue->empty() ) {
    // printf("Test 2 of recvUntimedData in link.cc\n");
    fflush(stdout);
    Activity* activity = untimedQueue->front();
    // printf("Test 3 of recvUntimedData in link.cc\n");
    fflush(stdout);
    if ( activity->getDeliveryTime() <= Simulation_impl::getSimulation()->untimed_phase ) {
    // printf("Test 4 of recvUntimedData in link.cc\n");
    fflush(stdout);
        event = static_cast<Event*>(activity);
    // printf("Test 5 of recvUntimedData in link.cc\n");
    fflush(stdout);
        untimedQueue->pop();
    // printf("Test 6 of recvUntimedData in link.cc\n");
    fflush(stdout);
    }
    }
    return event;
}

void Link::setDefaultTimeBase(TimeConverter* tc) {
    defaultTimeBase = tc;
}

TimeConverter* Link::getDefaultTimeBase() {
    return defaultTimeBase;
}

const TimeConverter* Link::getDefaultTimeBase() const {
    return defaultTimeBase;
}


} // namespace SST
