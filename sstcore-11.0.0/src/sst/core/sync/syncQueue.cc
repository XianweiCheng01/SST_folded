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
#include "sst/core/sync/syncQueue.h"

#include "sst/core/serialization/serializer.h"
#include "sst/core/event.h"
#include "sst/core/simulation.h"


namespace SST {

using namespace Core::ThreadSafe;
using namespace Core::Serialization;

SyncQueue::SyncQueue() :
    ActivityQueue(), buffer(nullptr), buf_size(0)
{
}

SyncQueue::~SyncQueue()
{
}

bool
SyncQueue::empty()
{
    std::lock_guard<Spinlock> lock(slock);
    return activities.empty();
}

int
SyncQueue::size()
{
    std::lock_guard<Spinlock> lock(slock);
    return activities.size();
}

void
SyncQueue::insert(Activity* activity)
{
    std::lock_guard<Spinlock> lock(slock);
    activities.push_back(activity);
}

Activity*
SyncQueue::pop()
{
    // NEED TO FATAL
    // if ( data.size() == 0 ) return nullptr;
    // std::vector<Activity*>::iterator it = data.begin();
    // Activity* ret_val = (*it);
    // data.erase(it);
    // return ret_val;
    return nullptr;
}

Activity*
SyncQueue::front()
{
    // NEED TO FATAL
    return nullptr;
}

void
SyncQueue::clear()
{
    std::lock_guard<Spinlock> lock(slock);
    activities.clear();
}

char*
SyncQueue::getData()
{
    // printf("Test 0 in getData() of syncQueue.cc\n");
    fflush(stdout);
    std::lock_guard<Spinlock> lock(slock);

    // printf("Test 1 in getData() of syncQueue.cc\n");
    fflush(stdout);
    serializer ser;

    // printf("Test 2 in getData() of syncQueue.cc\n");
    fflush(stdout);
    ser.start_sizing();

    // printf("Test 3 in getData() of syncQueue.cc\n");
    fflush(stdout);
    ser & activities;

    // printf("Test 4 in getData() of syncQueue.cc\n");
    fflush(stdout);
    size_t size = ser.size();

    // printf("Test 5 in getData() of syncQueue.cc\n");
    fflush(stdout);
    if ( buf_size < ( size + sizeof(SyncQueue::Header) ) ) {
        if ( buffer != nullptr ) {
            delete[] buffer;
        }

        buf_size = size + sizeof(SyncQueue::Header);
        buffer = new char[buf_size];
    }

    // printf("Test 6 in getData() of syncQueue.cc\n");
    fflush(stdout);
    ser.start_packing(buffer + sizeof(SyncQueue::Header), size);

    // printf("Test 7 in getData() of syncQueue.cc\n");
    fflush(stdout);
    ser & activities;

    // printf("Test 8 in getData() of syncQueue.cc\n");
    fflush(stdout);
    // Delete all the events
    for ( unsigned int i = 0; i < activities.size(); i++ ) {
        delete activities[i];
    }
    // printf("Test 9 in getData() of syncQueue.cc\n");
    fflush(stdout);
    activities.clear();

    // printf("Test 10 in getData() of syncQueue.cc\n");
    fflush(stdout);
    // Set the size field in the header
    static_cast<SyncQueue::Header*>(static_cast<void*>(buffer))->buffer_size = size + sizeof(SyncQueue::Header);

    // printf("Test 11 in getData() of syncQueue.cc\n");
    fflush(stdout);
    return buffer;
}

} // namespace SST
