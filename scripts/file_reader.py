#!/usr/bin/env python

import sys
import re

class FileReader:
   def __init__(self, cfg_filename):
      try:
         self.file_contents = open(cfg_filename).readlines()
      except:
         print "ERROR# 1: Unable to open file (%s) for reading" % (cfg_filename)
         sys.exit(-1)
      # Strip comments and spaces
      self.stripComments()
      self.stripSpaces()

   def stripComments(self):
      for i in range(0,len(self.file_contents)):
         self.file_contents[i] = self.file_contents[i].partition('#')[0]

   def stripSpaces(self):
      temp_file_contents = self.file_contents
      self.file_contents = []
      for line in temp_file_contents:
         stripped_line = line.strip()
         if stripped_line != '':
            self.file_contents.append(stripped_line)

   def readInt(self, param):
      return int(self.read(param))

   def readFloat(self, param):
      return float(self.read(param))

   def readString(self, param):
      return self.read(param)

   def read(self, param):
      param_parts = param.rpartition('/')
      if (param_parts[1] != '/'):
         print "ERROR# 2: Param (%s) not in proper format" % (param)
         sys.exit(-1)

      section_exp = "[%s]" % (param_parts[0])
      name_exp = "%s\s*=\s*([-.+\w]+)" % (param_parts[2])

      section_found = False
      
      for line in self.file_contents:
         if section_found:
            name_match = re.match(name_exp, line) 
            if name_match != None:
               return name_match.group(1)
         else:
            section_found = (section_exp == line)
      print "ERROR# 3: Could not find param (%s) in the cfg file" % (param)
      sys.exit(-1)
