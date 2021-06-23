# Derived classes to use for general llvm-apps applications

from kombit.core import *
import kombit.util
import re

class LLVMAppsCompileCog(CompileCog):
    def __init__(self, config_parser):
        self.name = config_parser.get(CompileCog.SECTION_NAME, "NAME")
        self.env_dict = kombit.util.getDictFromConfig(config_parser, CompileCog.SECTION_NAME, "ENV")
        CompileCog.__init__(self, config_parser)
        return

    def configure(self, application):
        kombit.util.runcmd("./configure.llvm", cwd=application.src_path)
        return

    def kompile(self, application, cleanSilent=False):
        env = os.environ
        env.update(self.env_dict)
        kombit.util.runcmd("make clean", cwd=application.src_path, silent=cleanSilent)
        kombit.util.runcmd("./relink.llvm %s" % (self.static_libs), cwd=application.src_path)
        return

    def cleanup(self, application):
        kombit.util.runcmd("make clean", cwd=application.src_path)
        return

class LLVMAppsInstrumentationCog(InstrumentationCog):
    def __init__(self, config_parser):
        InstrumentationCog.__init__(self, config_parser)
        self.name = config_parser.get(InstrumentationCog.SECTION_NAME, "NAME")
        self.env_dict = kombit.util.getDictFromConfig(config_parser, InstrumentationCog.SECTION_NAME, "ENV")
        try:
            self.pass_args_replace = config_parser.get(InstrumentationCog.SECTION_NAME, "LLVM_PASS_ARGS_REPLACE")
        except ConfigParser.NoOptionError:
            self.pass_args_replace = None
            pass
        try:
            self.llvm_second_stage_passes = config_parser.get(InstrumentationCog.SECTION_NAME, "LLVM_SECOND_STAGE_PASSES")
            self.llvm_second_stage_pass_args = config_parser.get(InstrumentationCog.SECTION_NAME, "LLVM_SECOND_STAGE_PASS_ARGS")
        except ConfigParser.NoOptionError:
            self.llvm_second_stage_passes = None
            self.llvm_second_stage_pass_args = None
            pass
        try:
            self.di_pass_args = config_parser.get(InstrumentationCog.SECTION_NAME, "DI_PASS_ARGS")
        except ConfigParser.NoOptionError:
            self.di_pass_args = None
            pass
        try:
            self.ir_file = config_parser.get(InstrumentationCog.SECTION_NAME, "IR_FILE")
        except ConfigParser.NoOptionError:
            self.ir_file = None
            pass
        try:
            self.ir_verify = config_parser.get(InstrumentationCog.SECTION_NAME, "IR_VERIFY")
        except ConfigParser.NoOptionError:
            self.ir_verify = None
            pass
        return

    def instrument(self, application):
        env = os.environ
        env.update(self.env_dict)
        env.update({"OPT_ARGS":"%s" % (self.opt_args)})
        env.update({"LLVM_PASS_ARGS":"%s" % (self.pass_args)})
        if None != self.pass_args_replace:
            env.update({"LLVM_PASS_ARGS_REPLACE":"%s" % (self.pass_args_replace)})
            print ("replacer: " + self.pass_args_replace)
        if None != self.llvm_second_stage_passes:
            env.update({"LLVM_SECOND_STAGE_PASSES":"%s" % (self.llvm_second_stage_passes)})
            print ("LLVM_SECOND_STAGE_PASSES: " + self.llvm_second_stage_passes)
        if None != self.llvm_second_stage_pass_args:
            env.update({"LLVM_SECOND_STAGE_PASS_ARGS":"%s" % (self.llvm_second_stage_pass_args)})
            print ("LLVM_SECOND_STAGE_PASS_ARGS: " + self.llvm_second_stage_pass_args)
        if None != self.di_pass_args:
            env.update({"DI_PASS_ARGS":"%s" % (self.di_pass_args)})
            kombit.util.runcmd("./build.llvm %s" % (self.opt_passes), env=env, cwd=application.src_path)
        assert True == self.verify(application), "IR based verification of instrumentation failed."
        return

    def verify(self, application):
        ''' Verifies existence of specified comma-separated strings in LLVM IR of the app. '''
        if not (self.ir_verify and self.ir_file):
            return True # No need to verify, all is well

        verif_strings = self.ir_verify.split(",")
        kombit.util.llvmDis(application, use_obj_path=True)
        ll_file = re.sub('.bc$', '.ll', self.ir_file)
        ll_file = re.sub('.bcl$', '.bcl.ll', self.ir_file)
        # ll_path = application.obj_path + '/' + ll_file
        ll_path = ll_file
        ll_file = os.path.basename(ll_path)
        print ("ll_path: %s" % (ll_path))
        if not os.path.isfile(ll_path):
            print ("ll_path: %s NOT FOUND" % ll_path)
            ll_path = application.install_path + '/' + ll_file
        assert True == os.path.isfile(ll_path), "IR .ll file not found"
        for v in verif_strings:
            print ("Looking for '%s' in file:%s" % (v, ll_path))
            ret, output = kombit.util.grepInFile(ll_path, v)
            if (ret != 0):
                print ("IR verify: Not found: '%s' in %s" % (v, ll_path))
                return False

        return True
