
import sst

verbose = 0

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu = sst.Component("Node 0", "macro.simple_node")
cpu.addParams({
      'id' : '0',
      'name' : 'simple',
      'app1.exe' : './graph500_reference_bfs_sssp',
      'app1.argv' : '16 16',
      'app1.launch_cmd' : 'aprun -n 8 -N 8',
      'memory.mtu' : '100GB',
      'memory.latency' : '15ns',
      'memory.name' : 'logp',
      'memory.bandwidth' : '10GB/s',
      # 'nic.name' : 'logp',
      # 'nic.injection.latency' : '2us',
      # 'nic.injection.bandwidth' : '10GB/s',

      'nic.name' : 'merlin',
      'nic.module' : 'merlin.linkcontrol',
      # 'nic.module' : 'merlin.reorderlinkcontrol',
      'nic.xbar_bw' : '1GB/s',
      'nic.link_bw' : '1GB/s',
      'nic.input_buf_size' : '2KB',
      # 'nic.num_ports' : '2',
      'nic.flit_size' : '72B',
      'nic.output_buf_size' : '2KB',
      'nic.id' : '0',
      'nic.topology' : 'merlin.singlerouter',

      'topology.auto' : 'true',
      'topology.geometry' : '[1,1,1]',
      'topology.name' : 'torus',
      'proc.frequency' : '2.1Ghz',
      'proc.ncores' : '8',
      'interconnect.topology.geometry' : '[1,1,1]',
      'interconnect.topology.auto' : 'true',
      'interconnect.topology.name' : 'torus',
      'auto' : 'true',
      'geometry' : '[1,1,1]',
      'name' : 'torus'
      # "do_write" : "1",
      # "num_loadstore" : "1000",
      # "commFreq" : "100",
      # "memSize" : "0x1000"
})

switch = sst.Component("chiprtr", "merlin.hr_router");
switch.addParams({
	"xbar_bw" : "1GB/s",
	"link_bw" : "1GB/s",
	"input_buf_size" : "2KB",
	"num_ports" : "2",
	"flit_size" : "72B",
	"output_buf_size" : "2KB",
	"id" : "0",
	"topology" : "merlin.singlerouter",
	"debug" : "1"
})
switch.setSubComponent("topology", "merlin.singlerouter")

# link_cpu_cache_link = sst.Link("link_cpu_cache_link")
# link_cpu_cache_link_1 = sst.Link("link_cpu_cache_link_1")

# link_cpu_cache_link.connect( (cpu, "output1", "1000ps"), (comp_chiprtr, "port0", "1000ps") )
# link_cpu_cache_link_1.connect( (cpu, "input1", "1000ps"), (comp_chiprtr, "port1", "1000ps") )

# switch = sst.Component("LogP 0", "macro.logp_switch")
# switch.addParams({
# 	'id' : '0',
# 	'out_in_latency' : '2us',
# 	'hop_latency' : '200ns',
# 	'name' : 'logp',
# 	'bandwidth' : '6GB/s',
# 	'topology.auto' : 'true',
# 	'topology.geometry' : '[1,1,1]',
# 	'topology.name' : 'torus'
# })

link_inj = sst.Link("logPinjection0->0")
link_ej = sst.Link("logPejection0->0")

link_inj.connect((cpu, "output1", "1000ps"), (switch, "port0", "1000ps"))
link_ej.connect((switch, "port1", "1000ps"), (cpu, "input1", "1000ps"))
# link_inj.connect((cpu, "output1", "1000ps"), (switch, "input0", "1000ps"))
# link_ej.connect((switch, "output0", "1000ps"), (cpu, "input1", "1000ps"))

# cpu.addLink(link_inj, "output1", "1000ps")
# cpu.addLink(link_ej, "input1", "1000ps")

# switch.addLink(link_inj, "input0", "1000ps")
# switch.addLink(link_ej, "output0", "1000ps")
# switch.addLink(link_inj, "port0", "1000ps")
# switch.addLink(link_ej, "port1", "1000ps")
