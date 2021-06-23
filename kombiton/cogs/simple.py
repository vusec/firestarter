# Derived classes to use for simple toy programs

from kombit.core import *
import os.path

class SimpleCompileCog(CompileCog):
    def __init__(self, config_parser):
    	self.name = config_parser.get(CompileCog.SECTION_NAME, "NAME")
    	self.cc = config_parser.get('DEFAULT', "CC")
    	self.cflags = config_parser.get(CompileCog.SECTION_NAME, "CFLAGS")
    	self.link_flags = config_parser.get(CompileCog.SECTION_NAME, "LFLAGS")
        self.stat_libdir = config_parser.get(CompileCog.SECTION_NAME, "STATIC_LIBS_DIR")
    	CompileCog.__init__(self, config_parser)
    	return

    def configure(self, application):
    	return

    def kompile(self, application):
	static_libs = " "
	for s in self.static_libs.split(" "):
	    static_libs = static_libs + " %s/%s.bcc" % (self.stat_libdir, s)
	print "static_libs: " + static_libs
	# cleanup
	util.runcmd("rm %s *.bc *.o" % application.name, cwd=application.src_path, silent=True)
	# generate .o file
    	util.runcmd("%s %s -c *.c" % (self.cc, self.cflags), cwd=application.src_path)
	# link with static libs
	util.runcmd("%s %s -o %s *.o %s" % (self.cc, static_libs, application.name, self.link_flags), cwd=application.src_path)
    	return

    def cleanup(self, application):
    	util.runcmd("rm *.o %s" % self.name,  cwd=application.src_path)
    	return

class SimpleInstrumentationCog(InstrumentationCog):
    def __init__(self, config_parser):
        InstrumentationCog.__init__(self, config_parser)
        self.name = config_parser.get(InstrumentationCog.SECTION_NAME, "NAME")
        self.env_dict = util.getDictFromConfig(config_parser, InstrumentationCog.SECTION_NAME, "ENV")
        self.passes_bin_dir = config_parser.get(InstrumentationCog.SECTION_NAME, "PASSES_BIN_DIR")
        self.cc = config_parser.get('DEFAULT', "CC")
    	self.link_flags = config_parser.get(CompileCog.SECTION_NAME, "LFLAGS")
        self.llvm_bin_dir = config_parser.get('DEFAULT', "LLVM_BIN_DIR")
        return

    def instrument(self, application):
        env = os.environ
        env.update(self.env_dict)
        env.update({"OPT_ARGS":"%s" % (self.opt_args)})
	load_args = " "
	for p in self.opt_passes.split(" "):
		full_path = "%s/%s.so" % (self.passes_bin_dir, p)
		if os.path.isfile(full_path):
			load_args = load_args + "-load=%s " % (full_path)
		else:
			load_args += "-%s " % p
        util.runcmd("%s/opt %s %s %s %s.0.5.precodegen.bc -o %s.bcl" % (self.llvm_bin_dir, self.opt_args, load_args, self.pass_args, application.name, application.name), env=env, cwd=application.src_path)
        util.runcmd("%s %s.bcl -o %s/%s %s" % (self.cc, application.name, application.install_path, application.name, self.link_flags), env=env, cwd=application.src_path)
        return

class SimpleRuntimeCog(RuntimeCog):
    def __init__(self, config_parser):
	RuntimeCog.__init__(self, config_parser)
	self.name = config_parser.get(RuntimeCog.SECTION_NAME, "NAME")
        self.env_dict = util.getDictFromConfig(config_parser, RuntimeCog.SECTION_NAME, "ENV")
	self.args = config_parser.get(RuntimeCog.SECTION_NAME, "CMD_ARGS")

    def setup(self, application, workspace):
	return
    
    def execute(self, application):
        util.runcmd("%s/%s %s" % (application.install_path, application.name, self.args), cwd=application.install_path)

    def windup(self, application):
	return

class SimpleWorkspace(Workspace):
    def setup(self, application, experiment_id):
	if not os.path.isdir(application.install_path):
                util.runcmd("mkdir -p %s" % application.install_path)
        if self.work_dir:
                if not os.path.isdir(self.work_dir):
                        util.runcmd("mkdir -p %s" % self.work_dir)
        if self.log_dir:
                if not os.path.isdir(self.log_dir):
                        util.runcmd("mkdir -p %s" % self.log_dir)	

    def windup(self, application, experiment_id):
	return
