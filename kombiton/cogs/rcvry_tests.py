# Derived classes to use for nginx

from kombit.core import *
import os
import re
import time
import cogs.llvmapps
import cogs.ngx

class RcvryTestsCompileCog(cogs.ngx.NgxCompileCog):
    pass

class RcvryTestsRuntimeCog(RuntimeCog):
    def __init__(self, config_parser):
	self.env = {}
	RuntimeCog.__init__(self, config_parser)
	self.name = config_parser.get(RuntimeCog.SECTION_NAME, "NAME")

    def setup(self, application, workspace):
	if self.env:
	    os.environ.update(self.env)
	
	return
   
    def execute(self, application):
	num_runs = 1
	if 'RUNS' in os.environ.keys():
		runs = int(os.environ['RUNS'])
		num_runs = runs if runs > num_runs else num_runs

	for r in range(num_runs):
		util.runcmd("./clientctl run", cwd=application.src_path)

    def windup(self, application):
	pass

class RcvryTestsWorkspace(Workspace):
    def __init__(self, config_parser):
	Workspace.__init__(self, config_parser)
	self.name = config_parser.get(Workspace.SECTION_NAME, "NAME")

    def setup(self, application, experiment_id):
	if self.work_dir:
                if not os.path.isdir(self.work_dir):
                        util.runcmd("mkdir -p %s" % self.work_dir)
        if self.log_dir:
                if not os.path.isdir(self.log_dir):
                        util.runcmd("mkdir -p %s" % self.log_dir)
	util.perf_prepare()
	logger.debug('No special %s setup required.' % 'workspace')
	return 

    def windup(self, application, experiment_id):
        print '[%s] Workspace windup: Nothing special required as yet.' % experiment_id
        util.llvmDis(application)
	util.perf_windup()
