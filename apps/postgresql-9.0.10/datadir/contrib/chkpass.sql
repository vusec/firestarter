/* $PostgreSQL: pgsql/contrib/chkpass/chkpass.sql.in,v 1.9.16.1 2010/07/28 20:34:12 petere Exp $ */

-- Adjust this setting to control where the objects get created.
SET search_path = public;

--
--	Input and output functions and the type itself:
--

CREATE OR REPLACE FUNCTION chkpass_in(cstring)
	RETURNS chkpass
	AS '$libdir/chkpass'
	LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION chkpass_out(chkpass)
	RETURNS cstring
	AS '$libdir/chkpass'
	LANGUAGE C STRICT;

CREATE TYPE chkpass (
	internallength = 16,
	input = chkpass_in,
	output = chkpass_out
);

CREATE OR REPLACE FUNCTION raw(chkpass)
	RETURNS text
	AS '$libdir/chkpass', 'chkpass_rout'
	LANGUAGE C STRICT;

--
--	The various boolean tests:
--

CREATE OR REPLACE FUNCTION eq(chkpass, text)
	RETURNS bool
	AS '$libdir/chkpass', 'chkpass_eq'
	LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION ne(chkpass, text)
	RETURNS bool
	AS '$libdir/chkpass', 'chkpass_ne'
	LANGUAGE C STRICT;

--
--	Now the operators.
--

CREATE OPERATOR = (
	leftarg = chkpass,
	rightarg = text,
	negator = <>,
	procedure = eq
);

CREATE OPERATOR <> (
	leftarg = chkpass,
	rightarg = text,
	negator = =,
	procedure = ne
);

COMMENT ON TYPE chkpass IS 'password type with checks';

--
--	eof
--
