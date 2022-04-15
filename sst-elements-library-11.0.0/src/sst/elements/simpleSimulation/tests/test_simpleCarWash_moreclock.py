# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "10000s")

# Define the simulation components
comp_clocker0 = sst.Component("clocker0", "simpleSimulation.simpleCarWash")
comp_clocker1 = sst.Component("clocker1", "simpleSimulation.simpleCarWash")
comp_clocker2 = sst.Component("clocker2", "simpleSimulation.simpleCarWash")
comp_clocker3 = sst.Component("clocker3", "simpleSimulation.simpleCarWash")
comp_clocker4 = sst.Component("clocker4", "simpleSimulation.simpleCarWash")
comp_clocker5 = sst.Component("clocker5", "simpleSimulation.simpleCarWash")
comp_clocker6 = sst.Component("clocker6", "simpleSimulation.simpleCarWash")
comp_clocker7 = sst.Component("clocker7", "simpleSimulation.simpleCarWash")
comp_clocker8 = sst.Component("clocker8", "simpleSimulation.simpleCarWash")
comp_clocker9 = sst.Component("clocker9", "simpleSimulation.simpleCarWash")

comp_clocker0.addParams({
      "clockcount" : """100000000""",
      "clock" : """1MHz"""
})

comp_clocker1.addParams({
      "clockcount" : """110000000""",
      "clock" : """1.1MHz"""
})

comp_clocker2.addParams({
      "clockcount" : """120000000""",
      "clock" : """1.2MHz"""
})

comp_clocker3.addParams({
      "clockcount" : """130000000""",
      "clock" : """1.3MHz"""
})

comp_clocker4.addParams({
      "clockcount" : """140000000""",
      "clock" : """1.4MHz"""
})

comp_clocker5.addParams({
      "clockcount" : """150000000""",
      "clock" : """1.5MHz"""
})

comp_clocker6.addParams({
      "clockcount" : """160000000""",
      "clock" : """1.6MHz"""
})

comp_clocker7.addParams({
      "clockcount" : """170000000""",
      "clock" : """1.7MHz"""
})

comp_clocker8.addParams({
      "clockcount" : """180000000""",
      "clock" : """1.8MHz"""
})

comp_clocker9.addParams({
      "clockcount" : """190000000""",
      "clock" : """1.9MHz"""
})


# Define the simulation links
# End of generated output.
