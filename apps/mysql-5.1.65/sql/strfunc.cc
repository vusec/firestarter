/*
   Copyright (c) 2003-2007 MySQL AB, 2009 Sun Microsystems, Inc.
   Use is subject to license terms.

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

/* Some useful string utility functions used by the MySQL server */

#include "mysql_priv.h"

/*
  Return bitmap for strings used in a set

  SYNOPSIS
  find_set()
  lib			Strings in set
  str			Strings of set-strings separated by ','
  err_pos		If error, set to point to start of wrong set string
  err_len		If error, set to the length of wrong set string
  set_warning		Set to 1 if some string in set couldn't be used

  NOTE
    We delete all end space from str before comparison

  RETURN
    bitmap of all sets found in x.
    set_warning is set to 1 if there was any sets that couldn't be set
*/

static const char field_separator=',';

ulonglong find_set(TYPELIB *lib, const char *str, uint length, CHARSET_INFO *cs,
                   char **err_pos, uint *err_len, bool *set_warning)
{
  CHARSET_INFO *strip= cs ? cs : &my_charset_latin1;
  const char *end= str + strip->cset->lengthsp(strip, str, length);
  ulonglong found= 0;
  *err_pos= 0;                  // No error yet
  if (str != end)
  {
    const char *start= str;    
    for (;;)
    {
      const char *pos= start;
      uint var_len;
      int mblen= 1;

      if (cs && cs->mbminlen > 1)
      {
        for ( ; pos < end; pos+= mblen)
        {
          my_wc_t wc;
          if ((mblen= cs->cset->mb_wc(cs, &wc, (const uchar *) pos, 
                                               (const uchar *) end)) < 1)
            mblen= 1; // Not to hang on a wrong multibyte sequence
          if (wc == (my_wc_t) field_separator)
            break;
        }
      }
      else
        for (; pos != end && *pos != field_separator; pos++) ;
      var_len= (uint) (pos - start);
      uint find= cs ? find_type2(lib, start, var_len, cs) :
                      find_type(lib, start, var_len, (bool) 0);
      if (!find)
      {
        *err_pos= (char*) start;
        *err_len= var_len;
        *set_warning= 1;
      }
      else
        found|= ((longlong) 1 << (find - 1));
      if (pos >= end)
        break;
      start= pos + mblen;
    }
  }
  return found;
}


static const char *on_off_default_names[]=
{
  "off","on","default", NullS
};

static const unsigned int on_off_default_names_len[]=
{
  sizeof("off") - 1,
  sizeof("on") - 1,
  sizeof("default") - 1
};

static TYPELIB on_off_default_typelib= {array_elements(on_off_default_names)-1,
                                        "", on_off_default_names,
                                        (unsigned int *)on_off_default_names_len};


/*
  Parse a TYPELIB name from the buffer

  SYNOPSIS
    parse_name()
      lib          Set of names to scan for.
      strpos INOUT Start of the buffer (updated to point to the next
                   character after the name)
      end          End of the buffer
      cs           Charset used in the buffer

  DESCRIPTION
    Parse a TYPELIB name from the buffer. The buffer is assumed to contain
    one of the names specified in the TYPELIB, followed by comma, '=', or
    end of the buffer.

  RETURN
    0	No matching name
    >0  Offset+1 in typelib for matched name
*/

static uint parse_name(TYPELIB *lib, const char **strpos, const char *end, 
                       CHARSET_INFO *cs)
{
  const char *pos= *strpos;
  const char *start= pos;

  /* Find the length */
  if (cs && cs->mbminlen > 1)
  {
    int mblen= 0;
    for ( ; pos < end; pos+= mblen)
    {
      my_wc_t wc;
      if ((mblen= cs->cset->mb_wc(cs, &wc, (const uchar *) pos,
                                           (const uchar *) end)) < 1)
        mblen= 1; // Not to hang on a wrong multibyte sequence
      if (wc == (my_wc_t) '=' || wc == (my_wc_t) ',')
        break;
    }
  }
  else
    for (; pos != end && *pos != '=' && *pos !=',' ; pos++) ;

  uint var_len= (uint) (pos - start);
  /* Determine which flag it is */
  uint find= cs ? find_type2(lib, start, var_len, cs) :
                  find_type(lib, start, var_len, (bool) 0);
  *strpos= pos;
  return find;
}


/* Read next character from the buffer in a charset-aware way */

static my_wc_t get_next_char(const char **pos, const char *end, CHARSET_INFO *cs)
{
  my_wc_t wc;
  if (*pos == end)
      return (my_wc_t)-1;

  if (cs && cs->mbminlen > 1)
  {
    int mblen;
    if ((mblen= cs->cset->mb_wc(cs, &wc, (const uchar *) *pos,
                                         (const uchar *) end)) < 1)
      mblen= 1; // Not to hang on a wrong multibyte sequence
    *pos += mblen;
    return wc;
  }
  else
    return *((*pos)++);
}


/*
  Parse and apply a set of flag assingments

  SYNOPSIS
    find_set_from_flags()
      lib               Flag names
      default_name      Number of "default" in the typelib
      cur_set           Current set of flags (start from this state)
      default_set       Default set of flags (use this for assign-default
                        keyword and flag=default assignments)
      str               String to be parsed
      length            Length of the string
      cs                String charset
      err_pos      OUT  If error, set to point to start of wrong set string
                        NULL on success
      err_len      OUT  If error, set to the length of wrong set string
      set_warning  OUT  TRUE <=> Some string in set couldn't be used

  DESCRIPTION
    Parse a set of flag assignments, that is, parse a string in form:

      param_name1=value1,param_name2=value2,... 
    
    where the names are specified in the TYPELIB, and each value can be
    either 'on','off', or 'default'. Setting the same name twice is not 
    allowed.
    
    Besides param=val assignments, we support the "default" keyword (keyword 
    #default_name in the typelib). It can be used one time, if specified it 
    causes us to build the new set over the default_set rather than cur_set
    value.
    
  RETURN
    Parsed set value if (*errpos == NULL)
    Otherwise undefined
*/

ulonglong find_set_from_flags(TYPELIB *lib, uint default_name,
                              ulonglong cur_set, ulonglong default_set,
                              const char *str, uint length, CHARSET_INFO *cs,
                              char **err_pos, uint *err_len, bool *set_warning)
{
  CHARSET_INFO *strip= cs ? cs : &my_charset_latin1;
  const char *end= str + strip->cset->lengthsp(strip, str, length);
  ulonglong flags_to_set= 0, flags_to_clear= 0;
  bool set_defaults= 0;
  *err_pos= 0;                  // No error yet
  if (str != end)
  {
    const char *start= str;    
    for (;;)
    {
      const char *pos= start;
      uint flag_no, value;

      if (!(flag_no= parse_name(lib, &pos, end, cs)))
        goto err;

      if (flag_no == default_name)
      {
        /* Using 'default' twice isn't allowed. */
        if (set_defaults)
          goto err;
        set_defaults= TRUE;
      }
      else
      {
        ulonglong bit=  ((longlong) 1 << (flag_no - 1));
        /* parse the '=on|off|default' */
        if ((flags_to_clear | flags_to_set) & bit ||
            get_next_char(&pos, end, cs) != '=' ||
            !(value= parse_name(&on_off_default_typelib, &pos, end, cs)))
        {
          goto err;
        }
        
        if (value == 1) // this is '=off'
          flags_to_clear|= bit;
        else if (value == 2) // this is '=on'
          flags_to_set|= bit;
        else // this is '=default' 
        {
          if (default_set & bit)
            flags_to_set|= bit;
          else
            flags_to_clear|= bit;
        }
      }
      if (pos >= end)
        break;

      if (get_next_char(&pos, end, cs) != ',')
        goto err;

      start=pos;
      continue;
   err:
      *err_pos= (char*)start;
      *err_len= end - start;
      *set_warning= TRUE;
      break;
    }
  }
  ulonglong res= set_defaults? default_set : cur_set;
  res|= flags_to_set;
  res&= ~flags_to_clear;
  return res;
}


/*
  Function to find a string in a TYPELIB
  (Same format as mysys/typelib.c)

  SYNOPSIS
   find_type()
   lib			TYPELIB (struct of pointer to values + count)
   find			String to find
   length		Length of string to find
   part_match		Allow part matching of value

 RETURN
  0 error
  > 0 position in TYPELIB->type_names +1
*/

uint find_type(const TYPELIB *lib, const char *find, uint length,
               bool part_match)
{
  uint found_count=0, found_pos=0;
  const char *end= find+length;
  const char *i;
  const char *j;
  for (uint pos=0 ; (j=lib->type_names[pos++]) ; )
  {
    for (i=find ; i != end && 
	   my_toupper(system_charset_info,*i) == 
	   my_toupper(system_charset_info,*j) ; i++, j++) ;
    if (i == end)
    {
      if (! *j)
	return(pos);
      found_count++;
      found_pos= pos;
    }
  }
  return(found_count == 1 && part_match ? found_pos : 0);
}


/*
  Find a string in a list of strings according to collation

  SYNOPSIS
   find_type2()
   lib			TYPELIB (struct of pointer to values + count)
   x			String to find
   length               String length
   cs			Character set + collation to use for comparison

  NOTES

  RETURN
    0	No matching value
    >0  Offset+1 in typelib for matched string
*/

uint find_type2(const TYPELIB *typelib, const char *x, uint length,
                CHARSET_INFO *cs)
{
  int pos;
  const char *j;
  DBUG_ENTER("find_type2");
  DBUG_PRINT("enter",("x: '%.*s'  lib: 0x%lx", length, x, (long) typelib));

  if (!typelib->count)
  {
    DBUG_PRINT("exit",("no count"));
    DBUG_RETURN(0);
  }

  for (pos=0 ; (j=typelib->type_names[pos]) ; pos++)
  {
    if (!my_strnncoll(cs, (const uchar*) x, length,
                          (const uchar*) j, typelib->type_lengths[pos]))
      DBUG_RETURN(pos+1);
  }
  DBUG_PRINT("exit",("Couldn't find type"));
  DBUG_RETURN(0);
} /* find_type */


/*
  Un-hex all elements in a typelib

  SYNOPSIS
   unhex_type2()
   interval       TYPELIB (struct of pointer to values + lengths + count)

  NOTES

  RETURN
    N/A
*/

void unhex_type2(TYPELIB *interval)
{
  for (uint pos= 0; pos < interval->count; pos++)
  {
    char *from, *to;
    for (from= to= (char*) interval->type_names[pos]; *from; )
    {
      /*
        Note, hexchar_to_int(*from++) doesn't work
        one some compilers, e.g. IRIX. Looks like a compiler
        bug in inline functions in combination with arguments
        that have a side effect. So, let's use from[0] and from[1]
        and increment 'from' by two later.
      */

      *to++= (char) (hexchar_to_int(from[0]) << 4) +
                     hexchar_to_int(from[1]);
      from+= 2;
    }
    interval->type_lengths[pos] /= 2;
  }
}


/*
  Check if the first word in a string is one of the ones in TYPELIB

  SYNOPSIS
    check_word()
    lib		TYPELIB
    val		String to check
    end		End of input
    end_of_word	Store value of last used byte here if we found word

  RETURN
    0	 No matching value
    > 1  lib->type_names[#-1] matched
	 end_of_word will point to separator character/end in 'val'
*/

uint check_word(TYPELIB *lib, const char *val, const char *end,
		const char **end_of_word)
{
  int res;
  const char *ptr;

  /* Fiend end of word */
  for (ptr= val ; ptr < end && my_isalpha(&my_charset_latin1, *ptr) ; ptr++)
    ;
  if ((res=find_type(lib, val, (uint) (ptr - val), 1)) > 0)
    *end_of_word= ptr;
  return res;
}


/*
  Converts a string between character sets

  SYNOPSIS
    strconvert()
    from_cs       source character set
    from          source, a null terminated string
    to            destination buffer
    to_length     destination buffer length

  NOTES
    'to' is always terminated with a '\0' character.
    If there is no enough space to convert whole string,
    only prefix is converted, and terminated with '\0'.

  RETURN VALUES
    result string length
*/


uint strconvert(CHARSET_INFO *from_cs, const char *from,
                CHARSET_INFO *to_cs, char *to, uint to_length, uint *errors)
{
  int cnvres;
  my_wc_t wc;
  char *to_start= to;
  uchar *to_end= (uchar*) to + to_length - 1;
  my_charset_conv_mb_wc mb_wc= from_cs->cset->mb_wc;
  my_charset_conv_wc_mb wc_mb= to_cs->cset->wc_mb;
  uint error_count= 0;

  while (1)
  {
    /*
      Using 'from + 10' is safe:
      - it is enough to scan a single character in any character set.
      - if remaining string is shorter than 10, then mb_wc will return
        with error because of unexpected '\0' character.
    */
    if ((cnvres= (*mb_wc)(from_cs, &wc,
                          (uchar*) from, (uchar*) from + 10)) > 0)
    {
      if (!wc)
        break;
      from+= cnvres;
    }
    else if (cnvres == MY_CS_ILSEQ)
    {
      error_count++;
      from++;
      wc= '?';
    }
    else
      break; // Impossible char.

outp:

    if ((cnvres= (*wc_mb)(to_cs, wc, (uchar*) to, to_end)) > 0)
      to+= cnvres;
    else if (cnvres == MY_CS_ILUNI && wc != '?')
    {
      error_count++;
      wc= '?';
      goto outp;
    }
    else
      break;
  }
  *to= '\0';
  *errors= error_count;
  return (uint32) (to - to_start);

}


/*
  Searches for a LEX_STRING in an LEX_STRING array.

  SYNOPSIS
    find_string_in_array()
      heap    The array
      needle  The string to search for

  NOTE
    The last LEX_STRING in the array should have str member set to NULL

  RETURN VALUES
    -1   Not found
    >=0  Ordinal position
*/

int find_string_in_array(LEX_STRING * const haystack, LEX_STRING * const needle,
                         CHARSET_INFO * const cs)
{
  const LEX_STRING *pos;
  for (pos= haystack; pos->str; pos++)
    if (!cs->coll->strnncollsp(cs, (uchar *) pos->str, pos->length,
                               (uchar *) needle->str, needle->length, 0))
    {
      return (pos - haystack);
    }
  return -1;
}
