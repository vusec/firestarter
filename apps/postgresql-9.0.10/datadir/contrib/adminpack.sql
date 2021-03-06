/* $PostgreSQL: pgsql/contrib/adminpack/adminpack.sql.in,v 1.6 2007/11/13 04:24:27 momjian Exp $ */

/* ***********************************************
 * Administrative functions for PostgreSQL
 * *********************************************** */

/* generic file access functions */

CREATE OR REPLACE FUNCTION pg_catalog.pg_file_write(text, text, bool)
RETURNS bigint
AS '$libdir/adminpack', 'pg_file_write'
LANGUAGE C VOLATILE STRICT;

CREATE OR REPLACE FUNCTION pg_catalog.pg_file_rename(text, text, text)
RETURNS bool
AS '$libdir/adminpack', 'pg_file_rename'
LANGUAGE C VOLATILE;

CREATE OR REPLACE FUNCTION pg_catalog.pg_file_rename(text, text)
RETURNS bool
AS 'SELECT pg_catalog.pg_file_rename($1, $2, NULL::pg_catalog.text);'
LANGUAGE SQL VOLATILE STRICT;

CREATE OR REPLACE FUNCTION pg_catalog.pg_file_unlink(text)
RETURNS bool
AS '$libdir/adminpack', 'pg_file_unlink'
LANGUAGE C VOLATILE STRICT;

CREATE OR REPLACE FUNCTION pg_catalog.pg_logdir_ls()
RETURNS setof record
AS '$libdir/adminpack', 'pg_logdir_ls'
LANGUAGE C VOLATILE STRICT;


/* Renaming of existing backend functions for pgAdmin compatibility */

CREATE OR REPLACE FUNCTION pg_catalog.pg_file_read(text, bigint, bigint)
RETURNS text
AS 'pg_read_file'
LANGUAGE INTERNAL VOLATILE STRICT;

CREATE OR REPLACE FUNCTION pg_catalog.pg_file_length(text)
RETURNS bigint
AS 'SELECT size FROM pg_catalog.pg_stat_file($1)'
LANGUAGE SQL VOLATILE STRICT;

CREATE OR REPLACE FUNCTION pg_catalog.pg_logfile_rotate()
RETURNS int4
AS 'pg_rotate_logfile'
LANGUAGE INTERNAL VOLATILE STRICT;
