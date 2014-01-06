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

#include <stdio.h>
#include "wikidatalib.h"
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <locale.h>
#include <ctype.h>

#include <regex.h>

static int regex_id = 0;

char *copy_and_rexec_escape (char *dest, const char *src, const char *stop)
{
   while (*src && src != stop)
     {
	if (*src == '^' || *src == '.' || *src == '$' || *src == '(' ||
	    *src == ')' || *src == '|' || *src == '*' || *src == '+' ||
	    *src == '?' || *src == '{' || *src == '\\')
	  *(dest ++) = '\\';
	*(dest ++) = *(src ++);
     }
   *dest = '\0';
   return dest;
}

void copy_and_mysql_escape (char *dest, const char *src)
{
   while (*src)
     {
	if (*src == '\\' || *src == '\'')
	  {
	     *(dest ++) = '\\';
	  }
	*(dest ++) = *(src ++);
     }
   *dest = '\0';
}

void copy (char *dest, const char *src, size_t len)
{
   while (len --)
     *(dest ++) = *(src ++);
   *dest = '\0';
}

void find_links (int recursion_level, MYSQL *mysql, const char *lang,
		 const char *rx, int base_item);

int main (int argc, char *argv[])
{
   setlocale (LC_CTYPE, "");

   const char *database;
   if (argc < 3)
     {
	printf ("Usage: %s lang regex name1 name2 [database]\n", argv[0]);
	return 1;
     }
   const char *lang = argv[1];
   const char *regex = argv[2];
   const char *name1 = argv[3];
   const char *name2 = argv[4];
   if (argc < 6)
     database = "wikidatawiki";
   else
     database = argv[5];

   MYSQL *mysql = open_named_database (database);
   print_dumpdate (mysql);
  
   printf ("* Searching started from %s:%s\n", lang, regex);
   fflush (stdout);
   // goto print;
   do_query (mysql,
	     "DROP TABLE IF EXISTS regexmerge_rx");
   do_query (mysql,
	     "CREATE TABLE regexmerge_rx ("
	     "id INT AUTO_INCREMENT PRIMARY KEY NOT NULL,"
	     "lang VARCHAR(12) NOT NULL,"
	     "rx VARCHAR(200) NOT NULL,"
	     "item INT UNSIGNED NOT NULL,"
	     "count MEDIUMINT UNSIGNED,"
	     "UNIQUE KEY (lang, rx)) "
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   do_query (mysql,
	     "DROP TABLE IF EXISTS regexmerge_item");
   do_query (mysql,
	     "CREATE TABLE regexmerge_item ("
	     "item INT UNSIGNED NOT NULL,"
	     "sub1 VARCHAR(200) NOT NULL,"
	     "sub2 VARCHAR(200) NOT NULL,"
	     "hash1 BINARY(16) NOT NULL,"
	     "hash2 BINARY(16) NOT NULL,"
	     "base_item INT NOT NULL,"
	     "base_lang VARCHAR(12) NOT NULL,"
	     "UNIQUE KEY (hash1, hash2, item)) "
	     "ENGINE=MyISAM "
	     "CHARSET binary");
   printf ("{{Collapse|\n");
   find_links (1, mysql, lang, regex, 0);
   printf ("|All searched regexs}}\n");

// print: ;
   printf ("== Different items which may refer to the same %s and %s combination ==\n",
	   name1, name2);
   MYSQL_RES *result =
     do_query_store (mysql,
		     "SELECT ri1.item, ri2.item, ri1.sub1, ri1.sub2, "
		     "  ri1.base_item, ri2.base_item, ri1.base_lang, ri2.base_lang, "
		     "  (SELECT i_links FROM item WHERE i_id = ri1.item), "
		     "  (SELECT i_links FROM item WHERE i_id = ri2.item), "
		     "  (SELECT i_use FROM item WHERE i_id = ri1.item), "
		     "  (SELECT i_use FROM item WHERE i_id = ri2.item) "
		     "FROM regexmerge_item ri1 "
		     "JOIN regexmerge_item ri2 "
		     "  ON ri1.hash1 = ri2.hash1 AND ri1.hash2 = ri2.hash2 AND "
		     "    ri1.item < ri2.item");
   MYSQL_ROW row;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item1 = row[0];
	const char *item2 = row[1];
	const char *sub1 = row[2];
	const char *sub2 = row[3];
	int base_item1 = atoi (row[4]);
	int base_item2 = atoi (row[5]);
	const char *base_lang1 = row[6];
	const char *base_lang2 = row[7];
	int links1 = atoi (row[8]);
	int links2 = atoi (row[9]);
	int use1 = atoi (row[10]);
	int use2 = atoi (row[11]);

	MYSQL_RES *result2 =
	  do_query_use (mysql,
			// "/*__PRINT_QUERY__*/"
			"SELECT COUNT(DISTINCT k_lang), "
			"  (SELECT k_text FROM link WHERE k_id = %s AND k_lang = '%s'), " 
			"  (SELECT k_text FROM link WHERE k_id = %s AND k_lang = '%s') " 
			"FROM link "
			"WHERE k_id IN (%s, %s)",
			item1, base_lang1, item2, base_lang2,
			item1, item2);
	MYSQL_ROW row2 = mysql_fetch_row (result2);
	if (! row2)
	  {
	     printf ("Unexpected mysql result (no rows), %s, %s.\n",
		     item1, item2);
	     exit (1);
	  }
	int links_both = atoi (row2[0]);
	const char *link1 = row2[1];
	const char *link2 = row2[2];
	
	printf ("* %s: %s, %s: %s\n",
		name2, sub2, name1, sub1);
	printf ("** [[Q%s]] ([[:%s:%s]], %d link%s",
		item1, base_lang1, link1, links1, links1 > 1 ? "s" : "");
	if (base_item1)
	  printf (", [[Q%d|model]]", base_item1);
	if (use1)
	  printf (", %d use%s", use1, use1 > 1 ? "s" : "");
	printf (")");
	printf ("\n");
	printf ("** [[Q%s]] ([[:%s:%s]], %d link%s",
		item2, base_lang2, link2, links2, links2 > 1 ? "s" : "");
	if (base_item2)
	  printf (", [[Q%d|model]]", base_item2);
	if (use2)
	  printf (", %d use%s", use2, use2 > 1 ? "s" : "");
	printf (")");
	int same = links1 + links2 - links_both;
	if (same)
	  printf (" '''%d link%s to same language%s'''",
		  same, same > 1 ? "s" : "", same > 1 ? "s" : "");
	printf ("\n");
	row2 = mysql_fetch_row (result2);
	if (row2)
	  {
	     printf ("Unexpected mysql result (more than one row), %s, %s.\n",
		     item1, item2);
	     exit (1);
	  }
	mysql_free_result (result2);
     }
   mysql_free_result (result);
   close_database (mysql);
}

void make_regex (char *rx, const char *link,
		 const char *sub1, size_t sub1_len,
		 const char *sub2, size_t sub2_len)
{
   char *ptr = rx;
   *(ptr ++) = '^';
   ptr = copy_and_rexec_escape (ptr, link, sub1);
   *(ptr ++) = '(';
   *(ptr ++) = '.';
   *(ptr ++) = '+';
   *(ptr ++) = ')';
   ptr = copy_and_rexec_escape (ptr, sub1 + sub1_len, sub2);
   *(ptr ++) = '(';
   *(ptr ++) = '.';
   *(ptr ++) = '+';
   *(ptr ++) = ')';
   ptr = copy_and_rexec_escape (ptr, sub2 + sub2_len, 0);
   *(ptr ++) = '$';
   *(ptr ++) = '\0';
}

void find_links (int recursion_level, MYSQL *mysql, const char *lang,
		 const char *rx, int base_item)
{
   // Check if the regex contains letters 
   const char *p;
   for (p = rx ; *p ; ++p)
     if (isalnum (*p) || (unsigned char) *p >= 128)
       break;
   if (!*p)
     return; // No letters found

   char rx_escaped[250];
   copy_and_mysql_escape (rx_escaped, rx);
   do_query (mysql,
	     "INSERT regexmerge_rx "
	     "VALUES (NULL, '%s', '%s', %d, 1) "
	     "ON DUPLICATE KEY UPDATE count = count + 1",
	     lang, rx_escaped, base_item);
   int affected_rows = mysql_affected_rows (mysql);
   if (affected_rows > 1)
     return; // Already searched this

   ++ regex_id;
   
   printf ("* Finding links with regex %s:%s\n",
	   lang, rx);
   fflush (stdout);

   regex_t preg;
   int regcomp_res = regcomp (&preg, rx, REG_EXTENDED);
   if (regcomp_res)
     {
	char errbuf[200];
	regerror (regcomp_res, &preg, errbuf, sizeof errbuf);
	printf ("regcomp() failed with code %d: %s\n", regcomp_res, errbuf);
	exit (1);
     }
   if (preg.re_nsub != 2)
     {
	printf ("regex should contain 2 parenthetical subexpressions.\n");
	exit (1);
     }
   MYSQL_RES *result = do_query_store
     (mysql,
	 "SELECT k_id, k_text "
	 "FROM link "
	 "WHERE k_lang = '%s' AND k_ns = 0",
	 lang);
   int regex_matches = 0;
   MYSQL_ROW row;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *link = row[1];

	regmatch_t pmatch[3];
	if (0 != regexec (&preg, link, 3, pmatch, 0))
	  continue; // Not a match
	++ regex_matches;
	char sub1[250];
	char sub2[250];
	size_t sub1_len = pmatch[1].rm_eo - pmatch[1].rm_so;
	size_t sub2_len = pmatch[2].rm_eo - pmatch[2].rm_so;
	copy (sub1, link + pmatch[1].rm_so, sub1_len);
	copy (sub2, link + pmatch[2].rm_so, sub2_len);

	char sub1_escaped[250];
	char sub2_escaped[250];
	copy_and_mysql_escape (sub1_escaped, sub1);
	copy_and_mysql_escape (sub2_escaped, sub2);
	do_query (mysql,
		  "INSERT IGNORE regexmerge_item "
		  "VALUES (%s, '%s', '%s', UNHEX(MD5('%s')), UNHEX(MD5('%s')), %d, '%s')",
		  item, sub1_escaped, sub2_escaped, sub1_escaped, sub2_escaped,
		  base_item, lang);
	if (recursion_level == 1)
	  {
	     // Find other links
	     MYSQL_RES *result2 = do_query_store
	       (mysql,
		   "SELECT k_lang, k_text "
		   "FROM link "
		   "WHERE k_id = %s AND k_lang != '%s'",
		   item, lang);
	     MYSQL_ROW row2;
	     while (NULL != (row2 = mysql_fetch_row (result2)))
	       {
		  const char *new_lang = row2[0];
		  const char *new_link = row2[1];

		  /* printf ("link=%s, sub1=%s, sub1_len=%zu, sub2=%s, sub2_len=%zu, "
		   "new_lang=%s, new_link=%s\n",
		   link, sub1, sub1_len, sub2, sub2_len, new_lang, new_link); */
		  // Check if the rexec subexpressions is also subexpressions here
		  char *new_sub1 = strstr (new_link, sub1);
		  if (! new_sub1)
		    continue;
		  char *new_sub2 = strstr (new_link, sub2);
		  if (! new_sub2)
		    continue;

		  // test for overlap
		  if (new_sub2 == new_sub1)
		    continue;
		  else if (new_sub2 > new_sub1 && new_sub2 < new_sub1 + sub1_len)
		    continue;
		  if (new_sub1 > new_sub2 && new_sub1 < new_sub2 + sub2_len)
		    continue;

		  // Make a new regex
		  char new_regex[1000];
		  if (new_sub1 < new_sub2)
		    make_regex (new_regex, new_link, new_sub1, sub1_len, new_sub2, sub2_len);
		  else
		    make_regex (new_regex, new_link, new_sub2, sub2_len, new_sub1, sub1_len);
		  find_links (recursion_level + 1, mysql, new_lang, new_regex, atoi (item));
	       }
	     mysql_free_result (result2);
	  }
     }
   mysql_free_result (result);
}
