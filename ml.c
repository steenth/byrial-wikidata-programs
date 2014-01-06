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

int main (int argc, char *argv[])
{
   const char *database;
   if (argc < 2)
     database = "wikidatawiki";
   else
     database = argv[1];

   MYSQL *mysql = open_named_database (database);
   MYSQL_ROW row;
   
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT i_id, k_text, k_ns, "
	 "  (SELECT l_text FROM label WHERE i_id = l_id AND l_lang = 'ml'), "
	 "  (SELECT d_text FROM descr WHERE i_id = d_id AND d_lang = 'ml'), "
	 "  (SELECT l_text FROM label WHERE i_id = l_id AND l_lang = 'en'), "
	 "  (SELECT d_text FROM descr WHERE i_id = d_id AND d_lang = 'en') "
	 "FROM link "
	 "JOIN item "
	 "ON i_id = k_id "
	 "WHERE i_links = 1 AND k_lang = 'ml' AND k_ns != 14 "
	 "LIMIT 1000000");

   printf ("== Items with only Malayalam link ==\n\n");
   printf ("Links to category pages excluded.\n");
   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Link\n");
   printf ("! Label and description\n");
   printf ("! English label and description\n");
   int count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *link = row[1];
	int ns = atoi (row[2]);
	const char *lml = row[3];
	const char *dml = row[4];
	const char *len = row[5];
	const char *den = row[6];

	printf ("|-\n");
	printf ("|[[Q%s]]||", item);
	print_link ("ml", ns, link);
	printf ("||%s, %s||%s, %s\n",
		(lml && strcmp (lml, link)) ? lml : "",
		dml ? dml : "",
		len ? len : "",
		den ? den : "");
	++ count;
     }
   printf ("|}\n");
   printf ("Total %d\n", count);
   mysql_free_result (result);

   result = do_query_use
     (mysql,
	 "SELECT i_id, k_text "
	 "FROM link "
	 "JOIN item "
	 "ON i_id = k_id "
	 "WHERE i_links = 1 AND k_lang = 'ml' AND k_ns = 14 "
	 "LIMIT 1000000");

   printf ("== Items with only link to Malayalam category pages ==\n\n");
   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Link\n");
   int count_cat = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *link = row[1];

	printf ("|-\n");
	printf ("|[[Q%s]]||", item);
	print_link ("ml", 14, link);
	printf ("\n");
	++ count_cat;
     }
   printf ("|}\n");
   printf ("Total %d\n", count_cat);
   mysql_free_result (result);

   close_database (mysql);
   return 0;
}
