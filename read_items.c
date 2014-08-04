/*
 * Copyright 2013 Byrial Jensen alias User:Byrial at Wikidata
 * and other Widiamedia projects
 *
 * This file is part of Byrial's Wikidata analysis tools.
 *
 * Byrial's Wikidata analysis tools is free software: you can
 * redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Byrial's Wikidata analysis tools is distributed in the hope
 * that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wikidatalib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <inttypes.h>
#include <unistd.h> // for sleep()/debug

const int lang_length = 12; // longest language code is zh-classical (12 bytes)
const int text_length = 1024;
const int verbose=0;

void fill_i_use (MYSQL *mysql)
{
   printf ("Fill in item.i_use\n");
   do_query (mysql,
	     "UPDATE item "
	     "SET i_use = "
	     " (SELECT COUNT(*) FROM statement WHERE s_item_value = i_id) + "
	     " (SELECT COUNT(*) FROM qualifier WHERE q_item_value = i_id) + "
	     " (SELECT COUNT(*) FROM refclaim WHERE rc_item_value = i_id)");
}

void print_next_text_and_exit (FILE *fp)
{
   for (int i = 0 ; i < 400 ; ++i)
     putchar (getc (fp));
   exit (1);
}

void goto_next_tag (FILE *fp, const char *tag)
{
   for (;;)
     {
	int ch;
	do {
	   ch = getc (fp);
	   if (ch == EOF)
	     return;
	} while (ch != '<');
	const char *t = tag;
	while ((ch = getc (fp)) == *(t ++));
	if (ch == '>')
	  return;
     }
}

void pass_text (FILE *fp, const char *text)
{
   const char *t = text;
   while (*t)
     {
	int ch = getc (fp);
	if (ch != *t)
	  {
	     printf ("Expected to find '%s' but got '%c' as the %td character.\n",
		     text, ch, t - text + 1);
	     printf ("Going to sleep to wait for debugging...");
	     volatile int s = 1;
	     while (s)
	       {
		  sleep (1);
	       }
	     exit (1);
	  }
	++ t;
     }
   return; // text is as expected
}

void get_quot_text (FILE *fp, char *t, size_t maxlen)
{
   pass_text (fp, "&quot;");
   for ( ; maxlen > 0 ; --maxlen)
     {
	*t = getc (fp);
	if (*t == EOF)
	  {
	     printf ("Unexpected EOF in get_quot_text.\n");
	     exit (1);
	  }
	if (*t == '&')
	  {
	     int next_char = getc (fp);
	     if (next_char == 'q')
	       {
		  *t = '\0';
		  pass_text (fp, "uot;");
		  return;
	       }
	     else if (next_char == 'a')
	       {
		  pass_text (fp, "mp;");
	       }
	     else if (next_char == 'l')
	       {
		  pass_text (fp, "t;");
		  *t = '<';
	       }
	     else if (next_char == 'g')
	       {
		  pass_text (fp, "t;");
		  *t = '>';
	       }
	     else
	       {
		  printf ("Unknown xml entity starting with &%c.\n", next_char);
		  exit (1);
	       }
	  }
	else if (*t == '\\')
	  {
	     int next_char = getc (fp);
	     if (next_char == 'u')
	       {
		  unsigned int code;
		  int ret = fscanf (fp, "%4x", &code);
		  if (ret != 1)
		    {
		       printf ("Could not read hex character value.\n");
		       exit (1);
		    }
		  // * "code" holds the code point if in range 0x0000 - 0xD7FF
		  //    or 0xE000 - 0xFFFF
		  // * It is lead UTF-16 surrogate if in range 0xD800 - 0xDBFF
		  // * It is trail UTF-16 surrogate if in range 0xDC00 - 0xDFFF 
		  if ((code & 0xF800) == 0xD800)
		    {
		       if ((code & 0x0400) == 0x0400)
			 {
			    printf ("Found trail UTF-16 surrogate without lead, "
				    "code=0x%04x.\n", code);
			    exit (1);
			 }
		       unsigned int trail;
		       int ret = fscanf (fp, "\\u%4x", &trail);
		       if (ret != 1)
			 {
			    printf ("Could not read trail UTF-16 surrogate.\n");
			    exit (1);
			 }
		       if ((trail & 0xFC00) != 0xDC00)       
			 {
			    printf ("Did not find trail UTF-16 surrogate.\n");
			    exit (1);
			 }
		       code = ((code & 0x03FF) << 10) | (trail & 0x03FF);
		    }
		  wchar_t wch = code;
		  if (maxlen < MB_CUR_MAX)
		    {
		       printf ("Not enough space for wctomb.\n");
		       exit (1);
		    }
		  int ret_wctomb = wctomb (t, wch);
		  if (ret_wctomb <= 0)
		    {
		       printf ("wctomb failed in get_quot_text, wch='%i'.\n",
			       (int) wch);
		       exit (1);
		    }
		  t += ret_wctomb - 1;
		  maxlen -= (ret_wctomb - 1);
	       }
	     else if (next_char == '/')
	       {
		  *t = '/';
	       }
	     else if (next_char == '\\')
	       {
		  *(t ++) = '\\'; // MySQL escape
		  -- maxlen;
		  *t = '\\';
	       }
	     else if (next_char == 't')
	       {
		  *(t ++) = '\\'; // MySQL escape
		  -- maxlen;
		  *t = '\t';
	       }
	     else if (next_char == 'r')
	       {
		  *(t ++) = '\\'; // MySQL escape
		  -- maxlen;
		  *t = '\r';
	       }
	     else if (next_char == 'n')
	       {
		  *(t ++) = '\\'; // MySQL escape
		  -- maxlen;
		  *t = '\n';
	       }
	     else if (next_char == '&') // &quot;
	       {
		  pass_text (fp, "quot;");
		  -- maxlen;
		  *(t ++) = '\\'; // MySQL escape
		  *t = '"';
	       }
	     else
	       {
		  printf ("Unexpected character %c after \\.\n", next_char);
		  exit (1);
	       }
	  }
	++ t;
     }
   printf ("maxlen exceeded in get_quot_text.\n");
   exit (1);
}

void pass_quot_text (FILE *fp, const char *text)
{
   pass_text (fp, "&quot;");
   pass_text (fp, text);
   pass_text (fp, "&quot;");
}

int read_section (FILE *fp, MYSQL *mysql, int q, const char *table);
int read_links (FILE *fp, MYSQL *mysql, int q, int *ns);
int read_aliases (FILE *fp, MYSQL *mysql, int q, const char *table, int *languages);
void skip_section (FILE *fp, int q, const char *name);
int read_statements (FILE *fp, MYSQL *mysql, int q);

void read_property (FILE *fp, MYSQL *mysql, int p)
{
   static unsigned int properties = 0;
   static unsigned int properties_with_labels = 0;
   static unsigned int labels = 0;
   static unsigned int properties_with_descs = 0;
   static unsigned int descs = 0;
   static unsigned int properties_with_aliases = 0;
   static unsigned int aliases = 0;

   ++ properties;

   goto_next_tag (fp, "text xml:space=\"preserve\"");
   pass_text (fp, "{");

   int found_labels = 0;
   int found_descriptions = 0;
   int found_alias_languages = 0;
   int found_aliases = 0;
   const char *datatype = 0;
   int delim;
   do
     {
	char section[64];
	get_quot_text (fp, section, sizeof section);
	pass_text (fp, ":");

	if(verbose)
		printf("property section %s\n", section);

	if (strcmp (section, "label") == 0)
	  {
	     found_labels = read_section (fp, mysql, p, "plabel");
	     if (found_labels)
	       ++ properties_with_labels;
	     labels += found_labels;
	  }
	else if (strcmp (section, "description") == 0)
	  {
	     found_descriptions = read_section (fp, mysql, p, "pdescr");
	     if (found_descriptions)
	       ++ properties_with_descs;
	     descs += found_descriptions;
	  }
	else if (strcmp (section, "aliases") == 0)
	  {
	     found_aliases = read_aliases (fp, mysql, p, "palias", &found_alias_languages);
	     if (found_aliases)
	       ++ properties_with_aliases;
	     aliases += found_aliases;
	  }
	else if (strcmp (section, "entity") == 0)
	  {
	     // Format changed for some(?) properties in the 2013-07-10 dump:
	     // Before: "p7"
	     // After: ["property",7]
	     int ch = getc (fp);
	     if (ch == '[')
	       {
		  int property = 0;
		  int ret = fscanf (fp, "&quot;property&quot;,%d]", &property);
		  if (ret != 1 || property != p)
		    {
		       printf ("Found unexpected entity section for p%d "
			       "(ret=%d, property=%d).\n", p, ret, property);
		       exit (1);
		    }
	       }
	     else
	       {
		  ungetc (ch, fp);
		  char entity[32];
		  get_quot_text (fp, entity, sizeof entity);
		  if (*entity != 'p' || atoi (entity + 1) != p)
		    {
		       printf ("Found unexpected entity section %s for p%d.\n", entity, p);
		       exit (1);
		    }
	       }
	  }
	else if (strcmp (section, "claims") == 0)
	  {
	     int c1 = getc (fp);
	     int c2 = getc (fp);
	     if (c1 != '[' || c2 != ']')
	       {
		  printf ("Found not empty claim section p%d.\n", p);
		  exit (1);
	       }
	  }
	else if (strcmp (section, "datatype") == 0)
	  {
	     char dt[32];
	     get_quot_text (fp, dt, sizeof dt);
	     if (strcmp (dt, "wikibase-item") == 0)
	       datatype = "item";
	     else if (strcmp (dt, "string") == 0)
	       datatype = "string";
	     else if (strcmp (dt, "commonsMedia") == 0)
	       datatype = "commonsMedia";
	     else if (strcmp (dt, "time") == 0)
	       datatype = "time";
	     else if (strcmp (dt, "globe-coordinate") == 0)
	       datatype = "coordinate";
	     else if (strcmp (dt, "quantity") == 0)
	       datatype = "quantity";
	     else
	       {
		  printf ("Found unknown datatype %s for p%d.\n", dt, p);
		  exit (1);
	       }
	  }
	else
	  {
	     printf ("Skipping unknown section %s for p%d.\n", section, p);
	     skip_section (fp, p, section);
	  }
	delim = getc (fp);
     } while (delim == ',');
   if (delim != '}')
     {
	printf ("Text for p%d ending in expected way with '%c'\n", p, delim);
	print_next_text_and_exit (fp);
	exit (1);
     }

   if (! datatype)
     {
	printf ("Didn't find datatype for p%d.\n", p);
	exit (1);
     }
   do_query (mysql,
	     "INSERT INTO property "
	     "VALUES (%d, %d, %d, %d, %d, '%s')",
	     p, found_labels, found_descriptions,
	     found_aliases, found_alias_languages, datatype);
}

void update_links (FILE *fp, MYSQL *mysql, int q)
{
   do_query (mysql, "DELETE FROM link WHERE k_id = %d", q);
   goto_next_tag (fp, "text xml:space=\"preserve\"");
   pass_text (fp, "{");
   int delim;
   int ns = -100; // -100 means no links, -10 means different namespaces found
   int found_links = 0;
   do
     {
	char section[64];
	get_quot_text (fp, section, sizeof section);
	pass_text (fp, ":");

	if (strcmp (section, "links") == 0)
	  {
	     found_links = read_links (fp, mysql, q, &ns);
	  }
	else
	  {
	     skip_section (fp, q, section);
	  }
	delim = getc (fp);
     } while (delim == ',');
   if (delim != '}')
     {
	printf ("Text for q%d ending in expected way with '%c'\n", q, delim);
	print_next_text_and_exit (fp);
	exit (1);
     }

   do_query (mysql,	     
	     "INSERT INTO item "
	     "VALUES (%d, 0, 0, 0, 0, %d, 0, 0, %d) "
	     "ON DUPLICATE KEY UPDATE i_links = %d, i_ns = %d",
	     q, 
	     found_links, ns,
	     found_links, ns);
}

void read_item (FILE *fp, MYSQL *mysql, int q)
{
   static unsigned int items = 0;
   static unsigned int items_with_labels = 0;
   static unsigned int labels = 0;
   static unsigned int items_with_descs = 0;
   static unsigned int descs = 0;
   static unsigned int items_with_aliases = 0;
   static unsigned int aliases = 0;
   static unsigned int items_with_links = 0;
   static unsigned int links = 0;
   static unsigned int items_with_statements = 0;
   static unsigned int statements = 0;

   items ++;

   goto_next_tag (fp, "text xml:space=\"preserve\"");
   pass_text (fp, "{");

   int found_labels = 0;
   int found_descriptions = 0;
   int found_alias_languages = 0;
   int found_aliases = 0;
   int found_links = 0;
   int found_statements = 0;
   int delim;
   int ns = -100; // -100 means no links, -10 means different namespaces found
   do
     {
	char section[64];
	get_quot_text (fp, section, sizeof section);
	pass_text (fp, ":");

	if(verbose)
		printf("item section %s\n", section);

	if (strcmp (section, "links") == 0)
	  {
	     found_links = read_links (fp, mysql, q, & ns);
	     if (found_links)
	       ++ items_with_links;
	     links += found_links;
	  }
	else if (strcmp (section, "label") == 0)
	  {
	     found_labels = read_section (fp, mysql, q, "label");
	     if (found_labels)
	       ++ items_with_labels;
	     labels += found_labels;
	  }
	else if (strcmp (section, "description") == 0)
	  {
	     found_descriptions = read_section (fp, mysql, q, "descr");
	     if (found_descriptions)
	       ++ items_with_descs;
	     descs += found_descriptions;
	  }
	else if (strcmp (section, "aliases") == 0)
	  {
	     found_aliases = read_aliases (fp, mysql, q, "alias",
					   &found_alias_languages);
	     if (found_aliases)
	       ++ items_with_aliases;
	     aliases += found_aliases;
	  }
	else if (strcmp (section, "claims") == 0)
	  {
	     found_statements = read_statements (fp, mysql, q);
	     if (found_statements)
	       ++ items_with_statements;
	     statements += found_statements;
	  }
	else if (strcmp (section, "entity") == 0)
	  {
	     // Format changed for some(?) items in the 2013-07-10 dump:
	     // Before: "q17"
	     // After: ["item",17]
	     int ch = getc (fp);
	     if (ch == '[')
	       {
		  int item = 0;
		  int ret = fscanf (fp, "&quot;item&quot;,%d]", &item);
		  if (ret != 1 || item != q)
		    {
		       printf ("Found unexpected entity section for q%d "
			       "(ret=%d, item=%d).\n", q, ret, item);
		       exit (1);
		    }
	       }
	     else
	       {
		  ungetc (ch, fp);
		  char entity[32];
		  get_quot_text (fp, entity, sizeof entity);
		  if (*entity != 'q' || atoi (entity + 1) != q)
		    {
		       printf ("Found unexpected entity section %s for q%d.\n", entity, q);
		       exit (1);
		    }
	       }	     
	  }
	else
	  {
	     printf ("Skipping unknown section for q%d.\n", q);
	     skip_section (fp, q, section);
	  }
	delim = getc (fp);
     } while (delim == ',');
   if (delim != '}')
     {
	printf ("Text for q%d ending in expected way with '%c'\n", q, delim);
	print_next_text_and_exit (fp);
	exit (1);
     }

   do_query (mysql,
	     "INSERT INTO item "
	     "VALUES (%d, %d, %d, %d, %d, %d, %d, 0, %d)",
	     q, found_labels, found_descriptions,
	     found_aliases, found_alias_languages, found_links,
	     found_statements, ns);

   if (false)
     {
	printf ("Found %u item, %u label, %u desc, %u alias, %u links, %u state\r",
		items, labels, descs, aliases, links, statements);
     }
}

void create_tables (MYSQL *mysql, bool drop_tables)
{
   if (table_exists (mysql, "item") && drop_tables)
     do_query (mysql, "DROP TABLE item");
   do_query (mysql,
	     "CREATE TABLE item ("
	     "i_id INT UNSIGNED NOT NULL,"
	     "i_labels SMALLINT UNSIGNED NOT NULL,"
	     "i_descrs SMALLINT UNSIGNED NOT NULL,"
	     "i_aliases SMALLINT UNSIGNED NOT NULL,"
	     "i_alias_langs SMALLINT UNSIGNED NOT NULL,"
	     "i_links SMALLINT UNSIGNED NOT NULL,"
	     "i_statements SMALLINT UNSIGNED NOT NULL,"
	     "i_use INT UNSIGNED NOT NULL,"
	     "i_ns SMALLINT NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   if (table_exists (mysql, "property") && drop_tables)
     do_query (mysql, "DROP TABLE property");
   do_query (mysql,
	     "CREATE TABLE property ("
	     "p_id INT UNSIGNED NOT NULL,"
	     "p_labels SMALLINT UNSIGNED NOT NULL,"
	     "p_descrs SMALLINT UNSIGNED NOT NULL,"
	     "p_aliases SMALLINT UNSIGNED NOT NULL,"
	     "p_alias_langs SMALLINT UNSIGNED NOT NULL,"
	     "p_datatype ENUM('item', 'string', 'commonsMedia', 'time', 'coordinate', 'quantity') NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   // This table is not used
   if (table_exists (mysql, "longtexts") && drop_tables)
     do_query (mysql, "DROP TABLE longtexts");
   do_query (mysql,
	     "CREATE TABLE longtexts ("
	     "lt_id INT UNSIGNED NOT NULL,"
	     "lt_lang VARCHAR(%d) NOT NULL,"
	     "lt_table VARCHAR(6) NOT NULL,"
	     "lt_text TEXT NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang_length);

   if (table_exists (mysql, "label") && drop_tables)
     do_query (mysql, "DROP TABLE label");
   do_query (mysql,
	     "CREATE TABLE label ("
	     "l_id INT UNSIGNED NOT NULL,"
	     "l_lang VARCHAR(%d) NOT NULL,"
	     "l_text VARCHAR(%d) NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang_length, text_length);

   if (table_exists (mysql, "plabel") && drop_tables)
     do_query (mysql, "DROP TABLE plabel");
   do_query (mysql,
	     "CREATE TABLE plabel ("
	     "pl_id INT UNSIGNED NOT NULL,"
	     "pl_lang VARCHAR(%d) NOT NULL,"
	     "pl_text VARCHAR(%d) NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang_length, text_length);

   if (table_exists (mysql, "descr") && drop_tables)
     do_query (mysql, "DROP TABLE descr");
   do_query (mysql,
	     "CREATE TABLE descr ("
	     "d_id INT UNSIGNED NOT NULL,"
	     "d_lang VARCHAR(%d) NOT NULL,"
	     "d_text VARCHAR(%d) NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang_length, text_length);

   if (table_exists (mysql, "pdescr") && drop_tables)
     do_query (mysql, "DROP TABLE pdescr");
   do_query (mysql,
	     "CREATE TABLE pdescr ("
	     "pd_id INT UNSIGNED NOT NULL,"
	     "pd_lang VARCHAR(%d) NOT NULL,"
	     "pd_text VARCHAR(%d) NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang_length, text_length);

   if (table_exists (mysql, "alias") && drop_tables)
     do_query (mysql, "DROP TABLE alias");
   do_query (mysql,
	     "CREATE TABLE alias ("
	     "a_id INT UNSIGNED NOT NULL,"
	     "a_lang VARCHAR(%d) NOT NULL,"
	     "a_text VARCHAR(%d) NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang_length, text_length);

   if (table_exists (mysql, "palias") && drop_tables)
     do_query (mysql, "DROP TABLE palias");
   do_query (mysql,
	     "CREATE TABLE palias ("
	     "pa_id INT UNSIGNED NOT NULL,"
	     "pa_lang VARCHAR(%d) NOT NULL,"
	     "pa_text VARCHAR(%d) NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang_length, text_length);

   if (table_exists (mysql, "link") && drop_tables)
     do_query (mysql, "DROP TABLE link");
   do_query (mysql,
	     "CREATE TABLE link ("
	     "k_id INT UNSIGNED NOT NULL,"
	     "k_project CHAR(1) NOT NULL,"
	     "k_lang VARCHAR(%d) NOT NULL,"
	     "k_ns SMALLINT NOT NULL,"
	     "k_text VARCHAR(%d) NOT NULL,"
	     "k_ns_alias VARCHAR(96) NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang_length, text_length);

   if (table_exists (mysql, "statement") && drop_tables)
     do_query (mysql, "DROP TABLE statement");
   do_query (mysql,
	     "CREATE TABLE statement ("
	     "s_statement INT UNSIGNED NOT NULL,"
	     "s_item INT NOT NULL,"
	     "s_qualifiers SMALLINT NOT NULL,"
	     "s_refs SMALLINT NOT NULL,"
	     "s_property MEDIUMINT NOT NULL,"
	     "s_datatype ENUM('novalue', 'somevalue', 'item',"
	     "    'string', 'time', 'coordinate', 'quantity', 'bad-coordinate', 'bad-time') NOT NULL,"
	     "s_item_value INT NOT NULL," // 0 serve as NULL VALUE
	     "s_string_value VARCHAR(1024),"
	     "s_item_property_count MEDIUMINT UNSIGNED NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   if (table_exists (mysql, "qualifier") && drop_tables)
     do_query (mysql, "DROP TABLE qualifier");
   do_query (mysql,
	     "CREATE TABLE qualifier ("
	     "q_statement INT UNSIGNED NOT NULL,"
	     "q_item INT NOT NULL,"
	     "q_qno SMALLINT NOT NULL,"
	     "q_property MEDIUMINT NOT NULL,"
	     "q_datatype ENUM('novalue', 'somevalue', 'item', 'string',"
	     "    'time', 'coordinate', 'quantity', 'bad-coordinate', 'bad-time') NOT NULL,"
	     "q_item_value INT NOT NULL," // 0 serve as NULL VALUE
	     "q_string_value VARCHAR(1024))"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   if (table_exists (mysql, "ref") && drop_tables)
     do_query (mysql, "DROP TABLE ref");
   do_query (mysql,
	     "CREATE TABLE ref ("
	     "r_statement INT UNSIGNED NOT NULL,"
	     "r_refno SMALLINT NOT NULL,"
	     "r_item INT NOT NULL,"
	     "r_claims SMALLINT NOT NULL)"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   if (table_exists (mysql, "refclaim") && drop_tables)
     do_query (mysql, "DROP TABLE refclaim");
   do_query (mysql,
	     "CREATE TABLE refclaim ("
	     "rc_statement INT UNSIGNED NOT NULL,"
	     "rc_refno SMALLINT NOT NULL,"
	     "rc_refclaimno SMALLINT NOT NULL,"
	     "rc_item INT NOT NULL,"
	     "rc_property MEDIUMINT NOT NULL,"
	     "rc_datatype ENUM('novalue', 'somevalue', 'item', 'string',"
	     "   'time', 'coordinate', 'quantity', 'bad-coordinate', 'bad-time') NOT NULL,"
	     "rc_item_value INT NOT NULL," // 0 serve as NULL VALUE
	     "rc_string_value VARCHAR(1024))"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   if (table_exists (mysql, "timevalue") && drop_tables)
     do_query (mysql, "DROP TABLE timevalue");
   do_query (mysql,
	     "CREATE TABLE timevalue ("
	     "tv_item INT NOT NULL,"
	     "tv_statement INT UNSIGNED NOT NULL,"
	     "tv_refno SMALLINT NOT NULL,"
	     "tv_refclaimno SMALLINT NOT NULL,"
	     "tv_qno INT NOT NULL,"
	     "tv_year BIGINT NOT NULL," // the lowest value of INT is -2,147,483,648
	     "tv_month INT NOT NULL,"
	     "tv_day INT NOT NULL,"
	     "tv_time INT NOT NULL,"
	     "tv_tz SMALLINT NOT NULL,"
	     "tv_before TINYINT NOT NULL,"
	     "tv_after TINYINT NOT NULL,"
	     "tv_precision TINYINT NOT NULL,"
	     "rc_model ENUM('1985727', " // Q1985727 = Proleptic Gregorian calendar 
	     "              '1985786'))" // Q1985786 = Proleptic Julian calendar 
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   if (table_exists (mysql, "coordinate") && drop_tables)
     do_query (mysql, "DROP TABLE coordinate");
   do_query (mysql,
	     "CREATE TABLE coordinate ("
	     "co_item INT NOT NULL,"
	     "co_statement INT UNSIGNED NOT NULL,"
	     "co_refno SMALLINT NOT NULL,"
	     "co_refclaimno SMALLINT NOT NULL,"
	     "co_qno INT NOT NULL,"
	     "co_latitude DOUBLE NOT NULL,"
	     "co_longitude DOUBLE NOT NULL,"
	     "co_altitude DOUBLE NULL,"
	     "co_globe INT NOT NULL,"
	     "co_precision DOUBLE NULL)" // "null" is in the dumps. What is it?
	     "ENGINE=MyISAM "
	     "CHARSET binary");
}

void add_indexes (MYSQL *mysql, int last_item)
{
   add_index ("item", mysql,
	      "ADD UNIQUE INDEX(i_id)");
   add_index ("property", mysql,
	      "ADD UNIQUE INDEX(p_id)");
   add_index ("label", mysql,
	      "ADD INDEX(l_id),"
	      "ADD INDEX(l_lang)");
   add_index ("descr", mysql,
	      "ADD INDEX(d_id),"
	      "ADD INDEX(d_lang)");
   add_index ("alias", mysql,
	      "ADD INDEX(a_id),"
	      "ADD INDEX(a_lang)");
   add_index ("plabel", mysql,
	      "ADD INDEX(pl_id),"
	      "ADD INDEX(pl_lang)");
   add_index ("pdescr", mysql,
	      "ADD INDEX(pd_id),"
	      "ADD INDEX(pd_lang)");
   add_index ("palias", mysql,
	      "ADD INDEX(pa_id),"
	      "ADD INDEX(pa_lang)");
   add_index ("link", mysql,
	      "ADD UNIQUE INDEX(k_id, k_project, k_lang),"
	      "ADD INDEX(k_project, k_lang, k_ns, k_text(8))");
   add_index ("statement", mysql,
	      "ADD UNIQUE INDEX(s_statement),"
	      "ADD INDEX(s_item),"
	      "ADD INDEX(s_property),"
	      "ADD INDEX(s_item_value),"
	      "ADD INDEX(s_string_value(15))");

   printf ("Update statement.s_item_property_count\n");
   do_query (mysql,
	     "CREATE TEMPORARY TABLE s("
	     "s_item INT NOT NULL,"
	     "s_property MEDIUMINT NOT NULL,"
	     "s_count MEDIUMINT UNSIGNED NOT NULL)");
   int step = 5000;
   for (int i = 1 ; i <= last_item ; i += step)
     {     
	printf ("Items %d-%d\r", i, i + step - 1);
	fflush (stdout);
	do_query (mysql,
		  "INSERT INTO s "
		  "SELECT s_item, s_property, COUNT(*) "
		  "FROM statement "
		  "WHERE s_item >= %d AND s_item < %d "
		  "GROUP BY s_item, s_property",
		  i, i + step);
	do_query (mysql,
		  "UPDATE statement "
		  "JOIN s USING (s_item, s_property) "
		  "SET s_item_property_count = s_count ");
	do_query (mysql,
		  "DELETE FROM s");
     }
   add_index ("statement", mysql,
	      "ADD INDEX(s_item_property_count)");

   add_index ("qualifier", mysql,
	      "ADD INDEX(q_statement),"
	      "ADD INDEX(q_item),"
	      "ADD INDEX(q_property),"
	      "ADD INDEX(q_item_value),"
	      "ADD INDEX(q_string_value(15))");

   add_index ("ref", mysql,
	      "ADD UNIQUE INDEX(r_statement, r_refno),"
	      "ADD INDEX(r_item)");

   add_index ("refclaim", mysql,
	      "ADD INDEX(rc_statement, rc_refno),"
	      "ADD INDEX(rc_item),"
	      "ADD INDEX(rc_property),"
	      "ADD INDEX(rc_item_value),"
	      "ADD INDEX(rc_string_value(15))");

   fill_i_use (mysql);

   add_index ("coordinate", mysql,
	      "ADD INDEX(co_item)");
}

void remove_item (MYSQL *mysql, int entity)
{
   do_query (mysql, "DELETE FROM item WHERE i_id = %d", entity);
   do_query (mysql, "DELETE FROM label WHERE l_id = %d", entity);
   do_query (mysql, "DELETE FROM descr WHERE d_id = %d", entity);
   do_query (mysql, "DELETE FROM alias WHERE a_id = %d", entity);
   do_query (mysql, "DELETE FROM link WHERE k_id = %d", entity);
   do_query (mysql, "DELETE FROM statement WHERE s_item = %d", entity);
   do_query (mysql, "DELETE FROM statement WHERE s_item = %d", entity);
   do_query (mysql, "DELETE FROM ref WHERE r_item = %d", entity);
   do_query (mysql, "DELETE FROM refclaim WHERE rc_item = %d", entity);
   do_query (mysql, "DELETE FROM timevalue WHERE tv_item = %d", entity);
   do_query (mysql, "DELETE FROM coordinate WHERE co_item = %d", entity);
}

void remove_property (MYSQL *mysql, int entity)
{
   do_query (mysql, "DELETE FROM property WHERE p_id = %d", entity);
   do_query (mysql, "DELETE FROM plabel WHERE pl_id = %d", entity);
   do_query (mysql, "DELETE FROM pdescr WHERE pd_id = %d", entity);
   do_query (mysql, "DELETE FROM palias WHERE pa_id = %d", entity);
}

int main (int argc, const char *argv[])
{
   const char *database;
   bool drop_tables = false;
   bool incremental_dump = false; 
   if (argc >= 2)
     {
	if (strcmp (argv[1], "drop") == 0)
	  {
	     drop_tables = true;
	     ++ argv;
	     -- argc;	       
	  }
	else if (strcmp (argv[1], "incr") == 0)
	  {
	     incremental_dump = true;
	     ++ argv;
	     -- argc;	       
	  }
     }
   if (argc < 2)
     database = "wikidatawiki";
   else
     database = argv[1];

   setlocale (LC_CTYPE, "");

   MYSQL *mysql = open_named_database (database);

   if (incremental_dump)
     do_query (mysql, "ALTER TABLE link DISABLE KEYS");
   else
     create_tables (mysql, drop_tables);

   do_query (mysql,
	     "LOCK TABLES "
	     "item WRITE, property WRITE, "
	     "label WRITE, descr WRITE, alias WRITE, "
	     "link WRITE, statement WRITE, qualifier WRITE, "
	     "ref WRITE, refclaim WRITE, "
	     "plabel WRITE, pdescr WRITE, palias WRITE, "
	     "timevalue WRITE, coordinate WRITE");

   FILE *fp = stdin;
   int found_items = 0;
   int found_properties = 0;
   int last_item = 0;

   for (;;)
     {
	goto_next_tag (fp, "page");
	if (feof (fp))
	  break;

	goto_next_tag (fp, "title");
	if (feof (fp))
	  break;
	int entity_letter = getc (fp);
	int entity_valid = 0;
	int entity = 0;
	if (entity_letter == 'Q') {
	  entity_valid = fscanf (fp, "%d", &entity);
	  if(verbose)
	     printf("%d\n", entity);
	}
	else if (entity_letter == 'P')
	  entity_valid = fscanf (fp, "roperty:P%d", &entity);
	goto_next_tag (fp, "ns");
	if (feof (fp))
	  break;
	int ns;
	int n = fscanf (fp, "%d", &ns);
	if (n != 1)
	  {
	     printf ("fscanf() for namespace failed, n=%d\n", n);
	     return 1;
	  }

	if(verbose) {
		printf("ns:%d val:%d\n", ns, entity_valid);
	}

	if (ns == 0)
	  {
	     if (entity_letter != 'Q' || entity_valid != 1)
	       {
		  printf ("found item without valid Q value (%c, %d)\n",
			  entity_letter, entity_valid);
		  return 1;
	       }
	     if (incremental_dump)
	       update_links (fp, mysql, entity);
	     else
	       read_item (fp, mysql, entity);
	     ++ found_items;
	     last_item = entity;
	  }
	else if (ns == 120 && ! incremental_dump)
	  {
	     if (entity_letter != 'P' || entity_valid != 1)
	       {
		  printf ("found property without valid P value (%c, %d)\n",
			 entity_letter, entity_valid);
		  return 1;
	       }
	     read_property (fp, mysql, entity);
	     ++ found_properties;
	  }
	if (incremental_dump)
	  {
	     if (found_items && found_items % 50 == 0)
	       {
		  printf ("Found %d items, %d properties.\r", found_items, found_properties);
		  fflush (stdout);
	       }
	  }
	else
	  {
	     if (found_items && found_items % 1000 == 0)
	       {
		  printf ("Found %d items, %d properties.\r", found_items, found_properties);
		  fflush (stdout);
	       }
	  }
     }
   printf ("Found %d items, %d properties.\n", found_items, found_properties);

   do_query (mysql,
	  "UNLOCK TABLE");

   if (incremental_dump)
     {
	printf ("Reenabling keys for link table.\n");
	do_query (mysql, "ALTER TABLE link ENABLE KEYS");
     }
   else
     add_indexes (mysql, last_item);

   close_database (mysql);
   return 0;
}

int read_section (FILE *fp, MYSQL *mysql, int q, const char *table)
{
   int founds = 0;
   int brace = getc (fp);
   if (brace == '{')
     {
	int delim;
	do
	  {
	     char lang[32];
	     get_quot_text (fp, lang, sizeof lang);
	     pass_text (fp, ":");
	     char label[2000];
	     get_quot_text (fp, label, sizeof label);
	     ++ founds;

	     // printf ("Found '%s:%s'\n", lang, label);
	     do_query (mysql,
		       "INSERT INTO %s "
		       "VALUES (%d, \"%s\", \"%s\")",
		       table, q, lang, label);

	     delim = getc (fp);
	     // if (strcmp(lang, "ru") == 0) printf ("ru:%s\n", label);
	  } while (delim == ',');
     }
   else if (brace == '[')
     {
	int end_brace = getc (fp);
	if (end_brace != ']')
	  {
	     printf ("q%d: section %s starting with [ not empty.\n", q, table);
	     exit (1);
	  }
     }
   else
     {
	printf ("q%d: brace not found in %s section.\n", q, table);
	exit (1);
     }
   return founds;
}


// # ;links&quot;:{&quot;enwiki&quot;:{&quot;name&quot;:&quot;Africa&quot;,&quot;badges&quot;:[]},&quot;dewiki&q

int read_links (FILE *fp, MYSQL *mysql, int q, int *ns)
{
   int founds = 0, xx;
   int brace = getc (fp);
   if (brace == '{')
     {
	int delim;
	do
	  {
	     char lang[32];
	     get_quot_text (fp, lang, sizeof lang);
	     pass_text (fp, ":");
	     char link[1024];
             xx=getc(fp);
	     if(verbose) {
		   printf("  read_links lang:%s next:%c\n", lang, xx);
	     }
	     if(xx=='{') {
	        char link_name[1024];
		do {
	        	get_quot_text (fp, link_name, sizeof link_name);
	     		if(verbose) {
		   		printf("    read_links lang:%s link_name:%s\n", lang, link_name);
			}
	                pass_text (fp, ":");
			if(strcmp(link_name, "name")==0)
	        		get_quot_text (fp, link, sizeof link);
			else if(strcmp(link_name, "badges")==0) {
	     			skip_section (fp, q, link_name);
			}
			else {
		   		printf("    read_links link_name:%s\n", link_name);
	     			skip_section (fp, q, link_name);
			}
	     	        delim = getc (fp);
	        } while (delim == ',');
             } else {
                ungetc(xx, fp);
	        get_quot_text (fp, link, sizeof link);
             }
	     ++ founds;
	     char project;

	     if(verbose) {
		   printf("  read_links lang:%s link:%s\n", lang, link);
	     }

	     size_t langlen = strlen (lang);
	     if (langlen > 5 && strcmp ("wiki", lang + langlen - 4) == 0)
	       {
		  project = 'w';
		  lang[langlen - 4] = '\0';
	       }
	     else if (langlen > 11 && strcmp ("wikivoyage", lang + langlen - 10) == 0)
	       {
		  project = 'y';
		  lang[langlen - 10] = '\0';
	       }
	     else if (langlen > 10 && strcmp ("wikiquote", lang + langlen - 9) == 0)
	       {
		  project = 'q';
		  lang[langlen - 9] = '\0';
	       }
	     else if (langlen > 11 && strcmp ("wikisource", lang + langlen - 10) == 0)
	       {
		  project = 's';
		  lang[langlen - 10] = '\0';
	       }
	     else
	       {
		  printf ("Link for q%d:%s not recognized.\n", q, lang);
		  exit (1);
	       }
	     for (char *p = lang ; *p ; ++p)
	       if (*p == '_')
		 *p = '-';
	     int found_ns = 0;
	     const char *link_without_namespace = link;
	     const char *colon = strchr (link, ':');
	     char alias[97] = "";
	     if (colon)
	       {
		  bool is_alias = false;
		  found_ns = find_namespace (link, project, lang, colon - link, &is_alias);
		  if (found_ns)
		    {
		       link_without_namespace = colon + 1;
		       
		       if (is_alias)
			 {
			    char *dest = alias;
			    const char *src = link;
			    while (src < colon)
			      *(dest ++) = *(src ++);
			    *dest = '\0';
			 }
		    }
	       }
	     do_query (mysql,
		       "INSERT INTO link "
		       "VALUES (%d, \"%c\", \"%s\", %d, \"%s\", \"%s\")",
		       q, project, lang, found_ns, link_without_namespace, alias);
	     // printf ("Link: q%d:%s:%s\n", q, lang, link);
	     if (founds == 1)
	       *ns = found_ns;
	     else if (found_ns != *ns)
	       *ns = -10; // -10 means that different namespaces are found
	     delim = getc (fp);
	  } while (delim == ',');
     }
   else if (brace == '[')
     {
	int end_brace = getc (fp);
	if (end_brace != ']')
	  {
	     printf ("q%d: section link starting with [ not empty.\n", q);
	     exit (1);
	  }
     }
   else
     {
	printf ("q%d: brace not found in link section.\n", q);
	exit (1);
     }
   return founds;
}

int read_aliases (FILE *fp, MYSQL *mysql, int q, const char *table, int *languages)
{
   int founds = 0;
   int brace = getc (fp);
   if (brace == '{')
     {
	int lang_delim;
	do
	  {
	     // get language
	     char lang[32];
	     get_quot_text (fp, lang, sizeof lang);
	     pass_text (fp, ":");
	     bool numbered_aliases = false;
	     char lang_brace = getc (fp);
	     if (lang_brace == '{')
	       numbered_aliases = true;
	     else if (lang_brace == '[')
	       {
		  int next = getc (fp);
		  if (next == ']') // empty language group
		    {
		       lang_delim = getc (fp);
		       continue;
		    }
		  ungetc (next, fp);
	       }
	     else
	       {
		  printf ("Did not find { or [ after langcode in alias "
			  "section for entity %d (table = %s).\n", q, table);
		  exit (1);
	       }

	     ++ *languages;
	     int alias_delim;
	     do
	       {
		  if (numbered_aliases)
		    {
		       char number[32];
		       get_quot_text (fp, number, sizeof number);
		       pass_text (fp, ":");
		    }
		  char alias[1024];
		  get_quot_text (fp, alias, sizeof alias);
		  ++ founds;
		  // printf ("Found alias q%d:%s:%s\n", q, lang, alias);

		  do_query (mysql,
			    "INSERT INTO %s "
			    "VALUES (%d, \"%s\", \"%s\")",
			    table, q, lang, alias);
		  alias_delim = getc (fp);
	       } while (alias_delim == ',');
	     lang_delim = getc (fp);
	  }
	while (lang_delim == ',');
     }
   else if (brace == '[')
     {
	int end_brace = getc (fp);
	if (end_brace != ']')
	  {
	     printf ("Did not find ] afer [ in empty alias "
		     "section for for entity %d (table = %s).\n", q, table);
	     exit (1);
	  }
     }
   else
     {
	printf ("Found unexpected brace %c for alias "
		"section for entity %d (table = %s).\n", brace, q, table);
	exit (1);
     }
   return founds;
}

void skip_section (FILE *fp, int q, const char *name)
{
   int ch = getc (fp);
   if (ch != '[' && ch != '{')
     {
	printf ("q%d: skip_section %s: first char is unexpected '%c'.\n", q, name, ch);
	exit (1);
     }
   int level = 1;
   bool in_quote = false;
   do
     {
	ch = getc (fp);
	switch (ch)
	  {
	   case '\\': // can escape a & so eat next character and break
	     ch = getc (fp);
	     break;

	   case '&':
	     ch = getc (fp);
	     if (ch == 'q') // &quot;
	       in_quote = ! in_quote;
	     break;

	   case '[':
	   case '{':
	     if (! in_quote)
	       ++ level;
	     break;

	   case '}':
	   case ']':
	     if (! in_quote)
	       -- level;
	     break;

	   case EOF:
	     printf ("q%d: skip_section %s: Reached EOF at level %d (%d).\n",
		     q, name, level, in_quote);
	     exit (1);

	   case '<':
	   case '>':
	     printf ("q%d: skip_section %s: Reached %c at level %d (%d).\n",
		     q, name, ch, level, in_quote);
	     exit (1);
	  }
     }
   while (level);
}

int read_item_value (FILE *fp, int *value)
{
   pass_text (fp, "{");
   pass_quot_text (fp, "entity-type");
   pass_text (fp, ":");
   pass_quot_text (fp, "item");
   pass_text (fp, ",");
   pass_quot_text (fp, "numeric-id");
   pass_text (fp, ":");
   int res = fscanf (fp, "%d", value);
   pass_text (fp, "}");
   return res;
}

struct time_value
{
   int64_t tv_year;
   int tv_month;
   int tv_day;
   int tv_hour;
   int tv_minute;
   int tv_second;
   int tv_tz;
   int tv_before;
   int tv_after;
   int tv_precision;
   int tv_model;
};

void read_time_value (FILE *fp, struct time_value *value)
{
   int res = fscanf (fp,
		     "{&quot;time&quot;:&quot;%" PRId64 "-%d-%dT%d:%d:%dZ&quot;,"
		     "&quot;timezone&quot;:%d,"
		     "&quot;before&quot;:%d,"
		     "&quot;after&quot;:%d,"
		     "&quot;precision&quot;:%d,"
		     "&quot;calendarmodel&quot;:"
		     "&quot;http:\\/\\/www.wikidata.org\\/entity\\/Q%d&quot;}",
		     &(value->tv_year),
		     &(value->tv_month),
		     &(value->tv_day),
		     &(value->tv_hour),
		     &(value->tv_minute),
		     &(value->tv_second),
		     &(value->tv_tz),
		     &(value->tv_before),
		     &(value->tv_after),
		     &(value->tv_precision),
		     &(value->tv_model));
   if (res != 11)
     {
	// This happens due to values stored in invalid format, see bug 49425
	fprintf (stderr, "\nAn error occured while parsing a timevalue (res = %d). "
		 "Setting precision to -1 and trying to recover.\n", res);
	value->tv_precision = -1;
	while ((! feof (fp)) && (getc (fp) != '}'))
	  ;	
     }
   if (value->tv_model != 1985727 && value->tv_model != 1985786 &&
       value->tv_precision != -1)
     printf ("New calendar model: %d\n", value->tv_model);
}


struct quantity
{
   // Keep the floating point numbers as strings to avoid conversions
   char value[32];
};

void read_quantity (FILE *fp, struct quantity *value, int q)
{
   char link_name[1024];
   char get_value[1024];
   int delim;

   delim=getc(fp);

   if(delim=='{') do { // xx
	
       	get_quot_text (fp, link_name, sizeof link_name);
	if(verbose) {
		printf("    read_quantity link_name:%s\n", link_name);
	}
	pass_text (fp, ":");
	if(strcmp(link_name, "amount")==0)
		get_quot_text (fp, get_value, sizeof get_value);
	else if(strcmp(link_name, "unit")==0)
		get_quot_text (fp, get_value, sizeof get_value);
	else if(strcmp(link_name, "upperBound")==0)
		get_quot_text (fp, get_value, sizeof get_value);
	else if(strcmp(link_name, "lowerBound")==0)
		get_quot_text (fp, get_value, sizeof get_value);
	else {
		printf("    read_quantity link_name:%s\n", link_name);
	     	skip_section (fp, q, link_name);
	}
        delim = getc (fp);
   } while (delim == ',');
   if (delim == ']')
		printf("    read_quantity end delim:%c\n", delim);
   if(verbose)
		printf("    read_quantity end delim:%c\n", delim);
   strcpy(value->value, "000");	
}

struct coordinate
{
   // Keep the floating point numbers as strings to avoid conversions
   char co_latitude[32];
   char co_longitude[32];
   char co_altitude[32];
   int co_globe;
   char co_precision[32];
};

void read_coordinate (FILE *fp, struct coordinate *value)
{
   // Note: the sequence changed in 2013-07-10 dump:
   // Before: latitude, longitude, altitude, globe, precision
   // After: latitude, longitude, altitude, precision, globe
   int res = fscanf (fp,
		     "{&quot;latitude&quot;:%[^,],"
		     "&quot;longitude&quot;:%[^,],"
		     "&quot;altitude&quot;:%[^,],",
		     &(value->co_latitude[0]),
		     &(value->co_longitude[0]),
		     &(value->co_altitude[0]));
   if (res != 3)
     {
	fprintf (stderr, "\nUnexpected coordinate format (res = %d)\n", res);
	exit (1);
     }
   char next[32];
   int res2 = fscanf (fp, "&quot;%[a-z]&quot;:", &next[0]);
   if (res2 != 1)
     {
	fprintf (stderr, "\nUnexpected coordinate format (res2 = %d)\n", res2);
	exit (1);
     }
   int res3;
   char globe[80];
   if (strcmp (next, "precision") == 0)
     res3 = fscanf (fp,
		    "%[^,],"
		    "&quot;globe&quot;:%[^}]}",
		    &(value->co_precision[0]), globe);
   else if (strcmp (next, "globe") == 0)
     res3 = fscanf (fp,
		    "%[^,],"
		    "&quot;precision&quot;:%[^}]}",
		    globe, &(value->co_precision[0]));
   else
     {
	fprintf (stderr, "\nUnexpected coordinate format (next=%s)\n", next);
	exit (1);
     }
   if (res3 != 2)
     {
	fprintf (stderr, "\nUnexpected coordinate format (next=%s, res3=%d)\n", next, res3);
	exit (1);
     }
   if (strcmp (globe, "null") == 0)
     value->co_globe = 0;
   else if (strcmp (globe, "&quot;earth&quot;") == 0)
     value->co_globe = -1;
   else
     {
	int res4 = sscanf (globe,
			   "&quot;http:\\/\\/www.wikidata.org\\/entity\\/Q%d&quot;",
			   &(value->co_globe));
	if (res4 != 1)
	  {
	     fprintf (stderr, "\nUnexpected coordinate format (next=%s, globe=%s, res4=%d)\n",
		      next, globe, res4);
	     exit (1);
	  }
	if (value->co_globe != 2 &&	 // Earth
	    value->co_globe != 111 &&	 // Mars
	    value->co_globe != 308 &&	 // Mercury
	    value->co_globe != 313 &&	 // Venus
	    value->co_globe != 405 &&	 // Moon
	    value->co_globe != 2565 &&	 // Titan (Saturn moon)
	    value->co_globe != 15034	 // Mimas (Saturn moon)
	   )
	  printf ("Coordinates for new globe: %d\n", value->co_globe);
     }
}

enum datatype { to_be_determined, novalue, somevalue, item, string, time,
     coordinate, bad_coordinate, quantity, bad_time };

struct claim
{
   int property;
   enum datatype datatype;
   char string_value[1024];
   int item_value;
   struct time_value time_value;
   struct coordinate coordinate;
   struct quantity quantity;
};

enum claim_type { ct_claim, ct_qualifier, ct_ref };

const char *format_claim_values (struct claim *claim)
{
   static char format[2048];
   int ret;
   switch (claim->datatype)
     {
      case novalue:
	ret = snprintf (format, sizeof format, "%d, \"novalue\", 0, NULL",
		       claim->property);
	break;
      case somevalue:
	ret = snprintf (format, sizeof format, "%d, \"somevalue\", 0, NULL",
		       claim->property);
	break;
      case item:
	ret = snprintf (format, sizeof format, "%d, \"item\", %d, NULL",
		       claim->property, claim->item_value);
	break;
      case string:
	ret = snprintf (format, sizeof format, "%d, \"string\", 0, \"%s\"",
		       claim->property, claim->string_value);
	break;
      case time:
	ret = snprintf (format, sizeof format, "%d, \"time\", 0, NULL",
		       claim->property);
	break;
      case coordinate:
	ret = snprintf (format, sizeof format, "%d, \"coordinate\", 0, NULL",
		       claim->property);
	break;
      case quantity:
	ret = snprintf (format, sizeof format, "%d, \"quantity\", 0, NULL",
		       claim->property);
	break;
      case bad_coordinate:
	ret = snprintf (format, sizeof format, "%d, \"bad-coordinate\", 0, NULL",
		       claim->property);
	break;
      case bad_time:
	ret = snprintf (format, sizeof format, "%d, \"bad-time\", 0, NULL",
		       claim->property);
	break;
      case to_be_determined:
      default:
	printf ("store_claim_format: datatype have wrong value %d.\n",
		claim->datatype);
	exit (1);
     }
   if (ret < 0 || ret >= (int) sizeof format)
     {
	printf ("store_claim_cormat: sprintf failed %d.\n", ret);
	exit (1);
     }
   return format;
}

void read_claim (FILE *fp, int q, struct claim *claim)
{
   pass_text (fp, "[");
   char value[16];
   get_quot_text (fp, value, sizeof value);
   if (strcmp (value, "value") == 0)
     claim->datatype = to_be_determined;
   else if (strcmp (value, "somevalue") == 0)
     claim->datatype = somevalue;
   else if (strcmp (value, "novalue") == 0)
     claim->datatype = novalue;
   else
     {
	printf ("q%d: unknown value: \"%s\" for claim.\n", q, value);
	exit (1);
     }

   pass_text (fp, ",");
   int res = fscanf (fp, "%d", &(claim->property));
   if (res != 1)
     {
	printf ("q%d: Property # not numeric.\n", q);
	exit (1);
     }

   if(verbose)
	printf("    read_claim q:%d pro%d\n", q, claim->property);
   if (claim->datatype == to_be_determined)
     {
	pass_text (fp, ",");
	char datatype[32];
	get_quot_text (fp, datatype, sizeof datatype);
	pass_text (fp, ",");

	if (strcmp (datatype, "wikibase-entityid") == 0)
	  {
	     claim->datatype = item;
	     int res = read_item_value (fp, & (claim->item_value));
	     if (res != 1)
	       {
		  printf ("q%d: error reading item # for property value.\n", q);
		  exit (1);
	       }
	  }
	else if (strcmp (datatype, "string") == 0)
	  {
	     claim->datatype = string;
	     get_quot_text (fp, claim->string_value, sizeof (claim->string_value));
	  }
	else if (strcmp (datatype, "time") == 0)
	  {
	     claim->datatype = time;
	     read_time_value (fp, & (claim->time_value));
	  }
	else if (strcmp (datatype, "globecoordinate") == 0)
	  {
	     claim->datatype = coordinate;
	     read_coordinate (fp, & (claim->coordinate));
	  }
	else if (strcmp (datatype, "quantity") == 0)
	  {
	     claim->datatype = quantity;
	     read_quantity (fp, & (claim->quantity), q);
	  }
	else if (strcmp (datatype, "bad") == 0)
	  {
	     if (claim->property == 625 ||
		 claim->property == 626)
	       // These are currently the only properties with coordinate as data type
	       {
		  claim->datatype = bad_coordinate;
		  read_coordinate (fp, & (claim->coordinate));
	       }
	     else
	       // Until now only coordinate and time values can be "bad"
	       {
		  claim->datatype = bad_time;
		  read_time_value (fp, & (claim->time_value));
	       }
	  }
	else
	  {
	     printf ("q%d: unknown datatype \"%s\" for property.\n", q, datatype);
	     exit (1);
	  }
     }
   pass_text (fp, "]");
}

void insert_value (MYSQL *mysql, int q, int statement, int ref, int refclaim,
		  int qualifier, struct claim *claim)
{
   if (claim->datatype == time)
     {
	struct time_value *value = &(claim->time_value);
	do_query (mysql,
		  "INSERT INTO timevalue "
		  "VALUES (%d, %d, %d, %d, %d, " 
		  "%" PRId64 ", %d, %d, "
		  "%d, "
		  "%d, %d, %d, %d, "
		  "'%d')",
		  q, statement, ref, refclaim, qualifier,
		  value->tv_year, value->tv_month, value->tv_day,
		  value->tv_hour * 3600 + value->tv_minute * 60 + value->tv_second,
		  value->tv_tz, value->tv_before, value->tv_after, value->tv_precision,
		  value->tv_model);
     }
   else if (claim->datatype == coordinate ||
	    claim->datatype == bad_coordinate)
     {
	struct coordinate *value = &(claim->coordinate);
	do_query (mysql,
		  "INSERT INTO coordinate "
		  "VALUES (%d, %d, %d, %d, %d, " 
		  "%s, %s, %s, "
		  "%d, %s)",
		  q, statement, ref, refclaim, qualifier,
		  value->co_latitude, value->co_longitude, value->co_altitude,
		  value->co_globe, value->co_precision);
     }
}

void read_statement (FILE *fp, MYSQL *mysql, int q, int *founds)
{
   static unsigned int statement_counter = 0;
   if (statement_counter == 0)
     {
	// In case we are adding an incremental dump
	MYSQL_RES *result = do_query_use
	  (mysql, "SELECT MAX(s_statement) FROM statement");
	MYSQL_ROW row = mysql_fetch_row (result);
	if (row[0])
	  statement_counter = atoi (row[0]);
	row = mysql_fetch_row (result);
	mysql_free_result (result);
     }

   int brace = getc (fp);
   if (brace != '{')
     {
	printf ("q%d: statement object start with '%c' ('{' expected).\n", q, brace);
	exit (1);
     }

   ++ statement_counter;
   int no_of_refs = 0;
   int no_of_qualifiers = 0;
   struct claim claim = { 0, to_be_determined, "", 0, {0,0,0,0,0,0,0,0,0,0,0}, {"","","",0,""}, {""} };
   int delim;
   do
     {
	char key[16];
	get_quot_text (fp, key, sizeof key);
	pass_text (fp, ":");

	if (strcmp (key, "m") == 0)
	  {
	     read_claim (fp, q, &claim);
	  }
	else if (strcmp (key, "q") == 0)
	  {
	     pass_text (fp, "[");
	     int next = getc (fp);

	     if (next != ']') // not empty list of qualifiers
	       {
		  ungetc (next, fp);
		  int delim;
		  do
		    {
		       struct claim qualifier;
		       read_claim (fp, q, &qualifier);
		       const char *claim_values = format_claim_values (& qualifier);
		       ++ no_of_qualifiers;
		       do_query (mysql,
				 "INSERT INTO qualifier "
				 "VALUES (%d, %d, %d, %s)",
				 statement_counter, q, no_of_qualifiers, claim_values);
		       insert_value (mysql, q, statement_counter, 0, 0, no_of_qualifiers, &qualifier);
		       delim = getc (fp);
		    }
		  while (delim == ',');
	       }
	  }
	else if (strcmp (key, "g") == 0)
	  {
	     char GUID[64];
	     get_quot_text (fp, GUID, sizeof GUID);
	  }
	else if (strcmp (key, "rank") == 0)
	  {
	     int rank;
	     int res = fscanf (fp, "%d", &rank);
	     if (res != 1)
	       {
		  printf ("q%d: error reading statement rank.\n", q);
		  exit (1);
	       }
	  }
	else if (strcmp (key, "refs") == 0)
	  {
	     pass_text (fp, "["); // start list of references
	     int start = getc (fp);
	     if (start != ']')
	       {
		  // The reference list is not empty
		  ungetc (start, fp);
		  int reference_delim;
		  do
		    {
		       pass_text (fp, "[");
		       int no_of_claims = 0;
		       ++ no_of_refs;
		       int claim_delim;
		       do
			 {
			    struct claim ref;
			    read_claim (fp, q, &ref);
			    const char *claim_values = format_claim_values (& ref);
			    ++ no_of_claims;
			    do_query (mysql,
				      "INSERT INTO refclaim "
				      "VALUES (%d, %d, %d, %d, %s)",
				      statement_counter, no_of_refs, no_of_claims, q, claim_values);
			    insert_value (mysql, q, statement_counter, no_of_refs, no_of_claims,
					  0, & ref);
			    claim_delim = getc (fp);
			 }
		       while (claim_delim == ',');
		       do_query (mysql,
				 "INSERT INTO ref "
				 "VALUES (%d, %d, %d, %d)",
				 statement_counter, no_of_refs, q, no_of_claims);
		       reference_delim = getc (fp);
		    }
		  while (reference_delim == ',');
	       }
	  }
	else
	  {
	     printf ("q%d: unknown key \"%s\" in statement object.\n", q, key);
	     exit (1);
	  }
	delim = getc (fp);
     } while (delim == ',');

   const char *claim_values = format_claim_values (& claim);
   do_query (mysql,
	     "INSERT INTO statement "
	     "VALUES (%d, %d, %d, %d, %s, 0)",
	     statement_counter, q, no_of_qualifiers, no_of_refs, claim_values);
   insert_value (mysql, q, statement_counter, 0, 0, 0, & claim);
   ++ *founds;
}

int read_statements (FILE *fp, MYSQL *mysql, int q)
{
   int founds = 0;
   int brace = getc (fp);
   if (brace == '[')
     {
	// list of statements
	int delim = getc (fp);
	if (delim == ']') // empty list
	  return 0;
	ungetc (delim, fp);
	do
	  {
	     read_statement (fp, mysql, q, &founds);
	     delim = getc (fp);
	  }
	while (delim == ',');
     }
   else
     {
	read_statement (fp, mysql, q, &founds);
     }
   return founds;
}
