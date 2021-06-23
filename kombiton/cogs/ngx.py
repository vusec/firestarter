# Derived classes to use for nginx

from kombit.core import *
import os
import re
import time
import sys
import cogs.llvmapps

class NgxCompileCog(cogs.llvmapps.LLVMAppsCompileCog):
	def __init__(self, config_parser):
		self.env = {}
		cogs.llvmapps.LLVMAppsCompileCog.__init__(self, config_parser)
		self.name = config_parser.get(CompileCog.SECTION_NAME, "NAME")
		self.lroot = config_parser.get(CompileCog.SECTION_NAME, "LROOT")
		self.libdirs = config_parser.get(CompileCog.SECTION_NAME, "LIBDIRS")
		os.environ.update(self.env_dict)

	def kompile(self, application):
		os.environ.update(self.env_dict)
		for d in self.libdirs.split(','):
			 kombit.util.runcmd("make -C %s/../llvm/static/%s clean" % (self.lroot, d), cwd=self.lroot)
			 kombit.util.runcmd("make -C %s/../llvm/static/%s install" % (self.lroot, d), cwd=self.lroot)

		cogs.llvmapps.LLVMAppsCompileCog.kompile(self, application)

class NgxRuntimeCog(RuntimeCog):
	def __init__(self, config_parser):
		self.env = {}
		RuntimeCog.__init__(self, config_parser)
		self.name = config_parser.get(RuntimeCog.SECTION_NAME, "NAME")
		try:
			self.ctl_args = config_parser.get(RuntimeCog.SECTION_NAME, "CTL_ARGS")
		except ConfigParser.NoOptionError:
			self.ctl_args = ""
		try:
			self.expect_err_exit = config_parser.get(RuntimeCog.SECTION_NAME, "EXPECT_ERR_EXIT")
		except ConfigParser.NoOptionError:
			self.expect_err_exit = False

	def _updateEnv(self):
		os.environ.update({'CLIENT_CP':'1', 'BENCH_TYPE':'1'})

	def setup(self, application, workspace):
		if self.env:
			os.environ.update(self.env)

		self.dont_start_server = 0
		if 'DONT_START_SERVER' in os.environ.keys():
			self.dont_start_server = int(os.environ['DONT_START_SERVER'])
			print("dont start server: %d" % (self.dont_start_server))

		if self.dont_start_server == 1:
			pass
		else:
			kombit.util.runcmd("./serverctl stop", cwd=application.src_path, silent=True)
			kombit.util.runcmd("./serverctl start %s &" % (self.ctl_args), cwd=application.src_path, silent=False)
			logger.debug('%s: Initiated starting the server.' % 'runtime')
			time.sleep(30) # Enough time to let the newly started processes stabilize

		if 'WARMUP' in os.environ.keys():
			if 1 == int(os.environ['WARMUP']):
				self._updateEnv();
				kombit.util.runcmd("./clientctl bench", cwd=application.src_path, silent=self.expect_err_exit)
		kombit.util.runcmd("rm runbench*.ini", cwd=application.src_path, silent=True)

		return
   
	def execute(self, application):
		num_runs = 1
		if 'RUNS' in os.environ.keys():
			runs = int(os.environ['RUNS'])
			num_runs = runs if runs > num_runs else num_runs

		self._updateEnv();
		for r in range(num_runs):
			 kombit.util.runcmd("./clientctl bench", cwd=application.src_path, silent=self.expect_err_exit)
			 kombit.util.runcmd("cp runbench.ini runbench.%d.ini" % r, cwd=application.src_path, silent=True)

	def windup(self, application):
		 kombit.util.runcmd("./serverctl stop", cwd=application.src_path)

class NgxTestsuiteRuntimeCog(NgxRuntimeCog):
	def _updateEnv(self):
		os.environ.update({'BENCH_TYPE':'2', 'BENCH_PERSISTENT':'1'})

class NgxProfilingRuntimeCog(NgxRuntimeCog):
	def _updateEnv(self):
		try:
			if None == os.environ['CLIENT_CP']:
				os.environ.update({'CLIENT_CP':'1'})
			if None == os.environ['BENCH_TYPE']:
				os.environ.update({'BENCH_TYPE':'1'})
		except:
			os.environ.update({'CLIENT_CP':'1', 'BENCH_TYPE':'1'})

	def setup(self, application, workspace):
		NgxRuntimeCog.setup(self, application, workspace);
		kombit.util.runcmd("rm /tmp/ltckpt_profile_dump.txt", cwd=application.src_path, silent=True)
		kombit.util.runcmd("rm ./ltckpt_profile_dump.txt", cwd=application.src_path, silent=True)

		# Manually change this for postgresql
		postgresql = False

		if 'EDFI_WARMUP__DISABLED' in os.environ.keys():
			if int(os.environ['EDFI_WARMUP']) == 1:
				self._updateEnv();
				kombit.util.runcmd("./clientctl bench", cwd=application.src_path, silent=self.expect_err_exit)
			kombit.util.runcmd("rm runbench*.ini", cwd=application.src_path, silent=True)
			os.environ.update({'EDFI_WARMUP':'0'})

		if self.dont_start_server == 0:
			self.sigtoutin = 1
			if 'SIGTOUTIN' in os.environ.keys():
				self.sigtoutin = int(os.environ['SIGTOUTIN'])
			if self.sigtoutin == 0:
				if not postgresql:
					kombit.util.runcmd("kill -SIGWINCH `pidof %s | tr ' ' '\\n' | sort -n | tail -1`" % application.name, cwd=application.src_path, silent=True)
				#	kombit.util.runcmd("kill -SIGPOLL `pidof %s | tr ' ' '\\n' | sort -n | head -1`" % application.name, cwd=application.src_path, silent=True)
				else:
					kombit.util.runcmd("for p in `pidof %s | tr ' ' '\\n' | sort -n`; do kill -SIGURG $p; done" % application.name, cwd=application.src_path, silent=True)
					print ("Sent SIGURG to reset profiling counters.")
					sys.stdout.flush()
			else:
#				kombit.util.runcmd("kill -SIGTTOU `pidof %s | tr ' ' '\\n' | sort -n | tail -1`" % application.name, cwd=application.src_path, silent=True)
				kombit.util.runcmd("for p in `pidof %s | tr ' ' '\\n' | sort -n`; do kill -SIGTTOU $p; done" % application.name, cwd=application.src_path, silent=True)
		return

	def windup(self, application):
		if self.dont_start_server == 0:
			if self.sigtoutin == 0:
				kombit.util.runcmd("for p in `pidof %s | tr ' ' '\\n' | sort -n`; do kill -SIGPROF $p; done" % application.name, cwd=application.src_path, silent=True)
		#		kombit.util.runcmd("kill -SIGPROF `pidof %s | tr ' ' '\\n' | sort -n | tail -1`" % application.name, cwd=application.src_path, silent=True)
				print ("Sent SIGPROF to dump profiling counters.")
				sys.stdout.flush()
			else:
				# kombit.util.runcmd("kill -SIGTTIN `pidof %s | tr ' ' '\\n' | sort -n | tail -1`" % application.name, cwd=application.src_path, silent=True)
				kombit.util.runcmd("for p in `pidof %s | tr ' ' '\\n' | sort -n`; do kill -SIGTTIN $p; done" % application.name, cwd=application.src_path, silent=True)
				kombit.util.runcmd("kill -SIGTSTP `pidof %s | tr ' ' '\\n' | sort -n | tail -1`" % application.name, cwd=application.src_path, silent=True)
		time.sleep(5)
		kombit.util.runcmd("cp /tmp/ltckpt_profile_dump.txt ./", cwd=application.src_path, silent=True)
		kombit.util.grepInFile("/tmp/ltckpt_profile_dump.txt", "\(TSX\|UNDOLOG\)")
		kombit.util.runcmd("cp /tmp/rcvry_*_dump.txt ./", cwd=application.src_path, silent=True)
		NgxRuntimeCog.windup(self, application)

class NgxWorkspace(Workspace):
	def __init__(self, config_parser):
		Workspace.__init__(self, config_parser)
		self.name = config_parser.get(Workspace.SECTION_NAME, "NAME")
		try:
			self.env_dict =  kombit.util.getDictFromConfig(config_parser, Workspace.SECTION_NAME, "ENV")
		except ConfigParser.NoOptionError:
			self.env_dict = {}
		try:
			rt_env_dict =  kombit.util.getDictFromConfig(config_parser, NgxRuntimeCog.SECTION_NAME, "ENV")
			if 'BENCH_MEM' in rt_env_dict.keys():
				if 1 == int(rt_env_dict['BENCH_MEM']):
					self.benchmem = True;
			else:
				self.benchmem = False;
		except ConfigParser.NoOptionError:
			self.benchmem = False;

	def _calcAvg(self, resLines, results_dir, avg_filename):
		total = 0
		num = len(resLines)
		for r in resLines:
			try:
				total = total + float(r)
			except ValueError:
				return
		avg = total/num
		kombit.util.runcmd("echo 'avg: %f ;\t num_runs: %d' | tee %s" % (avg, num, avg_filename), cwd=results_dir, silent=True)
		return

	def _copyLogs(self, application, dest_dir):
		kombit.util.runcmd("cp runbench.* %s" % dest_dir, cwd=application.src_path, silent=True)
		resLines = []
		if 'RUNBENCH_SECS' in self.env_dict.keys():
			if 1 == int(self.env_dict['RUNBENCH_SECS']):
				resLines =  kombit.util.cmdToOutLines("for f in `find . -name 'runbench.*.ini'`; do grep 'runbench_secs =' $f | cut -d'=' -f 2 | cut -d '(' -f 1 | tr -d ' '; done", cwd=dest_dir)
			print ("runbench_secs mode")
		else:
			resLines =  kombit.util.cmdToOutLines("for f in `find . -name 'runbench.*.ini'`; do grep 'requests_per_sec' $f | cut -d'=' -f 2 | tr -d ' '; done", cwd=dest_dir)

		self._calcAvg(resLines, dest_dir, "avg.txt")

		if True == self.benchmem:
			resLines =  kombit.util.cmdToOutLines("for f in `find . -name 'runbench.*.ini'`; do grep 'runbench_mean_rss' $f | cut -d'=' -f 2 | tr -d ' '; done", cwd=dest_dir)
			self._calcAvg(resLines, dest_dir, "avg_rss.txt")
			resLines =  kombit.util.cmdToOutLines("for f in `find . -name 'runbench.*.ini'`; do grep 'runbench_mean_pss' $f | cut -d'=' -f 2 | tr -d ' '; done", cwd=dest_dir)
			self._calcAvg(resLines, dest_dir, "avg_pss.txt")
		return

	def setup(self, application, experiment_id):
		if self.work_dir:
			if not os.path.isdir(self.work_dir):
				kombit.util.runcmd("mkdir -p %s" % self.work_dir)
		if self.log_dir:
			if not os.path.isdir(self.log_dir):
				kombit.util.runcmd("mkdir -p %s" % self.log_dir)
		kombit.util.perf_prepare()
		kombit.util.runcmd("rm runbench.*.ini", cwd=application.src_path, silent=True)
		logger.debug('No special %s setup required.' % 'workspace')
		return 

	def windup(self, application, experiment_id):
		print ('Workspace windup: Stopping ngx server for exp id: %s' % experiment_id)
		kombit.util.runcmd("./serverctl stop ", cwd=application.src_path)
		dest_dir = self.log_dir + "/" + experiment_id
		if not os.path.isdir(dest_dir):
			kombit.util.runcmd("mkdir -p %s" % dest_dir)
		self._copyLogs(application, dest_dir)
	#	 kombit.util.perf_windup()

class NgxProfilingWorkspace(NgxWorkspace):
	def _copyLogs(self, application, dest_dir):
		kombit.util.runcmd("cp runbench.* %s" % dest_dir, cwd=application.src_path, silent=True)
		kombit.util.runcmd("cp *_dump.txt %s" % dest_dir, cwd=application.src_path, silent=True)
		kombit.util.runcmd("cp rcvry_action_dump.txt.* %s/rcvry_action_dump.txt" % dest_dir, cwd=application.src_path, silent=True)
		data =  kombit.util.cmdToOut("tail -1 ./rcvry_action_dump.txt", cwd=dest_dir)
		print ("%s" % (data))
		self.add_site_info_to_ltckpt_profile_dump(application, dest_dir)
		self.add_site_info_to_rcvry_profile_dump(application, dest_dir)

	def siteid2libcallname(self, ckpt, site_id, ll_file):
		if (ckpt == "TSX"):
			lname =  kombit.util.cmdToOut("grep -B 50 'loop_tsx(i32 %s)' %s | grep HYBPREP_LIBCALL | cut -d@ -f2 | cut -d'(' -f 1 | tail -1" % (site_id, ll_file))
			return lname
		elif (ckpt == "UNDOLOG"):
			branchname =  kombit.util.cmdToOut("grep -B 2 'loop_undolog(i32 %s)' %s | grep '^.*: *' | cut -d: -f1" % (site_id, ll_file))
			lname =  kombit.util.cmdToOut("grep -B 5000 'loop_undolog(i32 %s)' %s | grep -B 50 'br.*label.*%s,' | grep HYBPREP_LIBCALL | tail -1 | cut -d@ -f2 | cut -d'(' -f 1" % (site_id, ll_file, branchname))
			return lname
		return ""

	def add_site_info_to_ltckpt_profile_dump(self, application, dump_dir):
		ll_file = None
		try:
			kombit.util.runcmd("rm *.ll", cwd=application.obj_path, silent=True)
			kombit.util.llvmDis(application, use_obj_path=True)
			ll_file = "%s/%s.bcl.ll" % (application.obj_path, application.name)
		except ConfigParser.NoOptionError:
			return
		dump_file = "%s/%s" % (dump_dir, "ltckpt_profile_dump.txt")
		dump_lname_file = "%s/%s" % (dump_dir, "ltckpt_profile_dump.info.txt")
		kombit.util.runcmd("rm %s" % (dump_lname_file), cwd=dump_dir, silent=True)
		vars = ["TSX", "UNDOLOG"]
		for v in vars:
			site_ids =  kombit.util.cmdToOutLines("grep '%s,' %s | cut -d',' -f1 | tr -d ' '" % (v, dump_file), cwd=dump_dir)
			print ("num site_ids: %d" % (len(site_ids)))
			site_id_map = {}
			for s in site_ids:
				libcallname = self.siteid2libcallname(v, s, ll_file)
				site_id_map[s] = libcallname
				print ("libcallname %20s" % (libcallname))
				# Now, replace "site_id," with "site_id, libcallname,"
				kombit.util.runcmd("grep '^ *%s, *%s,' %s | sed -E 's/(^ *[0-9][0-9]*),/\\1, %20s,/g' >> %s"
						% (s, v, dump_file, libcallname, dump_lname_file), cwd=dump_dir)
		return

	def add_site_info_to_rcvry_profile_dump(self, application, dump_dir):
		ll_file = None
		try:
			kombit.util.runcmd("rm *.ll", cwd=application.obj_path, silent=True)
			kombit.util.llvmDis(application, use_obj_path=True)
			ll_file = "%s/%s.bcl.ll" % (application.obj_path, application.name)
		except ConfigParser.NoOptionError:
			return
		dump_file = "%s/%s" % (dump_dir, "rcvry_profile_dump.txt")
		dump_lname_file = "%s/%s" % (dump_dir, "rcvry_profile_dump.info.txt")
		kombit.util.runcmd("rm %s" % (dump_lname_file), cwd=dump_dir, silent=True)
		site_ids =  kombit.util.cmdToOutLines("grep '^ *[0-9][0-9]*,' %s | cut -d',' -f1 | tr -d ' '" % (dump_file), cwd=dump_dir)
		print ("num site_ids: %d" % (len(site_ids)))
		site_id_map = {}
		for s in site_ids:
			libcallname =  kombit.util.cmdToOut("grep -B2 '@rcvry_prof_count_rcvry_branch_hits(i32 %s)' %s | head -1 | cut -d@ -f2 | cut -d'(' -f 1" % (s, ll_file), cwd=dump_dir)
			site_id_map[s] = libcallname
			# Now, replace "site_id," with "site_id, libcallname,"
			kombit.util.runcmd("grep '^ *%s,' %s | sed -E 's/(^ *[0-9][0-9]*),/\\1, %20s,/g' >> %s" % (s, dump_file, libcallname, dump_lname_file), cwd=dump_dir)
		return

class NgxTestsuiteWorkspace(NgxWorkspace):
	def _copyLogs(self, application, dest_dir):
		pass

