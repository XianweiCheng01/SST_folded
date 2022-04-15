# Automatically generated SST Python input
import sst
from mhlib import componentlist

# Define the simulation components
# verbose = 3
# verbose = 2
verbose = 0

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu = sst.Component("cpu", "macro.simple_node")
cpu.addParams({
      'id' : '7',
      'name' : 'simple',
      'app1.exe' : './graph500_reference_bfs_sssp',
      'app1.argv' : '16 16',
      'app1.launch_cmd' : 'aprun -n 8 -N 1',
      'memory.mtu' : '100GB',
      'memory.latency' : '15ns',
      'memory.name' : 'logp',
      'memory.bandwidth' : '10GB/s',
      'nic.name' : 'logp',
      'nic.injection.latency' : '2us',
      'nic.injection.bandwidth' : '10GB/s',
      'topology.auto' : 'true',
      'topology.geometry' : '[2,2,2]',
      'topology.name' : 'torus',
      'proc.frequency' : '2.1Ghz',
      'proc.ncores' : '8'
      # "do_write" : "1",
      # "num_loadstore" : "1000",
      # "commFreq" : "100",
      # "memSize" : "0x1000"
})
# iface = cpu.setSubComponent("memory", "memHierarchy.memInterface")

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
    "access_latency_cycles" : "4",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "debug" : DEBUG_L1,
    "debug_level" : DEBUG_LEVEL,
    "verbose" : verbose,
    "L1" : "1",
    "cache_size" : "2KiB"
})

no_l1cache = sst.Component("no_l1cache", "memHierarchy.Cache")
no_l1cache.addParams({
    "access_latency_cycles" : "4",
    "cache_frequency" : "2 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "debug" : DEBUG_L1,
    "debug_level" : DEBUG_LEVEL,
    "verbose" : verbose,
    "L1" : "1",
    "cache_size" : "2KiB"
})

# memctrl = cpu.setSubComponent("memory", "memHierarchy.MemController")
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "verbose" : verbose,
    "addr_range_end" : 512*1024*1024-1,
})

no_memctrl = sst.Component("memory", "memHierarchy.MemController")
no_memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "verbose" : verbose,
    "addr_range_end" : 512*1024*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "1000ns",
    "mem_size" : "512MiB"
})

no_memory = no_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
no_memory.addParams({
    "access_time" : "1000ns",
    "mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link_1 = sst.Link("link_cpu_cache_link_1")
# link_cpu_cache_link.connect( (iface, "port", "1000ps"), (l1cache, "high_network_0", "1000ps") )
link_cpu_cache_link.connect( (cpu, "output1", "1000ps"), (l1cache, "high_network_0", "1000ps") )
link_cpu_cache_link_1.connect( (cpu, "input1", "1000ps"), (no_l1cache, "high_network_0", "1000ps") )
# cpu.addLink(link_cpu_cache_link_1, "output1", "100ps")
# link_cpu_cache_link_1.connect( (cpu, "output1", "1000ps"), (l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link_1 = sst.Link("link_mem_bus_link_1")
link_mem_bus_link.connect( (l1cache, "low_network_0", "50ps"), (memctrl, "direct_link", "50ps") )
link_mem_bus_link_1.connect( (no_l1cache, "low_network_0", "50ps"), (no_memctrl, "direct_link", "50ps") )
# End of generated output.
