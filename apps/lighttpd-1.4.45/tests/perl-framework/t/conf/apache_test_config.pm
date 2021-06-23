# WARNING: this file is generated, do not edit
# generated on Fri Aug  7 01:11:31 2020
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
         'inherit_config' => {
                               'ServerAdmin' => 'you@example.com',
                               'ServerRoot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install',
                               'DocumentRoot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/htdocs',
                               'TypesConfig' => 'conf/mime.types',
                               'LoadFile' => [],
                               'LoadModule' => []
                             },
         'httpd_defines' => {
                              'APACHE_MPM_DIR' => 'server/mpm/prefork',
                              'APR_HAVE_IPV6 (IPv4-mapped addresses enabled)' => 1,
                              'DEFAULT_ERRORLOG' => 'logs/error_log',
                              'DEFAULT_LOCKFILE' => 'logs/accept.lock',
                              'APR_HAS_OTHER_CHILD' => 1,
                              'SUEXEC_BIN' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/suexec',
                              'DEFAULT_SCOREBOARD' => 'logs/apache_runtime_status',
                              'SERVER_CONFIG_FILE' => 'conf/httpd.conf',
                              'APR_HAS_MMAP' => 1,
                              'APR_USE_SYSVSEM_SERIALIZE' => 1,
                              'APR_USE_PTHREAD_SERIALIZE' => 1,
                              'APR_HAS_SENDFILE' => 1,
                              'DEFAULT_PIDLOG' => 'logs/httpd.pid',
                              'AP_HAVE_RELIABLE_PIPED_LOGS' => 1,
                              'AP_TYPES_CONFIG_FILE' => 'conf/mime.types',
                              'HTTPD_ROOT' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install',
                              'DYNAMIC_MODULE_LIMIT' => '128'
                            },
         'httpd_info' => {
                           'VERSION' => 'Apache/2.2.23 (Unix)',
                           'MODULE_MAGIC_NUMBER_MINOR' => '31',
                           'MODULE_MAGIC_NUMBER' => '20051115:31',
                           'MODULE_MAGIC_NUMBER_MAJOR' => '20051115',
                           'SERVER_MPM' => 'Prefork',
                           'BUILT' => 'Aug  6 2020 00:01:51'
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
         'vars' => {
                     'perlpod' => '/usr/share/perl/5.22/pod',
                     'proxy' => 'off',
                     't_logs' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/logs',
                     't_pid_file' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/logs/httpd.pid',
                     'ssl_module' => 'mod_ssl.c',
                     'error_document_port' => 8541,
                     'remote_addr' => '127.0.0.1',
                     'minclients' => 1,
                     'minclientsthreadedmpm' => 10,
                     'proxy_http_https_proxy_section_port' => 8555,
                     'cgi_module_name' => 'mod_cgi',
                     'cve_2011_3368_rewrite_port' => 8536,
                     'proxy_http_https_port' => 8553,
                     'thread_module_name' => 'worker',
                     'proxy_http_balancer_port' => 8548,
                     'perl' => '/usr/bin/perl',
                     'group' => 'koustubha',
                     'maxclients' => 3,
                     'proxyssl_url' => '',
                     'user' => 'koustubha',
                     'h2c_port' => 8533,
                     'mod_proxy_port' => 8545,
                     'threadsperchild' => 10,
                     'port' => 8529,
                     'php_module' => 'sapi_apache2.c',
                     'mod_include_port' => 8552,
                     't_state' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/state',
                     'core_port' => 8535,
                     'proxy_https_https_port' => 8554,
                     'inherit_documentroot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/htdocs',
                     'httpd' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/httpd',
                     'target' => 'httpd',
                     'proxy_https_https_proxy_section_port' => 8556,
                     'documentroot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/htdocs',
                     'cve_2011_3368_port' => 8539,
                     'defines' => '',
                     'remote_ip_port' => 8544,
                     'auth_module' => 'mod_auth_basic.c',
                     'serverroot' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t',
                     'ssl_optional_cc_port' => 8530,
                     'http_strict_port' => 8543,
                     'proxy_https_http_port' => 8557,
                     'ssl_pr33791_port' => 8531,
                     'ssl_ocsp_port' => 8532,
                     'src_dir' => undef,
                     't_conf' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf',
                     'access_module_name' => 'mod_authz_host',
                     'apxs' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/apxs',
                     'conf_dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/conf',
                     'maxspare' => 2,
                     'php_module_name' => 'sapi_apache2',
                     'mod_headers_port' => 8540,
                     'thread_module' => 'worker.c',
                     'httpd_conf' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/conf/httpd.conf',
                     'serveradmin' => 'you@example.com',
                     't_conf_file' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/httpd.conf',
                     'h2_port' => 8534,
                     'sslcaorg' => 'asf',
                     'sbindir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin',
                     'limitrequestlinex2' => 256,
                     'bindir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin',
                     'access_module' => 'mod_authz_host.c',
                     'ssl_module_name' => 'mod_ssl',
                     'top_dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework',
                     'sslproto' => 'all',
                     'http_unsafe_port' => 8542,
                     'maxclients_preset' => 0,
                     'proxy_http_nofwd_port' => 8538,
                     'cgi_module' => 'mod_cgi.c',
                     'scheme' => 'http',
                     'servername' => 'localhost',
                     'proxy_http_bal2_port' => 8547,
                     'auth_module_name' => 'mod_auth_basic',
                     'limitrequestline' => 128,
                     'proxy_http_reverse_port' => 8537,
                     'proxy_fcgi_port' => 8551,
                     'proxy_http_bal1_port' => 8546,
                     'sslca' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/ssl/ca',
                     'startserversthreadedmpm' => 1,
                     'maxclientsthreadedmpm' => 30,
                     'maxsparethreadedmpm' => 20,
                     't_dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t'
                   },
         'inc' => [],
         'cmodules_disabled' => {
                                  'mod_test_session.c' => 'requires Apache version 2.3',
                                  'mod_test_utilities.c' => 'requires Apache version 2.4'
                                },
         'vhosts' => {
                       'proxy_http_reverse' => {
                                                 'name' => 'localhost:8537',
                                                 'port' => 8537,
                                                 'hostport' => 'localhost:8537',
                                                 'servername' => 'localhost',
                                                 'namebased' => 0
                                               },
                       'proxy_https_http' => {
                                               'name' => 'localhost:8557',
                                               'port' => 8557,
                                               'hostport' => 'localhost:8557',
                                               'servername' => 'localhost',
                                               'namebased' => 0
                                             },
                       'cve_2011_3368' => {
                                            'namebased' => 0,
                                            'hostport' => 'localhost:8539',
                                            'servername' => 'localhost',
                                            'port' => 8539,
                                            'name' => 'localhost:8539'
                                          },
                       'http_unsafe' => {
                                          'name' => 'localhost:8542',
                                          'port' => 8542,
                                          'hostport' => 'localhost:8542',
                                          'servername' => 'localhost',
                                          'namebased' => 0
                                        },
                       'proxy_http_nofwd' => {
                                               'namebased' => 0,
                                               'hostport' => 'localhost:8538',
                                               'servername' => 'localhost',
                                               'port' => 8538,
                                               'name' => 'localhost:8538'
                                             },
                       'remote_ip' => {
                                        'namebased' => 0,
                                        'hostport' => 'localhost:8544',
                                        'servername' => 'localhost',
                                        'port' => 8544,
                                        'name' => 'localhost:8544'
                                      },
                       'h2' => {
                                 'namebased' => 5,
                                 'servername' => 'localhost',
                                 'hostport' => 'localhost:8534',
                                 'port' => 8534,
                                 'name' => 'localhost:8534'
                               },
                       'proxy_https_https_proxy_section' => {
                                                              'hostport' => 'localhost:8556',
                                                              'servername' => 'localhost',
                                                              'namebased' => 0,
                                                              'name' => 'localhost:8556',
                                                              'port' => 8556
                                                            },
                       'proxy_https_https' => {
                                                'port' => 8554,
                                                'name' => 'localhost:8554',
                                                'namebased' => 0,
                                                'servername' => 'localhost',
                                                'hostport' => 'localhost:8554'
                                              },
                       'http_strict' => {
                                          'port' => 8543,
                                          'name' => 'localhost:8543',
                                          'namebased' => 0,
                                          'servername' => 'localhost',
                                          'hostport' => 'localhost:8543'
                                        },
                       'ssl_pr33791' => {
                                          'namebased' => 0,
                                          'hostport' => 'localhost:8531',
                                          'servername' => 'localhost',
                                          'port' => 8531,
                                          'name' => 'localhost:8531'
                                        },
                       'ssl_ocsp' => {
                                       'namebased' => 0,
                                       'servername' => 'localhost',
                                       'hostport' => 'localhost:8532',
                                       'port' => 8532,
                                       'name' => 'localhost:8532'
                                     },
                       'cve_2011_3368_rewrite' => {
                                                    'servername' => 'localhost',
                                                    'hostport' => 'localhost:8536',
                                                    'namebased' => 0,
                                                    'name' => 'localhost:8536',
                                                    'port' => 8536
                                                  },
                       'proxy_http_bal2' => {
                                              'port' => 8547,
                                              'name' => 'localhost:8547',
                                              'namebased' => 0,
                                              'servername' => 'localhost',
                                              'hostport' => 'localhost:8547'
                                            },
                       'mod_headers' => {
                                          'servername' => 'localhost',
                                          'hostport' => 'localhost:8540',
                                          'namebased' => 0,
                                          'name' => 'localhost:8540',
                                          'port' => 8540
                                        },
                       'proxy_fcgi' => {
                                         'namebased' => 0,
                                         'servername' => 'localhost',
                                         'hostport' => 'localhost:8551',
                                         'port' => 8551,
                                         'name' => 'localhost:8551'
                                       },
                       'proxy_http_https' => {
                                               'port' => 8553,
                                               'name' => 'localhost:8553',
                                               'namebased' => 0,
                                               'servername' => 'localhost',
                                               'hostport' => 'localhost:8553'
                                             },
                       'core' => {
                                   'namebased' => 4,
                                   'hostport' => 'localhost:8535',
                                   'servername' => 'localhost',
                                   'port' => 8535,
                                   'name' => 'localhost:8535'
                                 },
                       'proxy_http_balancer' => {
                                                  'name' => 'localhost:8548',
                                                  'port' => 8548,
                                                  'servername' => 'localhost',
                                                  'hostport' => 'localhost:8548',
                                                  'namebased' => 0
                                                },
                       'mod_proxy' => {
                                        'name' => 'localhost:8545',
                                        'port' => 8545,
                                        'servername' => 'localhost',
                                        'hostport' => 'localhost:8545',
                                        'namebased' => 0
                                      },
                       'ssl_optional_cc' => {
                                              'namebased' => 0,
                                              'servername' => 'localhost',
                                              'hostport' => 'localhost:8530',
                                              'port' => 8530,
                                              'name' => 'localhost:8530'
                                            },
                       'proxy_http_bal1' => {
                                              'namebased' => 0,
                                              'hostport' => 'localhost:8546',
                                              'servername' => 'localhost',
                                              'port' => 8546,
                                              'name' => 'localhost:8546'
                                            },
                       'proxy_http_https_proxy_section' => {
                                                             'port' => 8555,
                                                             'name' => 'localhost:8555',
                                                             'namebased' => 0,
                                                             'hostport' => 'localhost:8555',
                                                             'servername' => 'localhost'
                                                           },
                       'error_document' => {
                                             'namebased' => 0,
                                             'hostport' => 'localhost:8541',
                                             'servername' => 'localhost',
                                             'port' => 8541,
                                             'name' => 'localhost:8541'
                                           },
                       'mod_include' => {
                                          'servername' => 'localhost',
                                          'hostport' => 'localhost:8552',
                                          'namebased' => 4,
                                          'name' => 'localhost:8552',
                                          'port' => 8552
                                        },
                       'h2c' => {
                                  'hostport' => 'localhost:8533',
                                  'servername' => 'localhost',
                                  'namebased' => 0,
                                  'name' => 'localhost:8533',
                                  'port' => 8533
                                }
                     },
         'preamble_hooks' => [
                               sub { "DUMMY" }
                             ],
         '_apxs' => {
                      'SBINDIR' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin',
                      'BINDIR' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin',
                      'LIBEXECDIR' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/modules',
                      'PREFIX' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install',
                      'SYSCONFDIR' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/conf',
                      'TARGET' => 'httpd'
                    },
         'postamble_hooks' => [
                                sub { "DUMMY" }
                              ],
         'APXS' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install/bin/apxs',
         'clean' => {
                      'files' => {
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/echo_post/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/client_add_filter/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/core.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_ssl/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/vhost_alias.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/authany/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/apache_test_config.pm' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/cache.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/httpd.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/include-ssi-exec.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/ssl/proxyssl.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/nntp_like/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/fold/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/memory_track/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/proxy.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_pass_brigade/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_rwrite/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/include.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/echo_post_chunk/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_apr_uri/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/apache_httpd_test.h' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/ssl/ssl.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/extra.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/conf/http2.conf' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/input_body_filter/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/logs/apache_runtime_status.sem' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/eat_post/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/random_chunk/Makefile' => 1,
                                   '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/list_modules/Makefile' => 1
                                 },
                      'dirs' => {
                                  '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/state' => 1,
                                  '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/logs' => 1
                                }
                    },
         'preamble' => [
                         '
'
                       ],
         'cmodules' => [
                         {
                           'sym' => 'echo_post_chunk_module',
                           'name' => 'mod_echo_post_chunk',
                           'subdir' => 'echo_post_chunk',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/echo_post_chunk'
                         },
                         {
                           'name' => 'mod_fold',
                           'sym' => 'fold_module',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/fold',
                           'subdir' => 'fold'
                         },
                         {
                           'name' => 'mod_list_modules',
                           'sym' => 'list_modules_module',
                           'subdir' => 'list_modules',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/list_modules'
                         },
                         {
                           'name' => 'mod_eat_post',
                           'sym' => 'eat_post_module',
                           'subdir' => 'eat_post',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/eat_post'
                         },
                         {
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/nntp_like',
                           'subdir' => 'nntp_like',
                           'sym' => 'nntp_like_module',
                           'name' => 'mod_nntp_like'
                         },
                         {
                           'subdir' => 'test_rwrite',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_rwrite',
                           'sym' => 'test_rwrite_module',
                           'name' => 'mod_test_rwrite'
                         },
                         {
                           'sym' => 'test_pass_brigade_module',
                           'name' => 'mod_test_pass_brigade',
                           'subdir' => 'test_pass_brigade',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_pass_brigade'
                         },
                         {
                           'sym' => 'authany_module',
                           'name' => 'mod_authany',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/authany',
                           'subdir' => 'authany'
                         },
                         {
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/client_add_filter',
                           'subdir' => 'client_add_filter',
                           'name' => 'mod_client_add_filter',
                           'sym' => 'client_add_filter_module'
                         },
                         {
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/random_chunk',
                           'subdir' => 'random_chunk',
                           'name' => 'mod_random_chunk',
                           'sym' => 'random_chunk_module'
                         },
                         {
                           'subdir' => 'test_ssl',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_ssl',
                           'name' => 'mod_test_ssl',
                           'sym' => 'test_ssl_module'
                         },
                         {
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/input_body_filter',
                           'subdir' => 'input_body_filter',
                           'name' => 'mod_input_body_filter',
                           'sym' => 'input_body_filter_module'
                         },
                         {
                           'sym' => 'memory_track_module',
                           'name' => 'mod_memory_track',
                           'subdir' => 'memory_track',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/memory_track'
                         },
                         {
                           'subdir' => 'echo_post',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/echo_post',
                           'name' => 'mod_echo_post',
                           'sym' => 'echo_post_module'
                         },
                         {
                           'sym' => 'test_apr_uri_module',
                           'name' => 'mod_test_apr_uri',
                           'dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules/test_apr_uri',
                           'subdir' => 'test_apr_uri'
                         }
                       ],
         'httpd_basedir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/install',
         'server' => bless( {
                              'config' => $VAR1,
                              'run' => bless( {
                                                'reconfigure' => '',
                                                'argv' => [],
                                                'conf_opts' => {
                                                                 'verbose' => undef,
                                                                 'save' => 1
                                                               },
                                                'server' => $VAR1->{'server'},
                                                'test_config' => $VAR1,
                                                'tests' => [],
                                                'opts' => {
                                                            'preamble' => [],
                                                            'postamble' => [],
                                                            'one-process' => 1,
                                                            'stop-httpd' => 1,
                                                            'start-httpd' => 1,
                                                            'header' => {},
                                                            'req_args' => {},
                                                            'breakpoint' => [],
                                                            'run-tests' => 1
                                                          }
                                              }, 'Apache::TestRun' ),
                              'mpm' => 'prefork',
                              'port_counter' => 8557,
                              'revminor' => 2,
                              'rev' => 2,
                              'version' => 'Apache/2.2.23',
                              'name' => 'localhost:8529'
                            }, 'Apache::TestServer' ),
         'apache_test_version' => '1.43',
         'hostport' => 'localhost:8529',
         'save' => 1,
         'cmodules_dir' => '/mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/c-modules',
         'mpm' => 'prefork',
         'verbose' => undef,
         'modules' => {
                        'mod_authz_user.c' => 1,
                        'prefork.c' => 1,
                        'mod_authn_default.c' => 1,
                        'mod_setenvif.c' => 1,
                        'mod_log_config.c' => 1,
                        'mod_authz_groupfile.c' => 1,
                        'mod_proxy_balancer.c' => 1,
                        'mod_proxy_scgi.c' => 1,
                        'mod_include.c' => 1,
                        'http_core.c' => 1,
                        'mod_actions.c' => 1,
                        'mod_version.c' => 1,
                        'mod_autoindex.c' => 1,
                        'mod_alias.c' => 1,
                        'mod_dir.c' => 1,
                        'mod_so.c' => 1,
                        'mod_env.c' => 1,
                        'mod_asis.c' => 1,
                        'mod_authz_host.c' => 1,
                        'mod_authz_default.c' => 1,
                        'mod_cgi.c' => 1,
                        'mod_mime.c' => 1,
                        'mod_filter.c' => 1,
                        'mod_userdir.c' => 1,
                        'mod_auth_basic.c' => 1,
                        'mod_headers.c' => 1,
                        'mod_proxy.c' => 1,
                        'mod_proxy_http.c' => 1,
                        'mod_proxy_connect.c' => 1,
                        'mod_proxy_ftp.c' => 1,
                        'mod_proxy_ajp.c' => 1,
                        'mod_authn_file.c' => 1,
                        'mod_negotiation.c' => 1,
                        'mod_status.c' => 1,
                        'core.c' => 1
                      }
       }, 'Apache::TestConfig' );
}

1;
