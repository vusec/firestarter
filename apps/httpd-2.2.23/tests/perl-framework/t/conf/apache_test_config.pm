# WARNING: this file is generated, do not edit
# generated on Mon Dec  7 01:11:52 2020
# 01: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestConfig.pm:1007
# 02: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestConfig.pm:1025
# 03: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestConfig.pm:1936
# 04: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestRun.pm:503
# 05: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestRun.pm:713
# 06: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestRun.pm:713
# 07: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/TEST:26

package apache_test_config;

sub new {
    bless( {
         'hostport' => 'localhost:8529',
         'httpd_defines' => {
                              'AP_TYPES_CONFIG_FILE' => 'conf/mime.types',
                              'DEFAULT_ERRORLOG' => 'logs/error_log',
                              'DEFAULT_SCOREBOARD' => 'logs/apache_runtime_status',
                              'SUEXEC_BIN' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/suexec',
                              'APR_HAS_OTHER_CHILD' => 1,
                              'APR_HAS_MMAP' => 1,
                              'APACHE_MPM_DIR' => 'server/mpm/prefork',
                              'AP_HAVE_RELIABLE_PIPED_LOGS' => 1,
                              'APR_HAS_SENDFILE' => 1,
                              'SERVER_CONFIG_FILE' => 'conf/httpd.conf',
                              'DYNAMIC_MODULE_LIMIT' => '128',
                              'APR_USE_SYSVSEM_SERIALIZE' => 1,
                              'DEFAULT_PIDLOG' => 'logs/httpd.pid',
                              'APR_HAVE_IPV6 (IPv4-mapped addresses enabled)' => 1,
                              'HTTPD_ROOT' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install',
                              'DEFAULT_LOCKFILE' => 'logs/accept.lock',
                              'APR_USE_PTHREAD_SERIALIZE' => 1
                            },
         'cmodules' => [
                         {
                           'sym' => 'echo_post_chunk_module',
                           'subdir' => 'echo_post_chunk',
                           'name' => 'mod_echo_post_chunk',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/echo_post_chunk'
                         },
                         {
                           'sym' => 'fold_module',
                           'subdir' => 'fold',
                           'name' => 'mod_fold',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/fold'
                         },
                         {
                           'sym' => 'list_modules_module',
                           'name' => 'mod_list_modules',
                           'subdir' => 'list_modules',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/list_modules'
                         },
                         {
                           'sym' => 'eat_post_module',
                           'subdir' => 'eat_post',
                           'name' => 'mod_eat_post',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/eat_post'
                         },
                         {
                           'subdir' => 'nntp_like',
                           'name' => 'mod_nntp_like',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/nntp_like',
                           'sym' => 'nntp_like_module'
                         },
                         {
                           'subdir' => 'test_rwrite',
                           'name' => 'mod_test_rwrite',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_rwrite',
                           'sym' => 'test_rwrite_module'
                         },
                         {
                           'sym' => 'test_pass_brigade_module',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_pass_brigade',
                           'subdir' => 'test_pass_brigade',
                           'name' => 'mod_test_pass_brigade'
                         },
                         {
                           'sym' => 'authany_module',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/authany',
                           'name' => 'mod_authany',
                           'subdir' => 'authany'
                         },
                         {
                           'sym' => 'client_add_filter_module',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/client_add_filter',
                           'name' => 'mod_client_add_filter',
                           'subdir' => 'client_add_filter'
                         },
                         {
                           'name' => 'mod_random_chunk',
                           'subdir' => 'random_chunk',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/random_chunk',
                           'sym' => 'random_chunk_module'
                         },
                         {
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_ssl',
                           'subdir' => 'test_ssl',
                           'name' => 'mod_test_ssl',
                           'sym' => 'test_ssl_module'
                         },
                         {
                           'sym' => 'input_body_filter_module',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/input_body_filter',
                           'subdir' => 'input_body_filter',
                           'name' => 'mod_input_body_filter'
                         },
                         {
                           'sym' => 'memory_track_module',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/memory_track',
                           'name' => 'mod_memory_track',
                           'subdir' => 'memory_track'
                         },
                         {
                           'sym' => 'echo_post_module',
                           'name' => 'mod_echo_post',
                           'subdir' => 'echo_post',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/echo_post'
                         },
                         {
                           'subdir' => 'test_apr_uri',
                           'name' => 'mod_test_apr_uri',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_apr_uri',
                           'sym' => 'test_apr_uri_module'
                         }
                       ],
         'httpd_basedir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install',
         'preamble_hooks' => [
                               sub { "DUMMY" }
                             ],
         'postamble_hooks' => [
                                sub { "DUMMY" }
                              ],
         'httpd_info' => {
                           'MODULE_MAGIC_NUMBER' => '20051115:31',
                           'MODULE_MAGIC_NUMBER_MINOR' => '31',
                           'SERVER_MPM' => 'Prefork',
                           'BUILT' => 'Dec  7 2020 01:08:45',
                           'VERSION' => 'Apache/2.2.23 (Unix)',
                           'MODULE_MAGIC_NUMBER_MAJOR' => '20051115'
                         },
         'vars' => {
                     'core_port' => 8535,
                     'minclientsthreadedmpm' => 10,
                     'top_dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework',
                     't_conf_file' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/httpd.conf',
                     'mod_include_port' => 8552,
                     'access_module' => 'mod_authz_host.c',
                     'ssl_module' => 'mod_ssl.c',
                     't_pid_file' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/logs/httpd.pid',
                     'httpd_conf' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/conf/httpd.conf',
                     'proxy_https_https_port' => 8554,
                     'maxclients_preset' => 0,
                     'maxspare' => 2,
                     'proxy_http_bal1_port' => 8546,
                     'inherit_documentroot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/htdocs',
                     'port' => 8529,
                     'maxsparethreadedmpm' => 20,
                     'proxy_http_https_proxy_section_port' => 8555,
                     'sbindir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin',
                     'serveradmin' => 'you@example.com',
                     'ssl_ocsp_port' => 8532,
                     't_dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t',
                     'ssl_pr33791_port' => 8531,
                     'maxclients' => 3,
                     'php_module_name' => 'sapi_apache2',
                     'mod_proxy_port' => 8545,
                     'remote_addr' => '127.0.0.1',
                     'auth_module' => 'mod_auth_basic.c',
                     'scheme' => 'http',
                     'minclients' => 1,
                     'group' => 'koustubha',
                     'perl' => '/usr/bin/perl',
                     'defines' => '',
                     'access_module_name' => 'mod_authz_host',
                     'maxclientsthreadedmpm' => 30,
                     'sslcaorg' => 'asf',
                     'remote_ip_port' => 8544,
                     'auth_module_name' => 'mod_auth_basic',
                     'proxyssl_url' => '',
                     't_logs' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/logs',
                     'proxy' => 'off',
                     't_conf' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf',
                     't_state' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/state',
                     'php_module' => 'sapi_apache2.c',
                     'documentroot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/htdocs',
                     'target' => 'httpd',
                     'proxy_http_nofwd_port' => 8538,
                     'cve_2011_3368_port' => 8539,
                     'proxy_http_reverse_port' => 8537,
                     'h2c_port' => 8533,
                     'thread_module_name' => 'worker',
                     'ssl_module_name' => 'mod_ssl',
                     'ssl_optional_cc_port' => 8530,
                     'http_unsafe_port' => 8542,
                     'http_strict_port' => 8543,
                     'serverroot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t',
                     'cve_2011_3368_rewrite_port' => 8536,
                     'h2_port' => 8534,
                     'servername' => 'localhost',
                     'cgi_module' => 'mod_cgi.c',
                     'proxy_http_bal2_port' => 8547,
                     'limitrequestlinex2' => 256,
                     'perlpod' => '/usr/share/perl/5.22/pod',
                     'startserversthreadedmpm' => 1,
                     'proxy_http_balancer_port' => 8548,
                     'proxy_https_https_proxy_section_port' => 8556,
                     'user' => 'koustubha',
                     'limitrequestline' => 128,
                     'proxy_http_https_port' => 8553,
                     'sslproto' => 'all',
                     'conf_dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/conf',
                     'sslca' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/ssl/ca',
                     'mod_headers_port' => 8540,
                     'cgi_module_name' => 'mod_cgi',
                     'apxs' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/apxs',
                     'bindir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin',
                     'threadsperchild' => 10,
                     'proxy_https_http_port' => 8557,
                     'src_dir' => undef,
                     'thread_module' => 'worker.c',
                     'proxy_fcgi_port' => 8551,
                     'error_document_port' => 8541,
                     'httpd' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/httpd'
                   },
         'apache_test_version' => '1.43',
         'mpm' => 'prefork',
         'modules' => {
                        'mod_proxy.c' => 1,
                        'mod_dir.c' => 1,
                        'mod_proxy_ftp.c' => 1,
                        'mod_setenvif.c' => 1,
                        'mod_userdir.c' => 1,
                        'mod_asis.c' => 1,
                        'mod_cgi.c' => 1,
                        'mod_headers.c' => 1,
                        'mod_authz_groupfile.c' => 1,
                        'core.c' => 1,
                        'mod_authz_host.c' => 1,
                        'mod_so.c' => 1,
                        'mod_proxy_connect.c' => 1,
                        'mod_actions.c' => 1,
                        'mod_autoindex.c' => 1,
                        'prefork.c' => 1,
                        'mod_env.c' => 1,
                        'mod_negotiation.c' => 1,
                        'mod_proxy_scgi.c' => 1,
                        'mod_status.c' => 1,
                        'mod_alias.c' => 1,
                        'mod_authz_default.c' => 1,
                        'mod_authn_file.c' => 1,
                        'mod_version.c' => 1,
                        'http_core.c' => 1,
                        'mod_filter.c' => 1,
                        'mod_proxy_ajp.c' => 1,
                        'mod_proxy_balancer.c' => 1,
                        'mod_auth_basic.c' => 1,
                        'mod_log_config.c' => 1,
                        'mod_authz_user.c' => 1,
                        'mod_include.c' => 1,
                        'mod_proxy_http.c' => 1,
                        'mod_mime.c' => 1,
                        'mod_authn_default.c' => 1
                      },
         'postamble' => [
                          '<IfModule mod_mime.c>
    TypesConfig "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/conf/mime.types"
</IfModule>
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/cache.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/core.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/extra.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/http2.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/include-ssi-exec.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/include.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/proxy.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/ssl/proxyssl.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/ssl/ssl.conf"
',
                          'Include "/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/vhost_alias.conf"
',
                          '
'
                        ],
         'APXS' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/apxs',
         'verbose' => undef,
         'server' => bless( {
                              'config' => $VAR1,
                              'rev' => 2,
                              'port_counter' => 8557,
                              'mpm' => 'prefork',
                              'version' => 'Apache/2.2.23',
                              'revminor' => 2,
                              'run' => bless( {
                                                'conf_opts' => {
                                                                 'verbose' => undef,
                                                                 'save' => 1
                                                               },
                                                'test_config' => $VAR1,
                                                'tests' => [],
                                                'server' => $VAR1->{'server'},
                                                'reconfigure' => '',
                                                'argv' => [],
                                                'opts' => {
                                                            'one-process' => 1,
                                                            'req_args' => {},
                                                            'start-httpd' => 1,
                                                            'stop-httpd' => 1,
                                                            'preamble' => [],
                                                            'postamble' => [],
                                                            'breakpoint' => [],
                                                            'run-tests' => 1,
                                                            'header' => {}
                                                          }
                                              }, 'Apache::TestRun' ),
                              'name' => 'localhost:8529'
                            }, 'Apache::TestServer' ),
         'inherit_config' => {
                               'LoadModule' => [],
                               'ServerRoot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install',
                               'LoadFile' => [],
                               'TypesConfig' => 'conf/mime.types',
                               'DocumentRoot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/htdocs',
                               'ServerAdmin' => 'you@example.com'
                             },
         'cmodules_disabled' => {
                                  'mod_test_utilities.c' => 'requires Apache version 2.4',
                                  'mod_test_session.c' => 'requires Apache version 2.3'
                                },
         'save' => 1,
         'clean' => {
                      'dirs' => {
                                  '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/state' => 1,
                                  '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/logs' => 1
                                },
                      'files' => {
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/echo_post/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/core.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/include.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/httpd.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/cache.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/vhost_alias.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/http2.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/memory_track/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/apache_test_config.pm' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/client_add_filter/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/proxy.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_apr_uri/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/logs/apache_runtime_status.sem' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/echo_post_chunk/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_ssl/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/random_chunk/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/include-ssi-exec.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/ssl/proxyssl.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/apache_httpd_test.h' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_pass_brigade/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/ssl/ssl.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/extra.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/nntp_like/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/input_body_filter/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/authany/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/eat_post/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/list_modules/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/fold/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_rwrite/Makefile' => 1
                                 }
                    },
         'inc' => [],
         'preamble' => [
                         '
'
                       ],
         'cmodules_dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules',
         '_apxs' => {
                      'TARGET' => 'httpd',
                      'BINDIR' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin',
                      'PREFIX' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install',
                      'LIBEXECDIR' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/modules',
                      'SYSCONFDIR' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/conf',
                      'SBINDIR' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin'
                    },
         'vhosts' => {
                       'remote_ip' => {
                                        'hostport' => 'localhost:8544',
                                        'servername' => 'localhost',
                                        'namebased' => 0,
                                        'port' => 8544,
                                        'name' => 'localhost:8544'
                                      },
                       'ssl_ocsp' => {
                                       'name' => 'localhost:8532',
                                       'namebased' => 0,
                                       'port' => 8532,
                                       'hostport' => 'localhost:8532',
                                       'servername' => 'localhost'
                                     },
                       'mod_headers' => {
                                          'name' => 'localhost:8540',
                                          'namebased' => 0,
                                          'port' => 8540,
                                          'servername' => 'localhost',
                                          'hostport' => 'localhost:8540'
                                        },
                       'cve_2011_3368_rewrite' => {
                                                    'name' => 'localhost:8536',
                                                    'port' => 8536,
                                                    'namebased' => 0,
                                                    'hostport' => 'localhost:8536',
                                                    'servername' => 'localhost'
                                                  },
                       'proxy_http_https_proxy_section' => {
                                                             'port' => 8555,
                                                             'namebased' => 0,
                                                             'name' => 'localhost:8555',
                                                             'servername' => 'localhost',
                                                             'hostport' => 'localhost:8555'
                                                           },
                       'proxy_https_https_proxy_section' => {
                                                              'hostport' => 'localhost:8556',
                                                              'servername' => 'localhost',
                                                              'namebased' => 0,
                                                              'port' => 8556,
                                                              'name' => 'localhost:8556'
                                                            },
                       'cve_2011_3368' => {
                                            'hostport' => 'localhost:8539',
                                            'servername' => 'localhost',
                                            'name' => 'localhost:8539',
                                            'port' => 8539,
                                            'namebased' => 0
                                          },
                       'h2c' => {
                                  'hostport' => 'localhost:8533',
                                  'servername' => 'localhost',
                                  'port' => 8533,
                                  'namebased' => 0,
                                  'name' => 'localhost:8533'
                                },
                       'proxy_http_https' => {
                                               'hostport' => 'localhost:8553',
                                               'servername' => 'localhost',
                                               'name' => 'localhost:8553',
                                               'namebased' => 0,
                                               'port' => 8553
                                             },
                       'http_unsafe' => {
                                          'servername' => 'localhost',
                                          'hostport' => 'localhost:8542',
                                          'namebased' => 0,
                                          'port' => 8542,
                                          'name' => 'localhost:8542'
                                        },
                       'proxy_http_nofwd' => {
                                               'hostport' => 'localhost:8538',
                                               'servername' => 'localhost',
                                               'name' => 'localhost:8538',
                                               'port' => 8538,
                                               'namebased' => 0
                                             },
                       'proxy_https_http' => {
                                               'port' => 8557,
                                               'namebased' => 0,
                                               'name' => 'localhost:8557',
                                               'servername' => 'localhost',
                                               'hostport' => 'localhost:8557'
                                             },
                       'error_document' => {
                                             'hostport' => 'localhost:8541',
                                             'servername' => 'localhost',
                                             'name' => 'localhost:8541',
                                             'namebased' => 0,
                                             'port' => 8541
                                           },
                       'proxy_https_https' => {
                                                'hostport' => 'localhost:8554',
                                                'servername' => 'localhost',
                                                'port' => 8554,
                                                'namebased' => 0,
                                                'name' => 'localhost:8554'
                                              },
                       'proxy_http_bal1' => {
                                              'hostport' => 'localhost:8546',
                                              'servername' => 'localhost',
                                              'namebased' => 0,
                                              'port' => 8546,
                                              'name' => 'localhost:8546'
                                            },
                       'mod_include' => {
                                          'namebased' => 4,
                                          'port' => 8552,
                                          'name' => 'localhost:8552',
                                          'hostport' => 'localhost:8552',
                                          'servername' => 'localhost'
                                        },
                       'proxy_http_reverse' => {
                                                 'servername' => 'localhost',
                                                 'hostport' => 'localhost:8537',
                                                 'name' => 'localhost:8537',
                                                 'namebased' => 0,
                                                 'port' => 8537
                                               },
                       'h2' => {
                                 'port' => 8534,
                                 'namebased' => 5,
                                 'name' => 'localhost:8534',
                                 'hostport' => 'localhost:8534',
                                 'servername' => 'localhost'
                               },
                       'ssl_optional_cc' => {
                                              'name' => 'localhost:8530',
                                              'port' => 8530,
                                              'namebased' => 0,
                                              'hostport' => 'localhost:8530',
                                              'servername' => 'localhost'
                                            },
                       'ssl_pr33791' => {
                                          'hostport' => 'localhost:8531',
                                          'servername' => 'localhost',
                                          'name' => 'localhost:8531',
                                          'namebased' => 0,
                                          'port' => 8531
                                        },
                       'proxy_fcgi' => {
                                         'servername' => 'localhost',
                                         'hostport' => 'localhost:8551',
                                         'port' => 8551,
                                         'namebased' => 0,
                                         'name' => 'localhost:8551'
                                       },
                       'proxy_http_bal2' => {
                                              'hostport' => 'localhost:8547',
                                              'servername' => 'localhost',
                                              'name' => 'localhost:8547',
                                              'namebased' => 0,
                                              'port' => 8547
                                            },
                       'core' => {
                                   'hostport' => 'localhost:8535',
                                   'servername' => 'localhost',
                                   'port' => 8535,
                                   'namebased' => 4,
                                   'name' => 'localhost:8535'
                                 },
                       'proxy_http_balancer' => {
                                                  'name' => 'localhost:8548',
                                                  'namebased' => 0,
                                                  'port' => 8548,
                                                  'servername' => 'localhost',
                                                  'hostport' => 'localhost:8548'
                                                },
                       'http_strict' => {
                                          'servername' => 'localhost',
                                          'hostport' => 'localhost:8543',
                                          'port' => 8543,
                                          'namebased' => 0,
                                          'name' => 'localhost:8543'
                                        },
                       'mod_proxy' => {
                                        'namebased' => 0,
                                        'port' => 8545,
                                        'name' => 'localhost:8545',
                                        'hostport' => 'localhost:8545',
                                        'servername' => 'localhost'
                                      }
                     }
       }, 'Apache::TestConfig' );
}

1;
