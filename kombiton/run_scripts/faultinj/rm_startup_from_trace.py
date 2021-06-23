#!/bin/python

import json
import os


all_traces = []
bbtrace = {}
startup_bbs = []
NumKeysAltered = 0
NumEmptyKeys = 0

def load_all_traces():
	global bbtrace, startup_bbs
	bbtfile = "/tmp/rcvry_bbtrace_dump.json"
	startupfile = "/tmp/rcvry_bbtrace_dump.json.startup"
	jd = json.JSONDecoder()
	line = ""
	with open(bbtfile) as jf:
		line = jf.readline();
	bbtrace = jd.decode(line)
	
	print "Len(bbtraces): %d" % len(bbtrace.keys())

	with open(startupfile) as jf:
		line = jf.readline();
		startup_bbs = list(set(jd.decode(line)))

	print "Len(startup_bbs): %d" % len(startup_bbs)
	return

def rm_startup_bbs():
	global NumKeysAltered, NumEmptyKeys
	trace_bbs = []
	for k in bbtrace.keys():
		print "k: %s" % (k)
		trace_bbs = list(set(bbtrace[k]))
		altered = False
		rm_bbs = []
		for t in trace_bbs:
			for s in startup_bbs:
				if (s != 0) and (t == s):
					if not s in rm_bbs:
						rm_bbs.append(s)
						altered = True
		if altered:
			NumKeysAltered = NumKeysAltered + 1
			for b in rm_bbs:
				bbtrace[k].remove(b)
				print "\tremoved: %d from key: %s" % (b, k)

	empty_keys = []
	for k in bbtrace.keys():
		t = bbtrace[k]
		if (len(t) == 1) and (t[0] == 0):
			empty_keys.append(k)

	for e in empty_keys:
		del bbtrace[e]
		NumEmptyKeys = NumEmptyKeys + 1

	return

def dump_to_json():
	donefile = "./rcvry_bbtrace_dump.startupremoved.json"
	with open(donefile, "w+") as df:
		json.dump(bbtrace, df)
	return

def get_empty_keys():
	global bbtrace
	empty = []
	for k in bbtrace.keys():
		t = bbtrace[k]
		if (len(t) == 1) and (t[0] == 0):
			empty.append(k)
	return empty

if  __name__ == "__main__":
	load_all_traces()
	print "Num trace-lists in bbtraces: %d" % (len(bbtrace.keys()))
	print "Num bbs in startup: %d" % (len(startup_bbs))
	rm_startup_bbs()
	print "Num trace-lists altered: %d" % (NumKeysAltered)
	print "Num trace-lists in altered bbtraces: %d" % (len(bbtrace.keys()))
	print "Num empty keys removed: %d" % (NumEmptyKeys)
	dump_to_json()
	
