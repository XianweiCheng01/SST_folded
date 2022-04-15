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

#include "sst/core/pollingLinkQueue.h"

namespace SST {

    PollingLinkQueue::PollingLinkQueue() : ActivityQueue() {}
    PollingLinkQueue::~PollingLinkQueue() {
    // Need to delete any events left in the queue
    std::multiset<Activity*,Activity::less_time>::iterator it;
    for ( it = data.begin(); it != data.end(); ++it ) {
        delete *it;
    }
    data.clear();
    }

    bool PollingLinkQueue::empty()
    {
    return data.empty();
    }

    int PollingLinkQueue::size()
    {
    return data.size();
    }

    void PollingLinkQueue::insert(Activity* activity)
    {
    // printf("Test 0 in insert in pollingLinkQueue.cc\n");
    fflush(stdout);
    data.insert(activity);
    // printf("Test 1 in insert in pollingLinkQueue.cc\n");
    fflush(stdout);
    }

    Activity* PollingLinkQueue::pop()
    {
    // printf("Test 0 in pop in pollingLinkQueue.cc\n");
    fflush(stdout);
    if ( data.size() == 0 ) return nullptr;
    std::multiset<Activity*,Activity::less_time>::iterator it = data.begin();
    Activity* ret_val = (*it);
    data.erase(it);
    // printf("Test 1 in pop in pollingLinkQueue.cc\n");
    fflush(stdout);
    return ret_val;
    }

    Activity* PollingLinkQueue::front()
    {
    // printf("Test 0 in front in pollingLinkQueue.cc\n");
    fflush(stdout);
    if ( data.size() == 0 ) return nullptr;
    // printf("Test 1 in front in pollingLinkQueue.cc\n");
    fflush(stdout);
    return *data.begin();
    }


} // namespace SST
