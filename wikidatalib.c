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

// For strdup()
#define _POSIX_C_SOURCE 200809L

/*
 * wikidatalib.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "wikidatalib.h"

// #define LANG2_TO_INT(lang) (lang[0] << 8 | lang[1])
// #define LANG3_TO_INT(lang) ((lang[0] << 8 | lang[1]) << 8 | lang[2])

static MYSQL *mysql_wiki = NULL;

/*
 * Get the dumpdate for the indicated database.
 * Returns a string to memory allocated with malloc(3). 
 */
char *get_dumpdate (MYSQL *mysql, const char *database)
{
   MYSQL_RES *result = do_query_use (mysql,
				     "SELECT %s.dumpdate FROM info",
				     database);
   MYSQL_ROW row = mysql_fetch_row (result);
   if (! row)
     {
	printf ("%s.info.dumpdate is not set.\n",
		database);
	exit (1);
     }
   char *dumpdate = strdup (row[0]);
   row = mysql_fetch_row (result);
   if (row)
     {
	printf ("Table %s.info has more than one row.\n",
		database);
	exit (1);
     }
   return dumpdate;
}

/*
 * Print the dumpdate of the opened database. Exit on error.
 */
void print_dumpdate (MYSQL *mysql)
{
   MYSQL_RES *result = do_query_use (mysql,
				     "SELECT dumpdate FROM info");
   MYSQL_ROW row = mysql_fetch_row (result);
   if (! row)
     {
	printf ("info.dumpdate is not set.\n");
	exit (1);
     }
   const char *dumpdate = row[0];
   printf ("Made from a database dump from %s.\n\n", dumpdate);
   row = mysql_fetch_row (result);
   if (row)
     {
	printf ("Table info has more than one row.\n");
	exit (1);
     }
   return;
}

/*
 * Return the namespace number if prefix is a namespace name
 * for the language lang, otherwise return 0.
 * Note: prefix must be escaped for injection to MySQL.
 */
int find_namespace (const char *prefix, char project,
		    const char *lang, int len, bool *is_alias)
{
   // Note 1: All links should have been normalized to use the canonical 
   // namespace names, but I have seen cases where the English
   // names are used (maybe inserted before the names was localized?)
   // for kbdwiki, bxrwiki and others. Therefore we start with a manual test
   // for the global used English names. This will also save database lookups.

   // Note 2: You cannot just lookup in the database using lang = 'en' for
   // all languages, as some wikis without a Portal namespace do have links
   // starting with Portal:
   switch (len)
     {
      case 4:
	if (strncmp (prefix, "Talk", 4) == 0)
	  return 1;
	if (strncmp (prefix, "User", 4) == 0)
	  return 2;
	if (strncmp (prefix, "File", 4) == 0)
	  return 6;
	if (strncmp (prefix, "Help", 4) == 0)
	  return 12;
	break;
      case 5:
	if (strncmp (prefix, "Media", 5) == 0)
	  return -2;
	break;
      case 6:
	if (strncmp (prefix, "Module", 6) == 0)
	  return 828;
	break;
      case 7:
	if (strncmp (prefix, "Special", 7) == 0)
	  return -1;
	if (strncmp (prefix, "Project", 7) == 0)
	  return 4;
	break;
      case 8:
	if (strncmp (prefix, "Template", 8) == 0)
	  return 10;
	if (strncmp (prefix, "Category", 8) == 0)
	  return 14;
	break;
      case 9:
	if (strncmp (prefix, "User talk", 9) == 0)
	  return 3;
	if (strncmp (prefix, "File talk", 9) == 0)
	  return 7;
	if (strncmp (prefix, "MediaWiki", 9) == 0)
	  return 8;
	if (strncmp (prefix, "Help talk", 9) == 0)
	  return 13;
	break;
      case 11:
	if (strncmp (prefix, "Module talk", 11) == 0)
	  return 829;
	break;
      case 12:
	if (strncmp (prefix, "Project talk", 12) == 0)
	  return 5;
	break;
      case 13:
	if (strncmp (prefix, "Template talk", 13) == 0)
	  return 11;
	if (strncmp (prefix, "Category talk", 13) == 0)
	  return 15;
	break;
      case 14:
	if (strncmp (prefix, "MediaWiki talk", 14) == 0)
	  return 9;
	break;
     }

   if (! mysql_wiki)
     mysql_wiki = open_named_database ("wiki");

   MYSQL_RES *result = do_query_use
     (mysql_wiki,
	 "SELECT ns, alias "
	 "FROM %c_namespace "
	 "WHERE lang = '%s' AND name = \"%.*s\"",
	 project, lang, len, prefix);
   if (! result)
     {
	printf ("find_namespace(): do_query_use failed, project=%c, "
		"lang=%s, returning 999.\n",
		project, lang);
	return 999;
     }
   MYSQL_ROW row = mysql_fetch_row (result);
   int ns = 0;
   if (row)
     {
	ns = atoi (row[0]);
	const char *alias = row[1];
	
	if  (*alias == '1')
	  *is_alias = true;
	row = mysql_fetch_row (result);
     }
   mysql_free_result (result);
   return ns;
}

/*
 * Print an interwiki link to stdout
 */
void print_link (const char *lang, int ns, const char *title)
{
   printf ("[[:%s:", lang);
   if (ns)
     {
	if (! mysql_wiki)
	  mysql_wiki = open_named_database ("wiki");
	MYSQL_RES *result = do_query_use
	  (mysql_wiki,
	      "SELECT name "
	      "FROM w_namespace "
	      "WHERE lang = '%s' AND ns = %d",
	      lang, ns);
	MYSQL_ROW row = mysql_fetch_row (result);
	if (! row)
	  {
	     printf ("\nNamespace %d for %s not found.\n",
		     ns, lang);
	     exit (1);
	  }
	printf ("%s:", row[0]);
	row = mysql_fetch_row (result);
	mysql_free_result (result);
     }
   printf ("%s]]", title);
}

/*
 * Return the local name for a namespace in a static string
 */
const char *get_namespace_name (const char *lang, int ns)
{
   static char name[100] = "";
   if (ns == 0)
     return "";
   if (! mysql_wiki)
     mysql_wiki = open_named_database ("wiki");
   MYSQL_RES *result = do_query_use
     (mysql_wiki,
	 "SELECT name "
	 "FROM w_namespace "
	 "WHERE lang = '%s' AND ns = %d",
	 lang, ns);
   MYSQL_ROW row = mysql_fetch_row (result);
   if (! row)
     {
	printf ("\nNamespace %d for %s not found.\n",
		ns, lang);
	exit (1);
     }
   strcpy (name, row[0]);
   row = mysql_fetch_row (result);
   mysql_free_result (result);
   return name;
}

/*
 * Read from a file until the searched text is reached.
 * Doesn't work if the first character in str isn't unique.
 */
void read_to (FILE *fp, const char *str)
{
   int ch;
search_for_first_character:
   do
     {	
	ch = getc (fp);
	if (ch == EOF)
	  return;
     }   
   while (ch != *str);
   // first character is found, check the next characters
   const char *p = str + 1;
   while (*p)
     {
	int ch = getc (fp);
	if (ch != *(p ++))
	  goto search_for_first_character;
     }
   return;
}

/*
 * Read a varchar value from a MySQL dump file, and return it in a user supplied variable.
 * Convert '_' to space
 */
const char *read_varchar (FILE *fp, char *value, size_t len)
{   
   char *pos = value;
   size_t space_left = len;
   while (space_left)
     {
	int ch = getc (fp);
	if (ch == EOF)
	  {
	     printf ("read_varchar: Unexpected EOF\n");
	     exit (1);
	  }
	else if (ch == '_')
	  {
	     *pos = ' ';
	  }
	else if (ch == '\'')
	  {
	     *pos = 0;
	     return value;
	  }
	else
	  *pos = ch;
	++pos;
	-- space_left;
	if (ch == '\\')
	  {
	     ch = getc (fp);
	     if (ch == EOF)
	       {
		  printf ("read_varchar: Unexpected EOF after \\\n");
		  exit (1);
	       }
	     *pos = ch;
	     ++pos;
	     -- space_left;
	  }
     }
   printf ("read_varchar: Unexpected long value\n");
   exit (1);
}

/*
 * Pass n apostrophes in a MySQL dump file
 */
void pass_n_apostrophes (FILE *fp, int n)
{
   while (n)
     {
	int ch = getc (fp);
	if (ch == EOF)
	  {
	     printf ("pass_n_apostrophes: Unexpected EOF\n");
	     exit (1);
	  }
	else if (ch == '\'')
	  {
	     -- n;
	  }
	else if (ch == '\\')
	  {
	     ch = getc (fp);
	     if (ch == EOF)
	       {
		  printf ("pass_n_apostrophes: Unexpected EOF after \\\n");
		  exit (1);
	       }
	  }
     }
}

/*
 * Add index indexes and analyze and optimize a table
 */
void add_index (const char *table, MYSQL *mysql, const char *indexes)
{
   printf ("Add indexes to %s table\n", table);
   do_query (mysql,
	     "ALTER TABLE %s "
	     "%s",
	     table, indexes);

   printf ("Analyze and optimize %s table\n", table);
   do_query_res (mysql, "ANALYZE TABLE %s", table);
   do_query_res (mysql, "OPTIMIZE TABLE %s", table);
}

/*
 * Open database
 */
MYSQL *open_database (void)
{
   return open_named_database (DATABASE_NAME);
}

/*
 * Open named database
 */
MYSQL *open_named_database (const char *name)
{
   MYSQL *mysql = malloc (sizeof (MYSQL));
   if (!mysql)
     {
	printf ("open_database: malloc() failed\n");
	exit (1);
     }
   mysql_init (mysql);
   mysql_options (mysql, MYSQL_READ_DEFAULT_GROUP, "link");
   if (!mysql_real_connect (mysql, DATABASE_HOST,
			    DATABASE_USER, DATABASE_PASSWD,
			    name, DATABASE_PORT, DATABASE_SOCKET, 0))
     {
	printf ("open_database: mysql_real_connect() failed: %s\n",
		mysql_error (mysql));
	exit (1);
     }
   return mysql;
}

/*
 * Close database
 */
void close_database (MYSQL *mysql)
{
   mysql_close (mysql);
   free (mysql);
}

/*
 * return true if the table exists
 */
bool table_exists (MYSQL *mysql, const char *table)
{
   char query[128];
   sprintf (query, "DESCRIBE `%s`", table); // take care: can overflow
   if (mysql_query (mysql, query))
     {
	// Error - table doesn't exist
	return false;
     }
   // Eat the result
   MYSQL_RES *result = mysql_use_result (mysql);
   if (!result)
     {
	printf ("table_exists(%s): mysql_use_result failed unexpedted\n",
		table);
	return false;
     }
   while (mysql_fetch_row (result))
     ;
   mysql_free_result (result);
   return true;
}

/*
 * return true if the table have no rows
 */
bool table_empty (MYSQL *mysql, const char *table)
{
   char query[128];
   sprintf (query, "SELECT 0 FROM %s LIMIT 1", table); // take care: can overflow
   if (mysql_query (mysql, query))
     {
	// Error - table probably doesn't exist
	return true;
     }
   // Eat the result
   MYSQL_RES *result = mysql_use_result (mysql);
   if (!result)
     {
	printf ("table_empty(%s): mysql_use_result failed unexpedted\n",
		table);
	return true;
     }
   MYSQL_ROW row = mysql_fetch_row (result);
   bool empty = (row == NULL);
   mysql_free_result (result);
   return empty;
}

/*
 * Do a SQL query build with a format and arguments like printf arguments
 * The caller is responsible to read data returned from the query, if any.
 * Exits on error
 */
void do_query (MYSQL *mysql, const char *format, ...)
{
   char query[2000];
   va_list ap;
   va_start (ap, format);
   int len = vsnprintf (query, sizeof query, format, ap);
   va_end (ap);
   if ((size_t) len >= sizeof query)
     {
	printf ("do_query: query string '%s' too small for query\n",
		query);
	exit (1);
     }

   if (*format == '/' && strstr (format, "__PRINT_QUERY__"))
     printf ("Mysql query: %s\n", query);

   if (mysql_query (mysql, query))
     {
	printf ("do_query: The query\n\n%s\n\nfailed: %s\n",
		query, mysql_error (mysql));
	exit (1);
     }
   return;
}

/*
 * Do a SQL query build with a format and arguments like printf arguments
 * Eat all returned data from the query
 * Exits on error
 */
void do_query_res (MYSQL *mysql, const char *format, ...)
{
   char query[1024];
   va_list ap;
   va_start (ap, format);
   int len = vsnprintf (query, sizeof query, format, ap);
   va_end (ap);
   if ((size_t) len >= sizeof query)
     {
	printf ("do_query_res: query string '%s' too small for query\n",
		query);
	exit (1);
     }

   if (*format == '/' && strstr (format, "__PRINT_QUERY__"))
     printf ("Mysql query: %s\n", query);

   if (mysql_query (mysql, query))
     {
	printf ("do_query_res: The query\n\n%s\n\nfailed: %s\n",
		query, mysql_error (mysql));
	exit (1);
     }
   // Eat the result
   MYSQL_RES *result = mysql_use_result (mysql);
   while (mysql_fetch_row (result))
     ;
   mysql_free_result (result);
   return;
}

/*
 * Do a SQL query build with a format and arguments like printf arguments
 * Returns a pointer to a result object obtained from mysql_use_result()
 * Exits on error
 */
MYSQL_RES *do_query_use (MYSQL *mysql, const char *format, ...)
{
   char query[2048];
   va_list ap;
   va_start (ap, format);
   int len = vsnprintf (query, sizeof query, format, ap);
   va_end (ap);
   if (len < 0)
     {
	printf ("do_query_use: vsnprintf failed: %m (format=%s), ", format);
	printf ("returning NULL - program may crash ...\n");
	return NULL;
     }

   if ((size_t) len >= sizeof query)
     {
	printf ("do_query_use: query string '%s' too small for query "
		"(formated length = %d, size = %zu\n",
		query, len, sizeof query);
	exit (1);
     }
   if (*format == '/' && strstr (format, "__PRINT_QUERY__"))
     printf ("Mysql query: %s\n", query);

   if (mysql_query (mysql, query))
     {
	printf ("do_query_use: The query\n\n%s\n\nfailed: %s\n",
		query, mysql_error (mysql));
	exit (1);
     }
   MYSQL_RES *result = mysql_use_result (mysql);
   if (! result)
     {
	printf ("do_query_use: mysql_use_result() failed: %s\n",
		mysql_error (mysql));
	exit (1);
     }
   return result;
}

/*
 * Do a SQL query build with a format and arguments like printf arguments
 * Returns a pointer to a result object obtained from mysql_store_result()
 * Exits on error
 */
MYSQL_RES *do_query_store (MYSQL *mysql, const char *format, ...)
{
   char query[1024];
   va_list ap;
   va_start (ap, format);
   int len = vsnprintf (query, sizeof query, format, ap);
   va_end (ap);
   if ((size_t) len >= sizeof query)
     {
	printf ("do_query_store: query string '%s' too small for query\n",
		query);
	exit (1);
     }

   if (*format == '/' && strstr (format, "__PRINT_QUERY__"))
     printf ("Mysql query: %s\n", query);

   if (mysql_query (mysql, query))
     {
	printf ("do_query_store: The query\n\n%s\n\nfailed: %s\n",
		query, mysql_error (mysql));
	exit (1);
     }
   MYSQL_RES *result = mysql_store_result (mysql);
   if (! result)
     {
	printf ("do_query_store: mysql_store_result() failed: %s\n",
		mysql_error (mysql));
	exit (1);
     }
   return result;
}
