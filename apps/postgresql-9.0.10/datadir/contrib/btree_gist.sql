/* $PostgreSQL: pgsql/contrib/btree_gist/btree_gist.sql.in,v 1.21 2009/06/11 18:30:03 tgl Exp $ */

-- Adjust this setting to control where the objects get created.
SET search_path = public;

CREATE OR REPLACE FUNCTION gbtreekey4_in(cstring)
RETURNS gbtreekey4
AS '$libdir/btree_gist', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbtreekey4_out(gbtreekey4)
RETURNS cstring
AS '$libdir/btree_gist', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey4 (
	INTERNALLENGTH = 4,
	INPUT  = gbtreekey4_in,
	OUTPUT = gbtreekey4_out
);

CREATE OR REPLACE FUNCTION gbtreekey8_in(cstring)
RETURNS gbtreekey8
AS '$libdir/btree_gist', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbtreekey8_out(gbtreekey8)
RETURNS cstring
AS '$libdir/btree_gist', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey8 (
	INTERNALLENGTH = 8,
	INPUT  = gbtreekey8_in,
	OUTPUT = gbtreekey8_out
);

CREATE OR REPLACE FUNCTION gbtreekey16_in(cstring)
RETURNS gbtreekey16
AS '$libdir/btree_gist', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbtreekey16_out(gbtreekey16)
RETURNS cstring
AS '$libdir/btree_gist', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey16 (
	INTERNALLENGTH = 16,
	INPUT  = gbtreekey16_in,
	OUTPUT = gbtreekey16_out
);

CREATE OR REPLACE FUNCTION gbtreekey32_in(cstring)
RETURNS gbtreekey32
AS '$libdir/btree_gist', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbtreekey32_out(gbtreekey32)
RETURNS cstring
AS '$libdir/btree_gist', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey32 (
	INTERNALLENGTH = 32,
	INPUT  = gbtreekey32_in,
	OUTPUT = gbtreekey32_out
);

CREATE OR REPLACE FUNCTION gbtreekey_var_in(cstring)
RETURNS gbtreekey_var
AS '$libdir/btree_gist', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbtreekey_var_out(gbtreekey_var)
RETURNS cstring
AS '$libdir/btree_gist', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE gbtreekey_var (
	INTERNALLENGTH = VARIABLE,
	INPUT  = gbtreekey_var_in,
	OUTPUT = gbtreekey_var_out,
	STORAGE = EXTENDED
);



--
--
--
-- oid ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_oid_consistent(internal,oid,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_oid_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_decompress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_var_decompress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_oid_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_oid_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_oid_union(bytea, internal)
RETURNS gbtreekey8
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_oid_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_oid_ops
DEFAULT FOR TYPE oid USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_oid_consistent (internal, oid, int2, oid, internal),
	FUNCTION	2	gbt_oid_union (bytea, internal),
	FUNCTION	3	gbt_oid_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_oid_penalty (internal, internal, internal),
	FUNCTION	6	gbt_oid_picksplit (internal, internal),
	FUNCTION	7	gbt_oid_same (internal, internal, internal),
	STORAGE		gbtreekey8;


--
--
--
-- int2 ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_int2_consistent(internal,int2,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int2_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int2_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int2_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int2_union(bytea, internal)
RETURNS gbtreekey4
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int2_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_int2_ops
DEFAULT FOR TYPE int2 USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_int2_consistent (internal, int2, int2, oid, internal),
	FUNCTION	2	gbt_int2_union (bytea, internal),
	FUNCTION	3	gbt_int2_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_int2_penalty (internal, internal, internal),
	FUNCTION	6	gbt_int2_picksplit (internal, internal),
	FUNCTION	7	gbt_int2_same (internal, internal, internal),
	STORAGE		gbtreekey4;

--
--
--
-- int4 ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_int4_consistent(internal,int4,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int4_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int4_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int4_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int4_union(bytea, internal)
RETURNS gbtreekey8
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int4_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_int4_ops
DEFAULT FOR TYPE int4 USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_int4_consistent (internal, int4, int2, oid, internal),
	FUNCTION	2	gbt_int4_union (bytea, internal),
	FUNCTION	3	gbt_int4_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_int4_penalty (internal, internal, internal),
	FUNCTION	6	gbt_int4_picksplit (internal, internal),
	FUNCTION	7	gbt_int4_same (internal, internal, internal),
	STORAGE		gbtreekey8;

--
--
--
-- int8 ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_int8_consistent(internal,int8,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int8_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int8_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int8_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int8_union(bytea, internal)
RETURNS gbtreekey16
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_int8_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_int8_ops
DEFAULT FOR TYPE int8 USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_int8_consistent (internal, int8, int2, oid, internal),
	FUNCTION	2	gbt_int8_union (bytea, internal),
	FUNCTION	3	gbt_int8_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_int8_penalty (internal, internal, internal),
	FUNCTION	6	gbt_int8_picksplit (internal, internal),
	FUNCTION	7	gbt_int8_same (internal, internal, internal),
	STORAGE		gbtreekey16;


--
--
--
-- float4 ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_float4_consistent(internal,float4,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float4_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float4_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float4_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float4_union(bytea, internal)
RETURNS gbtreekey8
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float4_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_float4_ops
DEFAULT FOR TYPE float4 USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_float4_consistent (internal, float4, int2, oid, internal),
	FUNCTION	2	gbt_float4_union (bytea, internal),
	FUNCTION	3	gbt_float4_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_float4_penalty (internal, internal, internal),
	FUNCTION	6	gbt_float4_picksplit (internal, internal),
	FUNCTION	7	gbt_float4_same (internal, internal, internal),
	STORAGE		gbtreekey8;




--
--
--
-- float8 ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_float8_consistent(internal,float8,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float8_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float8_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float8_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float8_union(bytea, internal)
RETURNS gbtreekey16
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_float8_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_float8_ops
DEFAULT FOR TYPE float8 USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_float8_consistent (internal, float8, int2, oid, internal),
	FUNCTION	2	gbt_float8_union (bytea, internal),
	FUNCTION	3	gbt_float8_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_float8_penalty (internal, internal, internal),
	FUNCTION	6	gbt_float8_picksplit (internal, internal),
	FUNCTION	7	gbt_float8_same (internal, internal, internal),
	STORAGE		gbtreekey16;


--
--
--
-- timestamp ops
-- 
--
--

CREATE OR REPLACE FUNCTION gbt_ts_consistent(internal,timestamp,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_tstz_consistent(internal,timestamptz,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
      
CREATE OR REPLACE FUNCTION gbt_ts_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_tstz_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_ts_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
   
CREATE OR REPLACE FUNCTION gbt_ts_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
      
CREATE OR REPLACE FUNCTION gbt_ts_union(bytea, internal)
RETURNS gbtreekey16
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_ts_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_timestamp_ops
DEFAULT FOR TYPE timestamp USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_ts_consistent (internal, timestamp, int2, oid, internal),
	FUNCTION	2	gbt_ts_union (bytea, internal),
	FUNCTION	3	gbt_ts_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_ts_penalty (internal, internal, internal),
	FUNCTION	6	gbt_ts_picksplit (internal, internal),
	FUNCTION	7	gbt_ts_same (internal, internal, internal),
	STORAGE		gbtreekey16;


-- Create the operator class
CREATE OPERATOR CLASS gist_timestamptz_ops
DEFAULT FOR TYPE timestamptz USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_tstz_consistent (internal, timestamptz, int2, oid, internal),
	FUNCTION	2	gbt_ts_union (bytea, internal),
	FUNCTION	3	gbt_tstz_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_ts_penalty (internal, internal, internal),
	FUNCTION	6	gbt_ts_picksplit (internal, internal),
	FUNCTION	7	gbt_ts_same (internal, internal, internal),
	STORAGE		gbtreekey16;


--
--
--
-- time ops
-- 
--
--

CREATE OR REPLACE FUNCTION gbt_time_consistent(internal,time,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_timetz_consistent(internal,timetz,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_time_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_timetz_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_time_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
   
CREATE OR REPLACE FUNCTION gbt_time_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
      
CREATE OR REPLACE FUNCTION gbt_time_union(bytea, internal)
RETURNS gbtreekey16
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_time_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_time_ops
DEFAULT FOR TYPE time USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_time_consistent (internal, time, int2, oid, internal),
	FUNCTION	2	gbt_time_union (bytea, internal),
	FUNCTION	3	gbt_time_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_time_penalty (internal, internal, internal),
	FUNCTION	6	gbt_time_picksplit (internal, internal),
	FUNCTION	7	gbt_time_same (internal, internal, internal),
	STORAGE		gbtreekey16;

CREATE OPERATOR CLASS gist_timetz_ops
DEFAULT FOR TYPE timetz USING gist 
AS
	OPERATOR	1	<   ,
	OPERATOR	2	<=  ,
	OPERATOR	3	=   ,
	OPERATOR	4	>=  ,
	OPERATOR	5	>   ,
	FUNCTION	1	gbt_timetz_consistent (internal, timetz, int2, oid, internal),
	FUNCTION	2	gbt_time_union (bytea, internal),
	FUNCTION	3	gbt_timetz_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_time_penalty (internal, internal, internal),
	FUNCTION	6	gbt_time_picksplit (internal, internal),
	FUNCTION	7	gbt_time_same (internal, internal, internal),
	STORAGE		gbtreekey16;


--
--
--
-- date ops
-- 
--
--

CREATE OR REPLACE FUNCTION gbt_date_consistent(internal,date,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_date_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_date_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
   
CREATE OR REPLACE FUNCTION gbt_date_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
      
CREATE OR REPLACE FUNCTION gbt_date_union(bytea, internal)
RETURNS gbtreekey8
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_date_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_date_ops
DEFAULT FOR TYPE date USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_date_consistent (internal, date, int2, oid, internal),
	FUNCTION	2	gbt_date_union (bytea, internal),
	FUNCTION	3	gbt_date_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_date_penalty (internal, internal, internal),
	FUNCTION	6	gbt_date_picksplit (internal, internal),
	FUNCTION	7	gbt_date_same (internal, internal, internal),
	STORAGE		gbtreekey8;


--
--
--
-- interval ops
-- 
--
--

CREATE OR REPLACE FUNCTION gbt_intv_consistent(internal,interval,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_intv_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_intv_decompress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_intv_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
   
CREATE OR REPLACE FUNCTION gbt_intv_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;
      
CREATE OR REPLACE FUNCTION gbt_intv_union(bytea, internal)
RETURNS gbtreekey32
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_intv_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_interval_ops
DEFAULT FOR TYPE interval USING gist 
AS
	OPERATOR	1	< ,
	OPERATOR	2	<= ,
	OPERATOR	3	= ,
	OPERATOR	4	>= ,
	OPERATOR	5	> ,
	FUNCTION	1	gbt_intv_consistent (internal, interval, int2, oid, internal),
	FUNCTION	2	gbt_intv_union (bytea, internal),
	FUNCTION	3	gbt_intv_compress (internal),
	FUNCTION	4	gbt_intv_decompress (internal),
	FUNCTION	5	gbt_intv_penalty (internal, internal, internal),
	FUNCTION	6	gbt_intv_picksplit (internal, internal),
	FUNCTION	7	gbt_intv_same (internal, internal, internal),
	STORAGE		gbtreekey32;

--
--
--
-- cash ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_cash_consistent(internal,money,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_cash_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_cash_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_cash_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_cash_union(bytea, internal)
RETURNS gbtreekey8
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_cash_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_cash_ops
DEFAULT FOR TYPE money USING gist 
AS
	OPERATOR	1	< ,
	OPERATOR	2	<= ,
	OPERATOR	3	= ,
	OPERATOR	4	>= ,
	OPERATOR	5	> ,
	FUNCTION	1	gbt_cash_consistent (internal, money, int2, oid, internal),
	FUNCTION	2	gbt_cash_union (bytea, internal),
	FUNCTION	3	gbt_cash_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_cash_penalty (internal, internal, internal),
	FUNCTION	6	gbt_cash_picksplit (internal, internal),
	FUNCTION	7	gbt_cash_same (internal, internal, internal),
	STORAGE		gbtreekey16;

--
--
--
-- macaddr ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_macad_consistent(internal,macaddr,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_macad_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_macad_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_macad_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_macad_union(bytea, internal)
RETURNS gbtreekey16
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_macad_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_macaddr_ops
DEFAULT FOR TYPE macaddr USING gist 
AS
	OPERATOR	1	< ,
	OPERATOR	2	<= ,
	OPERATOR	3	= ,
	OPERATOR	4	>= ,
	OPERATOR	5	> ,
	FUNCTION	1	gbt_macad_consistent (internal, macaddr, int2, oid, internal),
	FUNCTION	2	gbt_macad_union (bytea, internal),
	FUNCTION	3	gbt_macad_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_macad_penalty (internal, internal, internal),
	FUNCTION	6	gbt_macad_picksplit (internal, internal),
	FUNCTION	7	gbt_macad_same (internal, internal, internal),
	STORAGE		gbtreekey16;



--
--
--
-- text/ bpchar ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_text_consistent(internal,text,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bpchar_consistent(internal,bpchar,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_text_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bpchar_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_text_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_text_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_text_union(bytea, internal)
RETURNS gbtreekey_var
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_text_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_text_ops
DEFAULT FOR TYPE text USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_text_consistent (internal, text, int2, oid, internal),
	FUNCTION	2	gbt_text_union (bytea, internal),
	FUNCTION	3	gbt_text_compress (internal),
	FUNCTION	4	gbt_var_decompress (internal),
	FUNCTION	5	gbt_text_penalty (internal, internal, internal),
	FUNCTION	6	gbt_text_picksplit (internal, internal),
	FUNCTION	7	gbt_text_same (internal, internal, internal),
	STORAGE	                gbtreekey_var;


---- Create the operator class
CREATE OPERATOR CLASS gist_bpchar_ops
DEFAULT FOR TYPE bpchar USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_bpchar_consistent (internal, bpchar , int2, oid, internal),
	FUNCTION	2	gbt_text_union (bytea, internal),
	FUNCTION	3	gbt_bpchar_compress (internal),
	FUNCTION	4	gbt_var_decompress (internal),
	FUNCTION	5	gbt_text_penalty (internal, internal, internal),
	FUNCTION	6	gbt_text_picksplit (internal, internal),
	FUNCTION	7	gbt_text_same (internal, internal, internal),
	STORAGE	                gbtreekey_var;



--
--
-- bytea ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_bytea_consistent(internal,bytea,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bytea_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bytea_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bytea_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bytea_union(bytea, internal)
RETURNS gbtreekey_var
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bytea_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_bytea_ops
DEFAULT FOR TYPE bytea USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_bytea_consistent (internal, bytea, int2, oid, internal),
	FUNCTION	2	gbt_bytea_union (bytea, internal),
	FUNCTION	3	gbt_bytea_compress (internal),
	FUNCTION	4	gbt_var_decompress (internal),
	FUNCTION	5	gbt_bytea_penalty (internal, internal, internal),
	FUNCTION	6	gbt_bytea_picksplit (internal, internal),
	FUNCTION	7	gbt_bytea_same (internal, internal, internal),
	STORAGE	                gbtreekey_var;


--
--
--
-- numeric ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_numeric_consistent(internal,numeric,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_numeric_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_numeric_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_numeric_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_numeric_union(bytea, internal)
RETURNS gbtreekey_var
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_numeric_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_numeric_ops
DEFAULT FOR TYPE numeric USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_numeric_consistent (internal, numeric, int2, oid, internal),
	FUNCTION	2	gbt_numeric_union (bytea, internal),
	FUNCTION	3	gbt_numeric_compress (internal),
	FUNCTION	4	gbt_var_decompress (internal),
	FUNCTION	5	gbt_numeric_penalty (internal, internal, internal),
	FUNCTION	6	gbt_numeric_picksplit (internal, internal),
	FUNCTION	7	gbt_numeric_same (internal, internal, internal),
	STORAGE	                gbtreekey_var;

--
--
-- bit ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_bit_consistent(internal,bit,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bit_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bit_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bit_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bit_union(bytea, internal)
RETURNS gbtreekey_var
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_bit_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_bit_ops
DEFAULT FOR TYPE bit USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_bit_consistent (internal, bit, int2, oid, internal),
	FUNCTION	2	gbt_bit_union (bytea, internal),
	FUNCTION	3	gbt_bit_compress (internal),
	FUNCTION	4	gbt_var_decompress (internal),
	FUNCTION	5	gbt_bit_penalty (internal, internal, internal),
	FUNCTION	6	gbt_bit_picksplit (internal, internal),
	FUNCTION	7	gbt_bit_same (internal, internal, internal),
	STORAGE	                gbtreekey_var;


-- Create the operator class
CREATE OPERATOR CLASS gist_vbit_ops
DEFAULT FOR TYPE varbit USING gist 
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	gbt_bit_consistent (internal, bit, int2, oid, internal),
	FUNCTION	2	gbt_bit_union (bytea, internal),
	FUNCTION	3	gbt_bit_compress (internal),
	FUNCTION	4	gbt_var_decompress (internal),
	FUNCTION	5	gbt_bit_penalty (internal, internal, internal),
	FUNCTION	6	gbt_bit_picksplit (internal, internal),
	FUNCTION	7	gbt_bit_same (internal, internal, internal),
	STORAGE	                gbtreekey_var;



--
--
--
-- inet/cidr ops
--
--
--
-- define the GiST support methods
CREATE OR REPLACE FUNCTION gbt_inet_consistent(internal,inet,int2,oid,internal)
RETURNS bool
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_inet_compress(internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_inet_penalty(internal,internal,internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_inet_picksplit(internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_inet_union(bytea, internal)
RETURNS gbtreekey16
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION gbt_inet_same(internal, internal, internal)
RETURNS internal
AS '$libdir/btree_gist'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_inet_ops
DEFAULT FOR TYPE inet USING gist 
AS
	OPERATOR	1	<   ,
	OPERATOR	2	<=  ,
	OPERATOR	3	=   ,
	OPERATOR	4	>=  ,
	OPERATOR	5	>   ,
	FUNCTION	1	gbt_inet_consistent (internal, inet, int2, oid, internal),
	FUNCTION	2	gbt_inet_union (bytea, internal),
	FUNCTION	3	gbt_inet_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_inet_penalty (internal, internal, internal),
	FUNCTION	6	gbt_inet_picksplit (internal, internal),
	FUNCTION	7	gbt_inet_same (internal, internal, internal),
	STORAGE		gbtreekey16;

-- Create the operator class
CREATE OPERATOR CLASS gist_cidr_ops
DEFAULT FOR TYPE cidr USING gist 
AS
	OPERATOR	1	<  (inet, inet)  ,
	OPERATOR	2	<= (inet, inet)  ,
	OPERATOR	3	=  (inet, inet)  ,
	OPERATOR	4	>= (inet, inet)  ,
	OPERATOR	5	>  (inet, inet)  ,
	FUNCTION	1	gbt_inet_consistent (internal, inet, int2, oid, internal),
	FUNCTION	2	gbt_inet_union (bytea, internal),
	FUNCTION	3	gbt_inet_compress (internal),
	FUNCTION	4	gbt_decompress (internal),
	FUNCTION	5	gbt_inet_penalty (internal, internal, internal),
	FUNCTION	6	gbt_inet_picksplit (internal, internal),
	FUNCTION	7	gbt_inet_same (internal, internal, internal),
	STORAGE		gbtreekey16;
