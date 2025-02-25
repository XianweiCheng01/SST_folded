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

#ifndef sumi_api_COLLECTIVE_ACTOR_H
#define sumi_api_COLLECTIVE_ACTOR_H

#include <sumi/collective.h>
#include <sumi/collective_message.h>
#include <sumi/dense_rank_map.h>
#include <sumi/communicator.h>
#include <set>
#include <map>
#include <stdint.h>
#include <sstmac/common/sstmac_config.h>
#include <sprockit/allocator.h>

DeclareDebugSlot(sumi_collective_buffer)

#define sumi_case(x) case x: return #x

#define do_sumi_debug_print(...)
 /*
 if (sprockit::debug::slot_active(sprockit::dbg::sumi_collective_buffer)) \
   debug_print(__VA_ARGS__)
 */

namespace sumi {

void debug_print(const char* info, const std::string& rank_str,
            int partner, int round, int offset,
            int nelems, int type_size, const void* buffer);

struct Action
{
  typedef enum { send=0, recv=1, shuffle=2, unroll=3, resolve=4, join=5 } type_t;
  type_t type;
  int partner;
  int phys_partner;
  int join_counter;
  int round;
  int offset;
  int nelems;
  uint32_t id;
  sstmac::Timestamp start;

  static const char*
  tostr(type_t ty){
    switch(ty){
      sumi_case(send);
      sumi_case(recv);
      sumi_case(shuffle);
      sumi_case(unroll);
      sumi_case(resolve);
      sumi_case(join);
      default:
       spkt_abort_printf("Bad action type %d", ty);
       return "";
    }
  }

  std::string toString() const;

  static const uint32_t max_round = 500;

  static uint32_t messageId(type_t ty, int r, int p){
    //factor of two is for send or receive
    const int num_enums = 6;
    return p*max_round*num_enums + r*num_enums + ty;
  }

  static void details(uint32_t round, type_t& ty, int& r, int& p){
    const int num_enums = 6;
    uint32_t remainder = round;
    p = remainder / max_round / num_enums;
    remainder -= p*max_round*num_enums;

    r = remainder / num_enums;
    remainder -= r*num_enums;

    ty = (type_t) remainder;
  }

  Action(type_t ty, int r, int p) :
    type(ty), 
    partner(p),
    join_counter(0),
    round(r)
  {
    id = messageId(ty, r, p);
  }
};

struct RecvAction : public Action
{
  typedef enum {
    in_place=0,
    reduce=1,
    packed_temp_buf=2,
    unpack_temp_buf=3
  } buf_type_t;

  buf_type_t buf_type;

  static const char* tostr(buf_type_t ty){
    switch(ty){
      sumi_case(in_place);
      sumi_case(reduce);
      sumi_case(packed_temp_buf);
      sumi_case(unpack_temp_buf);
    }
  }

  typedef enum {
    rdvz_in_place=0,
    rdvz_reduce=1,
    rdvz_packed_temp_buf=2,
    rdvz_unpack_temp_buf=3,
    eager_in_place=4,
    eager_reduce=5,
    eager_packed_temp_buf=6,
    eager_unpack_temp_buf=7
  } recv_type_t;

  static recv_type_t recv_type(bool eager, buf_type_t ty){
    int shift = eager ? 4 : 0;
    return recv_type_t(ty + shift);
  }

  RecvAction(int round, int partner, buf_type_t bty) :
    Action(recv, round, partner),
    buf_type(bty)
  {
  }
};

struct SendAction : public Action
{
  typedef enum {
    in_place=0,
    prev_recv=1,
    temp_send=2
  } buf_type_t;

  buf_type_t buf_type;

  static const char* tostr(buf_type_t ty){
    switch(ty){
      sumi_case(in_place);
      sumi_case(prev_recv);
      sumi_case(temp_send);
    }
  }

  SendAction(int round, int partner, buf_type_t ty) :
    Action(send, round, partner),
    buf_type(ty)
  {
  }

};

struct ShuffleAction : public Action
{
  ShuffleAction(int round, int partner) :
    Action(shuffle, round, partner)
  {
  }
};

/**
 * @class collective_actor
 * Object that actually does the work (the actor)
 * in a collective. A separation (for now) is kept
 * between the actually collective and actors in the collective.
 * The actors are essentially virtual ranks allowing the collective
 * to run an algorith with a virtual number of ranks different from
 * the actual physical number.
 */
class CollectiveActor
{
 public:
  virtual std::string toString() const = 0;

  virtual ~CollectiveActor();

  bool complete() const {
    return complete_;
  }

  int tag() const {
    return tag_;
  }

  std::string rankStr() const;

  virtual void init() = 0;

 protected:
  CollectiveActor(CollectiveEngine* engine, int tag, int cq_id, Communicator* comm);

  int globalRank(int dom_rank) const;

  int domToGlobalDst(int dom_dst);

  std::string rankStr(int dom_rank) const;

  virtual void finalize(){}

 protected:
  Transport* my_api_;

  CollectiveEngine* engine_;

  int dom_me_;

  int dom_nproc_;

  int tag_;

  Communicator* comm_;

  int cq_id_;

  bool complete_;

};

class Slicer {
 public:
  static reduce_fxn null_reduce_fxn;

  virtual ~Slicer() = default;

  /**
   * @brief pack_in
   * @param packedBuf
   * @param unpackedBuf
   * @param offset
   * @param nelems
   * @return
   */
  virtual size_t packsendBuf(void* packedBuf, void* unpackedObj,
                int offset, int nelems) const = 0;

  virtual void unpackRecvBuf(void* packedBuf, void* unpackedObj,
                  int offset, int nelems) const = 0;

  virtual void memcpyPackedBufs(void* dst, void* src, int nelems) const = 0;

  virtual void unpackReduce(void*  /*packedBuf*/, void*  /*unpackedObj*/,
            int  /*offset*/, int  /*nelems*/) const {
    sprockit::abort("slicer for collective does not implement a reduce op");
  }

  virtual bool contiguous() const = 0;

  virtual int elementPackedSize() const = 0;
};

class DefaultSlicer :
 public Slicer
{

 public:
  DefaultSlicer(int ty_size, reduce_fxn f = null_reduce_fxn) :
    type_size(ty_size), fxn(f){}

  size_t packsendBuf(void* packedBuf, void* unpackedObj,
            int offset, int nelems) const override;

  void unpackRecvBuf(void* packedBuf, void* unpackedObj,
            int offset, int nelems) const override;

  virtual void memcpyPackedBufs(void *dst, void *src, int nelems) const override;

  virtual void unpackReduce(void *packedBuf, void *unpackedObj,
                  int offset, int nelems) const override;

  int elementPackedSize() const override {
    return type_size;
  }

  bool contiguous() const override {
    return true;
  }

  int type_size;
  reduce_fxn fxn;
};

/**
 * @class collective_actor
 * Object that actually does the work (the actor)
 * in a collective. A separation (for now) is kept
 * between the actually collective and actors in the collective.
 * The actors are essentially virtual ranks allowing the collective
 * to run an algorith with a virtual number of ranks different from
 * the actual physical number.
 */
class DagCollectiveActor :
 public CollectiveActor,
 public Communicator::RankCallback
{
 public:
  virtual std::string toString() const override = 0;

  virtual ~DagCollectiveActor();

  virtual void recv(CollectiveWorkMessage* msg, std::string func = __builtin_FUNCTION());

  virtual void start();

  typedef enum {
    eager_protocol,
    put_protocol,
    get_protocol } protocol_t;

  protocol_t protocolForAction(Action* ac) const;

  void deadlockCheck() const;

  CollectiveDoneMessage* doneMsg() const;

  void init() override {
    initTree();
    initDag();
    initBuffers();
  }

 private:
  template <class T, class U> using alloc = sprockit::threadSafeAllocator<std::pair<const T,U>>;
  typedef std::map<uint32_t, Action*, std::less<uint32_t>,
                   alloc<uint32_t,Action*>> active_map;
  typedef std::multimap<uint32_t, Action*, std::less<uint32_t>,
                   alloc<uint32_t,Action*>> pending_map;
  typedef std::multimap<uint32_t, CollectiveWorkMessage*, std::less<uint32_t>,
                   alloc<uint32_t,CollectiveWorkMessage*>> pending_msg_map;

 protected:
  DagCollectiveActor(Collective::type_t ty, CollectiveEngine* engine, void* dst, void * src,
                       int type_size, int tag, int cq_id, Communicator* comm,
                       reduce_fxn fxn = Slicer::null_reduce_fxn) :
    CollectiveActor(engine, tag, cq_id, comm),
    type_size_(type_size),
    send_buffer_(src),
    recv_buffer_(nullptr),
    result_buffer_(dst),
    type_(ty),
    slicer_(new DefaultSlicer(type_size, fxn))
  {
  }

  void addDependency(Action* precursor, Action* ac);
  void addAction(Action* ac);

  static bool isSharedRole(int role, int num_roles, int* my_roles){
    for (int r=0; r < num_roles; ++r){
      if (role == my_roles[r]){
        return true;
      }
    }
    return false;
  }

 private:
  virtual void initTree(){}
  virtual void initDag() = 0;
  virtual void initBuffers() = 0;
  virtual void finalizeBuffers() = 0;


  void addCommDependency(Action* precursor, Action* ac);
  void addDependencyToMap(uint32_t id, Action* ac);
  void rankResolved(int globalRank, int comm_rank) override;

  void checkCollectiveDone();

  void putDoneNotification();

  void startSend(Action* ac);
  void startRecv(Action* ac);
  void doSend(Action* ac);
  void doRecv(Action* ac);

  void startAction(Action* ac);

  void sendEagerMessage(Action* ac);
  void sendRdmaPutHeader(Action* ac);
  void sendRdmaGetHeader(Action* ac);

  void nextRoundReadyToPut(Action* ac,
    CollectiveWorkMessage* header);

  void nextRoundReadyToGet(Action* ac,
    CollectiveWorkMessage* header);

  void incomingHeader(CollectiveWorkMessage* msg);

  void dataRecved(CollectiveWorkMessage* msg, void* recvd_buffer);

  void dataRecved(Action* ac, CollectiveWorkMessage* msg, void *recvd_buffer);

  void dataSent(CollectiveWorkMessage* msg, std::string func = __builtin_FUNCTION());

  virtual void bufferAction(void* dst_buffer, void* msg_buffer, Action* ac) = 0;

  void* messageBuffer(void* buffer, int offset);

  /**
   * @brief set_send_buffer
   * @param ac  The action being performed (eager, rdma get, rdma put)
   * @param buf in/out parameter that will hold the correct buffer
   * @return The size of the buffer in bytes
   */
  void* getSendBuffer(Action* ac, uint64_t& nbytes);

  void* getRecvbuffer(Action* ac);

  virtual void startShuffle(Action* ac);

  void erasePending(uint32_t id, pending_msg_map& m);

  void reputPending(uint32_t id, pending_msg_map& m);

  /**
   * @brief Satisfy dependencies for any pending comms,
   *        Clean up pings, and check if collective done.
   *        This calls clear_action to do clean up.
   *        Should only be called for actions that became active
   * @param ac
   */
  void commActionDone(Action* ac);

  /**
   * @brief Satisfy dependences and check if done.
   *        Unlike action_done, this can be called for actions
   *        that stopped early and never became active
   * @param ac
   * @param m
   */
  void clearAction(Action* ac);

  void clearDependencies(Action* ac);

  Action* commActionDone(Action::type_t ty, int round, int partner, std::string func = __builtin_FUNCTION());

 protected:
  int type_size_;

  /**
  * @brief This where I send data from
  */
  void* send_buffer_;
  /**
  * @brief This is where I directly receive data from neighbors
  */
  void* recv_buffer_;
  /**
  * @brief This is where I accumulate or put results after a receive
  */
  void* result_buffer_;

  Collective::type_t type_;

  DefaultSlicer* slicer_;

 private:
  active_map active_comms_;
  pending_map pending_comms_;
  std::list<Action*> completed_actions_;
  std::list<Action*> ready_actions_;

  pending_msg_map pending_send_headers_;
  pending_msg_map pending_recv_headers_;

  struct action_compare {
    bool operator()(Action* l, Action* r) const {
      return l->id < r->id;
    }
  };

  std::set<Action*, action_compare> initial_actions_;

#ifdef FEATURE_TAG_SUMI_RESILIENCE
  void dense_partner_ping_failed(int dense_rank);
#endif
};

struct RecursiveDoubling {
  static void computeTree(int nproc, int &log2nproc, int &midpoint, int &pow2nproc);
};

/**
 * @class virtual_rank_map
 * Maps a given number of live processors
 * onto a set of virtual ranks.
 * For example, if you have 5 procs you might
 * want to run a virtual collective with 8 ranks
 * so you have a nice, neat power of 2
 */
class VirtualRankMap
{
 public:
  int virtualToReal(int rank) const;

  /**
   * @brief real_to_virtual
   * @param rank
   * @param virtual_ranks An array large enough to hold the number of ranks
   * @return The number of virtual ranks
   */
  int realToVirtual(int rank, int* virtual_ranks) const;

  int virtualNproc() const {
    return virtual_nproc_;
  }

  int nproc() const {
    return nproc_;
  }

  void init(int nproc, int virtual_nproc) {
    nproc_ = nproc;
    virtual_nproc_ = virtual_nproc;
  }

  VirtualRankMap(int nproc, int virtual_nproc) :
    nproc_(nproc), virtual_nproc_(virtual_nproc)
  {
  }

  VirtualRankMap() : nproc_(-1), virtual_nproc_(-1)
  {
  }

 protected:
  int nproc_;

  int virtual_nproc_;

};



}


#endif // COLLECTIVE_ACTOR_H
