/*
   Copyright (c) 2000, 2011, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

/**
  @file

  @details
  Mostly this file is used in the server. But a little part of it is used in
  mysqlbinlog too (definition of SELECT_DISTINCT and others).
  The consequence is that 90% of the file is wrapped in \#ifndef MYSQL_CLIENT,
  except the part which must be in the server and in the client.
*/

#ifndef MYSQL_PRIV_H
#define MYSQL_PRIV_H

#ifndef MYSQL_CLIENT

#include <my_global.h>
#include <mysql_version.h>
#include <mysql_embed.h>
#include <my_sys.h>
#include <my_time.h>
#include <m_string.h>
#include <hash.h>
#include <signal.h>
#include <thr_lock.h>
#include <my_base.h>			/* Needed by field.h */
#include <queues.h>
#include "sql_bitmap.h"
#include "sql_array.h"
#include "sql_plugin.h"
#include "scheduler.h"

class Parser_state;

/**
  Query type constants.

  QT_ORDINARY -- ordinary SQL query.
  QT_IS -- SQL query to be shown in INFORMATION_SCHEMA (in utf8 and without
  character set introducers).
*/
enum enum_query_type
{
  QT_ORDINARY,
  QT_IS
};

/* TODO convert all these three maps to Bitmap classes */
typedef ulonglong table_map;          /* Used for table bits in join */
#if MAX_INDEXES <= 64
typedef Bitmap<64>  key_map;          /* Used for finding keys */
#else
typedef Bitmap<((MAX_INDEXES+7)/8*8)> key_map; /* Used for finding keys */
#endif
typedef ulong nesting_map;  /* Used for flags of nesting constructs */
/*
  Used to identify NESTED_JOIN structures within a join (applicable only to
  structures that have not been simplified away and embed more the one
  element)
*/
typedef ulonglong nested_join_map;

/* query_id */
typedef ulonglong query_id_t;
extern query_id_t global_query_id;

/* increment query_id and return it.  */
inline query_id_t next_query_id() { return global_query_id++; }

/* useful constants */
extern MYSQL_PLUGIN_IMPORT const key_map key_map_empty;
extern MYSQL_PLUGIN_IMPORT key_map key_map_full;          /* Should be threaded as const */
extern MYSQL_PLUGIN_IMPORT const char *primary_key_name;

#include "mysql_com.h"
#include <violite.h>
#include "unireg.h"

void init_sql_alloc(MEM_ROOT *root, uint block_size, uint pre_alloc_size);
void *sql_alloc(size_t);
void *sql_calloc(size_t);
char *sql_strdup(const char *str);
char *sql_strmake(const char *str, size_t len);
void *sql_memdup(const void * ptr, size_t size);
void sql_element_free(void *ptr);
char *sql_strmake_with_convert(const char *str, size_t arg_length,
			       CHARSET_INFO *from_cs,
			       size_t max_res_length,
			       CHARSET_INFO *to_cs, size_t *result_length);
uint kill_one_thread(THD *thd, ulong id, bool only_kill_query);
void sql_kill(THD *thd, ulong id, bool only_kill_query);
bool net_request_file(NET* net, const char* fname);
char* query_table_status(THD *thd,const char *db,const char *table_name);

#define x_free(A)	{ my_free((uchar*) (A),MYF(MY_WME | MY_FAE | MY_ALLOW_ZERO_PTR)); }
#define safeFree(x)	{ if(x) { my_free((uchar*) x,MYF(0)); x = NULL; } }
#define PREV_BITS(type,A)	((type) (((type) 1 << (A)) -1))
#define all_bits_set(A,B) ((A) & (B) != (B))

/* Version numbers for deprecation messages */
#define VER_BETONY  "5.5"
#define VER_CELOSIA "5.6"

#define WARN_DEPRECATED(Thd,Ver,Old,New)                                             \
  do {                                                                               \
    DBUG_ASSERT(strncmp(Ver, MYSQL_SERVER_VERSION, sizeof(Ver)-1) > 0);              \
    if (((uchar*)Thd) != NULL)                                                       \
      push_warning_printf(((THD *)Thd), MYSQL_ERROR::WARN_LEVEL_WARN,                \
                        ER_WARN_DEPRECATED_SYNTAX,                                   \
                        ER(ER_WARN_DEPRECATED_SYNTAX),                               \
                        (Old), (New));                                               \
    else                                                                             \
      sql_print_warning("'%s' is deprecated and will be removed "                    \
                        "in a future release. Please use '%s' instead.",             \
                        (Old), (New));                                               \
  } while(0)

extern MYSQL_PLUGIN_IMPORT CHARSET_INFO *system_charset_info;
extern MYSQL_PLUGIN_IMPORT CHARSET_INFO *files_charset_info ;
extern MYSQL_PLUGIN_IMPORT CHARSET_INFO *national_charset_info;
extern MYSQL_PLUGIN_IMPORT CHARSET_INFO *table_alias_charset;


enum Derivation
{
  DERIVATION_IGNORABLE= 5,
  DERIVATION_COERCIBLE= 4,
  DERIVATION_SYSCONST= 3,
  DERIVATION_IMPLICIT= 2,
  DERIVATION_NONE= 1,
  DERIVATION_EXPLICIT= 0
};


typedef struct my_locale_st
{
  uint  number;
  const char *name;
  const char *description;
  const bool is_ascii;
  TYPELIB *month_names;
  TYPELIB *ab_month_names;
  TYPELIB *day_names;
  TYPELIB *ab_day_names;
  uint max_month_name_length;
  uint max_day_name_length;
#ifdef __cplusplus 
  my_locale_st(uint number_par,
               const char *name_par, const char *descr_par, bool is_ascii_par,
               TYPELIB *month_names_par, TYPELIB *ab_month_names_par,
               TYPELIB *day_names_par, TYPELIB *ab_day_names_par,
               uint max_month_name_length_par, uint max_day_name_length_par) : 
    number(number_par),
    name(name_par), description(descr_par), is_ascii(is_ascii_par),
    month_names(month_names_par), ab_month_names(ab_month_names_par),
    day_names(day_names_par), ab_day_names(ab_day_names_par),
    max_month_name_length(max_month_name_length_par),
    max_day_name_length(max_day_name_length_par)
  {}
#endif
} MY_LOCALE;

extern MY_LOCALE my_locale_en_US;
extern MY_LOCALE *my_locales[];
extern MY_LOCALE *my_default_lc_time_names;

MY_LOCALE *my_locale_by_name(const char *name);
MY_LOCALE *my_locale_by_number(uint number);

/*************************************************************************/

/**
 Object_creation_ctx -- interface for creation context of database objects
 (views, stored routines, events, triggers). Creation context -- is a set
 of attributes, that should be fixed at the creation time and then be used
 each time the object is parsed or executed.
*/

class Object_creation_ctx
{
public:
  Object_creation_ctx *set_n_backup(THD *thd);

  void restore_env(THD *thd, Object_creation_ctx *backup_ctx);

protected:
  Object_creation_ctx() {}
  virtual Object_creation_ctx *create_backup_ctx(THD *thd) const = 0;

  virtual void change_env(THD *thd) const = 0;

public:
  virtual ~Object_creation_ctx()
  { }
};

/*************************************************************************/

/**
 Default_object_creation_ctx -- default implementation of
 Object_creation_ctx.
*/

class Default_object_creation_ctx : public Object_creation_ctx
{
public:
  CHARSET_INFO *get_client_cs()
  {
    return m_client_cs;
  }

  CHARSET_INFO *get_connection_cl()
  {
    return m_connection_cl;
  }

protected:
  Default_object_creation_ctx(THD *thd);

  Default_object_creation_ctx(CHARSET_INFO *client_cs,
                              CHARSET_INFO *connection_cl);

protected:
  virtual Object_creation_ctx *create_backup_ctx(THD *thd) const;

  virtual void change_env(THD *thd) const;

protected:
  /**
    client_cs stores the value of character_set_client session variable.
    The only character set attribute is used.

    Client character set is included into query context, because we save
    query in the original character set, which is client character set. So,
    in order to parse the query properly we have to switch client character
    set on parsing.
  */
  CHARSET_INFO *m_client_cs;

  /**
    connection_cl stores the value of collation_connection session
    variable. Both character set and collation attributes are used.

    Connection collation is included into query context, becase it defines
    the character set and collation of text literals in internal
    representation of query (item-objects).
  */
  CHARSET_INFO *m_connection_cl;
};

/***************************************************************************
  Configuration parameters
****************************************************************************/

#define ACL_CACHE_SIZE		256
#define MAX_PASSWORD_LENGTH	32
#define HOST_CACHE_SIZE		128
#define MAX_ACCEPT_RETRY	10	// Test accept this many times
#define MAX_FIELDS_BEFORE_HASH	32
#define USER_VARS_HASH_SIZE     16
#define TABLE_OPEN_CACHE_MIN    64
#define TABLE_OPEN_CACHE_DEFAULT 64
#define TABLE_DEF_CACHE_DEFAULT 256
/**
  We must have room for at least 256 table definitions in the table
  cache, since otherwise there is no chance prepared
  statements that use these many tables can work.
  Prepared statements use table definition cache ids (table_map_id)
  as table version identifiers. If the table definition
  cache size is less than the number of tables used in a statement,
  the contents of the table definition cache is guaranteed to rotate
  between a prepare and execute. This leads to stable validation
  errors. In future we shall use more stable version identifiers,
  for now the only solution is to ensure that the table definition
  cache can contain at least all tables of a given statement.
*/
#define TABLE_DEF_CACHE_MIN     256

/*
  Stack reservation.
  Feel free to raise this by the smallest amount you can to get the
  "execution_constants" test to pass.
*/
#define STACK_MIN_SIZE          16000   // Abort if less stack during eval.

#define STACK_MIN_SIZE_FOR_OPEN 1024*80
#define STACK_BUFF_ALLOC        352     ///< For stack overrun checks
#ifndef MYSQLD_NET_RETRY_COUNT
#define MYSQLD_NET_RETRY_COUNT  10	///< Abort read after this many int.
#endif
#define TEMP_POOL_SIZE          128

#define QUERY_ALLOC_BLOCK_SIZE		8192
#define QUERY_ALLOC_PREALLOC_SIZE   	8192
#define TRANS_ALLOC_BLOCK_SIZE		4096
#define TRANS_ALLOC_PREALLOC_SIZE	4096
#define RANGE_ALLOC_BLOCK_SIZE		4096
#define ACL_ALLOC_BLOCK_SIZE		1024
#define UDF_ALLOC_BLOCK_SIZE		1024
#define TABLE_ALLOC_BLOCK_SIZE		1024
#define BDB_LOG_ALLOC_BLOCK_SIZE	1024
#define WARN_ALLOC_BLOCK_SIZE		2048
#define WARN_ALLOC_PREALLOC_SIZE	1024
#define PROFILE_ALLOC_BLOCK_SIZE  2048
#define PROFILE_ALLOC_PREALLOC_SIZE 1024

/*
  The following parameters is to decide when to use an extra cache to
  optimise seeks when reading a big table in sorted order
*/
#define MIN_FILE_LENGTH_TO_USE_ROW_CACHE (10L*1024*1024)
#define MIN_ROWS_TO_USE_TABLE_CACHE	 100
#define MIN_ROWS_TO_USE_BULK_INSERT	 100

/**
  The following is used to decide if MySQL should use table scanning
  instead of reading with keys.  The number says how many evaluation of the
  WHERE clause is comparable to reading one extra row from a table.
*/
#define TIME_FOR_COMPARE   5	// 5 compares == one read

/**
  Number of comparisons of table rowids equivalent to reading one row from a 
  table.
*/
#define TIME_FOR_COMPARE_ROWID  (TIME_FOR_COMPARE*2)

/*
  For sequential disk seeks the cost formula is:
    DISK_SEEK_BASE_COST + DISK_SEEK_PROP_COST * #blocks_to_skip  
  
  The cost of average seek 
    DISK_SEEK_BASE_COST + DISK_SEEK_PROP_COST*BLOCKS_IN_AVG_SEEK =1.0.
*/
#define DISK_SEEK_BASE_COST ((double)0.5)

#define BLOCKS_IN_AVG_SEEK  128

#define DISK_SEEK_PROP_COST ((double)0.5/BLOCKS_IN_AVG_SEEK)


/**
  Number of rows in a reference table when refereed through a not unique key.
  This value is only used when we don't know anything about the key
  distribution.
*/
#define MATCHING_ROWS_IN_OTHER_TABLE 10

/** Don't pack string keys shorter than this (if PACK_KEYS=1 isn't used). */
#define KEY_DEFAULT_PACK_LENGTH 8

/** Characters shown for the command in 'show processlist'. */
#define PROCESS_LIST_WIDTH 100
/* Characters shown for the command in 'information_schema.processlist' */
#define PROCESS_LIST_INFO_WIDTH 65535

#define PRECISION_FOR_DOUBLE 53
#define PRECISION_FOR_FLOAT  24

/* -[digits].E+## */
#define MAX_FLOAT_STR_LENGTH (FLT_DIG + 6)
/* -[digits].E+### */
#define MAX_DOUBLE_STR_LENGTH (DBL_DIG + 7)

/*
  Default time to wait before aborting a new client connection
  that does not respond to "initial server greeting" timely
*/
#define CONNECT_TIMEOUT		10

/* The following can also be changed from the command line */
#define DEFAULT_CONCURRENCY	10
#define DELAYED_LIMIT		100		/**< pause after xxx inserts */
#define DELAYED_QUEUE_SIZE	1000
#define DELAYED_WAIT_TIMEOUT	5*60		/**< Wait for delayed insert */
#define FLUSH_TIME		0		/**< Don't flush tables */
#define MAX_CONNECT_ERRORS	10		///< errors before disabling host

#ifdef __NETWARE__
#define IF_NETWARE(A,B) A
#else
#define IF_NETWARE(A,B) B
#endif

#if defined(__WIN__)
#undef	FLUSH_TIME
#define FLUSH_TIME	1800			/**< Flush every half hour */

#define INTERRUPT_PRIOR -2
#define CONNECT_PRIOR	-1
#define WAIT_PRIOR	0
#define QUERY_PRIOR	2
#else
#define INTERRUPT_PRIOR 10
#define CONNECT_PRIOR	9
#define WAIT_PRIOR	8
#define QUERY_PRIOR	6
#endif /* __WIN92__ */

	/* Bits from testflag */
#define TEST_PRINT_CACHED_TABLES 1
#define TEST_NO_KEY_GROUP	 2
#define TEST_MIT_THREAD		4
#define TEST_BLOCKING		8
#define TEST_KEEP_TMP_TABLES	16
#define TEST_READCHECK		64	/**< Force use of readcheck */
#define TEST_NO_EXTRA		128
#define TEST_CORE_ON_SIGNAL	256	/**< Give core if signal */
#define TEST_NO_STACKTRACE	512
#define TEST_SIGINT		1024	/**< Allow sigint on threads */
#define TEST_SYNCHRONIZATION    2048    /**< get server to do sleep in
                                           some places */
#endif

/*
   This is included in the server and in the client.
   Options for select set by the yacc parser (stored in lex->options).

   XXX:
   log_event.h defines OPTIONS_WRITTEN_TO_BIN_LOG to specify what THD
   options list are written into binlog. These options can NOT change their
   values, or it will break replication between version.

   context is encoded as following:
   SELECT - SELECT_LEX_NODE::options
   THD    - THD::options
   intern - neither. used only as
            func(..., select_node->options | thd->options | OPTION_XXX, ...)

   TODO: separate three contexts above, move them to separate bitfields.
*/

#define SELECT_DISTINCT         (ULL(1) << 0)     // SELECT, user
#define SELECT_STRAIGHT_JOIN    (ULL(1) << 1)     // SELECT, user
#define SELECT_DESCRIBE         (ULL(1) << 2)     // SELECT, user
#define SELECT_SMALL_RESULT     (ULL(1) << 3)     // SELECT, user
#define SELECT_BIG_RESULT       (ULL(1) << 4)     // SELECT, user
#define OPTION_FOUND_ROWS       (ULL(1) << 5)     // SELECT, user
#define OPTION_TO_QUERY_CACHE   (ULL(1) << 6)     // SELECT, user
#define SELECT_NO_JOIN_CACHE    (ULL(1) << 7)     // intern
#define OPTION_BIG_TABLES       (ULL(1) << 8)     // THD, user
#define OPTION_BIG_SELECTS      (ULL(1) << 9)     // THD, user
#define OPTION_LOG_OFF          (ULL(1) << 10)    // THD, user
#define OPTION_QUOTE_SHOW_CREATE (ULL(1) << 11)   // THD, user, unused
#define TMP_TABLE_ALL_COLUMNS   (ULL(1) << 12)    // SELECT, intern
#define OPTION_WARNINGS         (ULL(1) << 13)    // THD, user
#define OPTION_AUTO_IS_NULL     (ULL(1) << 14)    // THD, user, binlog
#define OPTION_FOUND_COMMENT    (ULL(1) << 15)    // SELECT, intern, parser
#define OPTION_SAFE_UPDATES     (ULL(1) << 16)    // THD, user
#define OPTION_BUFFER_RESULT    (ULL(1) << 17)    // SELECT, user
#define OPTION_BIN_LOG          (ULL(1) << 18)    // THD, user
#define OPTION_NOT_AUTOCOMMIT   (ULL(1) << 19)    // THD, user
#define OPTION_BEGIN            (ULL(1) << 20)    // THD, intern
#define OPTION_TABLE_LOCK       (ULL(1) << 21)    // THD, intern
#define OPTION_QUICK            (ULL(1) << 22)    // SELECT (for DELETE)
#define OPTION_KEEP_LOG         (ULL(1) << 23)    // THD, user

/* The following is used to detect a conflict with DISTINCT */
#define SELECT_ALL              (ULL(1) << 24)    // SELECT, user, parser

/** The following can be set when importing tables in a 'wrong order'
   to suppress foreign key checks */
#define OPTION_NO_FOREIGN_KEY_CHECKS    (ULL(1) << 26) // THD, user, binlog
/** The following speeds up inserts to InnoDB tables by suppressing unique
   key checks in some cases */
#define OPTION_RELAXED_UNIQUE_CHECKS    (ULL(1) << 27) // THD, user, binlog
#define SELECT_NO_UNLOCK                (ULL(1) << 28) // SELECT, intern
#define OPTION_SCHEMA_TABLE             (ULL(1) << 29) // SELECT, intern
/** Flag set if setup_tables already done */
#define OPTION_SETUP_TABLES_DONE        (ULL(1) << 30) // intern
/** If not set then the thread will ignore all warnings with level notes. */
#define OPTION_SQL_NOTES                (ULL(1) << 31) // THD, user
/**
  Force the used temporary table to be a MyISAM table (because we will use
  fulltext functions when reading from it.
*/
#define TMP_TABLE_FORCE_MYISAM          (ULL(1) << 32)
#define OPTION_PROFILING                (ULL(1) << 33)



/**
  Maximum length of time zone name that we support
  (Time zone name is char(64) in db). mysqlbinlog needs it.
*/
#define MAX_TIME_ZONE_NAME_LENGTH       (NAME_LEN + 1)

/* The rest of the file is included in the server only */
#ifndef MYSQL_CLIENT

/* Bits for different SQL modes modes (including ANSI mode) */
#define MODE_REAL_AS_FLOAT              1
#define MODE_PIPES_AS_CONCAT            2
#define MODE_ANSI_QUOTES                4
#define MODE_IGNORE_SPACE		8
#define MODE_NOT_USED			16
#define MODE_ONLY_FULL_GROUP_BY		32
#define MODE_NO_UNSIGNED_SUBTRACTION	64
#define MODE_NO_DIR_IN_CREATE		128
#define MODE_POSTGRESQL			256
#define MODE_ORACLE			512
#define MODE_MSSQL			1024
#define MODE_DB2			2048
#define MODE_MAXDB			4096
#define MODE_NO_KEY_OPTIONS             8192
#define MODE_NO_TABLE_OPTIONS           16384
#define MODE_NO_FIELD_OPTIONS           32768
#define MODE_MYSQL323                   65536L
#define MODE_MYSQL40                    (MODE_MYSQL323*2)
#define MODE_ANSI	                (MODE_MYSQL40*2)
#define MODE_NO_AUTO_VALUE_ON_ZERO      (MODE_ANSI*2)
#define MODE_NO_BACKSLASH_ESCAPES       (MODE_NO_AUTO_VALUE_ON_ZERO*2)
#define MODE_STRICT_TRANS_TABLES	(MODE_NO_BACKSLASH_ESCAPES*2)
#define MODE_STRICT_ALL_TABLES		(MODE_STRICT_TRANS_TABLES*2)
#define MODE_NO_ZERO_IN_DATE		(MODE_STRICT_ALL_TABLES*2)
#define MODE_NO_ZERO_DATE		(MODE_NO_ZERO_IN_DATE*2)
#define MODE_INVALID_DATES		(MODE_NO_ZERO_DATE*2)
#define MODE_ERROR_FOR_DIVISION_BY_ZERO (MODE_INVALID_DATES*2)
#define MODE_TRADITIONAL		(MODE_ERROR_FOR_DIVISION_BY_ZERO*2)
#define MODE_NO_AUTO_CREATE_USER	(MODE_TRADITIONAL*2)
#define MODE_HIGH_NOT_PRECEDENCE	(MODE_NO_AUTO_CREATE_USER*2)
#define MODE_NO_ENGINE_SUBSTITUTION     (MODE_HIGH_NOT_PRECEDENCE*2)
#define MODE_PAD_CHAR_TO_FULL_LENGTH    (ULL(1) << 31)

/* @@optimizer_switch flags. These must be in sync with optimizer_switch_typelib */
#define OPTIMIZER_SWITCH_INDEX_MERGE 1
#define OPTIMIZER_SWITCH_INDEX_MERGE_UNION 2
#define OPTIMIZER_SWITCH_INDEX_MERGE_SORT_UNION 4
#define OPTIMIZER_SWITCH_INDEX_MERGE_INTERSECT 8
#define OPTIMIZER_SWITCH_LAST 16

/* The following must be kept in sync with optimizer_switch_str in mysqld.cc */
#define OPTIMIZER_SWITCH_DEFAULT (OPTIMIZER_SWITCH_INDEX_MERGE | \
                                  OPTIMIZER_SWITCH_INDEX_MERGE_UNION | \
                                  OPTIMIZER_SWITCH_INDEX_MERGE_SORT_UNION | \
                                  OPTIMIZER_SWITCH_INDEX_MERGE_INTERSECT)


/*
  Replication uses 8 bytes to store SQL_MODE in the binary log. The day you
  use strictly more than 64 bits by adding one more define above, you should
  contact the replication team because the replication code should then be
  updated (to store more bytes on disk).

  NOTE: When adding new SQL_MODE types, make sure to also add them to
  the scripts used for creating the MySQL system tables
  in scripts/mysql_system_tables.sql and scripts/mysql_system_tables_fix.sql

*/

#define RAID_BLOCK_SIZE 1024

#define MY_CHARSET_BIN_MB_MAXLEN 1

/*
  Flags below are set when we perform
  context analysis of the statement and make
  subqueries non-const. It prevents subquery
  evaluation at context analysis stage.
*/

/*
  Don't evaluate this subquery during statement prepare even if
  it's a constant one. The flag is switched off in the end of
  mysqld_stmt_prepare.
*/ 
#define CONTEXT_ANALYSIS_ONLY_PREPARE 1
/*
  Special JOIN::prepare mode: changing of query is prohibited.
  When creating a view, we need to just check its syntax omitting
  any optimizations: afterwards definition of the view will be
  reconstructed by means of ::print() methods and written to
  to an .frm file. We need this definition to stay untouched.
*/ 
#define CONTEXT_ANALYSIS_ONLY_VIEW    2
/*
  Don't evaluate this subquery during derived table prepare even if
  it's a constant one.
*/
#define CONTEXT_ANALYSIS_ONLY_DERIVED 4

// uncachable cause
#define UNCACHEABLE_DEPENDENT   1
#define UNCACHEABLE_RAND        2
#define UNCACHEABLE_SIDEEFFECT	4
/// forcing to save JOIN for explain
#define UNCACHEABLE_EXPLAIN     8
/* For uncorrelated SELECT in an UNION with some correlated SELECTs */
#define UNCACHEABLE_UNITED     16
#define UNCACHEABLE_CHECKOPTION 32

/* Used to check GROUP BY list in the MODE_ONLY_FULL_GROUP_BY mode */
#define UNDEF_POS (-1)

/* BINLOG_DUMP options */

#define BINLOG_DUMP_NON_BLOCK   1

/* sql_show.cc:show_log_files() */
#define SHOW_LOG_STATUS_FREE "FREE"
#define SHOW_LOG_STATUS_INUSE "IN USE"

struct TABLE_LIST;
class String;
void view_store_options(THD *thd, TABLE_LIST *table, String *buff);

/* Options to add_table_to_list() */
#define TL_OPTION_UPDATING	1
#define TL_OPTION_FORCE_INDEX	2
#define TL_OPTION_IGNORE_LEAVES 4
#define TL_OPTION_ALIAS         8

/* Some portable defines */

#define portable_sizeof_char_ptr 8

#define tmp_file_prefix "#sql"			/**< Prefix for tmp tables */
#define tmp_file_prefix_length 4

/* Flags for calc_week() function.  */
#define WEEK_MONDAY_FIRST    1
#define WEEK_YEAR            2
#define WEEK_FIRST_WEEKDAY   4

#define STRING_BUFFER_USUAL_SIZE 80

/*
  Some defines for exit codes for ::is_equal class functions.
*/
#define IS_EQUAL_NO 0
#define IS_EQUAL_YES 1
#define IS_EQUAL_PACK_LENGTH 2

enum enum_parsing_place
{
  NO_MATTER,
  IN_HAVING,
  SELECT_LIST,
  IN_WHERE,
  IN_ON
};

struct st_table;

#define thd_proc_info(thd, msg)  set_thd_proc_info(thd, msg, __func__, __FILE__, __LINE__)
class THD;

enum enum_check_fields
{
  CHECK_FIELD_IGNORE,
  CHECK_FIELD_WARN,
  CHECK_FIELD_ERROR_FOR_NULL
};

#if defined(MYSQL_DYNAMIC_PLUGIN) && defined(_WIN32)
extern "C" THD *_current_thd_noinline();
#define _current_thd() _current_thd_noinline()
#else
/*
  THR_THD is a key which will be used to set/get THD* for a thread,
  using my_pthread_setspecific_ptr()/my_thread_getspecific_ptr().
*/
extern pthread_key(THD*, THR_THD);
inline THD *_current_thd(void)
{
  return my_pthread_getspecific_ptr(THD*,THR_THD);
}
#endif
#define current_thd _current_thd()


/** 
  The meat of thd_proc_info(THD*, char*), a macro that packs the last
  three calling-info parameters. 
*/
extern "C"
const char *set_thd_proc_info(THD *thd, const char *info, 
                              const char *calling_func, 
                              const char *calling_file, 
                              const unsigned int calling_line);

/**
  Enumerate possible types of a table from re-execution
  standpoint.
  TABLE_LIST class has a member of this type.
  At prepared statement prepare, this member is assigned a value
  as of the current state of the database. Before (re-)execution
  of a prepared statement, we check that the value recorded at
  prepare matches the type of the object we obtained from the
  table definition cache.

  @sa check_and_update_table_version()
  @sa Execute_observer
  @sa Prepared_statement::reprepare()
*/

enum enum_table_ref_type
{
  /** Initial value set by the parser */
  TABLE_REF_NULL= 0,
  TABLE_REF_VIEW,
  TABLE_REF_BASE_TABLE,
  TABLE_REF_I_S_TABLE,
  TABLE_REF_TMP_TABLE
};

/*
  External variables
*/
extern ulong server_id, concurrency;


typedef my_bool (*qc_engine_callback)(THD *thd, char *table_key,
                                      uint key_length,
                                      ulonglong *engine_data);
#include "sql_string.h"
#include "sql_list.h"
#include "sql_map.h"
#include "my_decimal.h"
#include "handler.h"
#include "parse_file.h"
#include "table.h"
#include "sql_error.h"
#include "field.h"				/* Field definitions */
#include "protocol.h"
#include "sql_udf.h"
#include "sql_profile.h"
#include "sql_partition.h"

class user_var_entry;
class Security_context;
enum enum_var_type
{
  OPT_DEFAULT= 0, OPT_SESSION, OPT_GLOBAL
};
class sys_var;
#ifdef MYSQL_SERVER
class Comp_creator;
typedef Comp_creator* (*chooser_compare_func_creator)(bool invert);
#endif
#include "item.h"
extern my_decimal decimal_zero;

/* sql_parse.cc */
void free_items(Item *item);
void cleanup_items(Item *item);
class THD;
void close_thread_tables(THD *thd);

#ifndef NO_EMBEDDED_ACCESS_CHECKS
bool check_one_table_access(THD *thd, ulong privilege, TABLE_LIST *tables);
bool check_single_table_access(THD *thd, ulong privilege,
			   TABLE_LIST *tables, bool no_errors);
bool check_routine_access(THD *thd,ulong want_access,char *db,char *name,
			  bool is_proc, bool no_errors);
bool check_some_access(THD *thd, ulong want_access, TABLE_LIST *table);
bool check_some_routine_access(THD *thd, const char *db, const char *name, bool is_proc);
#else
inline bool check_one_table_access(THD *thd, ulong privilege, TABLE_LIST *tables)
{ return false; }
inline bool check_single_table_access(THD *thd, ulong privilege,
			   TABLE_LIST *tables, bool no_errors)
{ return false; }
inline bool check_routine_access(THD *thd,ulong want_access,char *db,
                                 char *name, bool is_proc, bool no_errors)
{ return false; }
inline bool check_some_access(THD *thd, ulong want_access, TABLE_LIST *table)
{ return false; }
inline bool check_merge_table_access(THD *thd, char *db, TABLE_LIST *table_list)
{ return false; }
inline bool check_some_routine_access(THD *thd, const char *db,
                                      const char *name, bool is_proc)
{ return false; }
#endif /*NO_EMBEDDED_ACCESS_CHECKS*/

bool multi_update_precheck(THD *thd, TABLE_LIST *tables);
bool multi_delete_precheck(THD *thd, TABLE_LIST *tables);
int mysql_multi_update_prepare(THD *thd);
int mysql_multi_delete_prepare(THD *thd);
bool mysql_insert_select_prepare(THD *thd);
bool update_precheck(THD *thd, TABLE_LIST *tables);
bool delete_precheck(THD *thd, TABLE_LIST *tables);
bool insert_precheck(THD *thd, TABLE_LIST *tables);
bool create_table_precheck(THD *thd, TABLE_LIST *tables,
                           TABLE_LIST *create_table);
int append_query_string(THD *thd, CHARSET_INFO *csinfo,
                        String const *from, String *to);

void get_default_definer(THD *thd, LEX_USER *definer);
LEX_USER *create_default_definer(THD *thd);
LEX_USER *create_definer(THD *thd, LEX_STRING *user_name, LEX_STRING *host_name);
LEX_USER *get_current_user(THD *thd, LEX_USER *user);
bool check_string_byte_length(LEX_STRING *str, const char *err_msg,
                              uint max_byte_length);
bool check_string_char_length(LEX_STRING *str, const char *err_msg,
                              uint max_char_length, CHARSET_INFO *cs,
                              bool no_error);
bool check_host_name(LEX_STRING *str);

bool parse_sql(THD *thd,
               Parser_state *parser_state,
               Object_creation_ctx *creation_ctx);

enum enum_mysql_completiontype {
  ROLLBACK_RELEASE=-2, ROLLBACK=1,  ROLLBACK_AND_CHAIN=7,
  COMMIT_RELEASE=-1,   COMMIT=0,    COMMIT_AND_CHAIN=6
};

bool begin_trans(THD *thd);
bool end_active_trans(THD *thd);
int end_trans(THD *thd, enum enum_mysql_completiontype completion);

Item *negate_expression(THD *thd, Item *expr);

/* log.cc */
int vprint_msg_to_log(enum loglevel level, const char *format, va_list args);
void sql_print_error(const char *format, ...) ATTRIBUTE_FORMAT(printf, 1, 2);
void sql_print_warning(const char *format, ...) ATTRIBUTE_FORMAT(printf, 1, 2);
void sql_print_information(const char *format, ...)
  ATTRIBUTE_FORMAT(printf, 1, 2);
typedef void (*sql_print_message_func)(const char *format, ...)
  ATTRIBUTE_FORMAT_FPTR(printf, 1, 2);
extern sql_print_message_func sql_print_message_handlers[];

int error_log_print(enum loglevel level, const char *format,
                    va_list args);

bool slow_log_print(THD *thd, const char *query, uint query_length,
                    ulonglong current_utime);

bool general_log_print(THD *thd, enum enum_server_command command,
                       const char *format,...);

bool general_log_write(THD *thd, enum enum_server_command command,
                       const char *query, uint query_length);

#include "sql_class.h"
#include "sql_acl.h"
#include "tztime.h"
#ifdef MYSQL_SERVER
#include "sql_servers.h"
#include "opt_range.h"

#ifdef HAVE_QUERY_CACHE
struct Query_cache_query_flags
{
  unsigned int client_long_flag:1;
  unsigned int client_protocol_41:1;
  unsigned int result_in_binary_protocol:1;
  unsigned int more_results_exists:1;
  unsigned int in_trans:1;
  unsigned int autocommit:1;
  unsigned int pkt_nr;
  uint character_set_client_num;
  uint character_set_results_num;
  uint collation_connection_num;
  ha_rows limit;
  Time_zone *time_zone;
  ulong sql_mode;
  ulong max_sort_length;
  ulong group_concat_max_len;
  ulong default_week_format;
  ulong div_precision_increment;
  MY_LOCALE *lc_time_names;
};
#define QUERY_CACHE_FLAGS_SIZE sizeof(Query_cache_query_flags)
#include "sql_cache.h"
#define query_cache_store_query(A, B) query_cache.store_query(A, B)
#define query_cache_destroy() query_cache.destroy()
#define query_cache_result_size_limit(A) query_cache.result_size_limit(A)
#define query_cache_init() query_cache.init()
#define query_cache_resize(A) query_cache.resize(A)
#define query_cache_set_min_res_unit(A) query_cache.set_min_res_unit(A)
#define query_cache_invalidate3(A, B, C) query_cache.invalidate(A, B, C)
#define query_cache_invalidate1(A) query_cache.invalidate(A)
#define query_cache_send_result_to_client(A, B, C) \
  query_cache.send_result_to_client(A, B, C)
#define query_cache_invalidate_by_MyISAM_filename_ref \
  &query_cache_invalidate_by_MyISAM_filename
/* note the "maybe": it's a read without mutex */
#define query_cache_maybe_disabled(T)                                 \
  (T->variables.query_cache_type == 0 || query_cache.query_cache_size == 0)
#define query_cache_is_cacheable_query(L) \
  (((L)->sql_command == SQLCOM_SELECT) && (L)->safe_to_cache_query)
#else
#define QUERY_CACHE_FLAGS_SIZE 0
#define query_cache_store_query(A, B)
#define query_cache_destroy()
#define query_cache_result_size_limit(A)
#define query_cache_init()
#define query_cache_resize(A)
#define query_cache_set_min_res_unit(A)
#define query_cache_invalidate3(A, B, C)
#define query_cache_invalidate1(A)
#define query_cache_send_result_to_client(A, B, C) 0
#define query_cache_invalidate_by_MyISAM_filename_ref NULL

#define query_cache_abort(A)
#define query_cache_end_of_result(A)
#define query_cache_invalidate_by_MyISAM_filename_ref NULL
#define query_cache_maybe_disabled(T) 1
#define query_cache_is_cacheable_query(L) 0
#endif /*HAVE_QUERY_CACHE*/

/*
  Error injector Macros to enable easy testing of recovery after failures
  in various error cases.
*/
#ifndef ERROR_INJECT_SUPPORT

#define ERROR_INJECT(x) 0
#define ERROR_INJECT_ACTION(x,action) 0
#define ERROR_INJECT_CRASH(x) 0
#define ERROR_INJECT_VALUE(x) 0
#define ERROR_INJECT_VALUE_ACTION(x,action) 0
#define ERROR_INJECT_VALUE_CRASH(x) 0
#define SET_ERROR_INJECT_VALUE(x)

#else

inline bool check_and_unset_keyword(const char *dbug_str)
{
  const char *extra_str= "-d,";
  char total_str[200];
  if (_db_strict_keyword_ (dbug_str))
  {
    strxmov(total_str, extra_str, dbug_str, NullS);
    DBUG_SET(total_str);
    return 1;
  }
  return 0;
}


inline bool
check_and_unset_inject_value(int value)
{
  THD *thd= current_thd;
  if (thd->error_inject_value == (uint)value)
  {
    thd->error_inject_value= 0;
    return 1;
  }
  return 0;
}

/*
  ERROR INJECT MODULE:
  --------------------
  These macros are used to insert macros from the application code.
  The event that activates those error injections can be activated
  from SQL by using:
  SET SESSION dbug=+d,code;

  After the error has been injected, the macros will automatically
  remove the debug code, thus similar to using:
  SET SESSION dbug=-d,code
  from SQL.

  ERROR_INJECT_CRASH will inject a crash of the MySQL Server if code
  is set when macro is called. ERROR_INJECT_CRASH can be used in
  if-statements, it will always return FALSE unless of course it
  crashes in which case it doesn't return at all.

  ERROR_INJECT_ACTION will inject the action specified in the action
  parameter of the macro, before performing the action the code will
  be removed such that no more events occur. ERROR_INJECT_ACTION
  can also be used in if-statements and always returns FALSE.
  ERROR_INJECT can be used in a normal if-statement, where the action
  part is performed in the if-block. The macro returns TRUE if the
  error was activated and otherwise returns FALSE. If activated the
  code is removed.

  Sometimes it is necessary to perform error inject actions as a serie
  of events. In this case one can use one variable on the THD object.
  Thus one sets this value by using e.g. SET_ERROR_INJECT_VALUE(100).
  Then one can later test for it by using ERROR_INJECT_CRASH_VALUE,
  ERROR_INJECT_ACTION_VALUE and ERROR_INJECT_VALUE. This have the same
  behaviour as the above described macros except that they use the
  error inject value instead of a code used by DBUG macros.
*/
#define SET_ERROR_INJECT_VALUE(x) \
  current_thd->error_inject_value= (x)
#define ERROR_INJECT_CRASH(code) \
  DBUG_EVALUATE_IF(code, (abort(), 0), 0)
#define ERROR_INJECT_ACTION(code, action) \
  (check_and_unset_keyword(code) ? ((action), 0) : 0)
#define ERROR_INJECT(code) \
  check_and_unset_keyword(code)
#define ERROR_INJECT_VALUE(value) \
  check_and_unset_inject_value(value)
#define ERROR_INJECT_VALUE_ACTION(value,action) \
  (check_and_unset_inject_value(value) ? (action) : 0)
#define ERROR_INJECT_VALUE_CRASH(value) \
  ERROR_INJECT_VALUE_ACTION(value, (abort(), 0))

#endif

int write_bin_log(THD *thd, bool clear_error,
                  char const *query, ulong query_length);

/* sql_connect.cc */
int check_user(THD *thd, enum enum_server_command command, 
	       const char *passwd, uint passwd_len, const char *db,
	       bool check_count);
pthread_handler_t handle_one_connection(void *arg);
bool init_new_connection_handler_thread();
void reset_mqh(LEX_USER *lu, bool get_them);
bool check_mqh(THD *thd, uint check_command);
void time_out_user_resource_limits(THD *thd, USER_CONN *uc);
void decrease_user_connections(USER_CONN *uc);
bool thd_init_client_charset(THD *thd, uint cs_number);
inline bool is_supported_parser_charset(CHARSET_INFO *cs)
{
  return test(cs->mbminlen == 1);
}
bool setup_connection_thread_globals(THD *thd);

int mysql_create_db(THD *thd, char *db, HA_CREATE_INFO *create, bool silent);
bool mysql_alter_db(THD *thd, const char *db, HA_CREATE_INFO *create);
bool mysql_rm_db(THD *thd,char *db,bool if_exists, bool silent);
bool mysql_upgrade_db(THD *thd, LEX_STRING *old_db);
void mysql_binlog_send(THD* thd, char* log_ident, my_off_t pos, ushort flags);
void mysql_client_binlog_statement(THD *thd);
bool mysql_rm_table(THD *thd,TABLE_LIST *tables, my_bool if_exists,
                    my_bool drop_temporary);
int mysql_rm_table_part2(THD *thd, TABLE_LIST *tables, bool if_exists,
                         bool drop_temporary, bool drop_view, bool log_query);
bool quick_rm_table(handlerton *base,const char *db,
                    const char *table_name, uint flags);
void close_cached_table(THD *thd, TABLE *table);
bool mysql_rename_tables(THD *thd, TABLE_LIST *table_list, bool silent);
bool do_rename(THD *thd, TABLE_LIST *ren_table, char *new_db,
                      char *new_table_name, char *new_table_alias,
                      bool skip_error);

bool mysql_change_db(THD *thd, const LEX_STRING *new_db_name,
                     bool force_switch);

bool mysql_opt_change_db(THD *thd,
                         const LEX_STRING *new_db_name,
                         LEX_STRING *saved_db_name,
                         bool force_switch,
                         bool *cur_db_changed);

void mysql_parse(THD *thd, char *rawbuf, uint length,
                 const char ** semicolon);

bool mysql_test_parse_for_slave(THD *thd,char *inBuf,uint length);
bool is_update_query(enum enum_sql_command command);
bool is_log_table_write_query(enum enum_sql_command command);
bool alloc_query(THD *thd, const char *packet, uint packet_length);
void mysql_init_select(LEX *lex);
void mysql_reset_thd_for_next_command(THD *thd);
bool mysql_new_select(LEX *lex, bool move_down);
void create_select_for_variable(const char *var_name);
void mysql_init_multi_delete(LEX *lex);
bool multi_delete_set_locks_and_link_aux_tables(LEX *lex);
void init_max_user_conn(void);
void init_update_queries(void);
void free_max_user_conn(void);
pthread_handler_t handle_bootstrap(void *arg);
int mysql_execute_command(THD *thd);
bool do_command(THD *thd);
bool dispatch_command(enum enum_server_command command, THD *thd,
		      char* packet, uint packet_length);
void log_slow_statement(THD *thd);
bool check_dup(const char *db, const char *name, TABLE_LIST *tables);
bool records_are_comparable(const TABLE *table);
bool compare_records(const TABLE *table);
bool append_file_to_dir(THD *thd, const char **filename_ptr, 
                        const char *table_name);
void wait_while_table_is_used(THD *thd, TABLE *table,
                              enum ha_extra_function function);
bool table_cache_init(void);
void table_cache_free(void);
bool table_def_init(void);
void table_def_free(void);
void assign_new_table_id(TABLE_SHARE *share);
uint cached_open_tables(void);
uint cached_table_definitions(void);
void kill_mysql(void);
void close_connection(THD *thd, uint errcode, bool lock);
bool reload_acl_and_cache(THD *thd, ulong options, TABLE_LIST *tables, 
                          int *write_to_binlog);
#ifndef NO_EMBEDDED_ACCESS_CHECKS
bool check_access(THD *thd, ulong access, const char *db, ulong *save_priv,
		  bool no_grant, bool no_errors, bool schema_db);
bool check_table_access(THD *thd, ulong want_access, TABLE_LIST *tables,
			uint number, bool no_errors);
#else
inline bool check_access(THD *thd, ulong access, const char *db,
                         ulong *save_priv, bool no_grant, bool no_errors,
                         bool schema_db)
{
  if (save_priv)
    *save_priv= GLOBAL_ACLS;
  return false;
}
inline bool check_table_access(THD *thd, ulong want_access, TABLE_LIST *tables,
			uint number, bool no_errors)
{ return false; }
#endif /*NO_EMBEDDED_ACCESS_CHECKS*/

#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
bool check_global_access(THD *thd, ulong want_access);
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER

/*
  Support routine for SQL parser on partitioning syntax
*/
my_bool is_partition_management(LEX *lex);
/*
  General routine to change field->ptr of a NULL-terminated array of Field
  objects. Useful when needed to call val_int, val_str or similar and the
  field data is not in table->record[0] but in some other structure.
  set_key_field_ptr changes all fields of an index using a key_info object.
  All methods presume that there is at least one field to change.
*/

void set_field_ptr(Field **ptr, const uchar *new_buf, const uchar *old_buf);
void set_key_field_ptr(KEY *key_info, const uchar *new_buf,
                       const uchar *old_buf);

bool mysql_backup_table(THD* thd, TABLE_LIST* table_list);
bool mysql_restore_table(THD* thd, TABLE_LIST* table_list);

bool mysql_checksum_table(THD* thd, TABLE_LIST* table_list,
                          HA_CHECK_OPT* check_opt);
bool mysql_check_table(THD* thd, TABLE_LIST* table_list,
                       HA_CHECK_OPT* check_opt);
bool mysql_repair_table(THD* thd, TABLE_LIST* table_list,
                        HA_CHECK_OPT* check_opt);
bool mysql_analyze_table(THD* thd, TABLE_LIST* table_list,
                         HA_CHECK_OPT* check_opt);
bool mysql_optimize_table(THD* thd, TABLE_LIST* table_list,
                          HA_CHECK_OPT* check_opt);
bool mysql_assign_to_keycache(THD* thd, TABLE_LIST* table_list,
                              LEX_STRING *key_cache_name);
bool mysql_preload_keys(THD* thd, TABLE_LIST* table_list);
int reassign_keycache_tables(THD* thd, KEY_CACHE *src_cache,
                             KEY_CACHE *dst_cache);
TABLE *create_virtual_tmp_table(THD *thd, List<Create_field> &field_list);

bool mysql_xa_recover(THD *thd);

bool check_simple_select();
int mysql_alter_tablespace(THD* thd, st_alter_tablespace *ts_info);

SORT_FIELD * make_unireg_sortorder(ORDER *order, uint *length,
                                  SORT_FIELD *sortorder);
int setup_order(THD *thd, Item **ref_pointer_array, TABLE_LIST *tables,
		List<Item> &fields, List <Item> &all_fields, ORDER *order);
int setup_group(THD *thd, Item **ref_pointer_array, TABLE_LIST *tables,
		List<Item> &fields, List<Item> &all_fields, ORDER *order,
		bool *hidden_group_fields);
bool fix_inner_refs(THD *thd, List<Item> &all_fields, SELECT_LEX *select,
                   Item **ref_pointer_array, ORDER *group_list= NULL);

bool handle_select(THD *thd, LEX *lex, select_result *result,
                   ulong setup_tables_done_option);
bool mysql_select(THD *thd, Item ***rref_pointer_array,
                  TABLE_LIST *tables, uint wild_num,  List<Item> &list,
                  COND *conds, uint og_num, ORDER *order, ORDER *group,
                  Item *having, ORDER *proc_param, ulonglong select_type, 
                  select_result *result, SELECT_LEX_UNIT *unit, 
                  SELECT_LEX *select_lex);
void free_underlaid_joins(THD *thd, SELECT_LEX *select);
bool mysql_explain_union(THD *thd, SELECT_LEX_UNIT *unit,
                         select_result *result);
int mysql_explain_select(THD *thd, SELECT_LEX *sl, char const *type,
			 select_result *result);
bool mysql_union(THD *thd, LEX *lex, select_result *result,
                 SELECT_LEX_UNIT *unit, ulong setup_tables_done_option);
bool mysql_handle_derived(LEX *lex, bool (*processor)(THD *thd,
                                                      LEX *lex,
                                                      TABLE_LIST *table));
bool mysql_derived_prepare(THD *thd, LEX *lex, TABLE_LIST *t);
bool mysql_derived_filling(THD *thd, LEX *lex, TABLE_LIST *t);
Field *create_tmp_field(THD *thd, TABLE *table,Item *item, Item::Type type,
			Item ***copy_func, Field **from_field,
                        Field **def_field,
			bool group, bool modify_item,
			bool table_cant_handle_bit_fields,
                        bool make_copy_field,
                        uint convert_blob_length);
void sp_prepare_create_field(THD *thd, Create_field *sql_field);
int prepare_create_field(Create_field *sql_field, 
			 uint *blob_columns, 
			 int *timestamps, int *timestamps_with_niladic,
			 longlong table_flags);
bool mysql_create_table(THD *thd,const char *db, const char *table_name,
                        HA_CREATE_INFO *create_info,
                        Alter_info *alter_info,
                        bool tmp_table, uint select_field_count);
bool mysql_create_table_no_lock(THD *thd, const char *db,
                                const char *table_name,
                                HA_CREATE_INFO *create_info,
                                Alter_info *alter_info,
                                bool tmp_table, uint select_field_count);

bool mysql_alter_table(THD *thd, char *new_db, char *new_name,
                       HA_CREATE_INFO *create_info,
                       TABLE_LIST *table_list,
                       Alter_info *alter_info,
                       uint order_num, ORDER *order, bool ignore);
bool mysql_recreate_table(THD *thd, TABLE_LIST *table_list);
bool mysql_create_like_table(THD *thd, TABLE_LIST *table,
                             TABLE_LIST *src_table,
                             HA_CREATE_INFO *create_info);
bool mysql_rename_table(handlerton *base, const char *old_db,
                        const char * old_name, const char *new_db,
                        const char * new_name, uint flags);
bool mysql_prepare_update(THD *thd, TABLE_LIST *table_list,
                          Item **conds, uint order_num, ORDER *order);
int mysql_update(THD *thd,TABLE_LIST *tables,List<Item> &fields,
		 List<Item> &values,COND *conds,
		 uint order_num, ORDER *order, ha_rows limit,
		 enum enum_duplicates handle_duplicates, bool ignore);
bool mysql_multi_update(THD *thd, TABLE_LIST *table_list,
                        List<Item> *fields, List<Item> *values,
                        COND *conds, ulonglong options,
                        enum enum_duplicates handle_duplicates, bool ignore,
                        SELECT_LEX_UNIT *unit, SELECT_LEX *select_lex);
bool mysql_prepare_insert(THD *thd, TABLE_LIST *table_list, TABLE *table,
                          List<Item> &fields, List_item *values,
                          List<Item> &update_fields,
                          List<Item> &update_values, enum_duplicates duplic,
                          COND **where, bool select_insert,
                          bool check_fields, bool abort_on_warning);
bool mysql_insert(THD *thd,TABLE_LIST *table,List<Item> &fields,
                  List<List_item> &values, List<Item> &update_fields,
                  List<Item> &update_values, enum_duplicates flag,
                  bool ignore);
int check_that_all_fields_are_given_values(THD *thd, TABLE *entry,
                                           TABLE_LIST *table_list);
void prepare_triggers_for_insert_stmt(TABLE *table);
int mysql_prepare_delete(THD *thd, TABLE_LIST *table_list, Item **conds);
bool mysql_delete(THD *thd, TABLE_LIST *table_list, COND *conds,
                  SQL_I_List<ORDER> *order, ha_rows rows, ulonglong options,
                  bool reset_auto_increment);
bool mysql_truncate(THD *thd, TABLE_LIST *table_list, bool dont_send_ok);
bool mysql_create_or_drop_trigger(THD *thd, TABLE_LIST *tables, bool create);
uint create_table_def_key(THD *thd, char *key, TABLE_LIST *table_list,
                          bool tmp_table);
TABLE_SHARE *get_table_share(THD *thd, TABLE_LIST *table_list, char *key,
                             uint key_length, uint db_flags, int *error);
void release_table_share(TABLE_SHARE *share, enum release_type type);
TABLE_SHARE *get_cached_table_share(const char *db, const char *table_name);
TABLE *open_ltable(THD *thd, TABLE_LIST *table_list, thr_lock_type update,
                   uint lock_flags);
TABLE *open_table(THD *thd, TABLE_LIST *table_list, MEM_ROOT* mem,
		  bool *refresh, uint flags);
bool name_lock_locked_table(THD *thd, TABLE_LIST *tables);
bool reopen_name_locked_table(THD* thd, TABLE_LIST* table_list, bool link_in);
TABLE *table_cache_insert_placeholder(THD *thd, const char *key,
                                      uint key_length);
bool lock_table_name_if_not_cached(THD *thd, const char *db,
                                   const char *table_name, TABLE **table);
TABLE *find_locked_table(THD *thd, const char *db,const char *table_name);
void detach_merge_children(TABLE *table, bool clear_refs);
bool fix_merge_after_open(TABLE_LIST *old_child_list, TABLE_LIST **old_last,
                          TABLE_LIST *new_child_list, TABLE_LIST **new_last);
bool reopen_table(TABLE *table);
bool reopen_tables(THD *thd,bool get_locks,bool in_refresh);
thr_lock_type read_lock_type_for_table(THD *thd, LEX *lex,
                                       TABLE_LIST *table_list);
void close_data_files_and_morph_locks(THD *thd, const char *db,
                                      const char *table_name);
void close_handle_and_leave_table_as_lock(TABLE *table);
bool wait_for_tables(THD *thd);
bool table_is_used(TABLE *table, bool wait_for_name_lock);
TABLE *drop_locked_tables(THD *thd,const char *db, const char *table_name);
void abort_locked_tables(THD *thd,const char *db, const char *table_name);
void execute_init_command(THD *thd, sys_var_str *init_command_var,
			  rw_lock_t *var_mutex);
extern Field *not_found_field;
extern Field *view_ref_found;

enum find_item_error_report_type {REPORT_ALL_ERRORS, REPORT_EXCEPT_NOT_FOUND,
				  IGNORE_ERRORS, REPORT_EXCEPT_NON_UNIQUE,
                                  IGNORE_EXCEPT_NON_UNIQUE};
Field *
find_field_in_tables(THD *thd, Item_ident *item,
                     TABLE_LIST *first_table, TABLE_LIST *last_table,
                     Item **ref, find_item_error_report_type report_error,
                     bool check_privileges, bool register_tree_change);
Field *
find_field_in_table_ref(THD *thd, TABLE_LIST *table_list,
                        const char *name, uint length,
                        const char *item_name, const char *db_name,
                        const char *table_name, Item **ref,
                        bool check_privileges, bool allow_rowid,
                        uint *cached_field_index_ptr,
                        bool register_tree_change, TABLE_LIST **actual_table);
Field *
find_field_in_table(THD *thd, TABLE *table, const char *name, uint length,
                    bool allow_rowid, uint *cached_field_index_ptr);
Field *
find_field_in_table_sef(TABLE *table, const char *name);

#endif /* MYSQL_SERVER */

#ifdef HAVE_OPENSSL
#include <openssl/des.h>
struct st_des_keyblock
{
  DES_cblock key1, key2, key3;
};
struct st_des_keyschedule
{
  DES_key_schedule ks1, ks2, ks3;
};
extern char *des_key_file;
extern struct st_des_keyschedule des_keyschedule[10];
extern uint des_default_key;
extern pthread_mutex_t LOCK_des_key_file;
bool load_des_key_file(const char *file_name);
#endif /* HAVE_OPENSSL */

#ifdef MYSQL_SERVER
/* sql_do.cc */
bool mysql_do(THD *thd, List<Item> &values);

/* sql_analyse.h */
bool append_escaped(String *to_str, String *from_str);

/* sql_show.cc */
bool mysqld_show_open_tables(THD *thd,const char *wild);
bool mysqld_show_logs(THD *thd);
void append_identifier(THD *thd, String *packet, const char *name,
		       uint length);
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
int get_quote_char_for_identifier(THD *thd, const char *name, uint length);
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
void mysqld_list_fields(THD *thd,TABLE_LIST *table, const char *wild);
int mysqld_dump_create_info(THD *thd, TABLE_LIST *table_list, int fd);
bool mysqld_show_create(THD *thd, TABLE_LIST *table_list);
bool mysqld_show_create_db(THD *thd, char *dbname, HA_CREATE_INFO *create);

void mysqld_list_processes(THD *thd,const char *user,bool verbose);
int mysqld_show_status(THD *thd);
int mysqld_show_variables(THD *thd,const char *wild);
bool mysqld_show_storage_engines(THD *thd);
bool mysqld_show_authors(THD *thd);
bool mysqld_show_contributors(THD *thd);
bool mysqld_show_privileges(THD *thd);
bool mysqld_show_column_types(THD *thd);
bool mysqld_help (THD *thd, const char *text);
void calc_sum_of_all_status(STATUS_VAR *to);

void append_definer(THD *thd, String *buffer, const LEX_STRING *definer_user,
                    const LEX_STRING *definer_host);

int add_status_vars(SHOW_VAR *list);
void remove_status_vars(SHOW_VAR *list);
void init_status_vars();
void free_status_vars();
void reset_status_vars();

/* information schema */
extern LEX_STRING INFORMATION_SCHEMA_NAME;
/* log tables */
extern LEX_STRING MYSQL_SCHEMA_NAME;
extern LEX_STRING GENERAL_LOG_NAME;
extern LEX_STRING SLOW_LOG_NAME;

extern const LEX_STRING partition_keywords[];
ST_SCHEMA_TABLE *find_schema_table(THD *thd, const char* table_name);
ST_SCHEMA_TABLE *get_schema_table(enum enum_schema_tables schema_table_idx);
int prepare_schema_table(THD *thd, LEX *lex, Table_ident *table_ident,
                         enum enum_schema_tables schema_table_idx);
int make_schema_select(THD *thd,  SELECT_LEX *sel,
                       enum enum_schema_tables schema_table_idx);
int mysql_schema_table(THD *thd, LEX *lex, TABLE_LIST *table_list);
int fill_schema_user_privileges(THD *thd, TABLE_LIST *tables, COND *cond);
int fill_schema_schema_privileges(THD *thd, TABLE_LIST *tables, COND *cond);
int fill_schema_table_privileges(THD *thd, TABLE_LIST *tables, COND *cond);
int fill_schema_column_privileges(THD *thd, TABLE_LIST *tables, COND *cond);
bool get_schema_tables_result(JOIN *join,
                              enum enum_schema_table_state executed_place);
enum enum_schema_tables get_schema_table_idx(ST_SCHEMA_TABLE *schema_table);

inline bool is_schema_db(const char *name, size_t len)
{
  return (INFORMATION_SCHEMA_NAME.length == len &&
          !my_strcasecmp(system_charset_info,
                         INFORMATION_SCHEMA_NAME.str, name));  
}

inline bool is_schema_db(const char *name)
{
  return !my_strcasecmp(system_charset_info,
                        INFORMATION_SCHEMA_NAME.str, name);
}

/* sql_prepare.cc */

void mysqld_stmt_prepare(THD *thd, const char *packet, uint packet_length);
void mysqld_stmt_execute(THD *thd, char *packet, uint packet_length);
void mysqld_stmt_close(THD *thd, char *packet);
void mysql_sql_stmt_prepare(THD *thd);
void mysql_sql_stmt_execute(THD *thd);
void mysql_sql_stmt_close(THD *thd);
void mysqld_stmt_fetch(THD *thd, char *packet, uint packet_length);
void mysqld_stmt_reset(THD *thd, char *packet);
void mysql_stmt_get_longdata(THD *thd, char *pos, ulong packet_length);
void reinit_stmt_before_use(THD *thd, LEX *lex);

/* sql_handler.cc */
bool mysql_ha_open(THD *thd, TABLE_LIST *tables, bool reopen);
bool mysql_ha_close(THD *thd, TABLE_LIST *tables);
bool mysql_ha_read(THD *, TABLE_LIST *,enum enum_ha_read_modes,char *,
                   List<Item> *,enum ha_rkey_function,Item *,ha_rows,ha_rows);
void mysql_ha_flush(THD *thd);
void mysql_ha_rm_tables(THD *thd, TABLE_LIST *tables, bool is_locked);
void mysql_ha_cleanup(THD *thd);

/* sql_base.cc */
#define TMP_TABLE_KEY_EXTRA 8
void set_item_name(Item *item,char *pos,uint length);
bool add_field_to_list(THD *thd, LEX_STRING *field_name, enum enum_field_types type,
		       char *length, char *decimal,
		       uint type_modifier,
		       Item *default_value, Item *on_update_value,
		       LEX_STRING *comment,
		       char *change, List<String> *interval_list,
		       CHARSET_INFO *cs,
		       uint uint_geom_type);
Create_field * new_create_field(THD *thd, char *field_name, enum_field_types type,
				char *length, char *decimals,
				uint type_modifier, 
				Item *default_value, Item *on_update_value,
				LEX_STRING *comment, char *change, 
				List<String> *interval_list, CHARSET_INFO *cs,
				uint uint_geom_type);
void store_position_for_column(const char *name);
bool add_to_list(THD *thd, SQL_I_List<ORDER> &list, Item *group,bool asc);
bool push_new_name_resolution_context(THD *thd,
                                      TABLE_LIST *left_op,
                                      TABLE_LIST *right_op);
void add_join_on(TABLE_LIST *b,Item *expr);
void add_join_natural(TABLE_LIST *a,TABLE_LIST *b,List<String> *using_fields,
                      SELECT_LEX *lex);
bool add_proc_to_list(THD *thd, Item *item);
void unlink_open_table(THD *thd, TABLE *find, bool unlock);
void drop_open_table(THD *thd, TABLE *table, const char *db_name,
                     const char *table_name);
void update_non_unique_table_error(TABLE_LIST *update,
                                   const char *operation,
                                   TABLE_LIST *duplicate);

SQL_SELECT *make_select(TABLE *head, table_map const_tables,
			table_map read_tables, COND *conds,
                        bool allow_null_cond,  int *error);
extern Item **not_found_item;

/*
  This enumeration type is used only by the function find_item_in_list
  to return the info on how an item has been resolved against a list
  of possibly aliased items.
  The item can be resolved: 
   - against an alias name of the list's element (RESOLVED_AGAINST_ALIAS)
   - against non-aliased field name of the list  (RESOLVED_WITH_NO_ALIAS)
   - against an aliased field name of the list   (RESOLVED_BEHIND_ALIAS)
   - ignoring the alias name in cases when SQL requires to ignore aliases
     (e.g. when the resolved field reference contains a table name or
     when the resolved item is an expression)   (RESOLVED_IGNORING_ALIAS)    
*/
enum enum_resolution_type {
  NOT_RESOLVED=0,
  RESOLVED_IGNORING_ALIAS,
  RESOLVED_BEHIND_ALIAS,
  RESOLVED_WITH_NO_ALIAS,
  RESOLVED_AGAINST_ALIAS
};
Item ** find_item_in_list(Item *item, List<Item> &items, uint *counter,
                          find_item_error_report_type report_error,
                          enum_resolution_type *resolution);
bool get_key_map_from_key_list(key_map *map, TABLE *table,
                               List<String> *index_list);
bool insert_fields(THD *thd, Name_resolution_context *context,
		   const char *db_name, const char *table_name,
                   List_iterator<Item> *it, bool any_privileges);
bool setup_tables(THD *thd, Name_resolution_context *context,
                  List<TABLE_LIST> *from_clause, TABLE_LIST *tables,
                  TABLE_LIST **leaves, bool select_insert);
bool setup_tables_and_check_access(THD *thd, 
                                   Name_resolution_context *context,
                                   List<TABLE_LIST> *from_clause, 
                                   TABLE_LIST *tables, 
                                   TABLE_LIST **leaves, 
                                   bool select_insert,
                                   ulong want_access_first,
                                   ulong want_access);
int setup_wild(THD *thd, TABLE_LIST *tables, List<Item> &fields,
	       List<Item> *sum_func_list, uint wild_num);
bool setup_fields(THD *thd, Item** ref_pointer_array,
                  List<Item> &item, enum_mark_columns mark_used_columns,
                  List<Item> *sum_func_list, bool allow_sum_func);
inline bool setup_fields_with_no_wrap(THD *thd, Item **ref_pointer_array,
                                      List<Item> &item,
                                      enum_mark_columns mark_used_columns,
                                      List<Item> *sum_func_list,
                                      bool allow_sum_func)
{
  bool res;
  thd->lex->select_lex.no_wrap_view_item= TRUE;
  res= setup_fields(thd, ref_pointer_array, item, mark_used_columns, sum_func_list,
                    allow_sum_func);
  thd->lex->select_lex.no_wrap_view_item= FALSE;
  return res;
}
int setup_conds(THD *thd, TABLE_LIST *tables, TABLE_LIST *leaves,
		COND **conds);
int setup_ftfuncs(SELECT_LEX* select);
int init_ftfuncs(THD *thd, SELECT_LEX* select, bool no_order);
void wait_for_condition(THD *thd, pthread_mutex_t *mutex,
                        pthread_cond_t *cond);
int open_tables(THD *thd, TABLE_LIST **tables, uint *counter, uint flags);
/* open_and_lock_tables with optional derived handling */
int open_and_lock_tables_derived(THD *thd, TABLE_LIST *tables, bool derived);
/* simple open_and_lock_tables without derived handling */
inline int simple_open_n_lock_tables(THD *thd, TABLE_LIST *tables)
{
  return open_and_lock_tables_derived(thd, tables, FALSE);
}
/* open_and_lock_tables with derived handling */
inline int open_and_lock_tables(THD *thd, TABLE_LIST *tables)
{
  return open_and_lock_tables_derived(thd, tables, TRUE);
}
/* simple open_and_lock_tables without derived handling for single table */
TABLE *open_n_lock_single_table(THD *thd, TABLE_LIST *table_l,
                                thr_lock_type lock_type);
bool open_normal_and_derived_tables(THD *thd, TABLE_LIST *tables, uint flags);
int lock_tables(THD *thd, TABLE_LIST *tables, uint counter, bool *need_reopen);
int decide_logging_format(THD *thd, TABLE_LIST *tables);
TABLE *open_temporary_table(THD *thd, const char *path, const char *db,
			    const char *table_name, bool link_in_list);
bool rm_temporary_table(handlerton *base, char *path);
void free_io_cache(TABLE *entry);
void intern_close_table(TABLE *entry);
bool close_thread_table(THD *thd, TABLE **table_ptr);
void close_temporary_tables(THD *thd);
void close_tables_for_reopen(THD *thd, TABLE_LIST **tables);
TABLE_LIST *find_table_in_list(TABLE_LIST *table,
                               TABLE_LIST *TABLE_LIST::*link,
                               const char *db_name,
                               const char *table_name);
TABLE_LIST *unique_table(THD *thd, TABLE_LIST *table, TABLE_LIST *table_list,
                         bool check_alias);
TABLE *find_temporary_table(THD *thd, const char *db, const char *table_name);
TABLE *find_temporary_table(THD *thd, TABLE_LIST *table_list);
int drop_temporary_table(THD *thd, TABLE_LIST *table_list);
void close_temporary_table(THD *thd, TABLE *table, bool free_share,
                           bool delete_table);
void close_temporary(TABLE *table, bool free_share, bool delete_table);
bool rename_temporary_table(THD* thd, TABLE *table, const char *new_db,
			    const char *table_name);
void remove_db_from_cache(const char *db);
void flush_tables();
bool is_equal(const LEX_STRING *a, const LEX_STRING *b);
char *make_default_log_name(char *buff,const char* log_ext);

#ifdef WITH_PARTITION_STORAGE_ENGINE
uint fast_alter_partition_table(THD *thd, TABLE *table,
                                Alter_info *alter_info,
                                HA_CREATE_INFO *create_info,
                                TABLE_LIST *table_list,
                                char *db,
                                const char *table_name,
                                uint fast_alter_partition);
uint set_part_state(Alter_info *alter_info, partition_info *tab_part_info,
                    enum partition_state part_state);
uint prep_alter_part_table(THD *thd, TABLE *table, Alter_info *alter_info,
                           HA_CREATE_INFO *create_info,
                           handlerton *old_db_type,
                           bool *partition_changed,
                           uint *fast_alter_partition);
#endif

/* bits for last argument to remove_table_from_cache() */
#define RTFC_NO_FLAG                0x0000
#define RTFC_OWNED_BY_THD_FLAG      0x0001
#define RTFC_WAIT_OTHER_THREAD_FLAG 0x0002
#define RTFC_CHECK_KILLED_FLAG      0x0004
bool remove_table_from_cache(THD *thd, const char *db, const char *table,
                             uint flags);

#define NORMAL_PART_NAME 0
#define TEMP_PART_NAME 1
#define RENAMED_PART_NAME 2
void create_partition_name(char *out, const char *in1,
                           const char *in2, uint name_variant,
                           bool translate);
void create_subpartition_name(char *out, const char *in1,
                              const char *in2, const char *in3,
                              uint name_variant);

typedef struct st_lock_param_type
{
  TABLE_LIST table_list;
  ulonglong copied;
  ulonglong deleted;
  THD *thd;
  HA_CREATE_INFO *create_info;
  Alter_info *alter_info;
  TABLE *table;
  KEY *key_info_buffer;
  const char *db;
  const char *table_name;
  uchar *pack_frm_data;
  enum thr_lock_type old_lock_type;
  uint key_count;
  uint db_options;
  size_t pack_frm_len;
  partition_info *part_info;
} ALTER_PARTITION_PARAM_TYPE;

void mem_alloc_error(size_t size);

enum ddl_log_entry_code
{
  /*
    DDL_LOG_EXECUTE_CODE:
      This is a code that indicates that this is a log entry to
      be executed, from this entry a linked list of log entries
      can be found and executed.
    DDL_LOG_ENTRY_CODE:
      An entry to be executed in a linked list from an execute log
      entry.
    DDL_IGNORE_LOG_ENTRY_CODE:
      An entry that is to be ignored
  */
  DDL_LOG_EXECUTE_CODE = 'e',
  DDL_LOG_ENTRY_CODE = 'l',
  DDL_IGNORE_LOG_ENTRY_CODE = 'i'
};

enum ddl_log_action_code
{
  /*
    The type of action that a DDL_LOG_ENTRY_CODE entry is to
    perform.
    DDL_LOG_DELETE_ACTION:
      Delete an entity
    DDL_LOG_RENAME_ACTION:
      Rename an entity
    DDL_LOG_REPLACE_ACTION:
      Rename an entity after removing the previous entry with the
      new name, that is replace this entry.
  */
  DDL_LOG_DELETE_ACTION = 'd',
  DDL_LOG_RENAME_ACTION = 'r',
  DDL_LOG_REPLACE_ACTION = 's'
};


typedef struct st_ddl_log_entry
{
  const char *name;
  const char *from_name;
  const char *handler_name;
  uint next_entry;
  uint entry_pos;
  enum ddl_log_entry_code entry_type;
  enum ddl_log_action_code action_type;
  /*
    Most actions have only one phase. REPLACE does however have two
    phases. The first phase removes the file with the new name if
    there was one there before and the second phase renames the
    old name to the new name.
  */
  char phase;
} DDL_LOG_ENTRY;

typedef struct st_ddl_log_memory_entry
{
  uint entry_pos;
  struct st_ddl_log_memory_entry *next_log_entry;
  struct st_ddl_log_memory_entry *prev_log_entry;
  struct st_ddl_log_memory_entry *next_active_log_entry;
} DDL_LOG_MEMORY_ENTRY;


bool write_ddl_log_entry(DDL_LOG_ENTRY *ddl_log_entry,
                           DDL_LOG_MEMORY_ENTRY **active_entry);
bool write_execute_ddl_log_entry(uint first_entry,
                                   bool complete,
                                   DDL_LOG_MEMORY_ENTRY **active_entry);
bool deactivate_ddl_log_entry(uint entry_no);
void release_ddl_log_memory_entry(DDL_LOG_MEMORY_ENTRY *log_entry);
bool sync_ddl_log();
void release_ddl_log();
void execute_ddl_log_recovery();
bool execute_ddl_log_entry(THD *thd, uint first_entry);

extern pthread_mutex_t LOCK_gdl;

#define WFRM_WRITE_SHADOW 1
#define WFRM_INSTALL_SHADOW 2
#define WFRM_PACK_FRM 4
#define WFRM_KEEP_SHARE 8
bool mysql_write_frm(ALTER_PARTITION_PARAM_TYPE *lpt, uint flags);
int abort_and_upgrade_lock_and_close_table(ALTER_PARTITION_PARAM_TYPE *lpt);
void close_open_tables_and_downgrade(ALTER_PARTITION_PARAM_TYPE *lpt);
void mysql_wait_completed_table(ALTER_PARTITION_PARAM_TYPE *lpt, TABLE *my_table);

/* Functions to work with system tables. */
bool open_system_tables_for_read(THD *thd, TABLE_LIST *table_list,
                                 Open_tables_state *backup);
void close_system_tables(THD *thd, Open_tables_state *backup);
TABLE *open_system_table_for_update(THD *thd, TABLE_LIST *one_table);

TABLE *open_performance_schema_table(THD *thd, TABLE_LIST *one_table,
                                     Open_tables_state *backup);
void close_performance_schema_table(THD *thd, Open_tables_state *backup);

bool close_cached_tables(THD *thd, TABLE_LIST *tables, bool have_lock,
                         bool wait_for_refresh, bool wait_for_placeholders);
bool close_cached_connection_tables(THD *thd, bool wait_for_refresh,
                                    LEX_STRING *connect_string,
                                    bool have_lock = FALSE);
void copy_field_from_tmp_record(Field *field,int offset);
bool fill_record(THD *thd, Field **field, List<Item> &values,
                 bool ignore_errors);
bool fill_record_n_invoke_before_triggers(THD *thd, List<Item> &fields,
                                          List<Item> &values,
                                          bool ignore_errors,
                                          Table_triggers_list *triggers,
                                          enum trg_event_type event);
bool fill_record_n_invoke_before_triggers(THD *thd, Field **field,
                                          List<Item> &values,
                                          bool ignore_errors,
                                          Table_triggers_list *triggers,
                                          enum trg_event_type event);
OPEN_TABLE_LIST *list_open_tables(THD *thd, const char *db, const char *wild);

inline TABLE_LIST *find_table_in_global_list(TABLE_LIST *table,
                                             const char *db_name,
                                             const char *table_name)
{
  return find_table_in_list(table, &TABLE_LIST::next_global,
                            db_name, table_name);
}

inline TABLE_LIST *find_table_in_local_list(TABLE_LIST *table,
                                            const char *db_name,
                                            const char *table_name)
{
  return find_table_in_list(table, &TABLE_LIST::next_local,
                            db_name, table_name);
}


/* sql_calc.cc */
bool eval_const_cond(COND *cond);

/* sql_load.cc */
int mysql_load(THD *thd, sql_exchange *ex, TABLE_LIST *table_list,
	        List<Item> &fields_vars, List<Item> &set_fields,
                List<Item> &set_values_list,
                enum enum_duplicates handle_duplicates, bool ignore,
                bool local_file);
int write_record(THD *thd, TABLE *table, COPY_INFO *info);

/* sql_manager.cc */
extern bool volatile  mqh_used;
void start_handle_manager();
void stop_handle_manager();
bool mysql_manager_submit(void (*action)());


/* sql_test.cc */
#ifndef DBUG_OFF
void print_where(COND *cond,const char *info, enum_query_type query_type);
void print_cached_tables(void);
void TEST_filesort(SORT_FIELD *sortorder,uint s_length);
void print_plan(JOIN* join,uint idx, double record_count, double read_time,
                double current_read_time, const char *info);
#endif
void mysql_print_status();
/* key.cc */
int find_ref_key(KEY *key, uint key_count, uchar *record, Field *field,
                 uint *key_length, uint *keypart);
void key_copy(uchar *to_key, uchar *from_record, KEY *key_info, uint key_length);
void key_restore(uchar *to_record, uchar *from_key, KEY *key_info,
                 uint key_length);
bool key_cmp_if_same(TABLE *form,const uchar *key,uint index,uint key_length);
void key_unpack(String *to,TABLE *form,uint index);
bool is_key_used(TABLE *table, uint idx, const MY_BITMAP *fields);
int key_cmp(KEY_PART_INFO *key_part, const uchar *key, uint key_length);
extern "C" int key_rec_cmp(void *key_info, uchar *a, uchar *b);

bool init_errmessage(void);
#endif /* MYSQL_SERVER */
void sql_perror(const char *message);

bool fn_format_relative_to_data_home(char * to, const char *name,
				     const char *dir, const char *extension);
/**
  Test a file path to determine if the path is compatible with the secure file
  path restriction.
*/
bool is_secure_file_path(char *path);

#ifdef MYSQL_SERVER
File open_binlog(IO_CACHE *log, const char *log_file_name,
                 const char **errmsg);

/* mysqld.cc */
extern void MYSQLerror(const char*);
void refresh_status(THD *thd);
my_bool mysql_rm_tmp_tables(void);
void handle_connection_in_main_thread(THD *thd);
void create_thread_to_handle_connection(THD *thd);
void unlink_thd(THD *thd);
bool one_thread_per_connection_end(THD *thd, bool put_in_cache);
void flush_thread_cache();

/* item_func.cc */
extern bool check_reserved_words(LEX_STRING *name);
extern enum_field_types agg_field_type(Item **items, uint nitems);

/* strfunc.cc */
ulonglong find_set(TYPELIB *lib, const char *x, uint length, CHARSET_INFO *cs,
		   char **err_pos, uint *err_len, bool *set_warning);
ulonglong find_set_from_flags(TYPELIB *lib, uint default_name,
                              ulonglong cur_set, ulonglong default_set,
                              const char *str, uint length, CHARSET_INFO *cs,
                              char **err_pos, uint *err_len, bool *set_warning);
uint find_type(const TYPELIB *lib, const char *find, uint length,
               bool part_match);
uint find_type2(const TYPELIB *lib, const char *find, uint length,
                CHARSET_INFO *cs);
void unhex_type2(TYPELIB *lib);
uint check_word(TYPELIB *lib, const char *val, const char *end,
		const char **end_of_word);
int find_string_in_array(LEX_STRING * const haystack, LEX_STRING * const needle,
                         CHARSET_INFO * const cs);


bool is_keyword(const char *name, uint len);

#define MY_DB_OPT_FILE "db.opt"
bool my_database_names_init(void);
void my_database_names_free(void);
bool check_db_dir_existence(const char *db_name);
bool load_db_opt(THD *thd, const char *path, HA_CREATE_INFO *create);
bool load_db_opt_by_name(THD *thd, const char *db_name,
                         HA_CREATE_INFO *db_create_info);
CHARSET_INFO *get_default_db_collation(THD *thd, const char *db_name);
bool my_dbopt_init(void);
void my_dbopt_cleanup(void);
extern int creating_database; // How many database locks are made
extern int creating_table;    // How many mysql_create_table() are running

/*
  External variables
*/

extern time_t server_start_time, flush_status_time;
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
extern uint mysql_data_home_len;

extern MYSQL_PLUGIN_IMPORT char  *mysql_data_home;
extern char server_version[SERVER_VERSION_LENGTH];
extern MYSQL_PLUGIN_IMPORT char mysql_real_data_home[];
extern char mysql_unpacked_real_data_home[];

extern CHARSET_INFO *character_set_filesystem;
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
extern char *opt_mysql_tmpdir, mysql_charsets_dir[],
            def_ft_boolean_syntax[sizeof(ft_boolean_syntax)];
extern int mysql_unpacked_real_data_home_len;
#define mysql_tmpdir (my_tmpdir(&mysql_tmpdir_list))
extern MYSQL_PLUGIN_IMPORT MY_TMPDIR mysql_tmpdir_list;
extern const LEX_STRING command_name[];

extern const char *first_keyword, *delayed_user, *binary_keyword;
extern MYSQL_PLUGIN_IMPORT const char  *my_localhost;
extern MYSQL_PLUGIN_IMPORT const char **errmesg;			/* Error messages */

extern const char *myisam_recover_options_str;
extern const char *in_left_expr_name, *in_additional_cond, *in_having_cond;
extern const char * const TRG_EXT;
extern const char * const TRN_EXT;
extern Eq_creator eq_creator;
extern Ne_creator ne_creator;
extern Gt_creator gt_creator;
extern Lt_creator lt_creator;
extern Ge_creator ge_creator;
extern Le_creator le_creator;
extern char language[FN_REFLEN];
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
extern MYSQL_PLUGIN_IMPORT char reg_ext[FN_EXTLEN];
extern MYSQL_PLUGIN_IMPORT uint reg_ext_length;
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
extern char glob_hostname[FN_REFLEN], mysql_home[FN_REFLEN];
extern char pidfile_name[FN_REFLEN], system_time_zone[30], *opt_init_file;
extern char log_error_file[FN_REFLEN], *opt_tc_log_file;
extern ulonglong log_10_int[20];
extern ulonglong keybuff_size;
extern ulonglong thd_startup_options;
extern ulong thread_id;
extern ulong binlog_cache_use, binlog_cache_disk_use;
extern ulong aborted_threads,aborted_connects;
extern ulong delayed_insert_timeout;
extern ulong delayed_insert_limit, delayed_queue_size;
extern ulong delayed_insert_threads, delayed_insert_writes;
extern ulong delayed_rows_in_use,delayed_insert_errors;
extern ulong slave_open_temp_tables;
extern ulong query_cache_size, query_cache_min_res_unit;
extern ulong slow_launch_threads, slow_launch_time;
extern ulong table_cache_size, table_def_size;
extern MYSQL_PLUGIN_IMPORT ulong max_connections;
extern ulong max_connect_errors, connect_timeout;
extern ulong slave_net_timeout, slave_trans_retries;
extern uint max_user_connections;
extern ulong what_to_log,flush_time;
extern ulong query_buff_size;
extern ulong slave_max_allowed_packet;
extern ulong max_prepared_stmt_count, prepared_stmt_count;
extern ulong binlog_cache_size, open_files_limit;
extern ulonglong max_binlog_cache_size;
extern ulong max_binlog_size, max_relay_log_size;
extern ulong opt_binlog_rows_event_max_size;
extern ulong rpl_recovery_rank, thread_cache_size, thread_pool_size;
extern ulong back_log;
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
extern ulong MYSQL_PLUGIN_IMPORT specialflag;
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
extern ulong current_pid;
extern ulong expire_logs_days, sync_binlog_period, sync_binlog_counter;
extern ulong opt_tc_log_size, tc_log_max_pages_used, tc_log_page_size;
extern ulong tc_log_page_waits;
extern my_bool relay_log_purge, opt_innodb_safe_binlog, opt_innodb;
extern uint test_flags,select_errors,ha_open_options;
extern uint protocol_version, mysqld_port, dropping_tables;
extern uint delay_key_write_options;
extern ulong max_long_data_size;
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
extern MYSQL_PLUGIN_IMPORT uint lower_case_table_names;
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
extern bool opt_endinfo, using_udf_functions;
extern my_bool locked_in_memory;
extern bool opt_using_transactions;
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
extern MYSQL_PLUGIN_IMPORT bool mysqld_embedded;
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
extern bool opt_large_files, server_id_supplied;
extern bool opt_update_log, opt_bin_log, opt_error_log;
extern my_bool opt_log, opt_slow_log;
extern ulong log_output_options;
extern my_bool opt_log_queries_not_using_indexes;
extern bool opt_disable_networking, opt_skip_show_db;
extern bool opt_skip_name_resolve;
extern bool opt_ignore_builtin_innodb;
extern my_bool opt_character_set_client_handshake;
extern bool volatile abort_loop, shutdown_in_progress;
extern bool in_bootstrap;
extern uint volatile thread_count, thread_running, global_read_lock;
extern uint connection_count;
extern my_bool opt_sql_bin_update, opt_safe_user_create, opt_no_mix_types;
extern my_bool opt_safe_show_db, opt_local_infile, opt_myisam_use_mmap;
extern my_bool opt_slave_compressed_protocol, use_temp_pool;
extern ulong slave_exec_mode_options;
extern my_bool opt_readonly, lower_case_file_system;
extern my_bool opt_enable_named_pipe, opt_sync_frm, opt_allow_suspicious_udfs;
extern my_bool opt_secure_auth;
extern char* opt_secure_file_priv;
extern my_bool opt_log_slow_admin_statements, opt_log_slow_slave_statements;
extern my_bool sp_automatic_privileges, opt_noacl;
extern my_bool opt_old_style_user_limits, trust_function_creators;
extern uint opt_crash_binlog_innodb;
extern char *shared_memory_base_name, *mysqld_unix_port;
extern my_bool opt_enable_shared_memory;
extern char *default_tz_name;
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
extern my_bool opt_large_pages;
extern uint opt_large_page_size;
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
extern char *opt_logname, *opt_slow_logname;
extern const char *log_output_str;

extern MYSQL_PLUGIN_IMPORT MYSQL_BIN_LOG mysql_bin_log;
extern LOGGER logger;
extern TABLE_LIST general_log, slow_log;
extern FILE *bootstrap_file;
extern int bootstrap_error;
extern FILE *stderror_file;
/*
  THR_MALLOC is a key which will be used to set/get MEM_ROOT** for a thread,
  using my_pthread_setspecific_ptr()/my_thread_getspecific_ptr().
*/
extern pthread_key(MEM_ROOT**,THR_MALLOC);
extern pthread_mutex_t LOCK_mysql_create_db,LOCK_Acl,LOCK_open, LOCK_lock_db,
       LOCK_mapped_file,LOCK_user_locks, LOCK_status,
       LOCK_error_log, LOCK_delayed_insert, LOCK_uuid_generator,
       LOCK_delayed_status, LOCK_delayed_create, LOCK_crypt, LOCK_timezone,
       LOCK_slave_list, LOCK_active_mi, LOCK_manager, LOCK_global_read_lock,
       LOCK_global_system_variables, LOCK_user_conn,
       LOCK_prepared_stmt_count,
       LOCK_bytes_sent, LOCK_bytes_received, LOCK_connection_count;
extern MYSQL_PLUGIN_IMPORT pthread_mutex_t LOCK_thread_count;
#ifdef HAVE_OPENSSL
extern pthread_mutex_t LOCK_des_key_file;
#endif
extern pthread_mutex_t LOCK_server_started;
extern pthread_cond_t COND_server_started;
extern int mysqld_server_started;
extern rw_lock_t LOCK_grant, LOCK_sys_init_connect, LOCK_sys_init_slave;
extern rw_lock_t LOCK_system_variables_hash;
extern pthread_cond_t COND_refresh, COND_thread_count, COND_manager;
extern pthread_cond_t COND_global_read_lock;
extern pthread_attr_t connection_attrib;
extern I_List<THD> threads;
extern I_List<NAMED_LIST> key_caches;
extern MY_BITMAP temp_pool;
extern String my_empty_string;
extern const String my_null_string;
extern SHOW_VAR status_vars[];
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
extern MYSQL_PLUGIN_IMPORT struct system_variables global_system_variables;
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
extern struct system_variables max_system_variables;
extern struct system_status_var global_status_var;
extern struct rand_struct sql_rand;

extern const char *opt_date_time_formats[];
extern KNOWN_DATE_TIME_FORMAT known_date_time_formats[];

extern String null_string;
extern HASH open_cache, lock_db_cache;
extern TABLE *unused_tables;
extern const char* any_db;
extern struct my_option my_long_options[];
extern const LEX_STRING view_type;
extern scheduler_functions thread_scheduler;
extern TYPELIB thread_handling_typelib;
extern uint8 uc_update_queries[SQLCOM_END+1];
extern uint sql_command_flags[];
extern TYPELIB log_output_typelib;

/* optional things, have_* variables */
extern SHOW_COMP_OPTION have_community_features;

extern handlerton *partition_hton;
extern handlerton *myisam_hton;
extern handlerton *heap_hton;

extern SHOW_COMP_OPTION have_ssl, have_symlink, have_dlopen;
extern SHOW_COMP_OPTION have_query_cache;
extern SHOW_COMP_OPTION have_geometry, have_rtree_keys;
extern SHOW_COMP_OPTION have_crypt;
extern SHOW_COMP_OPTION have_compress;

extern int orig_argc;
extern char **orig_argv;
extern const char *load_default_groups[];

#ifndef __WIN__
extern pthread_t signal_thread;
#endif

#ifdef HAVE_OPENSSL
extern struct st_VioSSLFd * ssl_acceptor_fd;
#endif /* HAVE_OPENSSL */

MYSQL_LOCK *mysql_lock_tables(THD *thd, TABLE **table, uint count,
                              uint flags, bool *need_reopen);
/* mysql_lock_tables() and open_table() flags bits */
#define MYSQL_LOCK_IGNORE_GLOBAL_READ_LOCK      0x0001
#define MYSQL_LOCK_IGNORE_FLUSH                 0x0002
#define MYSQL_LOCK_NOTIFY_IF_NEED_REOPEN        0x0004
#define MYSQL_OPEN_TEMPORARY_ONLY               0x0008
#define MYSQL_LOCK_IGNORE_GLOBAL_READ_ONLY      0x0010
#define MYSQL_LOCK_PERF_SCHEMA                  0x0020

void mysql_unlock_tables(THD *thd, MYSQL_LOCK *sql_lock);
void mysql_unlock_read_tables(THD *thd, MYSQL_LOCK *sql_lock);
void mysql_unlock_some_tables(THD *thd, TABLE **table,uint count);
void mysql_lock_remove(THD *thd, MYSQL_LOCK *locked,TABLE *table,
                       bool always_unlock);
void mysql_lock_abort(THD *thd, TABLE *table, bool upgrade_lock);
void mysql_lock_downgrade_write(THD *thd, TABLE *table,
                                thr_lock_type new_lock_type);
bool mysql_lock_abort_for_thread(THD *thd, TABLE *table);
MYSQL_LOCK *mysql_lock_merge(MYSQL_LOCK *a,MYSQL_LOCK *b);
TABLE_LIST *mysql_lock_have_duplicate(THD *thd, TABLE_LIST *needle,
                                      TABLE_LIST *haystack);
bool lock_global_read_lock(THD *thd);
void unlock_global_read_lock(THD *thd);
bool wait_if_global_read_lock(THD *thd, bool abort_on_refresh,
                              bool is_not_commit);
void start_waiting_global_read_lock(THD *thd);
bool make_global_read_lock_block_commit(THD *thd);
bool set_protect_against_global_read_lock(void);
void unset_protect_against_global_read_lock(void);
void broadcast_refresh(void);

/* Lock based on name */
int lock_and_wait_for_table_name(THD *thd, TABLE_LIST *table_list);
int lock_table_name(THD *thd, TABLE_LIST *table_list, bool check_in_use);
void unlock_table_name(THD *thd, TABLE_LIST *table_list);
bool wait_for_locked_table_names(THD *thd, TABLE_LIST *table_list);
bool lock_table_names(THD *thd, TABLE_LIST *table_list);
void unlock_table_names(THD *thd, TABLE_LIST *table_list,
			TABLE_LIST *last_table);
bool lock_table_names_exclusively(THD *thd, TABLE_LIST *table_list);
bool is_table_name_exclusively_locked_by_this_thread(THD *thd, 
                                                     TABLE_LIST *table_list);
bool is_table_name_exclusively_locked_by_this_thread(THD *thd, uchar *key,
                                                     int key_length);


/* old unireg functions */

void unireg_init(ulong options);
void unireg_end(void) __attribute__((noreturn));
bool mysql_create_frm(THD *thd, const char *file_name,
                      const char *db, const char *table,
		      HA_CREATE_INFO *create_info,
		      List<Create_field> &create_field,
		      uint key_count,KEY *key_info,handler *db_type);
int rea_create_table(THD *thd, const char *path,
                     const char *db, const char *table_name,
                     HA_CREATE_INFO *create_info,
  		     List<Create_field> &create_field,
                     uint key_count,KEY *key_info,
                     handler *file);
int format_number(uint inputflag,uint max_length,char * pos,uint length,
		  char * *errpos);

/* table.cc */
TABLE_SHARE *alloc_table_share(TABLE_LIST *table_list, char *key,
                               uint key_length);
void init_tmp_table_share(THD *thd, TABLE_SHARE *share, const char *key,
                          uint key_length,
                          const char *table_name, const char *path);
void free_table_share(TABLE_SHARE *share);
int open_table_def(THD *thd, TABLE_SHARE *share, uint db_flags);
void open_table_error(TABLE_SHARE *share, int error, int db_errno, int errarg);
int open_table_from_share(THD *thd, TABLE_SHARE *share, const char *alias,
                          uint db_stat, uint prgflag, uint ha_open_flags,
                          TABLE *outparam, bool is_create_table);
int readfrm(const char *name, uchar **data, size_t *length);
int writefrm(const char* name, const uchar* data, size_t len);
int closefrm(TABLE *table, bool free_share);
int read_string(File file, uchar* *to, size_t length);
void free_blobs(TABLE *table);
void free_field_buffers_larger_than(TABLE *table, uint32 size);
int set_zone(int nr,int min_zone,int max_zone);
ulong convert_period_to_month(ulong period);
ulong convert_month_to_period(ulong month);
void get_date_from_daynr(long daynr,uint *year, uint *month,
			 uint *day);
my_time_t TIME_to_timestamp(THD *thd, const MYSQL_TIME *t, my_bool *not_exist);
bool str_to_time_with_warn(const char *str,uint length,MYSQL_TIME *l_time);
timestamp_type str_to_datetime_with_warn(const char *str, uint length,
                                         MYSQL_TIME *l_time, uint flags);
void localtime_to_TIME(MYSQL_TIME *to, struct tm *from);
void calc_time_from_sec(MYSQL_TIME *to, long seconds, long microseconds);

void make_truncated_value_warning(THD *thd, MYSQL_ERROR::enum_warning_level level,
                                  const char *str_val,
				  uint str_length, timestamp_type time_type,
                                  const char *field_name);

bool date_add_interval(MYSQL_TIME *ltime, interval_type int_type, INTERVAL interval);
bool calc_time_diff(MYSQL_TIME *l_time1, MYSQL_TIME *l_time2, int l_sign,
                    longlong *seconds_out, long *microseconds_out);

extern LEX_STRING interval_type_to_name[];

extern DATE_TIME_FORMAT *date_time_format_make(timestamp_type format_type,
					       const char *format_str,
					       uint format_length);
extern DATE_TIME_FORMAT *date_time_format_copy(THD *thd,
					       DATE_TIME_FORMAT *format);
const char *get_date_time_format_str(KNOWN_DATE_TIME_FORMAT *format,
				     timestamp_type type);
extern bool make_date_time(DATE_TIME_FORMAT *format, MYSQL_TIME *l_time,
			   timestamp_type type, String *str);
void make_datetime(const DATE_TIME_FORMAT *format, const MYSQL_TIME *l_time,
                   String *str);
void make_date(const DATE_TIME_FORMAT *format, const MYSQL_TIME *l_time,
               String *str);
void make_time(const DATE_TIME_FORMAT *format, const MYSQL_TIME *l_time,
               String *str);
int my_time_compare(MYSQL_TIME *a, MYSQL_TIME *b);
longlong get_datetime_value(THD *thd, Item ***item_arg, Item **cache_arg,
                            Item *warn_item, bool *is_null);

int test_if_number(char *str,int *res,bool allow_wildcards);
void change_byte(uchar *,uint,char,char);
void init_read_record(READ_RECORD *info, THD *thd, TABLE *reg_form,
		      SQL_SELECT *select, int use_record_cache, 
                      bool print_errors, bool disable_rr_cache);
void init_read_record_idx(READ_RECORD *info, THD *thd, TABLE *table, 
                          bool print_error, uint idx);
void end_read_record(READ_RECORD *info);
ha_rows filesort(THD *thd, TABLE *form,struct st_sort_field *sortorder,
		 uint s_length, SQL_SELECT *select,
		 ha_rows max_rows, bool sort_positions,
                 ha_rows *examined_rows);
void filesort_free_buffers(TABLE *table, bool full);
void change_double_for_sort(double nr,uchar *to);
double my_double_round(double value, longlong dec, bool dec_unsigned,
                       bool truncate);
int get_quick_record(SQL_SELECT *select);

int calc_weekday(long daynr,bool sunday_first_day_of_week);
uint calc_week(MYSQL_TIME *l_time, uint week_behaviour, uint *year);
void find_date(char *pos,uint *vek,uint flag);
TYPELIB *convert_strings_to_array_type(char * *typelibs, char * *end);
TYPELIB *typelib(MEM_ROOT *mem_root, List<String> &strings);
ulong get_form_pos(File file, uchar *head, TYPELIB *save_names);
ulong make_new_entry(File file,uchar *fileinfo,TYPELIB *formnames,
		     const char *newname);
ulong next_io_size(ulong pos);
void append_unescaped(String *res, const char *pos, uint length);
int create_frm(THD *thd, const char *name, const char *db, const char *table,
               uint reclength, uchar *fileinfo,
	       HA_CREATE_INFO *create_info, uint keys);
void update_create_info_from_table(HA_CREATE_INFO *info, TABLE *form);
int rename_file_ext(const char * from,const char * to,const char * ext);
bool check_db_name(LEX_STRING *db);
bool check_column_name(const char *name);
bool check_table_name(const char *name, uint length, bool check_for_path_chars);
char *get_field(MEM_ROOT *mem, Field *field);
bool get_field(MEM_ROOT *mem, Field *field, class String *res);
int wild_case_compare(CHARSET_INFO *cs, const char *str,const char *wildstr);
char *fn_rext(char *name);

/* Conversion functions */
#endif /* MYSQL_SERVER */
#if defined MYSQL_SERVER || defined INNODB_COMPATIBILITY_HOOKS
uint strconvert(CHARSET_INFO *from_cs, const char *from,
                CHARSET_INFO *to_cs, char *to, uint to_length, uint *errors);
/* depends on errmsg.txt Database `db`, Table `t` ... */
#define EXPLAIN_FILENAME_MAX_EXTRA_LENGTH 63
enum enum_explain_filename_mode
{
  EXPLAIN_ALL_VERBOSE= 0,
  EXPLAIN_PARTITIONS_VERBOSE,
  EXPLAIN_PARTITIONS_AS_COMMENT
};
uint explain_filename(THD* thd, const char *from, char *to, uint to_length,
                      enum_explain_filename_mode explain_mode);
uint filename_to_tablename(const char *from, char *to, uint to_length);
uint tablename_to_filename(const char *from, char *to, uint to_length);
uint check_n_cut_mysql50_prefix(const char *from, char *to, uint to_length);
bool check_mysql50_prefix(const char *name);
#endif /* MYSQL_SERVER || INNODB_COMPATIBILITY_HOOKS */
#ifdef MYSQL_SERVER
uint build_table_filename(char *buff, size_t bufflen, const char *db,
                          const char *table, const char *ext, uint flags);
const char *get_canonical_filename(handler *file, const char *path,
                                   char *tmp_path);

#define MYSQL50_TABLE_NAME_PREFIX         "#mysql50#"
#define MYSQL50_TABLE_NAME_PREFIX_LENGTH  9

uint build_table_shadow_filename(char *buff, size_t bufflen, 
                                 ALTER_PARTITION_PARAM_TYPE *lpt);
/* Flags for conversion functions. */
#define FN_FROM_IS_TMP  (1 << 0)
#define FN_TO_IS_TMP    (1 << 1)
#define FN_IS_TMP       (FN_FROM_IS_TMP | FN_TO_IS_TMP)
#define NO_FRM_RENAME   (1 << 2)
#define FRM_ONLY        (1 << 3)

/* from hostname.cc */
struct in_addr;
char * ip_to_hostname(struct in_addr *in,uint *errors);
void inc_host_errors(struct in_addr *in);
void reset_host_errors(struct in_addr *in);
bool hostname_cache_init();
void hostname_cache_free();
void hostname_cache_refresh(void);

/* sql_cache.cc */
extern bool sql_cache_init();
extern void sql_cache_free();
extern int sql_cache_hit(THD *thd, char *inBuf, uint length);

/* item_func.cc */
Item *get_system_var(THD *thd, enum_var_type var_type, LEX_STRING name,
		     LEX_STRING component);
int get_var_with_binlog(THD *thd, enum_sql_command sql_command,
                        LEX_STRING &name, user_var_entry **out_entry);
/* log.cc */
bool flush_error_log(void);

/* sql_list.cc */
void free_list(I_List <i_string_pair> *list);
void free_list(I_List <i_string> *list);

/* sql_yacc.cc */
#ifndef DBUG_OFF
extern void turn_parser_debug_on();
#endif

/* frm_crypt.cc */
#ifdef HAVE_CRYPTED_FRM
SQL_CRYPT *get_crypt_for_frm(void);
#endif

/* password.c */
extern "C" void my_make_scrambled_password_323(char *to, const char *password,
                                               size_t pass_len);
extern "C" void my_make_scrambled_password(char *to, const char *password,
                                           size_t pass_len);

#include "sql_view.h"

/* Some inline functions for more speed */

inline bool add_item_to_list(THD *thd, Item *item)
{
  return thd->lex->current_select->add_item_to_list(thd, item);
}

inline bool add_value_to_list(THD *thd, Item *value)
{
  return thd->lex->value_list.push_back(value);
}

inline bool add_order_to_list(THD *thd, Item *item, bool asc)
{
  return thd->lex->current_select->add_order_to_list(thd, item, asc);
}

inline bool add_group_to_list(THD *thd, Item *item, bool asc)
{
  return thd->lex->current_select->add_group_to_list(thd, item, asc);
}

inline void mark_as_null_row(TABLE *table)
{
  table->null_row=1;
  table->status|=STATUS_NULL_ROW;
  bfill(table->null_flags,table->s->null_bytes,255);
}

inline void table_case_convert(char * name, uint length)
{
  if (lower_case_table_names)
    files_charset_info->cset->casedn(files_charset_info,
                                     name, length, name, length);
}

inline const char *table_case_name(HA_CREATE_INFO *info, const char *name)
{
  return ((lower_case_table_names == 2 && info->alias) ? info->alias : name);
}

inline ulong sql_rnd_with_mutex()
{
  pthread_mutex_lock(&LOCK_thread_count);
  ulong tmp=(ulong) (my_rnd(&sql_rand) * 0xffffffff); /* make all bits random */
  pthread_mutex_unlock(&LOCK_thread_count);
  return tmp;
}

Comp_creator *comp_eq_creator(bool invert);
Comp_creator *comp_ge_creator(bool invert);
Comp_creator *comp_gt_creator(bool invert);
Comp_creator *comp_le_creator(bool invert);
Comp_creator *comp_lt_creator(bool invert);
Comp_creator *comp_ne_creator(bool invert);

Item * all_any_subquery_creator(Item *left_expr,
				chooser_compare_func_creator cmp,
				bool all,
				SELECT_LEX *select_lex);

/**
  clean/setup table fields and map.

  @param table        TABLE structure pointer (which should be setup)
  @param table_list   TABLE_LIST structure pointer (owner of TABLE)
  @param tablenr     table number
*/


inline void setup_table_map(TABLE *table, TABLE_LIST *table_list, uint tablenr)
{
  table->used_fields= 0;
  table->const_table= 0;
  table->null_row= 0;
  table->status= STATUS_NO_RECORD;
  table->maybe_null= table_list->outer_join;
  TABLE_LIST *embedding= table_list->embedding;
  while (!table->maybe_null && embedding)
  {
    table->maybe_null= embedding->outer_join;
    embedding= embedding->embedding;
  }
  table->tablenr= tablenr;
  table->map= (table_map) 1 << tablenr;
  table->force_index= table_list->force_index;
  table->force_index_order= table->force_index_group= 0;
  table->covering_keys= table->s->keys_for_keyread;
  table->merge_keys.clear_all();
}


/**
  convert a hex digit into number.
*/

inline int hexchar_to_int(char c)
{
  if (c <= '9' && c >= '0')
    return c-'0';
  c|=32;
  if (c <= 'f' && c >= 'a')
    return c-'a'+10;
  return -1;
}

/**
  return true if the table was created explicitly.
*/
inline bool is_user_table(TABLE * table)
{
  const char *name= table->s->table_name.str;
  return strncmp(name, tmp_file_prefix, tmp_file_prefix_length);
}

/*
  Some functions that are different in the embedded library and the normal
  server
*/

#ifndef EMBEDDED_LIBRARY
extern "C" void unireg_abort(int exit_code) __attribute__((noreturn));
void kill_delayed_threads(void);
bool check_stack_overrun(THD *thd, long margin, uchar *dummy);
#else
extern "C" void unireg_clear(int exit_code);
#define unireg_abort(exit_code) do { unireg_clear(exit_code); DBUG_RETURN(exit_code); } while(0)
inline void kill_delayed_threads(void) {}
#define check_stack_overrun(A, B, C) 0
#endif

/* Used by handlers to store things in schema tables */
#define IS_FILES_FILE_ID              0
#define IS_FILES_FILE_NAME            1
#define IS_FILES_FILE_TYPE            2
#define IS_FILES_TABLESPACE_NAME      3
#define IS_FILES_TABLE_CATALOG        4
#define IS_FILES_TABLE_SCHEMA         5
#define IS_FILES_TABLE_NAME           6
#define IS_FILES_LOGFILE_GROUP_NAME   7
#define IS_FILES_LOGFILE_GROUP_NUMBER 8
#define IS_FILES_ENGINE               9
#define IS_FILES_FULLTEXT_KEYS       10
#define IS_FILES_DELETED_ROWS        11
#define IS_FILES_UPDATE_COUNT        12
#define IS_FILES_FREE_EXTENTS        13
#define IS_FILES_TOTAL_EXTENTS       14
#define IS_FILES_EXTENT_SIZE         15
#define IS_FILES_INITIAL_SIZE        16
#define IS_FILES_MAXIMUM_SIZE        17
#define IS_FILES_AUTOEXTEND_SIZE     18
#define IS_FILES_CREATION_TIME       19
#define IS_FILES_LAST_UPDATE_TIME    20
#define IS_FILES_LAST_ACCESS_TIME    21
#define IS_FILES_RECOVER_TIME        22
#define IS_FILES_TRANSACTION_COUNTER 23
#define IS_FILES_VERSION             24
#define IS_FILES_ROW_FORMAT          25
#define IS_FILES_TABLE_ROWS          26
#define IS_FILES_AVG_ROW_LENGTH      27
#define IS_FILES_DATA_LENGTH         28
#define IS_FILES_MAX_DATA_LENGTH     29
#define IS_FILES_INDEX_LENGTH        30
#define IS_FILES_DATA_FREE           31
#define IS_FILES_CREATE_TIME         32
#define IS_FILES_UPDATE_TIME         33
#define IS_FILES_CHECK_TIME          34
#define IS_FILES_CHECKSUM            35
#define IS_FILES_STATUS              36
#define IS_FILES_EXTRA               37
void init_fill_schema_files_row(TABLE* table);
bool schema_table_store_record(THD *thd, TABLE *table);

/* sql/item_create.cc */
int item_create_init();
void item_create_cleanup();

bool show_create_trigger(THD *thd, const sp_name *trg_name);

inline void lex_string_set(LEX_STRING *lex_str, const char *c_str)
{
  lex_str->str= (char *) c_str;
  lex_str->length= strlen(c_str);
}

bool load_charset(MEM_ROOT *mem_root,
                  Field *field,
                  CHARSET_INFO *dflt_cs,
                  CHARSET_INFO **cs);

bool load_collation(MEM_ROOT *mem_root,
                    Field *field,
                    CHARSET_INFO *dflt_cl,
                    CHARSET_INFO **cl);

#endif /* MYSQL_SERVER */
extern "C" int test_if_data_home_dir(const char *dir);

#endif /* MYSQL_CLIENT */

#endif /* MYSQL_PRIV_H */
