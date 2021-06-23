# Derived classes to use for ltrace for server apps

from kombit.core import *
import cogs.ngx

#class LtraceRuntimeCog(cogs.ngx.NgxRuntimeCog):
class LtraceRuntimeCog(cogs.ngx.NgxTestsuiteRuntimeCog):
	spid = 0
	def setup(self, application, workspace):
		util.runcmd("rm -f ./.tmp/ltrace.*", cwd=application.src_path, silent=True)
		cogs.ngx.NgxRuntimeCog.setup(self, application, workspace);
		spid = util.cmdToOutLines("ps --no-headers -C %s | tail -n 1 | sed 's/^ //g' | cut -d' ' -f 1"
				   % (application.name))
		util.runcmd("sudo ./serverctl ltrace %s" % spid[0], cwd=application.src_path, silent=False)
		return

   # execute() and windup() remain the same as the parent class

class LtraceWorkspace(cogs.ngx.NgxWorkspace):

    def windup(self, application, experiment_id):
	cogs.ngx.NgxWorkspace.windup(self, application, experiment_id)
	dest_dir = self.log_dir + "/" + experiment_id
	util.runcmd("cut -d' ' -f 3 %s | cut -d'(' -f 1 | grep -v '[<+\-].*' | sort | uniq > %s/ltrace.libcalls.txt"
			% ("./.tmp/ltrace.*", dest_dir), cwd=application.src_path, silent=False)
	util.runcmd("cp ./.tmp/ltrace.* %s" % dest_dir, cwd=application.src_path, silent=True)
	return	
