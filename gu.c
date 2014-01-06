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
	 "  (SELECT l_text FROM label WHERE l_id = i_id AND l_lang = 'gu'), "
	 "  (SELECT d_text FROM descr WHERE d_id = i_id AND d_lang = 'gu'), "
	 "  (SELECT l_text FROM label WHERE l_id = i_id AND l_lang = 'en'), "
	 "  d_text "
	 "FROM link "
	 "JOIN item "
	 "  ON k_id = i_id "
	 "LEFT JOIN descr "
	 "  ON d_id = i_id AND d_lang = 'en' "
	 "WHERE k_lang = 'gu' AND i_links = 1 "
	 "  AND (d_text IS NULL OR "
	 "       d_text != 'village in Gujarat state, India') "
	 "limit 1000000");

   printf ("== Items with only Gujarati link ==\n\n");
   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Gujarati link\n");
   printf ("! Gujarati label and description\n");
   printf ("! English label and description\n");
   int count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *link = row[1];
	int ns = atoi (row[2]);
	const char *lgu = row[3];
	const char *dgu = row[4];
	const char *len = row[5];
	const char *den = row[6];
	printf ("|-\n");
	printf ("|[[Q%s]]||", item);
	print_link ("gu", ns, link);
	printf ("||%s, %s||%s, %s\n",
		(lgu && strcmp (lgu, link)) ? lgu : "",
		dgu ? dgu : "",
		len ? len : "",
		den ? den : "");
	++ count;
     }
   printf ("|}\n");
   printf ("Total %d\n", count);
   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
