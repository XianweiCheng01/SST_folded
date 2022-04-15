
import sst

verbose = 0

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

class Params(dict):
    def __missing__(self, key):
        print("Please enter %s: "%key)
        val = input()
        self[key] = val
        return val
    def subset(self, keys, optKeys = []):
        ret = dict((k, self[k]) for k in keys)
        #ret.update(dict((k, self[k]) for k in (optKeys and self)))
        for k in optKeys:
            if k in self:
                ret[k] = self[k]
        return ret
    def subsetWithRename(self, keys):
        ret = dict()
        for k,nk in keys:
            if k in self:
                ret[nk] = self[k]
        return ret
    # Needed to avoid asking for input when a key isn't present
#    def optional_subset(self, keys):
#        return

_params = Params()
debug = 0
# _params["shape"] = "8x8x8"
# _params["shape"] = "4x4x8"
# _params["shape"] = "4x4x4"
_params["shape"] = "2x2x2"
# _params["shape"] = "4"
_params["width"] = "1x1x1"
_params["local_ports"] = 1
_params["num_dims"] = "3"
# _params["num_dims"] = 1

_params["link_bw"] = "4GB/s"
_params["link_lat"] = "20ns"
_params["flit_size"] = "8B"
_params["xbar_bw"] = "4GB/s"
_params["input_latency"] = "20ns"
_params["output_latency"] = "20ns"
_params["input_buf_size"] = "4kB"
_params["output_buf_size"] = "4kB"

_params["xbar_arb"] = "merlin.xbar_arb_lru"

sst.setStatisticLoadLevel(3)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "./TestOutput.csv","separator" : "," } )

dims = [int(x) for x in _params["shape"].split('x')]
nd = len(dims)
dimwidths = [int(x) for x in _params["width"].split('x')]

local_ports = _params["local_ports"]
radix = local_ports + 2 * sum(dimwidths)

peers = 1

for x in dims:
	peers = peers * x

peers = peers * local_ports

_params["num_peers"] = peers
_params["num_dims"] = nd
_params["topology"] = _params["topology"] = "merlin.torus"
_params["debug"] = debug
_params["num_ports"] = radix
# _params["local_ports"] = local_ports

num_routers = _params["num_peers"] // _params["local_ports"]

links = dict()
def getLink(leftName, rightName, num):
    name = "link.%s:%s:%d"%(leftName, rightName, num)
    if name not in links:
        links[name] = sst.Link(name)
    return links[name]

swap_keys = [("shape","shape"),("width","width"),("local_ports","local_ports")]

_topo_params = _params.subsetWithRename(swap_keys);

topoKeys = []
topoOptKeys = []

topoKeys.extend(["topology", "debug", "num_ports", "flit_size", "link_bw", "xbar_bw", "shape", "width", "local_ports","input_latency","output_latency","input_buf_size","output_buf_size"])
topoOptKeys.extend(["xbar_arb","num_vns","vn_remap","vn_remap_shm","portcontrol:output_arb","portcontrol:arbitration:qos_settings","portcontrol:arbitration:arb_vns","portcontrol:arbitration:arb_vcs"])

def _idToLoc(rtr_id, nd, dims):
    foo = list()
    for i in range(nd-1, 0, -1):
        div = 1
        for j in range(0, i):
            div = div * dims[j]
        value = (rtr_id // div)
        foo.append(value)
        rtr_id = rtr_id - (value * div)
    foo.append(rtr_id)
    foo.reverse()
    return foo

def _formatShape(arr):
    return 'x'.join([str(x) for x in arr])

def getRouterNameForId(rtr_id):
    return "rtr.%d"%rtr_id

def _instanceRouter(rtr_id, rtr_type):
    return sst.Component(getRouterNameForId(rtr_id), rtr_type)

def macroNode(nID):
    node = sst.Component("node.%d"%nID, "macro.simple_node")
    node.addParam("id", nID)
    node.addParam("nic.id", nID)
    node.addParams({
      # 'id' : '3',
      'name' : 'simple',
      'app1.exe' : './graph500_reference_bfs_sssp',
      # 'app1.exe' : './hello_world',
      'app1.argv' : '16 16',
      # 'app1.argv' : '16 64',
      # 'app1.argv' : '26 512',
      # 'app1.argv' : '20 16',
      # 'app1.argv' : '20 64',
      # 'app1.argv' : '26 16',
      # 'app1.argv' : '26 64',
      # 'app1.launch_cmd' : 'aprun -n 64 -N 1',
      'app1.launch_cmd' : 'aprun -n 8 -N 1',
      # 'app1.launch_cmd' : 'aprun -n 64 -N 1',
      # 'app1.launch_cmd' : 'aprun -n 512 -N 1',
      # 'app1.launch_cmd' : 'aprun -n 128 -N 1',
      # 'app1.launch_cmd' : 'aprun -n 1 -N 1',
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
      # 'nic.id' : '3',
      'nic.topology' : 'merlin.torus',

      'topology.auto' : 'true',
      'topology.geometry' : '[2,2,2]',
      # 'topology.geometry' : '[2,2,4]',
      # 'topology.geometry' : '[8,8,8]',
      # 'topology.geometry' : '[4,4,8]',
      # 'topology.geometry' : '[4,4,4]',
      'topology.name' : 'torus',
      'proc.frequency' : '2.1Ghz',
      'proc.ncores' : '4',
      'interconnect.topology.geometry' : '[2,2,2]',
      # 'interconnect.topology.geometry' : '[2,2,4]',
      # 'interconnect.topology.geometry' : '[8,8,8]',
      # 'interconnect.topology.geometry' : '[4,4,8]',
      # 'interconnect.topology.geometry' : '[4,4,4]',
      'interconnect.topology.auto' : 'true',
      'interconnect.topology.name' : 'torus',
      'auto' : 'true',
      'geometry' : '[2,2,2]',
      # 'geometry' : '[2,2,4]',
      # 'geometry' : '[8,8,8]',
      # 'geometry' : '[4,4,8]',
      # 'geometry' : '[4,4,4]',
      'name' : 'torus'
      # "do_write" : "1",
      # "num_loadstore" : "1000",
      # "commFreq" : "100",
      # "memSize" : "0x1000"
    })
    return (node, "input1", "1000ps")

# _getEndPoint = macroNode

for i in range(num_routers):
    mydims = _idToLoc(i, nd, dims)
    mylocstr = _formatShape(mydims)
    rtr = _instanceRouter(i, "merlin.hr_router")
    rtr.addParams(_params.subset(topoKeys, topoOptKeys))
    rtr.addParam("id", i)
    topology = rtr.setSubComponent("topology", "merlin.torus")
    topology.addParams(_topo_params)
    port = 0
    for dim in range(nd):
        theirdims = mydims[:]
        theirdims[dim] = (mydims[dim] + 1)%dims[dim]
        theirlocstr = _formatShape(theirdims)
        for num in range(dimwidths[dim]):
            rtr.addLink(getLink(mylocstr, theirlocstr, num), "port%d"%port, _params["link_lat"])
            port = port + 1
        theirdims[dim] = ((mydims[dim] - 1) + dims[dim])%dims[dim]
        theirlocstr = _formatShape(theirdims)
        for num in range(dimwidths[dim]):
            rtr.addLink(getLink(theirlocstr, mylocstr, num), "port%d"%port, _params["link_lat"])
            port = port + 1
    for n in  range(_params["local_ports"]):
        nodeID = int(_params["local_ports"]) * i + n
        ep = macroNode(nodeID)
        if ep:
            nicLink = sst.Link("nic.%d:%d"%(i, n))
            nicLink.connect(ep, (rtr, "port%d"%port, _params["link_lat"]))
        port = port + 1

sst.enableAllStatisticsForComponentType("merlin.linkcontrol")
sst.enableAllStatisticsForComponentType("merlin.hr_router")
sst.enableAllStatisticsForComponentType("merlin.torus")
