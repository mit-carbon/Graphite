#!/usr/bin/env python

from xml.dom.minidom import parse
from optparse import OptionParser
import re
import os
import sys

def findMatch(str, line):
   exp = r"%s = ([-e0-9\.]+)" % str
   match = re.search(exp, line)
   if (match):
      return match.group(1)
   else:
      return None
      
def getArchComponent(dom,name):
   all_components = dom.getElementsByTagName('component')
   for component in all_components:
      if (component.getAttribute('name') == name):
         return component
   return None

def setAttribute(component,name,value):
   all_params = component.getElementsByTagName('param')
   for param in all_params:
      if (param.getAttribute('name') == name):
         param.setAttribute('value', value)
         return
   all_stats = component.getElementsByTagName('stat')
   for stat in all_stats:
      if (stat.getAttribute('name') == name):
         stat.setAttribute('value', value)
         return
   print "** Unrecognized Attribute Name [%s] **" % name
   sys.exit(-2)

def createMcPATInput(options, mcpat_dir):
   # Create a document object
   dom = parse(mcpat_dir + "/" + options.input_file)

   clockrate = "%d" % (options.frequency * 1000)
  
   # Common Attributes
   core = getArchComponent(dom,'core0')
   setAttribute(core, "clock_rate", clockrate)
   system_comp = getArchComponent(dom,'system')
   setAttribute(system_comp, "core_tech_node", options.technology_node)
   setAttribute(system_comp, "total_cycles", options.total_cycles)
   setAttribute(system_comp, "idle_cycles", "0")
   setAttribute(system_comp, "busy_cycles", options.total_cycles)
   
   # Some Default Args
   buffer_size = 8
   if (options.type == "directory"):
      directory = getArchComponent(dom,'L1Directory0')

      Directory_type = "1"
      Dir_config = "%d,%d,%d,%d,%d,%d" % (options.size, options.blocksize, options.associativity, 1, 1, options.delay)
      buffer_sizes = "%d,%d,%d,%d" % (buffer_size,buffer_size,buffer_size,buffer_size)
      ports = "1,1,1"
      
      setAttribute(directory, "Directory_type", Directory_type)
      setAttribute(directory, "Dir_config", Dir_config)
      setAttribute(directory, "buffer_sizes", buffer_sizes)
      setAttribute(directory, "clockrate", clockrate)
      setAttribute(directory, "ports", ports)
      
      setAttribute(directory, "read_accesses", options.read_accesses)
      setAttribute(directory, "write_accesses", options.write_accesses)
      setAttribute(directory, "read_misses", options.read_misses)
      setAttribute(directory, "write_misses", options.write_misses)

   elif (options.type == "data"):
      cache = getArchComponent(dom,'L20')
      
      L2_config = "%d,%d,%d,%d,%d,%d,%d,%d" % (options.size, options.blocksize, options.associativity, 1, 1, options.delay, options.blocksize, 1)
      buffer_sizes = "%d,%d,%d,%d" % (buffer_size,buffer_size,buffer_size,buffer_size)
      ports = "1,1,1"

      setAttribute(cache, "L2_config", L2_config)
      setAttribute(cache, "buffer_sizes", buffer_sizes)
      setAttribute(cache, "clockrate", clockrate)
      setAttribute(cache, "ports", ports)

      setAttribute(cache, "read_accesses", options.read_accesses)
      setAttribute(cache, "write_accesses", options.write_accesses)
      setAttribute(cache, "read_misses", options.read_misses)
      setAttribute(cache, "write_misses", options.write_misses)
   
   else:
      print "** Unrecognized Cache Type [%s] **" % options.type
      sys.exit(-1)

   output_file = open(mcpat_dir + '/.mcpat_input.xml', 'w')
   dom.writexml(output_file)
   output_file.close()

def parseMcPATOutput(component, mcpat_dir):
   file = open(mcpat_dir + '/.mcpat.out')
   lines = file.readlines()
   file.close()

   area = None
   peak_dynamic_power = None
   subthreshold_leakage_power = None
   gate_leakage_power = None
   runtime_dynamic_power = None

   reached = False
   for line in lines:
      if (reached):
         if (area == None):
            area = findMatch("Area", line)
         if (peak_dynamic_power == None):
            peak_dynamic_power = findMatch("Peak Dynamic", line)
         if (subthreshold_leakage_power == None):
            subthreshold_leakage_power = findMatch("Subthreshold Leakage", line)
         if (gate_leakage_power == None):
            gate_leakage_power = findMatch("Gate Leakage", line)
         if (runtime_dynamic_power == None):
            runtime_dynamic_power = findMatch("Runtime Dynamic", line)
         if (runtime_dynamic_power != None):
            break
         
      component_str = r"^%s" % component
      if re.match(component_str, line):
         reached = True

   return (area, peak_dynamic_power, subthreshold_leakage_power, gate_leakage_power, runtime_dynamic_power)
         
# Main Program Starts Here

# Parse the Command Line Options
parser = OptionParser()
parser.add_option("-t", "--type", dest="type", help="Cache Type (data,directory)")
parser.add_option("--technology-node", dest="technology_node", help="Technology Node (in nm)")
parser.add_option("-s", "--size", dest="size", type="int", help="Cache Size (in Bytes)")
parser.add_option("-b", "--blocksize", dest="blocksize", type="int", help="Block Size (in Bytes)")
parser.add_option("-a", "--associativity", dest="associativity", type="int", help="Associativity")
parser.add_option("-d", "--delay", dest="delay", type="int", help="Cache Delay")
parser.add_option("-f", "--frequency", dest="frequency", type="float", help="Frequency (in GHz)")
parser.add_option("-m", "--mcpat-home", dest="mcpat_home", help="McPAT home directory")
parser.add_option("-g", "--graphite-home", dest="graphite_home", help="Graphite home directory")
parser.add_option("-i", "--input-file", dest="input_file", help="Default McPAT input file")
parser.add_option("-o", "--output-file", dest="output_file", help="Output file")
parser.add_option("--read-accesses", dest="read_accesses", help="Number of Read Accesses", default="0")
parser.add_option("--write-accesses", dest="write_accesses", help="Number of Write Accesses", default="0")
parser.add_option("--read-misses", dest="read_misses", help="Number of Read Misses", default="0")
parser.add_option("--write-misses", dest="write_misses", help="Number of Write Misses", default="0")
parser.add_option("--total-cycles", dest="total_cycles", help="Total Cycles", default="100000")
(options,args) = parser.parse_args()

# Get McPAT directory inside Graphite
mcpat_dir = options.graphite_home + "/common/mcpat";

# Create McPAT Input File
createMcPATInput(options, mcpat_dir)

# Run McPAT
mcpatCmd = options.mcpat_home + "/mcpat -infile " + mcpat_dir + "/.mcpat_input.xml > " + mcpat_dir + "/.mcpat.out"
os.system(mcpatCmd)

# Parse McPAT Output
if (options.type == "data"):
   component = "L2"
elif (options.type == "directory"):
   component = "First Level Directory"
else:
   print "** Unrecognized Cache Type [%s] **" % options.type
   sys.exit(-1)
(area, peak_dynamic_power, subthreshold_leakage_power, gate_leakage_power, runtime_dynamic_power) = parseMcPATOutput(component, mcpat_dir)

# Write the Output
output_file = open(mcpat_dir + "/" + options.output_file, 'w')
output_str = "%s\n%s\n%s\n%s\n%s" % (area, peak_dynamic_power, subthreshold_leakage_power, gate_leakage_power, runtime_dynamic_power)
output_file.write(output_str)
#print output_str

# Remove the Temporary Files
rmCmd = "rm -f " + mcpat_dir + "/.mcpat_input.xml " + mcpat_dir + "/.mcpat.out"
os.system(rmCmd)
