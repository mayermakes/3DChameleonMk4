#!/usr/bin/python

# 3D Chameleon - T2 and T3 reverser post processor
# Copyright 2020 - 3D Chameleon, LLC, All Rights Reserved

import re
import os
import sys

reverse_mod = False

# set io files
file_input = sys.argv[1]
file_output = re.sub('.gcode.pp$', '.reversedrive.gcode', file_input)

if os.path.exists("{}.backup".format(file_input)):
  os.remove("{}.backup".format(file_input))

if os.path.exists(file_output):
  os.remove(file_output)

if (file_output == file_input):
	file_output = "{}.reversedrive".format(file_input)

with open(file_input) as input:
	with open(file_output, 'w', newline='') as output:

		#print >> output, ";     3D Chameleon Postprocessor"
		#print >> output, "; Copyright 2020 by 3D Chameleon, LLC"
		#print >> output, ";        All Rights Reserved"
		#print >> output, ";"
		#print >> output, ";"

		print(";     3D Chameleon Postprocessor", file=output, end='\n')
		print("; Copyright 2020 by 3D Chameleon, LLC", file=output, end='\n')
		print(";        All Rights Reserved", file=output, end='\n')
		print(";", file=output, end='\n')
		print(";", file=output, end='\n')
		
		for line in input:
			line = line.strip()

			# ignore extruder
			if (re.search('- 3DC Process Tool 0 -', line)):
				reverse_mod = False

			# ignore extruder
			if (re.search('- 3DC Process Tool 1 -', line)):
				reverse_mod = False
			
			# flip extruder direction
			if (re.search('- 3DC Process Tool 2 -', line)):
				reverse_mod = True
				
			# flip extruder direction
			if (re.search('- 3DC Process Tool 3 -', line)):
				reverse_mod = True
			
			# reverse E values if in T2 or T3
			if(reverse_mod):
				if('G92 E' in line):
					line = line.replace("E","E-")
					line = line.replace("E-0", "E0")
					
				if(('G0' in line) or ('G1' in line)):
					if ('E-' in line):
						line = line.replace("E-", "E")
					else:
						if ('E' in line):
							line = line.replace("E", "E-")
			
			# normal output; copy to new file
			#print >> output, line
			print(line, file=output, end='\n')

# backup old file
os.rename(file_input,"{}.backup".format(file_input))

# rename temp file to original filename
os.rename(file_output, file_input)