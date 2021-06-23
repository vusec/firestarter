from kombit.core import *
import cogs.llvmapps
import json
import os
import random
import re

backup_dir_suffix = "/backup"

class EDFIFirstStageInstrumentationCog(cogs.llvmapps.LLVMAppsInstrumentationCog):
    def instrument(self, application):
        cogs.llvmapps.LLVMAppsInstrumentationCog.instrument(self, application)
        if self.ir_file:
            backup_dir = application.install_path + backup_dir_suffix
            if not os.path.isdir(backup_dir):
                util.runcmd("mkdir -p %s" % (backup_dir))
            util.runcmd("cp %s %s" % (self.ir_file, backup_dir))

class EDFISecondStageInstrumentationCog(cogs.llvmapps.LLVMAppsInstrumentationCog):
    def pickBasicBlockForFaultInjection(self, dict_window_bbtrace, window_id):
        BBs = dict_window_bbtrace[window_id]
        assert(0 != len(BBs))
        pickedBB = BBs[random.randint(0, len(BBs)-1)] if (1 < len(BBs)) else BBs[0]
        print("picked BB-ID: %d" % (pickedBB))
        return pickedBB
    
    def getLibcallOfWindow(self, window_id):
        libcall_name = ""
        if self.ir_file:
            ll_file = re.sub('.bc$', '.ll', self.ir_file)
            ll_file = re.sub('.bcl$', '.bcl.ll', self.ir_file)
            libcall_name = util.cmdToOut("grep -B 35 'store.*%s.*rcvry_current_site_id' %s | grep 'HYBRY_LIBCALL' | tail -1 | grep -o '@.*(' | tr -d '@' | tr -d '('" % (window_id, ll_file))
        return libcall_name


    def __init__(self, config_parser):
        cogs.llvmapps.LLVMAppsInstrumentationCog.__init__(self, config_parser)
        # Load the json file, pick a random BB from the window ID specified thru ENV
        self.bbtrace_file = config_parser.get(InstrumentationCog.SECTION_NAME, "BBTRACE_FILE")
        self.dict_window_bbtrace = {}
        bbID = -1
        assert (None != os.environ['FAULT_SITE_INDEX'])
        fault_site_index = int(os.environ['FAULT_SITE_INDEX'])
        if (-1 == fault_site_index):
            assert(None != os.environ['BB_ID'])
            bbID = int(os.environ['BB_ID'])
        else:
            with open(self.bbtrace_file) as bbtrace_json:
                self.dict_window_bbtrace = json.load(bbtrace_json)
            print ("initial size of dict: %d" % (len(self.dict_window_bbtrace)))
            empty_keys = []
            for k in self.dict_window_bbtrace.keys():
                if 0 == len(self.dict_window_bbtrace[k]):
                    print ("empty key: %s" % (k))
                    empty_keys.append(k)
                elif 1 == len(self.dict_window_bbtrace[k]) and (self.dict_window_bbtrace[k][0] == 0):
                    empty_keys.append(k)

            for k in empty_keys:
                del self.dict_window_bbtrace[k]
            print ("after processing, size of dict: %d" % (len(self.dict_window_bbtrace)))

            windows = self.dict_window_bbtrace.keys()
            print ("fault site index: %d" % (fault_site_index))
            if len(windows) <= fault_site_index:
                print ("Exhausted number of windows.")
                quit()
            self.window_id = windows[fault_site_index]
            self.libcall_name = self.getLibcallOfWindow(self.window_id)
            print ("window_id selected: %s (libcall: %s)" % (self.window_id, self.libcall_name))
            bbID = self.pickBasicBlockForFaultInjection(self.dict_window_bbtrace, self.window_id)
        self.llvm_second_stage_pass_args += str(bbID)

    def instrument(self, application):
        # First, copy over the original bcl file from backup_dir
        backup_dir = application.install_path + backup_dir_suffix
        if os.path.isdir(backup_dir):
            util.runcmd("cp %s/*.bcl %s" % (backup_dir, os.path.dirname(self.ir_file)))
        env = os.environ
        env.update(self.env_dict)
        env.update({"OPT_ARGS":"%s" % (self.opt_args)})
        env.update({"LLVM_SECOND_STAGE_PASSES":"%s" % (self.llvm_second_stage_passes)})
		# print ("LLVM_SECOND_STAGE_PASSES: %s" % (self.llvm_second_stage_passes))
        env.update({"LLVM_SECOND_STAGE_PASS_ARGS":"%s" % (self.llvm_second_stage_pass_args)})
        # print ("LLVM_SECOND_STAGE_PASS_ARGS: %s" % (self.llvm_second_stage_pass_args))
        env.update({"LLVM_SECOND_STAGE_PASSES_ONLY":"1"})
        print ("pwd : %s" %(application.src_path))
        util.runcmd("./build.llvm", env=env, cwd=application.src_path)
