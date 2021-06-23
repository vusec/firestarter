import os
import re
import sys
import util

# get reqs per second
def get_rps_all_runs(ini_dir):
	ini_files = [ os.path.join(ini_dir, f) for f in os.listdir(ini_dir) if os.path.isfile(os.path.join(ini_dir, f)) and re.match(".*\.[0-9]*\.ini", f) ]
	rps = []
	for f in ini_files:
		ret, data = util.grepInFile(f, "requests_per_sec")
		nums = re.split(" = ", data[0])
		rps.append(nums[1])
	return rps

def get_avg_rps(rps_list):
	sum = 0.0;
        for r in rps_list:
                sum += float(r)
        avg = sum / len(rps_list)
	return round(avg, 3), len(rps_list)

def print_usage(progname):
	print "Usage: %s <path to dir containing ini files>", progname
	quit()
	
def main():
	if len(sys.argv) <= 1:
		print_usage(sys.argv[0])

	dirname = sys.argv[1];
	if False == os.path.isdir(dirname):
		print_usage(sys.argv[0])

	rps = get_rps_all_runs(dirname)
	num, avg = get_avg_rps(rps)
	print "num runs = %d : avg = %.3f" % (avg, num)

if __name__ == "__main__":
	main() 
