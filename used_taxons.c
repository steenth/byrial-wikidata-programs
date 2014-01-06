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
   print_dumpdate (mysql);
   printf ("=== Items used as value for [[Property:P105|taxon rank]] ===\n\n");

   MYSQL_ROW row;
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT COUNT(s_item_value), s_item_value, "
	 "  (SELECT l_text FROM label WHERE l_id = s_item_value AND l_lang = 'en'), "
	 "  (SELECT d_text FROM descr WHERE d_id = s_item_value AND d_lang = 'en'), "
	 "  s_item "
	 "FROM statement "
	 "WHERE s_property = 105 AND s_item_value != 0 "
	 "GROUP BY s_item_value "
	 "ORDER BY 3, 4");
   
   printf ("{| class='wikitable sortable'\n");
   printf ("! # of uses\n");
   printf ("! Item\n");
   printf ("! English label and description\n");
   printf ("! Used by\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *count = row[0];
	const char *item = row[1];
	const char *label = row[2];
	const char *description = row[3];
	const char *used_by = row[4];
	printf ("|-\n");
	printf ("| %s || [[Q%s]] || %s, %s || [[Q%s]]%s\n",
		count, item, label ? label : "", description ? description : "",
		used_by, (atoi (count) > 1) ? " ..." : ""); 
     }
   printf ("|}\n");

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
