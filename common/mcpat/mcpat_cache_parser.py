#!/usr/bin/env python

from xml.dom.minidom import parse
from optparse import OptionParser
import re
import os
import sys
import time

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
   print "ERROR: McPAT Cache Parser: Unrecognized Attribute Name (%s)" % (name)
   sys.exit(-6)

def createMcPATInput(options, mcpat_input_filename):
   # Create a document object
   try:
      dom = parse(options.input_file)
   except IOError:
      print "ERROR: McPAT Cache Parser: Could not open input file (%s)" % (options.input_file)
      sys.exit(-3)

   clockrate = "%d" % (options.frequency * 1000)
  
   # Common Attributes
   core = getArchComponent(dom,'core0')
   setAttribute(core, "clock_rate", clockrate)
   system_comp = getArchComponent(dom,'system')
   setAttribute(system_comp, "core_tech_node", options.technology_node)
   setAttribute(system_comp, "total_cycles", "0")
   setAttribute(system_comp, "idle_cycles", "0")
   setAttribute(system_comp, "busy_cycles", "0")
   
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
      
      setAttribute(directory, "read_accesses", "0")
      setAttribute(directory, "write_accesses", "0")
      setAttribute(directory, "read_misses", "0")
      setAttribute(directory, "write_misses", "0")

   elif (options.type == "data"):
      cache = getArchComponent(dom,'L20')
      
      L2_config = "%d,%d,%d,%d,%d,%d,%d,%d" % (options.size, options.blocksize, options.associativity, 1, 1, options.delay, options.blocksize, 1)
      buffer_sizes = "%d,%d,%d,%d" % (buffer_size,buffer_size,buffer_size,buffer_size)
      ports = "1,1,1"

      setAttribute(cache, "L2_config", L2_config)
      setAttribute(cache, "buffer_sizes", buffer_sizes)
      setAttribute(cache, "clockrate", clockrate)
      setAttribute(cache, "ports", ports)

      setAttribute(cache, "read_accesses", "0")
      setAttribute(cache, "write_accesses", "0")
      setAttribute(cache, "read_misses", "0")
      setAttribute(cache, "write_misses", "0")
   
   else:
      print "ERROR: McPAT Cache Parser: Unrecognized Cache Type (%s)" % (options.type)
      sys.exit(-5)

   try:
      output_file = open(mcpat_input_filename, 'w')
   except IOError:
      print "ERROR: McPAT Cache Parser: Could not open intermediate file (%s) for writing" % (mcpat_input_filename)
      sys.exit(-4)
   else:
      dom.writexml(output_file)
      output_file.close()

def parseMcPATOutput(component, mcpat_output_filename):
  
   try:
      file = open(mcpat_output_filename)
   except IOError:
      print "ERROR: McPAT Cache Parser: Could not open intermediate file (%s) for reading" % (mcpat_output_filename)
      sys.exit(-4)
   else:
      lines = file.readlines()
      file.close()

   area = None
   subthreshold_leakage_power = None
   gate_leakage_power = None
   tag_array_read_energy = None
   tag_array_write_energy = None
   data_array_read_energy = None
   data_array_write_energy = None

   reached = False
   for line in lines:
      if (reached):
         if (area == None):
            area = findMatch("Area", line)
         if (subthreshold_leakage_power == None):
            subthreshold_leakage_power = findMatch("Subthreshold Leakage", line)
         if (gate_leakage_power == None):
            gate_leakage_power = findMatch("Gate Leakage", line)
         if (tag_array_read_energy == None):
            tag_array_read_energy = findMatch("Tag Array Read Energy", line)
         if (tag_array_write_energy == None):
            tag_array_write_energy = findMatch("Tag Array Write Energy", line)
         if (data_array_read_energy == None):
            data_array_read_energy = findMatch("Data Array Read Energy", line)
         if (data_array_write_energy == None):
            data_array_write_energy = findMatch("Data Array Write Energy", line)
         if (data_array_write_energy != None):
            break
         
      component_str = r"^%s" % component
      if re.match(component_str, line):
         reached = True

   return (area, subthreshold_leakage_power, gate_leakage_power, tag_array_read_energy, tag_array_write_energy, data_array_read_energy, data_array_write_energy)
         
# Main Program Starts Here

# Parse the Command Line Options
parser = OptionParser()
parser.add_option("--suffix", dest="suffix", help="Suffix", default="")
parser.add_option("--type", dest="type", help="Cache Type (data,directory)")
parser.add_option("--technology-node", dest="technology_node", help="Technology Node (in nm)")
parser.add_option("--size", dest="size", type="int", help="Cache Size (in Bytes)")
parser.add_option("--blocksize", dest="blocksize", type="int", help="Block Size (in Bytes)")
parser.add_option("--associativity", dest="associativity", type="int", help="Associativity")
parser.add_option("--delay", dest="delay", type="int", help="Cache Delay")
parser.add_option("--frequency", dest="frequency", type="float", help="Frequency (in GHz)")
parser.add_option("--mcpat-home", dest="mcpat_home", help="McPAT home directory")
parser.add_option("--input-file", dest="input_file", help="Default McPAT input file")
parser.add_option("--output-file", dest="output_file", help="Output file")
(options,args) = parser.parse_args()

# Intermediate Files
mcpat_input_filename = ".mcpat.xml.input.%s" % (options.suffix)
mcpat_output_filename = ".mcpat.output.%s" % (options.suffix)

# Create McPAT Input File
createMcPATInput(options, mcpat_input_filename)

# Run McPAT
mcpat_executable = options.mcpat_home + "/mcpat"

if (os.path.exists(mcpat_executable) == False):
   print "ERROR: Could not find McPAT executable (%s).\n" \
         "ERROR: Make sure [general/McPAT_home] is set correctly in carbon_sim.cfg and McPAT is compiled" % (mcpat_executable)
   sys.exit(-1)

mcpat_cmd = "%s -infile %s > %s" % (mcpat_executable, mcpat_input_filename, mcpat_output_filename)
if (os.system(mcpat_cmd) != 0):
   print "ERROR: Could not run McPAT successfully\n"
   sys.exit(-2)

# Parse McPAT Output
if (options.type == "data"):
   component = "L2"
elif (options.type == "directory"):
   component = "First Level Directory"
else:
   print "ERROR: McPAT Cache Parser: Unrecognized Cache Type (%s)" % (options.type)
   sys.exit(-5)
(area, subthreshold_leakage_power, gate_leakage_power, tag_array_read_energy, tag_array_write_energy, data_array_read_energy, data_array_write_energy) = parseMcPATOutput(component, mcpat_output_filename)

# Write the Output
output_filename = "%s.%s" % (options.output_file, options.suffix)
try:
   output_file = open(output_filename, 'w')
except IOError:
   print "ERROR: McPAT Cache Parser: Could not open output file (%s)" % (output_filename)
   sys.exit(-3)
else:
   output_str = "%s\n%s\n%s\n%s\n%s\n%s\n%s" % (area, subthreshold_leakage_power, gate_leakage_power, \
         tag_array_read_energy, tag_array_write_energy, data_array_read_energy, data_array_write_energy)
   output_file.write(output_str)
   output_file.close()

# Remove the Temporary Files
rm_cmd = "rm -f %s %s" % (mcpat_input_filename, mcpat_output_filename)
if (os.system(rm_cmd) != 0):
   print "ERROR: Could not remove temporary files (%s) and (%s)" % (mcpat_input_filename, mcpat_output_filename)
   sys.exit(-4)
