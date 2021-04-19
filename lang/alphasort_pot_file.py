#!/usr/bin/env python3
"Alphabetically (in quotes) sort entries in POT file."

import polib
import os
from optparse import OptionParser

##
## METHODS
##

def keyfunc(e):
	if e.msgctxt is None:
		return e.msgid
	else:
		return e.msgid + e.msgctxt

def sort_entries(entries):
	return sorted(entries, key=keyfunc)

##
##  MAIN
##

cmd_usage = 'usage: %prog [options] filename'
parser = OptionParser(usage=cmd_usage)
(options, args) = parser.parse_args()

if len(args) != 1:
    print("Expected 1 argument")
    exit(1)

filename = args[0]

if not os.path.isfile(filename):
    print("Error: File not found.")
    exit(1)

pot = polib.pofile(filename)
entries = [e for e in pot]
entries_sorted = sort_entries(entries)

res = polib.POFile()
res.metadata = pot.metadata
for e in entries_sorted:
    res.append(e)
res.save(filename, newline='\n')
