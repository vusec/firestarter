#! /bin/python

import json
import os
import subprocess

all_traces = []
startup_bbs = []
merged_bbtrace = {}
NumKeysAltered = 0
NumEmptyKeys = 0

def get_trace_files():
	j_files = []
	j_files_startup = []
	j_files_trace = []

	s = subprocess.check_output(['find', '/tmp/', '-maxdepth', '1', '-name', "rcvry_bbtrace_dump.json*"])
	j_files = [ f for f in s.split("\n") if f ]
	j_files_startup = [ startup for startup in j_files if ".startup" in startup ]
	j_files_trace = [ trace for trace in j_files if not ".startup" in trace ]
	print "Num startup trace files: %d" % (len(j_files_startup))
	print "Num trace files: %d" % (len(j_files_trace))
	return j_files_trace, j_files_startup

def load_all_traces():
	jfile = "/tmp/rcvry_bbtrace_dump.json"
	jd = json.JSONDecoder()

	trace_files, startup_files = get_trace_files()
	for jfile in trace_files:
		print "Opening json file: %s" % (jfile)
		with open(jfile) as jf:
			trace = jf.readline();
			while trace:
				bbtrace = jd.decode(trace)
				all_traces.append(bbtrace)
				trace = jf.readline();
	for jfile in startup_files:
		print "Opening startup json file: %s" % (jfile)
		with open(jfile) as jf:
			startup = jf.readline();
			while startup:
				bbs = jd.decode(startup)
				for b in [bb for bb in bbs if bb not in startup_bbs]:
					startup_bbs.append(b)
				startup = jf.readline();
	return

def merge_bbtraces():
	for i in range(0, len(all_traces)):
		bbt = {}
		bbt = all_traces[i]
		# iterate over each trace dictionary
		for site in bbt.keys():
			if (site in merged_bbtrace.keys()):
				for bb in bbt[site]:
					if not (bb in merged_bbtrace[site]):
						merged_bbtrace[site].append(bb)
			else:
				merged_bbtrace[site] = bbt[site]
	return

def rm_startup_bbs():
		global NumKeysAltered, NumEmptyKeys
		trace_bbs = []
		for k in merged_bbtrace.keys():
				# print "k: %s" % (k)
				trace_bbs = list(set(merged_bbtrace[k]))
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
								merged_bbtrace[k].remove(b)
								# print "\tremoved: %d from key: %s" % (b, k)

		empty_keys = []
		for k in merged_bbtrace.keys():
				t = merged_bbtrace[k]
				if (len(t) == 1) and (t[0] == 0):
						empty_keys.append(k)

		for e in empty_keys:
				del merged_bbtrace[e]
				NumEmptyKeys = NumEmptyKeys + 1
		return

def dump_to_file(what, dest_json):
	with open(dest_json, "w+") as c:
		json.dump(what, c)
	return

def dump_merged_to_file():
	common_json = "./rcvry_bbtrace_dump.merged.json"
	with open(common_json, "w+") as c:
		json.dump(merged_bbtrace, c)
	return

if __name__ == "__main__":
	load_all_traces()
	print "Num trace-lists in all_traces: %d" % (len(all_traces))
	merge_bbtraces()
	print "Num sites after the merge: %d" % (len(merged_bbtrace.keys()))

	print "Num trace-lists in merged_bbtrace: %d" % (len(merged_bbtrace.keys()))
	print "Num bbs in startup: %d" % (len(startup_bbs))
	rm_startup_bbs()
	print "Num trace-lists altered: %d" % (NumKeysAltered)
	print "Num trace-lists in altered merged_bbtrace: %d" % (len(merged_bbtrace.keys()))
	print "Num empty keys removed: %d" % (NumEmptyKeys)
	dump_to_file(merged_bbtrace, "./rcvry_bbtrace_dump.merged.json")
	dump_to_file(startup_bbs, "./rcvry_startup_dump.merged.json")

