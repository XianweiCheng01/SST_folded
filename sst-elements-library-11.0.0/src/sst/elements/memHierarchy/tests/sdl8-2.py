import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0

l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : DEBUG_L3,
      "debug_level" : 10,
      "verbose" : 2,
})
# l3Tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
      #"debug" : 1,
      #"debug_level" : 10,
      "network_bw" : "25GB/s",
      "group" : 1,
      "verbose" : 2,
})

comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")

comp_dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : DEBUG_DIR,
      "debug_level" : "10",
      "entry_cache_size" : "16384",
      "addr_range_end" : "0x1F000000",
      "addr_range_start" : "0x0",
      "verbose" : 2,
})
dirNIC = comp_dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
      "network_bw" : "25GB/s",
      "group" : 2,
      "verbose" : 2,
      #"debug" : 1,
      #"debug_level" : 10,
})
# dirMemLink = comp_dirctrl.setSubComponent("memlink", "memHierarchy.MemLink") # Not on a network, just a direct link

link_cache_net = sst.Link("link_cache_net")
link_cache_net.connect( (l3NIC, "port", "10000ps"), (comp_chiprtr, "port1", "2000ps") )

link_dir_net = sst.Link("link_dir_net")
link_dir_net.connect( (comp_chiprtr, "port0", "2000ps"), (dirNIC, "port", "2000ps") )

