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

#include <stdio.h>
#include "wikidatalib.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <time.h>
#include <locale.h>

char *copy_and_escape (char *dest, const char *src, int so, int eo)
{
   for (int o = so ; o < eo ; ++o)
     {
	if (src[o] == '\\' || src[o] == '\'')
	  *(dest ++) = '\\';
	*(dest ++) = src[o];
     }
   *dest = '\0';
   return dest;
}

char *copy_and_escape2 (char *dest, const char *src)
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
   return dest;
}

char *copy_and_escape_hash (char *dest, const char *src)
{
   for (int i = 16 ; i ; -- i)
     {
	if (*src == '\0')
	  {
	     *(dest ++) = '\\';
	     *(dest ++) = '0';
	     ++ src;
	  }
	else
	  {
	     if (*src == '\\' || *src == '\'')
	       {
		  *(dest ++) = '\\';
	       }
	     *(dest ++) = *(src ++);
	  }
     }
   *dest = '\0';
   return dest;
}

void create_numbermerge_lang_table (MYSQL *mysql, MYSQL *mysql2, const char *lang)
{
   char table[32];
   sprintf (table, "numbermerge_%s", lang); // Unsafe, but OK here
   if (table_exists (mysql, table))
     return;
   printf ("Finding links with numbers to language %s ... ", lang);
   fflush (stdout);
   time_t start = time (NULL);

   char *regex;
   if (strcmp (lang, "hi") == 0)
//     regex = "^([^0-9०-९]*)([0-9०-९]+)([^0-9०-९]*)$";
     regex = "^([^0-9०१२३४५६७८९]*)([0-9०१२३४५६७८९]+)([^0-9०१२३४५६७८९]*)$";
   else 
     regex = "^([^0-9]*)([0-9]+)([^0-9]*)$";
   regex_t preg;
   int regcomp_res = regcomp (& preg, regex, REG_EXTENDED);
   if (regcomp_res)
     {
	char errbuf[200];
	regerror (regcomp_res, & preg, errbuf, sizeof errbuf);	
	printf ("regcomp() failed with code %d: %s\n", regcomp_res, errbuf);
	exit (1);
     }
   if (preg.re_nsub != 3)
     {
	printf ("re_nsub not as expected after regcomp()\n");
	exit (1);
     }

   do_query (mysql,
	     "CREATE TABLE %s ("
	     "item INT NOT NULL,"
	     "ns SMALLINT NOT NULL,"
	     "link VARCHAR(256) NOT NULL,"
	     "hash BINARY(16) NOT NULL,"
	     "number INT UNSIGNED NOT NULL,"
	     "KEY (hash),"
	     "KEY (item))"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     table);

   // Find links from lang1 to lang2 with numbers
   MYSQL_RES *result = do_query_use
     (mysql,
	 // "/* __PRINT_QUERY__ */"
	 "SELECT k_id, k_ns, k_text "
	 "FROM link "
	 "WHERE k_lang = '%s' ",
	 lang);

   int links = 0;
   int regex_matches = 0;
   int leading_zeroes = 0;
   MYSQL_ROW row;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	++ links;
	const char *item = row[0];
	const char *ns = row[1];
	const char *link = row[2];

	// Check if the link match the regex
	regmatch_t pmatch[4];
	int regexec_res = regexec (& preg, link, 4, pmatch, 0);
	if (regexec_res)
	  continue;
	if (pmatch[1].rm_so)
	  {
	     printf ("Error: start offset of first matched subexpression not 0.\n");
	     exit (1);
	  }
	++ regex_matches;

	char pattern[1024];
	char *p = pattern;
	p = copy_and_escape2 (p, ns);
	p = copy_and_escape (p, link, pmatch[1].rm_so, pmatch[1].rm_eo);
	*(p ++) = '#';
	copy_and_escape (p, link, pmatch[3].rm_so, pmatch[3].rm_eo);

	char link_escaped[1024];
	copy_and_escape2 (link_escaped, link);

	const char *digits = link + pmatch[2].rm_so;
	const int digits_len = pmatch[2].rm_eo - pmatch[2].rm_so;
	if (digits[0] == '0' && digits_len > 1)
	  {
	     ++ leading_zeroes;
	     continue;
	  }

	long unsigned number = 0;
	if (strcmp (lang, "hi") == 0)
	  {
	     while (1)
	       {
		  if (*digits >= '0' && *digits <= '9')
		    {
		       number = number * 10 + (*digits - '0');
		       ++ digits;
		    }
		  else if (digits[0] == (char) 0xe0 && digits[1] == (char) 0xa5 &&
			   digits[2] >= (char) 0xa6 && digits[2] <= (char) 0xaf)
		    {
		       number = number * 10 + (digits[2] - (char) 0xa6);
		       digits += 3;
		    }
		  else
		    break;
	       }	     
	  }
	else
	  number = strtoul (digits, NULL, 10);
	// printf ("Link: %s, pattern: %s, number: %lu\n", link, pattern, number);

	do_query (mysql2,
		  "INSERT %s "
		  "VALUES (%s, %s, '%s', UNHEX(MD5('%s')), %lu)",
		  table,
		  item, ns, link_escaped, pattern, number);
     }
   printf ("found %d links, %d with one number (%d with leading zero dropped) "
	   "in %ld seconds.\n\n",
	   links, regex_matches, leading_zeroes, time (NULL) - start);
   mysql_free_result (result);
}

void print_same_number_cases (MYSQL *mysql, const char *lang1, const char *lang2,
			      const char *hash1_escaped, int same)
{
   const int limit = 5;
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT n1.item, n1.number "
	 "FROM numbermerge_%s n1 "
	 "JOIN numbermerge_%s n2 "
	 "  ON n1.item = n2.item "
	 "WHERE n1.hash = '%s' AND n1.number = n2.number "
	 "ORDER BY n1.number "
	 "LIMIT %d",
	 lang1, lang2, hash1_escaped, limit);
   MYSQL_ROW row;
   int found = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *number = row[1];
	if (found)
	  printf (", ");
	printf ("[[Q%s|%s]]", item, number);
	++ found;
     }
   mysql_free_result (result);
   if (same > limit)
     printf (" ...");
   if (found < same && found < limit)
     printf (", '''Error:''' Found only %d numbers, expected to find %d",
	     found, same);
}

int main (int argc, char *argv[])
{

   setlocale (LC_CTYPE, "");

   const char *database;
   if (argc < 3)
     {
	printf ("Usage: %s lang1 lang2 [database]\n", argv[0]);
	return 1;
     }
   const char *lang1 = argv[1];
   const char *lang2 = argv[2];

   if (argc < 4)
     database = "wikidatawiki";
   else
     database = argv[3];

   MYSQL *mysql = open_named_database (database);
   MYSQL *mysql2 = open_named_database (database);
   MYSQL *mysql3 = open_named_database (database);

   create_numbermerge_lang_table (mysql, mysql2, lang1);
   create_numbermerge_lang_table (mysql, mysql2, lang2);

   printf ("Find links with number from %s to %s ... ", lang1, lang2);
   time_t start = time (NULL);
   
   do_query (mysql,
	     "DROP TABLE IF EXISTS numbermerge_%s_%s",
	     lang1, lang2);
   do_query (mysql,
	     "CREATE TABLE numbermerge_%s_%s ("
	     "hash1 BINARY(16) NOT NULL,"
	     "hash2 BINARY(16) NOT NULL,"
	     "same MEDIUMINT UNSIGNED,"
	     "notsame MEDIUMINT UNSIGNED,"
	     "UNIQUE KEY (hash1, hash2)) "
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     lang1, lang2);

   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT n1.hash, n1.number, n2.hash, n2.number "
	 "FROM numbermerge_%s n1 "
	 "LEFT JOIN numbermerge_%s n2 "
	 "  ON n1.item = n2.item",
	 lang1, lang2);
   MYSQL_ROW row;
   int found = 0;
   int not_found = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *hash1 = row[0];
	const char *number1 = row[1];
	const char *hash2 = row[2];
	const char *number2 = row[3];
	
	char hash1_escaped[32];
	copy_and_escape_hash (hash1_escaped, hash1);

	if (hash2)
	  {
	     ++ found;
	     char hash2_escaped[32];
	     copy_and_escape_hash (hash2_escaped, hash2);
	     bool same = strcmp (number1, number2) == 0;
	     // Update numbermerge_lang1_lang2
	     do_query
	       (mysql2,
		   "INSERT numbermerge_%s_%s "
		   "VALUES ('%s', '%s', %d, %d)"
		   "ON DUPLICATE KEY UPDATE %s = %s + 1",
		   lang1, lang2,
		   hash1_escaped, hash2_escaped, same, ! same,
		   same ? "same" : "notsame", same ? "same" : "notsame");
	  }
	else
	  {
	     ++ not_found;
	     /*
	     do_query
	       (mysql2,
		   "INSERT numbermerge_%s_%s "
		   "VALUES ('%s', '', 0, 1)"
		   "ON DUPLICATE KEY UPDATE notsame = notsame + 1",
		   lang1, lang2, hash1_escaped);
	      */
	  }
     }
   mysql_free_result (result);
   printf ("found %d pages in %s with link to %s "
	   "and %d pages in %s without link %s in %ld seconds.\n\n",
	   found, lang1, lang2,
	   not_found, lang1, lang2, time (NULL) - start);

   printf ("== Merge candidates in %s: and %s: based on common patterns with numbers ==\n",
	  lang1, lang2);
   
   result = do_query_use
     (mysql,
	 "SELECT hash1, hash2, COUNT(*), same, notsame "
	 "FROM numbermerge_%s_%s "
	 "GROUP BY hash1 "
	 "ORDER BY hash1",
	 lang1, lang2);
   int more_patterns = 0;
   int always_different_number = 0;
   int checked = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *hash1 = row[0];
	const char *hash2 = row[1];
	int count = atoi (row[2]);
	int same = atoi (row[3]);
	int notsame = atoi (row[4]);
	// printf ("count=%d, same=%d, notsame=%d\n", count, same, notsame);
	if (count > 1)
	  {
	     ++ more_patterns;
	     continue;
	  }
	if (same == 0 && notsame > 0)
	  {
	     ++ always_different_number;
	     continue;
	  }
	++ checked;

	char hash1_escaped[16];
	copy_and_escape_hash (hash1_escaped, hash1);
	char hash2_escaped[16];
	copy_and_escape_hash (hash2_escaped, hash2);

	if (notsame)
	  {
	     printf ("* Pattern found %d time%s with same numbers (",
		     same, same > 1 ? "s" : "");
	     print_same_number_cases (mysql2, lang1, lang2, hash1_escaped, same);
	     printf (") and %d time%s with different numbers:\n",
		     notsame, notsame > 1 ? "s" : "");

	     // Find the not same cases
	     MYSQL_RES *result2 = do_query_use
	       (mysql2,
		   "SELECT n1.item, n1.ns, n1.link, n2.ns, n2.link, n1.number, n2.number "
		   "FROM numbermerge_%s n1 "
		   "JOIN numbermerge_%s n2 "
		   "  ON n1.number != n2.number AND n1.item = n2.item "
		   "WHERE n1.hash = '%s' AND n2.hash = '%s'",
		   lang1, lang2, hash1_escaped, hash2_escaped);
	     MYSQL_ROW row2;
	     int found = 0;
	     while (NULL != (row2 = mysql_fetch_row (result2)))
	       {
		  ++ found;
		  const char *item = row2[0];
		  int ns1 = atoi (row2[1]);
		  const char *link1 = row2[2];
		  int ns2 = atoi (row2[3]);
		  const char *link2 = row2[4];
		  const char *number1 = row2[5];
		  const char *number2 = row2[6];

		  printf ("** '''%s ≠ %s''': [[Q%s]] links to ",
			  number1, number2, item);
		  print_link (lang1, ns1, link1);
		  printf (" and ");
		  print_link (lang2, ns2, link2);
		  printf ("\n");
		  fflush (stdout);
	       }
	     if (found < notsame)
	       printf ("** '''Error:''' Found only %d cases with different numbers, "
		       "expected to find %d cases.\n",
		       found, notsame);
	     mysql_free_result (result2);
	  }
	
	// Find merge candidates
	MYSQL_RES *result2 = do_query_use
	  (mysql2,
	      "SELECT n1.item, n1.ns, n1.link, n2.item, n2.ns, n2.link "
	      "FROM numbermerge_%s n1 "
	      "JOIN numbermerge_%s n2 "
	      "  ON n1.number = n2.number AND n1.item != n2.item "
	      "WHERE n1.hash = '%s' AND n2.hash = '%s'",
	      lang1, lang2, hash1_escaped, hash2_escaped);
	MYSQL_ROW row2;
	int pattern_count = 0;
	while (NULL != (row2 = mysql_fetch_row (result2)))
	  {	
	     const char *item1 = row2[0];
	     int ns1 = atoi (row2[1]);
	     const char *link1 = row2[2];
	     const char *item2 = row2[3];
	     int ns2 = atoi (row2[4]);
	     const char *link2 = row2[5];

	     if (pattern_count == 0 && notsame == 0)
	       {
		  printf ("* Pattern found %d time%s (with number ", same, same > 1 ? "s" : "");
		  
		  print_same_number_cases (mysql3, lang1, lang2, hash1_escaped, same);
		  printf ("):\n");
	       }
	     ++ pattern_count;

	     // Check if the items have links for the same langauge
	     MYSQL_RES *result3 = do_query_use
	       (mysql3,
		   "SELECT COUNT(DISTINCT k_lang), "
		   " (SELECT i_links FROM item WHERE i_id = %s), "
		   " (SELECT i_links FROM item WHERE i_id = %s) "
		   "FROM link "
		   "WHERE k_id IN (%s, %s)",
		   item1, item2,
		   item1, item2);
	     MYSQL_ROW row3 = mysql_fetch_row (result3);
	     int distinct_links = atoi (row3[0]);
	     int links1 = atoi (row3[1]);
	     int links2 = atoi (row3[2]);
	     row3 = mysql_fetch_row (result3);
	     if (row3)
	       {
		  printf ("Unexpected row fetched from MySQL.\n");
		  return 1;
	       }		  
	     mysql_free_result (result3);

	     printf ("** [[Q%s]] (", item1);
	     print_link (lang1, ns1, link1);
	     printf (", %d link%s), [[Q%s]] (",
		     links1, links1 > 1 ? "s" : "", item2);
	     print_link (lang2, ns2, link2);
	     printf (", %d link%s)",
		     links2, links2 > 1 ? "s" : "");
	     if (links1 + links2 > distinct_links)
	       printf (", '''%d link%s to same language%s'''",
		       links1 + links2 - distinct_links,
		       (links1 + links2 - distinct_links) > 1 ? "s" : "",
		       (links1 + links2 - distinct_links) > 1 ? "s" : "");
	     printf ("\n");
	     fflush (stdout);
	  }
	mysql_free_result (result2);

     }
   mysql_free_result (result);

   printf ("Checked %d patterns in %s in %ld seconds", checked, lang1, time (NULL) - start);
   printf (" (%d dropped due to links to more than one pattern in %s,", more_patterns, lang2);
   printf (" %d dropped due to use of different numbers)\n", always_different_number);
   close_database (mysql);
   close_database (mysql2);
   close_database (mysql3);
   return 0;
}
