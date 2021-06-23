#! /bin/python

import json
import os

all_traces = {}
common_traced = []

def load_all_traces():
    files = os.listdir(".")
    jsons = [ f for f in files if '.json' in f ]
    for jfile in jsons:
	bbtrace = {}
	jkey = int(jfile.rsplit('_')[3].rsplit('.')[0])
	print jkey
	with open(jfile) as trace:
	    bbtrace = json.load(trace)
	all_traces[jkey] = bbtrace.keys()
    return

def fetch_commons():
    for l in all_traces.keys():
	for k in all_traces.keys():
	    if l == k:
		continue
	    lsites = all_traces[l]
	    ksites = all_traces[k]
	    # iterate over every pair of trace lists
	    for site in lsites:
	        if site in common_traced:
		    continue
		if site in ksites:
		    common_traced.append(site)
    return

def dump_commons_to_file():
    common_json = "./rcvry_bbtrace_dump.common.json"
    with open(common_json, "w+") as c:
	json.dump(common_traced, c)
    return

def elim_commons_from_fulltrace():
    fulltrace_file = "/tmp/rcvry_bbtrace_dump.json"
    fulltrace = {}
    # load
    with open(fulltrace_file) as full:
	fulltrace = json.load(full)

    # eliminate commons
    for c in common_traced:
	if c in fulltrace.keys():
	    del fulltrace[c]

    # dump
    with open(fulltrace_file, "w") as full:
	json.dump(fulltrace, full)

    return

if __name__ == "__main__":
    load_all_traces()
    print "Num trace-lists in all_traces: %d" % (len(all_traces))
    fetch_commons()
    print "commons: "
    print common_traced
    dump_commons_to_file()
    elim_commons_from_fulltrace()

