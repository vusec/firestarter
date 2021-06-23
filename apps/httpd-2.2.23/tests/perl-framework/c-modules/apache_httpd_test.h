/* WARNING: this file is generated, do not edit
generated on Mon Dec  7 01:11:48 2020
01: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestConfig.pm:1007
02: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestConfig.pm:1025
03: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestConfigC.pm:445
04: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestConfigC.pm:96
05: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestRun.pm:501
06: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestRun.pm:713
07: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/Apache-Test/lib/Apache/TestRun.pm:713
08: /mnt/hdd/koustubha/repos/apprecovery/apps/httpd-2.2.23/tests/perl-framework/t/TEST:26
 */
#ifndef APACHE_HTTPD_TEST_H
#define APACHE_HTTPD_TEST_H

/* headers present in both 1.x and 2.x */
#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_log.h"
#include "http_main.h"
#include "http_core.h"
#include "ap_config.h"

#ifdef APACHE1
#define AP_METHOD_BIT  1
typedef size_t apr_size_t;
typedef array_header apr_array_header_t;
#define APR_OFF_T_FMT "ld"
#define APR_SIZE_T_FMT "lu"
#endif /* APACHE1 */

#ifdef APACHE2
#ifndef APACHE_HTTPD_TEST_HOOK_ORDER
#define APACHE_HTTPD_TEST_HOOK_ORDER APR_HOOK_MIDDLE
#endif
#include "ap_compat.h"
#endif /* APACHE2 */

#endif /* APACHE_HTTPD_TEST_H */

#ifndef APACHE_HTTPD_TEST_POST_READ_REQUEST
#define APACHE_HTTPD_TEST_POST_READ_REQUEST NULL
#endif
#ifndef APACHE_HTTPD_TEST_TRANSLATE_NAME
#define APACHE_HTTPD_TEST_TRANSLATE_NAME NULL
#endif
#ifndef APACHE_HTTPD_TEST_HEADER_PARSER
#define APACHE_HTTPD_TEST_HEADER_PARSER NULL
#endif
#ifndef APACHE_HTTPD_TEST_ACCESS_CHECKER
#define APACHE_HTTPD_TEST_ACCESS_CHECKER NULL
#endif
#ifndef APACHE_HTTPD_TEST_CHECK_USER_ID
#define APACHE_HTTPD_TEST_CHECK_USER_ID NULL
#endif
#ifndef APACHE_HTTPD_TEST_AUTH_CHECKER
#define APACHE_HTTPD_TEST_AUTH_CHECKER NULL
#endif
#ifndef APACHE_HTTPD_TEST_TYPE_CHECKER
#define APACHE_HTTPD_TEST_TYPE_CHECKER NULL
#endif
#ifndef APACHE_HTTPD_TEST_FIXUPS
#define APACHE_HTTPD_TEST_FIXUPS NULL
#endif
#ifndef APACHE_HTTPD_TEST_HANDLER
#define APACHE_HTTPD_TEST_HANDLER NULL
#endif
#ifndef APACHE_HTTPD_TEST_LOG_TRANSACTION
#define APACHE_HTTPD_TEST_LOG_TRANSACTION NULL
#endif
#ifndef APACHE_HTTPD_TEST_CHILD_INIT
#define APACHE_HTTPD_TEST_CHILD_INIT NULL
#endif
#ifndef APACHE_HTTPD_TEST_PER_DIR_CREATE
#define APACHE_HTTPD_TEST_PER_DIR_CREATE NULL
#endif
#ifndef APACHE_HTTPD_TEST_PER_DIR_MERGE
#define APACHE_HTTPD_TEST_PER_DIR_MERGE NULL
#endif
#ifndef APACHE_HTTPD_TEST_PER_SRV_CREATE
#define APACHE_HTTPD_TEST_PER_SRV_CREATE NULL
#endif
#ifndef APACHE_HTTPD_TEST_PER_SRV_MERGE
#define APACHE_HTTPD_TEST_PER_SRV_MERGE NULL
#endif
#ifndef APACHE_HTTPD_TEST_COMMANDS
#define APACHE_HTTPD_TEST_COMMANDS NULL
#endif
#ifndef APACHE_HTTPD_TEST_EXTRA_HOOKS
#define APACHE_HTTPD_TEST_EXTRA_HOOKS(p) do { } while (0)
#endif
#define APACHE_HTTPD_TEST_MODULE(name) \
    static void name ## _register_hooks(apr_pool_t *p) \
{ \
    if (APACHE_HTTPD_TEST_POST_READ_REQUEST != NULL) \
        ap_hook_post_read_request(APACHE_HTTPD_TEST_POST_READ_REQUEST, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_TRANSLATE_NAME != NULL) \
        ap_hook_translate_name(APACHE_HTTPD_TEST_TRANSLATE_NAME, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_HEADER_PARSER != NULL) \
        ap_hook_header_parser(APACHE_HTTPD_TEST_HEADER_PARSER, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_ACCESS_CHECKER != NULL) \
        ap_hook_access_checker(APACHE_HTTPD_TEST_ACCESS_CHECKER, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_CHECK_USER_ID != NULL) \
        ap_hook_check_user_id(APACHE_HTTPD_TEST_CHECK_USER_ID, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_AUTH_CHECKER != NULL) \
        ap_hook_auth_checker(APACHE_HTTPD_TEST_AUTH_CHECKER, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_TYPE_CHECKER != NULL) \
        ap_hook_type_checker(APACHE_HTTPD_TEST_TYPE_CHECKER, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_FIXUPS != NULL) \
        ap_hook_fixups(APACHE_HTTPD_TEST_FIXUPS, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_HANDLER != NULL) \
        ap_hook_handler(APACHE_HTTPD_TEST_HANDLER, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_LOG_TRANSACTION != NULL) \
        ap_hook_log_transaction(APACHE_HTTPD_TEST_LOG_TRANSACTION, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
     if (APACHE_HTTPD_TEST_CHILD_INIT != NULL) \
        ap_hook_child_init(APACHE_HTTPD_TEST_CHILD_INIT, \
                   NULL, NULL, \
                   APACHE_HTTPD_TEST_HOOK_ORDER); \
 \
    APACHE_HTTPD_TEST_EXTRA_HOOKS(p); \
} \
 \
module AP_MODULE_DECLARE_DATA name ## _module = { \
    STANDARD20_MODULE_STUFF, \
    APACHE_HTTPD_TEST_PER_DIR_CREATE, \
    APACHE_HTTPD_TEST_PER_DIR_MERGE, \
    APACHE_HTTPD_TEST_PER_SRV_CREATE, \
    APACHE_HTTPD_TEST_PER_SRV_MERGE, \
    APACHE_HTTPD_TEST_COMMANDS, \
    name ## _register_hooks, /* register hooks */ \
}
