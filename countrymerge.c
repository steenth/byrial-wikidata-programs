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

/*
 * This program uses ahocorasick, an LGPL implementation of the Aho-Corasick 
 * algorithm as a C library by Kamiar Kanani.
 * Get it from http://multifast.sourceforge.net/
 */

// For strdup()
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include "wikidatalib.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <time.h>
#include <locale.h>
#include "ahocorasick.h"

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

struct match_handler_param 
{
   const char *item;
   const char *ns;
   const char *link;
   MYSQL *mysql;
};

static char table[32];
static int matches = 0;

int match_handler (AC_MATCH_t *m, void *vp)
{
   struct match_handler_param *param = vp;
   // Use the longest match which will be first.
   /*
   if (m->match_num != 1)
     {
	printf ("Found %d matches at once for Q%s. Will only handle one.\n",
		m->match_num, param->item);
	for (unsigned int i = 0 ; i < m->match_num ; i++)
	  printf ("* Match %d = %s\n", i, m->patterns[i].astring);
     }
   */
   char pattern[1024];
   char *p = pattern;
   p = copy_and_escape2 (p, param->ns);
   *(p ++) = ':';
   p = copy_and_escape (p, param->link, 0, m->position - m->patterns[0].length);
   *(p ++) = '#';
   copy_and_escape2 (p, param->link + m->position);

   char link_escaped[1024];
   copy_and_escape2 (link_escaped, param->link);

   char country_escaped[1024];
   copy_and_escape2 (country_escaped, m->patterns[0].astring);

   //printf ("link = %s, pattern = \"%s\", country = %s, position = %ld, len = %d\n",
   //	   param->link, pattern, m->patterns[0].astring, m->position, m->patterns[0].length);
   
   do_query (param->mysql,
	     "INSERT `%s` "
	     "VALUES (%s, %s, '%s', UNHEX(MD5('%s')), %lu, '%s') "
	     "ON DUPLICATE KEY UPDATE "
	     "  hash=UNHEX(MD5('%s')),"
	     "  country=%lu,"
	     "  country_text='%s'",
	     table,
	     param->item, param->ns, link_escaped,
	     pattern, m->patterns[0].rep.number, country_escaped,
	     pattern, m->patterns[0].rep.number, country_escaped);
   ++ matches;
   return 0; // continue searching - get the last match for cases some search strings
   // are prefixes of other searchstrings
}

void create_countrymerge_lang_table (MYSQL *mysql, MYSQL *mysql2, const char *lang)
{
   sprintf (table, "countrymerge_%s", lang); // Unsafe, but OK here
   if (table_exists (mysql, table))
     return;
   int patterns = 0;
   printf ("Finding links with country names to language %s ... ", lang);
   fflush (stdout);
   time_t start = time (NULL);

   AC_AUTOMATA_t *acap = ac_automata_init (match_handler);
   
   // Find the search strings
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT s_item, "
	 "  (SELECT k_text FROM link "
	 "   WHERE k_id = s_item AND k_lang = '%s' and k_project = 'w') "
	 "FROM statement "
	 "WHERE s_property = 31 AND s_item_value = 3624078 "
	 // p31=instance of, q3624078=sovereign state

	 "UNION "
	 "SELECT k_id, k_text "
	 "FROM link "
	 "WHERE k_lang = '%s' AND k_project = 'w' AND "
	 "  k_id IN (15,18,46,48,49,51,538, "
	 // q15=Africa, q18=South America, q46=Europe, q48=Asia,
	 // q49=North America, q51=Antarctica, q538=Oceania
	 "  16957,713750,180573,172640,36704,33946,28513) "
	 // q16957=East Germany, q713750=West Germany, q180573=South Vietnam
	 // q172640=North Vietnam, q36704=Yugoslavia, q33946=Czechoslovakia
	 // q28513=Austria-Hungary
   
	 "UNION "
	 "SELECT s_item_value, "
	 "  (SELECT l_text FROM label WHERE l_id = s_item_value AND l_lang = '%s') "
	 "FROM statement "
	 "WHERE s_property = 150 " // p150=subdivides into (administrative)
	 "  AND s_item IN (16,20,30,31,35,38,39,40,43,96,145,155,159,183,668,878,29999) "
	 // q16=Canada, q20=Norway, q30=USA, q31=Belgium, q35=Denmark, q38=Italy,
	 // q39=Switzerland, q40=Austria, q43=Turkey, q96=Mexico, q145=UK,
	 // q155=Brazil, q159=Russia, q183=Germany, q668=India, q878=UEA,
	 // q29999=Kingdom of the Netherlands
	 "  AND s_item_value != 1428 " // q1428=Georgia (US state)
	 
	 "UNION "
	 "SELECT country, word "
	 "FROM country_adjective_approved "
	 "WHERE lang = '%s' "

	 , lang, lang, lang, lang);
   
   MYSQL_ROW row;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int item = atoi (row[0]);
	const char *link = row[1];
	if (link == NULL)
	  continue;
	AC_PATTERN_t ac_pattern;
	ac_pattern.astring = strdup (link);
	ac_pattern.length = strlen (link);
	ac_pattern.rep.number = item;
	AC_ERROR_t ret = ac_automata_add (acap, &ac_pattern);
	if (ret == ACERR_SUCCESS)
	  ++ patterns;
     }
   mysql_free_result (result);
   ac_automata_finalize (acap);

   // Create table
   do_query (mysql,
	     "CREATE TABLE `%s` ("
	     "item INT NOT NULL,"
	     "ns SMALLINT NOT NULL,"
	     "link VARCHAR(256) NOT NULL,"
	     "hash BINARY(16) NOT NULL,"
	     "country INT UNSIGNED NOT NULL,"
	     "country_text VARCHAR(128) NOT NULL,"
	     "KEY (hash),"
	     "UNIQUE KEY (item))"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     table);

   // Find links to lang with countries
   result = do_query_store
     (mysql,
	 "SELECT k_id, k_ns, k_text "
	 "FROM link "
	 "WHERE k_lang = '%s' AND k_project = 'w'",
	 lang);

   int links = 0;
   matches = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	++ links;
	const char *item = row[0];
	const char *ns = row[1];
	const char *link = row[2];

	// Check if the link has a country as substring
	AC_TEXT_t ac_text;
	// ac_text.astring = link;
	//   libahocorasick will not modify the string, so it should have used const.
	ac_text.astring = (char *) link;
        ac_text.length = strlen (link);
	struct match_handler_param param;
	param.item = item;
	param.ns = ns;
	param.link = link;
	param.mysql = mysql2;
	ac_automata_search (acap, &ac_text, &param);
	ac_automata_reset (acap);
     }
   printf ("searched %d links for %d patterns, found %d matches in %ld seconds.\n\n",
	   links, patterns, matches, time (NULL) - start);
   ac_automata_release (acap);
   mysql_free_result (result);
}

void print_same_country_cases (MYSQL *mysql, const char *lang1, const char *lang2,
			       const char *hash1_escaped, int same)
{
   const int limit = 5;
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT n1.item, n1.country_text "
	 "FROM `countrymerge_%s` n1 "
	 "JOIN `countrymerge_%s` n2 "
	 "  ON n1.item = n2.item "
	 "WHERE n1.hash = '%s' AND n1.country = n2.country "
	 // "ORDER BY n1.country "
	 "LIMIT %d",
	 lang1, lang2, hash1_escaped, limit);
   MYSQL_ROW row;
   int found = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *country_text = row[1];
	if (found)
	  printf (", ");
	printf ("[[Q%s|%s]]", item, country_text);
	++ found;
     }
   mysql_free_result (result);
   if (same > limit)
     printf (" ...");
   if (found < same && found < limit)
     printf (", '''Error:''' Found only %d countries, expected to find %d",
	     found, same);
}

int print_notsame_country_cases (MYSQL *mysql, const char *lang1, const char *lang2,
				 const char *hash1_escaped, const char *hash2_escaped,
				 int same, int notsame)
{
   MYSQL_RES *result = do_query_store
     (mysql,
	 "SELECT n1.item, n1.ns, n1.link, n2.ns, n2.link, "
	 "  n1.country_text, n2.country_text, "
	 "  (SELECT partof "
	 "   FROM countrymerge_partof "
	 "   WHERE country = n1.country), "
	 "  (SELECT partof "
	 "   FROM countrymerge_partof "
	 "   WHERE country = n2.country) "
	 "FROM `countrymerge_%s` n1 "
	 "JOIN `countrymerge_%s` n2 "
	 "  ON n1.country != n2.country AND n1.item = n2.item "
	 "WHERE n1.hash = '%s' AND n2.hash = '%s'",
	 lang1, lang2, hash1_escaped, hash2_escaped);
   MYSQL_ROW row;
   int printed = 0;
   int suppressed = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	int ns1 = atoi (row[1]);
	const char *link1 = row[2];
	int ns2 = atoi (row[3]);
	const char *link2 = row[4];
	const char *country1 = row[5];
	const char *country2 = row[6];
	int country1_partof = atoi (row[7]);
	int country2_partof = atoi (row[8]);

	if (country1_partof == country2_partof)
	  {
	     ++ suppressed;
	     continue;
	  }
	
	if (printed == 0)
	  {
	     printf ("* Pattern found %d time%s with same countries (",
		     same, same > 1 ? "s" : "");
	     print_same_country_cases (mysql, lang1, lang2, hash1_escaped, same);
	     printf (") and %d time%s with different countries:\n",
		     notsame, notsame > 1 ? "s" : "");
	  }
	
	printf ("** '''%s ≠ %s''': [[Q%s]] links to ",
		country1, country2, item);
	print_link (lang1, ns1, link1);
	printf (" and ");
	print_link (lang2, ns2, link2);
	printf ("\n");
	fflush (stdout);
	++ printed;
     }
   if (printed + suppressed != notsame)
     printf ("** '''Program error:''' Found %d cases with different countries, "
	     "expected to find %d cases.\n",
	     printed + suppressed, notsame);
   mysql_free_result (result);
   return printed;
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
   print_dumpdate (mysql);

   create_countrymerge_lang_table (mysql, mysql2, lang1);
   create_countrymerge_lang_table (mysql, mysql2, lang2);

   printf ("Finding links with countries from %s to %s ... ", lang1, lang2);
   time_t start = time (NULL);
   
   do_query (mysql,
	     "DROP TABLE IF EXISTS `countrymerge_%s_%s`",
	     lang1, lang2);
   do_query (mysql,
	     "CREATE TABLE `countrymerge_%s_%s` ("
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
	 "SELECT n1.hash, n1.country, n2.hash, n2.country "
	 "FROM `countrymerge_%s` n1 "
	 "LEFT JOIN `countrymerge_%s` n2 "
	 "  ON n1.item = n2.item",
	 lang1, lang2);
   MYSQL_ROW row;
   int found = 0;
   int not_found = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *hash1 = row[0];
	const char *country1 = row[1];
	const char *hash2 = row[2];
	const char *country2 = row[3];
	
	char hash1_escaped[32];
	copy_and_escape_hash (hash1_escaped, hash1);

	if (hash2)
	  {
	     ++ found;
	     char hash2_escaped[32];
	     copy_and_escape_hash (hash2_escaped, hash2);
	     bool same = strcmp (country1, country2) == 0;
	     // Update countrymerge_lang1_lang2
	     do_query
	       (mysql2,
		   "INSERT `countrymerge_%s_%s` "
		   "VALUES ('%s', '%s', %d, %d)"
		   "ON DUPLICATE KEY UPDATE %s = %s + 1",
		   lang1, lang2,
		   hash1_escaped, hash2_escaped, same, ! same,
		   same ? "same" : "notsame", same ? "same" : "notsame");
	  }
	else
	  {
	     ++ not_found;
	  }
     }
   mysql_free_result (result);
   printf ("found %d pages in %s with link to %s "
	   "and %d pages in %s without link %s in %ld seconds.\n\n",
	   found, lang1, lang2,
	   not_found, lang1, lang2, time (NULL) - start);

   printf ("== Merge candidates in %s: and %s: based on common patterns with countries ==\n",
	  lang1, lang2);
   
   result = do_query_use
     (mysql,
	 "SELECT hash1, hash2, COUNT(*), same, notsame "
	 "FROM `countrymerge_%s_%s` "
	 "GROUP BY hash1 "
	 "ORDER BY hash1",
	 lang1, lang2);
   int more_patterns = 0;
   int always_different_country = 0;
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
	     ++ always_different_country;
	     continue;
	  }
	++ checked;

	char hash1_escaped[16];
	copy_and_escape_hash (hash1_escaped, hash1);
	char hash2_escaped[16];
	copy_and_escape_hash (hash2_escaped, hash2);

	if (notsame)
	  {
	     notsame = print_notsame_country_cases (mysql2, lang1, lang2, hash1_escaped,
						    hash2_escaped, same, notsame);
	  }
	
	// Find merge candidates
	MYSQL_RES *result2 = do_query_use
	  (mysql2,
	      "SELECT n1.item, n1.ns, n1.link, n2.item, n2.ns, n2.link "
	      "FROM `countrymerge_%s` n1 "
	      "JOIN `countrymerge_%s` n2 "
	      "  ON n1.country = n2.country AND n1.item != n2.item "
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
		  printf ("* Pattern found %d time%s (with country ", same, same > 1 ? "s" : "");
		  
		  print_same_country_cases (mysql3, lang1, lang2, hash1_escaped, same);
		  printf ("):\n");
	       }
	     ++ pattern_count;

	     // Check if the items is used and have links for the same langauge
	     MYSQL_RES *result3 = do_query_use
	       (mysql3,
		   "SELECT COUNT(DISTINCT k_lang, k_project), "
		   " (SELECT i_links FROM item WHERE i_id = %s), "
		   " (SELECT i_links FROM item WHERE i_id = %s), "
		   " (SELECT i_use FROM item WHERE i_id = %s), "
		   " (SELECT i_use FROM item WHERE i_id = %s) "
		   "FROM link "
		   "WHERE k_id IN (%s, %s)",
		   item1, item2,
		   item1, item2,
		   item1, item2);
	     MYSQL_ROW row3 = mysql_fetch_row (result3);
	     int distinct_links = atoi (row3[0]);
	     int links1 = atoi (row3[1]);
	     int links2 = atoi (row3[2]);
	     int use1 = atoi (row3[3]);
	     int use2 = atoi (row3[4]);
	     row3 = mysql_fetch_row (result3);
	     if (row3)
	       {
		  printf ("Unexpected row fetched from MySQL.\n");
		  return 1;
	       }		  
	     mysql_free_result (result3);

	     printf ("** [[Q%s]] (", item1);
	     print_link (lang1, ns1, link1);
	     printf (", %d link%s", 
		     links1, links1 > 1 ? "s" : "");
	     if (use1)
	       printf (", %d use%s", use1, use1 > 1 ? "s" : "");
	     printf ("), [[Q%s]] (", item2);
	     print_link (lang2, ns2, link2);
	     printf (", %d link%s", links2, links2 > 1 ? "s" : "");
	     if (use2)
	       printf (", %d use%s", use2, use2 > 1 ? "s" : "");
	     printf (")");
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
   printf (" %d dropped due to use of different countries)\n", always_different_country);
   close_database (mysql);
   close_database (mysql2);
   close_database (mysql3);
   return 0;
}
