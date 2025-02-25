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

#include <sstmac/software/libraries/library.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/common/sstmac_env.h>

namespace sstmac {
namespace sw {

Library::Library(const std::string& libname, SoftwareId sid, OperatingSystem* os, std::string func) :
  os_(os),
  sid_(sid), 
  addr_(os->addr()),
  libname_(libname) 
{
    std::cout << "Test 0-1 in Library of libraries/library.cc with " << libname_ << " from " << func << std::endl;
    fflush(stdout);
  os_->registerLib(this);
    std::cout << "Test 1 in Library of libraries/library.cc" << std::endl;
    fflush(stdout);
}

Library::~Library()
{
  os_->unregisterLib(this);
}

void
Library::incomingEvent(Event*  /*ev*/)
{
  spkt_throw_printf(sprockit::UnimplementedError,
    "%s::incomingEvent: this library should only block, never receive incoming",
     toString().c_str());
}

void
Library::incomingRequest(Request*  /*ev*/)
{
  spkt_throw_printf(sprockit::UnimplementedError,
    "%s::incomingRequest: this library should only block, never receive incoming",
     toString().c_str());
}

}
} //end namespace
