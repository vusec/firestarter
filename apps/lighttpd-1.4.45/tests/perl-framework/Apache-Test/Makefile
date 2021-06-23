# This Makefile is for the Apache::Test extension to perl.
#
# It was generated automatically by MakeMaker version
# 7.0401 (Revision: 70401) from the contents of
# Makefile.PL. Don't edit this file, edit Makefile.PL instead.
#
#       ANY CHANGES MADE HERE WILL BE LOST!
#
#   MakeMaker ARGV: ()
#

#   MakeMaker Parameters:

#     BUILD_REQUIRES => {  }
#     CONFIGURE_REQUIRES => {  }
#     NAME => q[Apache::Test]
#     NO_META => q[0]
#     PREREQ_PM => { Cwd=>q[2.06], File::Spec=>q[0.8] }
#     TEST_REQUIRES => {  }
#     VERSION => q[1.43-dev]
#     clean => { FILES=>q[.mypacklist t/TEST t/REPORT Makefile.old] }
#     dist => { COMPRESS=>q[gzip -9f], PREOP=>q[find $(DISTVNAME) -type d -print|xargs chmod 0755 && find $(DISTVNAME) -type f -print|xargs chmod 0644], SUFFIX=>q[gz], TO_UNIX=>q[find $(DISTVNAME) -type f -print|xargs dos2unix] }

# --- MakeMaker post_initialize section:


# --- MakeMaker const_config section:

# These definitions are from config.sh (via /usr/lib/x86_64-linux-gnu/perl/5.22/Config.pm).
# They may have been overridden via Makefile.PL or on the command line.
AR = ar
CC = x86_64-linux-gnu-gcc
CCCDLFLAGS = -fPIC
CCDLFLAGS = -Wl,-E
DLEXT = so
DLSRC = dl_dlopen.xs
EXE_EXT = 
FULL_AR = /usr/bin/ar
LD = x86_64-linux-gnu-gcc
LDDLFLAGS = -shared -L/usr/local/lib -fstack-protector-strong
LDFLAGS =  -fstack-protector-strong -L/usr/local/lib
LIBC = libc-2.23.so
LIB_EXT = .a
OBJ_EXT = .o
OSNAME = linux
OSVERS = 3.16.0
RANLIB = :
SITELIBEXP = /usr/local/share/perl/5.22.1
SITEARCHEXP = /usr/local/lib/x86_64-linux-gnu/perl/5.22.1
SO = so
VENDORARCHEXP = /usr/lib/x86_64-linux-gnu/perl5/5.22
VENDORLIBEXP = /usr/share/perl5


# --- MakeMaker constants section:
AR_STATIC_ARGS = cr
DIRFILESEP = /
DFSEP = $(DIRFILESEP)
NAME = Apache::Test
NAME_SYM = Apache_Test
VERSION = 1.43-dev
VERSION_MACRO = VERSION
VERSION_SYM = 1_43_dev
DEFINE_VERSION = -D$(VERSION_MACRO)=\"$(VERSION)\"
XS_VERSION = 1.43-dev
XS_VERSION_MACRO = XS_VERSION
XS_DEFINE_VERSION = -D$(XS_VERSION_MACRO)=\"$(XS_VERSION)\"
INST_ARCHLIB = ../blib/arch
INST_SCRIPT = ../blib/script
INST_BIN = ../blib/bin
INST_LIB = ../blib/lib
INST_MAN1DIR = ../blib/man1
INST_MAN3DIR = ../blib/man3
MAN1EXT = 1p
MAN3EXT = 3pm
INSTALLDIRS = site
DESTDIR = 
PREFIX = $(SITEPREFIX)
PERLPREFIX = /usr
SITEPREFIX = /usr/local
VENDORPREFIX = /usr
INSTALLPRIVLIB = /usr/share/perl/5.22
DESTINSTALLPRIVLIB = $(DESTDIR)$(INSTALLPRIVLIB)
INSTALLSITELIB = /usr/local/share/perl/5.22.1
DESTINSTALLSITELIB = $(DESTDIR)$(INSTALLSITELIB)
INSTALLVENDORLIB = /usr/share/perl5
DESTINSTALLVENDORLIB = $(DESTDIR)$(INSTALLVENDORLIB)
INSTALLARCHLIB = /usr/lib/x86_64-linux-gnu/perl/5.22
DESTINSTALLARCHLIB = $(DESTDIR)$(INSTALLARCHLIB)
INSTALLSITEARCH = /usr/local/lib/x86_64-linux-gnu/perl/5.22.1
DESTINSTALLSITEARCH = $(DESTDIR)$(INSTALLSITEARCH)
INSTALLVENDORARCH = /usr/lib/x86_64-linux-gnu/perl5/5.22
DESTINSTALLVENDORARCH = $(DESTDIR)$(INSTALLVENDORARCH)
INSTALLBIN = /usr/bin
DESTINSTALLBIN = $(DESTDIR)$(INSTALLBIN)
INSTALLSITEBIN = /usr/local/bin
DESTINSTALLSITEBIN = $(DESTDIR)$(INSTALLSITEBIN)
INSTALLVENDORBIN = /usr/bin
DESTINSTALLVENDORBIN = $(DESTDIR)$(INSTALLVENDORBIN)
INSTALLSCRIPT = /usr/bin
DESTINSTALLSCRIPT = $(DESTDIR)$(INSTALLSCRIPT)
INSTALLSITESCRIPT = /usr/local/bin
DESTINSTALLSITESCRIPT = $(DESTDIR)$(INSTALLSITESCRIPT)
INSTALLVENDORSCRIPT = /usr/bin
DESTINSTALLVENDORSCRIPT = $(DESTDIR)$(INSTALLVENDORSCRIPT)
INSTALLMAN1DIR = /usr/share/man/man1
DESTINSTALLMAN1DIR = $(DESTDIR)$(INSTALLMAN1DIR)
INSTALLSITEMAN1DIR = /usr/local/man/man1
DESTINSTALLSITEMAN1DIR = $(DESTDIR)$(INSTALLSITEMAN1DIR)
INSTALLVENDORMAN1DIR = /usr/share/man/man1
DESTINSTALLVENDORMAN1DIR = $(DESTDIR)$(INSTALLVENDORMAN1DIR)
INSTALLMAN3DIR = /usr/share/man/man3
DESTINSTALLMAN3DIR = $(DESTDIR)$(INSTALLMAN3DIR)
INSTALLSITEMAN3DIR = /usr/local/man/man3
DESTINSTALLSITEMAN3DIR = $(DESTDIR)$(INSTALLSITEMAN3DIR)
INSTALLVENDORMAN3DIR = /usr/share/man/man3
DESTINSTALLVENDORMAN3DIR = $(DESTDIR)$(INSTALLVENDORMAN3DIR)
PERL_LIB = /usr/share/perl/5.22
PERL_ARCHLIB = /usr/lib/x86_64-linux-gnu/perl/5.22
PERL_ARCHLIBDEP = /usr/lib/x86_64-linux-gnu/perl/5.22
LIBPERL_A = libperl.a
FIRST_MAKEFILE = Makefile
MAKEFILE_OLD = Makefile.old
MAKE_APERL_FILE = Makefile.aperl
PERLMAINCC = $(CC)
PERL_INC = /usr/lib/x86_64-linux-gnu/perl/5.22/CORE
PERL_INCDEP = /usr/lib/x86_64-linux-gnu/perl/5.22/CORE
PERL = "/usr/bin/perl"
FULLPERL = "/usr/bin/perl"
ABSPERL = $(PERL)
PERLRUN = $(PERL)
FULLPERLRUN = $(FULLPERL)
ABSPERLRUN = $(ABSPERL)
PERLRUNINST = $(PERLRUN) "-I$(INST_ARCHLIB)" "-I$(INST_LIB)"
FULLPERLRUNINST = $(FULLPERLRUN) "-I$(INST_ARCHLIB)" "-I$(INST_LIB)"
ABSPERLRUNINST = $(ABSPERLRUN) "-I$(INST_ARCHLIB)" "-I$(INST_LIB)"
PERL_CORE = 0
PERM_DIR = 755
PERM_RW = 644
PERM_RWX = 755

MAKEMAKER   = /usr/share/perl/5.22/ExtUtils/MakeMaker.pm
MM_VERSION  = 7.0401
MM_REVISION = 70401

# FULLEXT = Pathname for extension directory (eg Foo/Bar/Oracle).
# BASEEXT = Basename part of FULLEXT. May be just equal FULLEXT. (eg Oracle)
# PARENT_NAME = NAME without BASEEXT and no trailing :: (eg Foo::Bar)
# DLBASE  = Basename part of dynamic library. May be just equal BASEEXT.
MAKE = make
FULLEXT = Apache/Test
BASEEXT = Test
PARENT_NAME = Apache
DLBASE = $(BASEEXT)
VERSION_FROM = 
OBJECT = 
LDFROM = $(OBJECT)
LINKTYPE = dynamic
BOOTDEP = 

# Handy lists of source code files:
XS_FILES = 
C_FILES  = 
O_FILES  = 
H_FILES  = 
MAN1PODS = 
MAN3PODS = lib/Apache/Test.pm \
	lib/Apache/TestConfig.pm \
	lib/Apache/TestHandler.pm \
	lib/Apache/TestMB.pm \
	lib/Apache/TestMM.pm \
	lib/Apache/TestReport.pm \
	lib/Apache/TestRequest.pm \
	lib/Apache/TestRun.pm \
	lib/Apache/TestRunPHP.pm \
	lib/Apache/TestRunPerl.pm \
	lib/Apache/TestServer.pm \
	lib/Apache/TestSmoke.pm \
	lib/Apache/TestTrace.pm \
	lib/Apache/TestUtil.pm \
	lib/Bundle/ApacheTest.pm

# Where is the Config information that we are using/depend on
CONFIGDEP = $(PERL_ARCHLIBDEP)$(DFSEP)Config.pm $(PERL_INCDEP)$(DFSEP)config.h

# Where to build things
INST_LIBDIR      = $(INST_LIB)/Apache
INST_ARCHLIBDIR  = $(INST_ARCHLIB)/Apache

INST_AUTODIR     = $(INST_LIB)/auto/$(FULLEXT)
INST_ARCHAUTODIR = $(INST_ARCHLIB)/auto/$(FULLEXT)

INST_STATIC      = 
INST_DYNAMIC     = 
INST_BOOT        = 

# Extra linker info
EXPORT_LIST        = 
PERL_ARCHIVE       = 
PERL_ARCHIVEDEP    = 
PERL_ARCHIVE_AFTER = 


TO_INST_PM = lib/Apache/Test.pm \
	lib/Apache/Test5005compat.pm \
	lib/Apache/TestBuild.pm \
	lib/Apache/TestClient.pm \
	lib/Apache/TestCommon.pm \
	lib/Apache/TestCommonPost.pm \
	lib/Apache/TestConfig.pm \
	lib/Apache/TestConfigC.pm \
	lib/Apache/TestConfigPHP.pm \
	lib/Apache/TestConfigParrot.pm \
	lib/Apache/TestConfigParse.pm \
	lib/Apache/TestConfigPerl.pm \
	lib/Apache/TestHandler.pm \
	lib/Apache/TestHarness.pm \
	lib/Apache/TestHarnessPHP.pm \
	lib/Apache/TestMB.pm \
	lib/Apache/TestMM.pm \
	lib/Apache/TestPerlDB.pm \
	lib/Apache/TestReport.pm \
	lib/Apache/TestReportPerl.pm \
	lib/Apache/TestRequest.pm \
	lib/Apache/TestRun.pm \
	lib/Apache/TestRunPHP.pm \
	lib/Apache/TestRunParrot.pm \
	lib/Apache/TestRunPerl.pm \
	lib/Apache/TestSSLCA.pm \
	lib/Apache/TestServer.pm \
	lib/Apache/TestSmoke.pm \
	lib/Apache/TestSmokePerl.pm \
	lib/Apache/TestSort.pm \
	lib/Apache/TestTrace.pm \
	lib/Apache/TestUtil.pm \
	lib/Bundle/ApacheTest.pm

PM_TO_BLIB = lib/Apache/Test.pm \
	../blib/lib/Apache/Test.pm \
	lib/Apache/Test5005compat.pm \
	../blib/lib/Apache/Test5005compat.pm \
	lib/Apache/TestBuild.pm \
	../blib/lib/Apache/TestBuild.pm \
	lib/Apache/TestClient.pm \
	../blib/lib/Apache/TestClient.pm \
	lib/Apache/TestCommon.pm \
	../blib/lib/Apache/TestCommon.pm \
	lib/Apache/TestCommonPost.pm \
	../blib/lib/Apache/TestCommonPost.pm \
	lib/Apache/TestConfig.pm \
	../blib/lib/Apache/TestConfig.pm \
	lib/Apache/TestConfigC.pm \
	../blib/lib/Apache/TestConfigC.pm \
	lib/Apache/TestConfigPHP.pm \
	../blib/lib/Apache/TestConfigPHP.pm \
	lib/Apache/TestConfigParrot.pm \
	../blib/lib/Apache/TestConfigParrot.pm \
	lib/Apache/TestConfigParse.pm \
	../blib/lib/Apache/TestConfigParse.pm \
	lib/Apache/TestConfigPerl.pm \
	../blib/lib/Apache/TestConfigPerl.pm \
	lib/Apache/TestHandler.pm \
	../blib/lib/Apache/TestHandler.pm \
	lib/Apache/TestHarness.pm \
	../blib/lib/Apache/TestHarness.pm \
	lib/Apache/TestHarnessPHP.pm \
	../blib/lib/Apache/TestHarnessPHP.pm \
	lib/Apache/TestMB.pm \
	../blib/lib/Apache/TestMB.pm \
	lib/Apache/TestMM.pm \
	../blib/lib/Apache/TestMM.pm \
	lib/Apache/TestPerlDB.pm \
	../blib/lib/Apache/TestPerlDB.pm \
	lib/Apache/TestReport.pm \
	../blib/lib/Apache/TestReport.pm \
	lib/Apache/TestReportPerl.pm \
	../blib/lib/Apache/TestReportPerl.pm \
	lib/Apache/TestRequest.pm \
	../blib/lib/Apache/TestRequest.pm \
	lib/Apache/TestRun.pm \
	../blib/lib/Apache/TestRun.pm \
	lib/Apache/TestRunPHP.pm \
	../blib/lib/Apache/TestRunPHP.pm \
	lib/Apache/TestRunParrot.pm \
	../blib/lib/Apache/TestRunParrot.pm \
	lib/Apache/TestRunPerl.pm \
	../blib/lib/Apache/TestRunPerl.pm \
	lib/Apache/TestSSLCA.pm \
	../blib/lib/Apache/TestSSLCA.pm \
	lib/Apache/TestServer.pm \
	../blib/lib/Apache/TestServer.pm \
	lib/Apache/TestSmoke.pm \
	../blib/lib/Apache/TestSmoke.pm \
	lib/Apache/TestSmokePerl.pm \
	../blib/lib/Apache/TestSmokePerl.pm \
	lib/Apache/TestSort.pm \
	../blib/lib/Apache/TestSort.pm \
	lib/Apache/TestTrace.pm \
	../blib/lib/Apache/TestTrace.pm \
	lib/Apache/TestUtil.pm \
	../blib/lib/Apache/TestUtil.pm \
	lib/Bundle/ApacheTest.pm \
	../blib/lib/Bundle/ApacheTest.pm

# install into the same location as mod_perl 2.0
INSTALLSITELIB = $(INSTALLSITEARCH)
DESTINSTALLSITELIB = $(DESTINSTALLSITEARCH)


# --- MakeMaker platform_constants section:
MM_Unix_VERSION = 7.0401
PERL_MALLOC_DEF = -DPERL_EXTMALLOC_DEF -Dmalloc=Perl_malloc -Dfree=Perl_mfree -Drealloc=Perl_realloc -Dcalloc=Perl_calloc


# --- MakeMaker tool_autosplit section:
# Usage: $(AUTOSPLITFILE) FileToSplit AutoDirToSplitInto
AUTOSPLITFILE = $(ABSPERLRUN)  -e 'use AutoSplit;  autosplit($$$$ARGV[0], $$$$ARGV[1], 0, 1, 1)' --



# --- MakeMaker tool_xsubpp section:


# --- MakeMaker tools_other section:
SHELL = /bin/sh
CHMOD = chmod
CP = cp
MV = mv
NOOP = $(TRUE)
NOECHO = @
RM_F = rm -f
RM_RF = rm -rf
TEST_F = test -f
TOUCH = touch
UMASK_NULL = umask 0
DEV_NULL = > /dev/null 2>&1
MKPATH = $(ABSPERLRUN) -MExtUtils::Command -e 'mkpath' --
EQUALIZE_TIMESTAMP = $(ABSPERLRUN) -MExtUtils::Command -e 'eqtime' --
FALSE = false
TRUE = true
ECHO = echo
ECHO_N = echo -n
UNINST = 0
VERBINST = 0
MOD_INSTALL = $(ABSPERLRUN) -MExtUtils::Install -e 'install([ from_to => {@ARGV}, verbose => '\''$(VERBINST)'\'', uninstall_shadows => '\''$(UNINST)'\'', dir_mode => '\''$(PERM_DIR)'\'' ]);' --
DOC_INSTALL = $(ABSPERLRUN) -MExtUtils::Command::MM -e 'perllocal_install' --
UNINSTALL = $(ABSPERLRUN) -MExtUtils::Command::MM -e 'uninstall' --
WARN_IF_OLD_PACKLIST = $(ABSPERLRUN) -MExtUtils::Command::MM -e 'warn_if_old_packlist' --
MACROSTART = 
MACROEND = 
USEMAKEFILE = -f
FIXIN = $(ABSPERLRUN) -MExtUtils::MY -e 'MY->fixin(shift)' --
CP_NONEMPTY = $(ABSPERLRUN) -MExtUtils::Command::MM -e 'cp_nonempty' --


# --- MakeMaker makemakerdflt section:
makemakerdflt : all
	$(NOECHO) $(NOOP)


# --- MakeMaker dist section skipped.

# --- MakeMaker macro section:


# --- MakeMaker depend section:


# --- MakeMaker cflags section:


# --- MakeMaker const_loadlibs section:


# --- MakeMaker const_cccmd section:


# --- MakeMaker post_constants section:


# --- MakeMaker pasthru section:

PASTHRU = LIBPERL_A="$(LIBPERL_A)"\
	LINKTYPE="$(LINKTYPE)"\
	LD="$(LD)"\
	PREFIX="$(PREFIX)"


# --- MakeMaker special_targets section:
.SUFFIXES : .xs .c .C .cpp .i .s .cxx .cc $(OBJ_EXT)

.PHONY: all config static dynamic test linkext manifest blibdirs clean realclean disttest distdir



# --- MakeMaker c_o section:


# --- MakeMaker xs_c section:


# --- MakeMaker xs_o section:


# --- MakeMaker top_targets section:
all :: pure_all manifypods
	$(NOECHO) $(NOOP)


pure_all :: config pm_to_blib subdirs linkext
	$(NOECHO) $(NOOP)

subdirs :: $(MYEXTLIB)
	$(NOECHO) $(NOOP)

config :: $(FIRST_MAKEFILE) blibdirs
	$(NOECHO) $(NOOP)

help :
	perldoc ExtUtils::MakeMaker


# --- MakeMaker blibdirs section:
blibdirs : $(INST_LIBDIR)$(DFSEP).exists $(INST_ARCHLIB)$(DFSEP).exists $(INST_AUTODIR)$(DFSEP).exists $(INST_ARCHAUTODIR)$(DFSEP).exists $(INST_BIN)$(DFSEP).exists $(INST_SCRIPT)$(DFSEP).exists $(INST_MAN1DIR)$(DFSEP).exists $(INST_MAN3DIR)$(DFSEP).exists
	$(NOECHO) $(NOOP)

# Backwards compat with 6.18 through 6.25
blibdirs.ts : blibdirs
	$(NOECHO) $(NOOP)

$(INST_LIBDIR)$(DFSEP).exists :: Makefile.PL
	$(NOECHO) $(MKPATH) $(INST_LIBDIR)
	$(NOECHO) $(CHMOD) $(PERM_DIR) $(INST_LIBDIR)
	$(NOECHO) $(TOUCH) $(INST_LIBDIR)$(DFSEP).exists

$(INST_ARCHLIB)$(DFSEP).exists :: Makefile.PL
	$(NOECHO) $(MKPATH) $(INST_ARCHLIB)
	$(NOECHO) $(CHMOD) $(PERM_DIR) $(INST_ARCHLIB)
	$(NOECHO) $(TOUCH) $(INST_ARCHLIB)$(DFSEP).exists

$(INST_AUTODIR)$(DFSEP).exists :: Makefile.PL
	$(NOECHO) $(MKPATH) $(INST_AUTODIR)
	$(NOECHO) $(CHMOD) $(PERM_DIR) $(INST_AUTODIR)
	$(NOECHO) $(TOUCH) $(INST_AUTODIR)$(DFSEP).exists

$(INST_ARCHAUTODIR)$(DFSEP).exists :: Makefile.PL
	$(NOECHO) $(MKPATH) $(INST_ARCHAUTODIR)
	$(NOECHO) $(CHMOD) $(PERM_DIR) $(INST_ARCHAUTODIR)
	$(NOECHO) $(TOUCH) $(INST_ARCHAUTODIR)$(DFSEP).exists

$(INST_BIN)$(DFSEP).exists :: Makefile.PL
	$(NOECHO) $(MKPATH) $(INST_BIN)
	$(NOECHO) $(CHMOD) $(PERM_DIR) $(INST_BIN)
	$(NOECHO) $(TOUCH) $(INST_BIN)$(DFSEP).exists

$(INST_SCRIPT)$(DFSEP).exists :: Makefile.PL
	$(NOECHO) $(MKPATH) $(INST_SCRIPT)
	$(NOECHO) $(CHMOD) $(PERM_DIR) $(INST_SCRIPT)
	$(NOECHO) $(TOUCH) $(INST_SCRIPT)$(DFSEP).exists

$(INST_MAN1DIR)$(DFSEP).exists :: Makefile.PL
	$(NOECHO) $(MKPATH) $(INST_MAN1DIR)
	$(NOECHO) $(CHMOD) $(PERM_DIR) $(INST_MAN1DIR)
	$(NOECHO) $(TOUCH) $(INST_MAN1DIR)$(DFSEP).exists

$(INST_MAN3DIR)$(DFSEP).exists :: Makefile.PL
	$(NOECHO) $(MKPATH) $(INST_MAN3DIR)
	$(NOECHO) $(CHMOD) $(PERM_DIR) $(INST_MAN3DIR)
	$(NOECHO) $(TOUCH) $(INST_MAN3DIR)$(DFSEP).exists



# --- MakeMaker linkext section:

linkext :: $(LINKTYPE)
	$(NOECHO) $(NOOP)


# --- MakeMaker dlsyms section:


# --- MakeMaker dynamic_bs section:

BOOTSTRAP =


# --- MakeMaker dynamic section:

dynamic :: $(FIRST_MAKEFILE) $(BOOTSTRAP) $(INST_DYNAMIC)
	$(NOECHO) $(NOOP)


# --- MakeMaker dynamic_lib section:


# --- MakeMaker static section:

## $(INST_PM) has been moved to the all: target.
## It remains here for awhile to allow for old usage: "make static"
static :: $(FIRST_MAKEFILE) $(INST_STATIC)
	$(NOECHO) $(NOOP)


# --- MakeMaker static_lib section:


# --- MakeMaker manifypods section:

POD2MAN_EXE = $(PERLRUN) "-MExtUtils::Command::MM" -e pod2man "--"
POD2MAN = $(POD2MAN_EXE)


manifypods : pure_all  \
	lib/Apache/Test.pm \
	lib/Apache/TestConfig.pm \
	lib/Apache/TestHandler.pm \
	lib/Apache/TestMB.pm \
	lib/Apache/TestMM.pm \
	lib/Apache/TestReport.pm \
	lib/Apache/TestRequest.pm \
	lib/Apache/TestRun.pm \
	lib/Apache/TestRunPHP.pm \
	lib/Apache/TestRunPerl.pm \
	lib/Apache/TestServer.pm \
	lib/Apache/TestSmoke.pm \
	lib/Apache/TestTrace.pm \
	lib/Apache/TestUtil.pm \
	lib/Bundle/ApacheTest.pm
	$(NOECHO) $(POD2MAN) --section=$(MAN3EXT) --perm_rw=$(PERM_RW) -u \
	  lib/Apache/Test.pm $(INST_MAN3DIR)/Apache::Test.$(MAN3EXT) \
	  lib/Apache/TestConfig.pm $(INST_MAN3DIR)/Apache::TestConfig.$(MAN3EXT) \
	  lib/Apache/TestHandler.pm $(INST_MAN3DIR)/Apache::TestHandler.$(MAN3EXT) \
	  lib/Apache/TestMB.pm $(INST_MAN3DIR)/Apache::TestMB.$(MAN3EXT) \
	  lib/Apache/TestMM.pm $(INST_MAN3DIR)/Apache::TestMM.$(MAN3EXT) \
	  lib/Apache/TestReport.pm $(INST_MAN3DIR)/Apache::TestReport.$(MAN3EXT) \
	  lib/Apache/TestRequest.pm $(INST_MAN3DIR)/Apache::TestRequest.$(MAN3EXT) \
	  lib/Apache/TestRun.pm $(INST_MAN3DIR)/Apache::TestRun.$(MAN3EXT) \
	  lib/Apache/TestRunPHP.pm $(INST_MAN3DIR)/Apache::TestRunPHP.$(MAN3EXT) \
	  lib/Apache/TestRunPerl.pm $(INST_MAN3DIR)/Apache::TestRunPerl.$(MAN3EXT) \
	  lib/Apache/TestServer.pm $(INST_MAN3DIR)/Apache::TestServer.$(MAN3EXT) \
	  lib/Apache/TestSmoke.pm $(INST_MAN3DIR)/Apache::TestSmoke.$(MAN3EXT) \
	  lib/Apache/TestTrace.pm $(INST_MAN3DIR)/Apache::TestTrace.$(MAN3EXT) \
	  lib/Apache/TestUtil.pm $(INST_MAN3DIR)/Apache::TestUtil.$(MAN3EXT) \
	  lib/Bundle/ApacheTest.pm $(INST_MAN3DIR)/Bundle::ApacheTest.$(MAN3EXT) 




# --- MakeMaker processPL section:


# --- MakeMaker installbin section:


# --- MakeMaker subdirs section:

# The default clean, realclean and test targets in this Makefile
# have automatically been given entries for each subdir.


subdirs ::
	$(NOECHO) cd Apache-TestItSelf && $(MAKE) $(USEMAKEFILE) $(FIRST_MAKEFILE) all $(PASTHRU)


# --- MakeMaker clean_subdirs section:
clean_subdirs :
	$(ABSPERLRUN)  -e 'exit 0 unless chdir '\''Apache-TestItSelf'\'';  system '\''$(MAKE) clean'\'' if -f '\''$(FIRST_MAKEFILE)'\'';' --


# --- MakeMaker clean section:

# Delete temporary files but do not touch installed files. We don't delete
# the Makefile here so a later make realclean still has a makefile to use.

clean :: test_clean  clean_subdirs
	- $(RM_F) \
	  $(BASEEXT).bso $(BASEEXT).def \
	  $(BASEEXT).exp $(BASEEXT).x \
	  $(BOOTSTRAP) $(INST_ARCHAUTODIR)/extralibs.all \
	  $(INST_ARCHAUTODIR)/extralibs.ld $(MAKE_APERL_FILE) \
	  *$(LIB_EXT) *$(OBJ_EXT) \
	  *perl.core MYMETA.json \
	  MYMETA.yml blibdirs.ts \
	  core core.*perl.*.? \
	  core.[0-9] core.[0-9][0-9] \
	  core.[0-9][0-9][0-9] core.[0-9][0-9][0-9][0-9] \
	  core.[0-9][0-9][0-9][0-9][0-9] lib$(BASEEXT).def \
	  mon.out perl \
	  perl$(EXE_EXT) perl.exe \
	  perlmain.c pm_to_blib \
	  pm_to_blib.ts so_locations \
	  tmon.out 
	- $(RM_RF) \
	  .mypacklist Makefile.old \
	  blib t/REPORT \
	  t/TEST 
	  $(NOECHO) $(RM_F) $(MAKEFILE_OLD)
	- $(MV) $(FIRST_MAKEFILE) $(MAKEFILE_OLD) $(DEV_NULL)


# --- MakeMaker realclean_subdirs section:
realclean_subdirs :
	- $(ABSPERLRUN)  -e 'chdir '\''Apache-TestItSelf'\'';  system '\''$(MAKE) $(USEMAKEFILE) $(MAKEFILE_OLD) realclean'\'' if -f '\''$(MAKEFILE_OLD)'\'';' --
	- $(ABSPERLRUN)  -e 'chdir '\''Apache-TestItSelf'\'';  system '\''$(MAKE) $(USEMAKEFILE) $(FIRST_MAKEFILE) realclean'\'' if -f '\''$(FIRST_MAKEFILE)'\'';' --


# --- MakeMaker realclean section:
# Delete temporary files (via clean) and also delete dist files
realclean purge ::  clean realclean_subdirs
	- $(RM_F) \
	  $(MAKEFILE_OLD) $(FIRST_MAKEFILE) 
	- $(RM_RF) \
	  $(DISTVNAME) 


# --- MakeMaker metafile section:
metafile : create_distdir
	$(NOECHO) $(ECHO) Generating META.yml
	$(NOECHO) $(ECHO) '---' > META_new.yml
	$(NOECHO) $(ECHO) 'abstract: unknown' >> META_new.yml
	$(NOECHO) $(ECHO) 'author:' >> META_new.yml
	$(NOECHO) $(ECHO) '  - unknown' >> META_new.yml
	$(NOECHO) $(ECHO) 'build_requires:' >> META_new.yml
	$(NOECHO) $(ECHO) '  ExtUtils::MakeMaker: '\''0'\''' >> META_new.yml
	$(NOECHO) $(ECHO) 'configure_requires:' >> META_new.yml
	$(NOECHO) $(ECHO) '  ExtUtils::MakeMaker: '\''0'\''' >> META_new.yml
	$(NOECHO) $(ECHO) 'dynamic_config: 1' >> META_new.yml
	$(NOECHO) $(ECHO) 'generated_by: '\''ExtUtils::MakeMaker version 7.0401, CPAN::Meta::Converter version 2.150001'\''' >> META_new.yml
	$(NOECHO) $(ECHO) 'license: unknown' >> META_new.yml
	$(NOECHO) $(ECHO) 'meta-spec:' >> META_new.yml
	$(NOECHO) $(ECHO) '  url: http://module-build.sourceforge.net/META-spec-v1.4.html' >> META_new.yml
	$(NOECHO) $(ECHO) '  version: '\''1.4'\''' >> META_new.yml
	$(NOECHO) $(ECHO) 'name: Apache-Test' >> META_new.yml
	$(NOECHO) $(ECHO) 'no_index:' >> META_new.yml
	$(NOECHO) $(ECHO) '  directory:' >> META_new.yml
	$(NOECHO) $(ECHO) '    - t' >> META_new.yml
	$(NOECHO) $(ECHO) '    - inc' >> META_new.yml
	$(NOECHO) $(ECHO) 'requires:' >> META_new.yml
	$(NOECHO) $(ECHO) '  Cwd: '\''2.06'\''' >> META_new.yml
	$(NOECHO) $(ECHO) '  File::Spec: '\''0.8'\''' >> META_new.yml
	$(NOECHO) $(ECHO) 'version: 1.43-dev' >> META_new.yml
	-$(NOECHO) $(MV) META_new.yml $(DISTVNAME)/META.yml
	$(NOECHO) $(ECHO) Generating META.json
	$(NOECHO) $(ECHO) '{' > META_new.json
	$(NOECHO) $(ECHO) '   "abstract" : "unknown",' >> META_new.json
	$(NOECHO) $(ECHO) '   "author" : [' >> META_new.json
	$(NOECHO) $(ECHO) '      "unknown"' >> META_new.json
	$(NOECHO) $(ECHO) '   ],' >> META_new.json
	$(NOECHO) $(ECHO) '   "dynamic_config" : 1,' >> META_new.json
	$(NOECHO) $(ECHO) '   "generated_by" : "ExtUtils::MakeMaker version 7.0401, CPAN::Meta::Converter version 2.150001",' >> META_new.json
	$(NOECHO) $(ECHO) '   "license" : [' >> META_new.json
	$(NOECHO) $(ECHO) '      "unknown"' >> META_new.json
	$(NOECHO) $(ECHO) '   ],' >> META_new.json
	$(NOECHO) $(ECHO) '   "meta-spec" : {' >> META_new.json
	$(NOECHO) $(ECHO) '      "url" : "http://search.cpan.org/perldoc?CPAN::Meta::Spec",' >> META_new.json
	$(NOECHO) $(ECHO) '      "version" : "2"' >> META_new.json
	$(NOECHO) $(ECHO) '   },' >> META_new.json
	$(NOECHO) $(ECHO) '   "name" : "Apache-Test",' >> META_new.json
	$(NOECHO) $(ECHO) '   "no_index" : {' >> META_new.json
	$(NOECHO) $(ECHO) '      "directory" : [' >> META_new.json
	$(NOECHO) $(ECHO) '         "t",' >> META_new.json
	$(NOECHO) $(ECHO) '         "inc"' >> META_new.json
	$(NOECHO) $(ECHO) '      ]' >> META_new.json
	$(NOECHO) $(ECHO) '   },' >> META_new.json
	$(NOECHO) $(ECHO) '   "prereqs" : {' >> META_new.json
	$(NOECHO) $(ECHO) '      "build" : {' >> META_new.json
	$(NOECHO) $(ECHO) '         "requires" : {' >> META_new.json
	$(NOECHO) $(ECHO) '            "ExtUtils::MakeMaker" : "0"' >> META_new.json
	$(NOECHO) $(ECHO) '         }' >> META_new.json
	$(NOECHO) $(ECHO) '      },' >> META_new.json
	$(NOECHO) $(ECHO) '      "configure" : {' >> META_new.json
	$(NOECHO) $(ECHO) '         "requires" : {' >> META_new.json
	$(NOECHO) $(ECHO) '            "ExtUtils::MakeMaker" : "0"' >> META_new.json
	$(NOECHO) $(ECHO) '         }' >> META_new.json
	$(NOECHO) $(ECHO) '      },' >> META_new.json
	$(NOECHO) $(ECHO) '      "runtime" : {' >> META_new.json
	$(NOECHO) $(ECHO) '         "requires" : {' >> META_new.json
	$(NOECHO) $(ECHO) '            "Cwd" : "2.06",' >> META_new.json
	$(NOECHO) $(ECHO) '            "File::Spec" : "0.8"' >> META_new.json
	$(NOECHO) $(ECHO) '         }' >> META_new.json
	$(NOECHO) $(ECHO) '      }' >> META_new.json
	$(NOECHO) $(ECHO) '   },' >> META_new.json
	$(NOECHO) $(ECHO) '   "release_status" : "stable",' >> META_new.json
	$(NOECHO) $(ECHO) '   "version" : "1.43-dev"' >> META_new.json
	$(NOECHO) $(ECHO) '}' >> META_new.json
	-$(NOECHO) $(MV) META_new.json $(DISTVNAME)/META.json


# --- MakeMaker signature section:
signature :
	cpansign -s


# --- MakeMaker dist_basics section skipped.

# --- MakeMaker dist_core section skipped.

# --- MakeMaker distdir section skipped.

# --- MakeMaker dist_test section skipped.

# --- MakeMaker dist_ci section skipped.

# --- MakeMaker distmeta section:
distmeta : create_distdir metafile
	$(NOECHO) cd $(DISTVNAME) && $(ABSPERLRUN) -MExtUtils::Manifest=maniadd -e 'exit unless -e q{META.yml};' \
	  -e 'eval { maniadd({q{META.yml} => q{Module YAML meta-data (added by MakeMaker)}}) }' \
	  -e '    or print "Could not add META.yml to MANIFEST: $$$${'\''@'\''}\n"' --
	$(NOECHO) cd $(DISTVNAME) && $(ABSPERLRUN) -MExtUtils::Manifest=maniadd -e 'exit unless -f q{META.json};' \
	  -e 'eval { maniadd({q{META.json} => q{Module JSON meta-data (added by MakeMaker)}}) }' \
	  -e '    or print "Could not add META.json to MANIFEST: $$$${'\''@'\''}\n"' --



# --- MakeMaker distsignature section:
distsignature : create_distdir
	$(NOECHO) cd $(DISTVNAME) && $(ABSPERLRUN) -MExtUtils::Manifest=maniadd -e 'eval { maniadd({q{SIGNATURE} => q{Public-key signature (added by MakeMaker)}}) }' \
	  -e '    or print "Could not add SIGNATURE to MANIFEST: $$$${'\''@'\''}\n"' --
	$(NOECHO) cd $(DISTVNAME) && $(TOUCH) SIGNATURE
	cd $(DISTVNAME) && cpansign -s



# --- MakeMaker install section skipped.

# --- MakeMaker force section:
# Phony target to force checking subdirectories.
FORCE :
	$(NOECHO) $(NOOP)


# --- MakeMaker perldepend section:


# --- MakeMaker makefile section:
# We take a very conservative approach here, but it's worth it.
# We move Makefile to Makefile.old here to avoid gnu make looping.
$(FIRST_MAKEFILE) : Makefile.PL $(CONFIGDEP)
	$(NOECHO) $(ECHO) "Makefile out-of-date with respect to $?"
	$(NOECHO) $(ECHO) "Cleaning current config before rebuilding Makefile..."
	-$(NOECHO) $(RM_F) $(MAKEFILE_OLD)
	-$(NOECHO) $(MV)   $(FIRST_MAKEFILE) $(MAKEFILE_OLD)
	- $(MAKE) $(USEMAKEFILE) $(MAKEFILE_OLD) clean $(DEV_NULL)
	$(PERLRUN) Makefile.PL 
	$(NOECHO) $(ECHO) "==> Your Makefile has been rebuilt. <=="
	$(NOECHO) $(ECHO) "==> Please rerun the $(MAKE) command.  <=="
	$(FALSE)



# --- MakeMaker staticmake section:

# --- MakeMaker makeaperl section ---
MAP_TARGET    = ../perl
FULLPERL      = "/usr/bin/perl"


# --- MakeMaker test section:
PASSENV = APACHE_TEST_APXS=$(APACHE_TEST_APXS) APACHE_TEST_GROUP=$(APACHE_TEST_GROUP) APACHE_TEST_HTTPD=$(APACHE_TEST_HTTPD) APACHE_TEST_PORT=$(APACHE_TEST_PORT) APACHE_TEST_USER=$(APACHE_TEST_USER)
TEST_FILES =
TEST_VERBOSE = 0

test_clean :
	$(FULLPERL) -I$(INST_ARCHLIB) -I$(INST_LIB) \
	t/TEST $(APACHE_TEST_EXTRA_ARGS) -clean

run_tests :
	$(PASSENV) \
	$(FULLPERL) -I$(INST_ARCHLIB) -I$(INST_LIB) \
	t/TEST $(APACHE_TEST_EXTRA_ARGS) -bugreport -verbose=$(TEST_VERBOSE) $(TEST_FILES)

test :: pure_all test_clean run_tests

test_config :
	$(PASSENV) \
	$(FULLPERL) -I$(INST_ARCHLIB) -I$(INST_LIB) \
	t/TEST $(APACHE_TEST_EXTRA_ARGS) -conf

cmodules: test_config
	cd c-modules && $(MAKE) all

cmodules_clean: test_config
	cd c-modules && $(MAKE) clean

testcover :
	@echo "Cannot run testcover action unless Devel::Cover is installed"
	@echo "Don't forget to rebuild your Makefile after installing Devel::Cover"


# --- MakeMaker ppd section:
# Creates a PPD (Perl Package Description) for a binary distribution.
ppd :
	$(NOECHO) $(ECHO) '<SOFTPKG NAME="$(DISTNAME)" VERSION="$(VERSION)">' > $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '    <ABSTRACT></ABSTRACT>' >> $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '    <AUTHOR></AUTHOR>' >> $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '    <IMPLEMENTATION>' >> $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '        <REQUIRE NAME="Cwd::" VERSION="2.06" />' >> $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '        <REQUIRE NAME="File::Spec" VERSION="0.8" />' >> $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '        <ARCHITECTURE NAME="x86_64-linux-gnu-thread-multi-5.22" />' >> $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '        <CODEBASE HREF="" />' >> $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '    </IMPLEMENTATION>' >> $(DISTNAME).ppd
	$(NOECHO) $(ECHO) '</SOFTPKG>' >> $(DISTNAME).ppd


# --- MakeMaker pm_to_blib section:

pm_to_blib : $(FIRST_MAKEFILE) $(TO_INST_PM)
	$(NOECHO) $(ABSPERLRUN) -MExtUtils::Install -e 'pm_to_blib({@ARGV}, '\''$(INST_LIB)/auto'\'', q[$(PM_FILTER)], '\''$(PERM_DIR)'\'')' -- \
	  lib/Apache/Test.pm ../blib/lib/Apache/Test.pm \
	  lib/Apache/Test5005compat.pm ../blib/lib/Apache/Test5005compat.pm \
	  lib/Apache/TestBuild.pm ../blib/lib/Apache/TestBuild.pm \
	  lib/Apache/TestClient.pm ../blib/lib/Apache/TestClient.pm \
	  lib/Apache/TestCommon.pm ../blib/lib/Apache/TestCommon.pm \
	  lib/Apache/TestCommonPost.pm ../blib/lib/Apache/TestCommonPost.pm \
	  lib/Apache/TestConfig.pm ../blib/lib/Apache/TestConfig.pm \
	  lib/Apache/TestConfigC.pm ../blib/lib/Apache/TestConfigC.pm \
	  lib/Apache/TestConfigPHP.pm ../blib/lib/Apache/TestConfigPHP.pm \
	  lib/Apache/TestConfigParrot.pm ../blib/lib/Apache/TestConfigParrot.pm \
	  lib/Apache/TestConfigParse.pm ../blib/lib/Apache/TestConfigParse.pm \
	  lib/Apache/TestConfigPerl.pm ../blib/lib/Apache/TestConfigPerl.pm \
	  lib/Apache/TestHandler.pm ../blib/lib/Apache/TestHandler.pm \
	  lib/Apache/TestHarness.pm ../blib/lib/Apache/TestHarness.pm \
	  lib/Apache/TestHarnessPHP.pm ../blib/lib/Apache/TestHarnessPHP.pm \
	  lib/Apache/TestMB.pm ../blib/lib/Apache/TestMB.pm \
	  lib/Apache/TestMM.pm ../blib/lib/Apache/TestMM.pm \
	  lib/Apache/TestPerlDB.pm ../blib/lib/Apache/TestPerlDB.pm \
	  lib/Apache/TestReport.pm ../blib/lib/Apache/TestReport.pm \
	  lib/Apache/TestReportPerl.pm ../blib/lib/Apache/TestReportPerl.pm \
	  lib/Apache/TestRequest.pm ../blib/lib/Apache/TestRequest.pm \
	  lib/Apache/TestRun.pm ../blib/lib/Apache/TestRun.pm \
	  lib/Apache/TestRunPHP.pm ../blib/lib/Apache/TestRunPHP.pm \
	  lib/Apache/TestRunParrot.pm ../blib/lib/Apache/TestRunParrot.pm \
	  lib/Apache/TestRunPerl.pm ../blib/lib/Apache/TestRunPerl.pm \
	  lib/Apache/TestSSLCA.pm ../blib/lib/Apache/TestSSLCA.pm \
	  lib/Apache/TestServer.pm ../blib/lib/Apache/TestServer.pm \
	  lib/Apache/TestSmoke.pm ../blib/lib/Apache/TestSmoke.pm \
	  lib/Apache/TestSmokePerl.pm ../blib/lib/Apache/TestSmokePerl.pm \
	  lib/Apache/TestSort.pm ../blib/lib/Apache/TestSort.pm \
	  lib/Apache/TestTrace.pm ../blib/lib/Apache/TestTrace.pm \
	  lib/Apache/TestUtil.pm ../blib/lib/Apache/TestUtil.pm \
	  lib/Bundle/ApacheTest.pm ../blib/lib/Bundle/ApacheTest.pm 
	$(NOECHO) $(TOUCH) pm_to_blib


# --- MakeMaker selfdocument section:


# --- MakeMaker postamble section:
tag :
	svn copy -m '$(VERSION_SYM) tag' https://svn.apache.org/repos/asf/perl/Apache-Test/trunk https://svn.apache.org/repos/asf/perl/Apache-Test/tags/$(VERSION_SYM)


# End.
