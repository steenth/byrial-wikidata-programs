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
#include <time.h>

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

int print_lang_line (MYSQL *mysql, const char *lang1, const char *lang2,
		     const char *hash_escaped, int ns, int expected_count)
{
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT item, link, i_links, i_use "
	 "FROM commonsmerge_%s_%s "
	 "JOIN item ON item = i_id "
	 "WHERE hash = '%s' AND ns = %d",
	 lang1, lang2, hash_escaped, ns);

   MYSQL_ROW row;
   int found = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *link = row[1];
	int links = atoi (row[2]);
	int uses = atoi (row[3]);

	printf ("%s [[Q%s]] (",
		found ? "," : "**",
		item);
	print_link (lang1, ns, link);
	printf (", %d link%s",
		links, (links > 1) ? "s" : "");
	if (uses)
	  printf (", %d use%s",
		  uses, (uses > 1) ? "s" : "");
	printf (")");
	++ found;
     }
   if (expected_count != found)
     {
	printf ("Error: unexpected number of items found in print_lang_line, "
		"found = %d, expected = %d\n",
		found, expected_count);
	exit (1);
     }
   printf ("\n");
   mysql_free_result (result);
   return found;
}

void create_commonsmerge_lang_table (MYSQL *mysql, const char *lang1, const char *lang2)
{
   char table[40];
   sprintf (table, "commonsmerge_%s_%s", lang1, lang2); // Unsafe, but OK here
   if (table_exists (mysql, table))
     {
	return;
	// do_query (mysql, "DROP TABLE %s", table);
     }
   printf ("Finding items with {{P|373}} and link to %s but not %s ... ", lang1, lang2);
   fflush (stdout);
   time_t start = time (NULL);
 
   do_query (mysql,
	     "CREATE TABLE %s ("
	     "item INT NOT NULL,"
	     "ns SMALLINT NOT NULL,"
	     "link VARCHAR(256) NOT NULL,"
	     "hash BINARY(16) NOT NULL,"
	     "string VARCHAR(256) NOT NULL,"
	     "KEY (ns, hash),"
	     "KEY (item))"
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     table);

   do_query (mysql,
	     "INSERT %s "
	     "SELECT k1.k_id, k1.k_ns, k1.k_text, UNHEX(MD5(s_string_value)), s_string_value "
	     "FROM link k1 "
	     "JOIN statement "
	     "  ON k1.k_id = s_item "
	     "LEFT JOIN link k2 "
	     "  ON k2.k_id = k1.k_id AND k2.k_lang = '%s' "
	     "WHERE s_property = 373 AND s_datatype = 'string' "
	     "  AND k1.k_lang = '%s' AND k2.k_id IS NULL",
	     table,
	     lang2,
	     lang1);
   printf ("done in %ld seconds.<br/ >\n", time (NULL) - start);
   fflush (stdout);
}

int main (int argc, char *argv[])
{
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
   print_dumpdate (mysql);

   printf ("== Merge candicates based on same Commons category (%s-%s) ==\n",
	   lang1, lang2);

   create_commonsmerge_lang_table (mysql, lang1, lang2);
   create_commonsmerge_lang_table (mysql, lang2, lang1);

   time_t start = time (NULL);
   bool in_categories = false;
   
   printf ("\n=== Items for articles ===\n\n");

   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT n1.hash, n1.ns, n1.string, "
	 "  (SELECT COUNT(*) FROM commonsmerge_%s_%s where hash = n1.hash AND ns = n1.ns), "
	 "  (SELECT COUNT(*) FROM commonsmerge_%s_%s where hash = n1.hash AND ns = n1.ns) "
	 "FROM commonsmerge_%s_%s n1 "
	 "JOIN commonsmerge_%s_%s n2 "
	 "  ON n1.hash = n2.hash AND n1.ns = n2.ns "
	 "GROUP BY n1.ns, n1.hash",
	 lang1, lang2,
	 lang2, lang1,
	 lang1, lang2,
	 lang2, lang1);
   MYSQL_ROW row;
   int commons_cats = 0;
   int skipped_shared_lang = 0;
   int found_lang1 = 0;
   int found_lang2 = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *hash = row[0];
	int ns = atoi (row[1]);
	const char *string = row[2];
	int count1 = atoi (row[3]);
	int count2 = atoi (row[4]);

	char hash_escaped[32];
	copy_and_escape_hash (hash_escaped, hash);

	// Check for links to the same language
	MYSQL_RES *result2 = do_query_use
	  (mysql2,
	      "SELECT k1.k_lang "
	      "FROM commonsmerge_%s_%s n1 "
	      "JOIN link k1 "
	      "  ON k1.k_id = n1.item "
	      "JOIN commonsmerge_%s_%s n2 "
	      "  ON n2.hash = n1.hash AND n2.ns = n1.ns "
	      "JOIN link k2 "
	      "  ON k2.k_id = n2.item "
	      "WHERE n1.hash = '%s' AND n1.ns = %d"
	      "  AND k1.k_lang = k2.k_lang "
	      "LIMIT 1",
	      lang1, lang2,
	      lang2, lang1,
	      hash_escaped, ns);
	MYSQL_ROW row2 = mysql_fetch_row (result2);
	if (row2)
	  {
	     // A row is fetched - there is a language conclict
	     ++ skipped_shared_lang;
	     row2 = mysql_fetch_row (result2);
	     if (row2)
	       {
		  printf ("Fetched unexpected row.\n");
		  exit (1);
	       }
	     mysql_free_result (result2);
	     continue;
	  }
	mysql_free_result (result2);

	++ commons_cats;

	if (ns && ! in_categories)
	  {
	     printf ("\n=== Items for categories ===\n\n");
	     in_categories = true;
	  }

	printf ("* %s:\n", string);
	found_lang1 += print_lang_line (mysql2, lang1, lang2, hash_escaped, ns, count1);
	found_lang2 += print_lang_line (mysql2, lang2, lang1, hash_escaped, ns, count2);
	fflush (stdout);
     }
   printf ("Found %d shared commons categories (skipped %d with links to a shared language), "
	   "%d %s links, and %d %s links in %ld seconds.\n",
	   commons_cats, skipped_shared_lang, found_lang1, lang1,
	   found_lang2, lang2, time (NULL) - start);
   mysql_free_result (result);
   close_database (mysql);
   close_database (mysql2);
   return 0;
}
