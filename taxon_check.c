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
   printf ("== Items with same value for taxon name "
	   "(possibly duplicates or wrong taxon name) ==\n\n");

   MYSQL_ROW row;
   MYSQL_RES *result = do_query_use
     (mysql,
	 // "/* __PRINT_QUERY__ */ "
	 "SELECT "
	 "   s1.s_item, "
	 "   (SELECT l_text FROM label WHERE s1.s_item = l_id AND l_lang = 'en'), "
	 "   s1rank.s_item_value, "
	 "   (SELECT l_text FROM label WHERE s1rank.s_item_value = l_id AND l_lang = 'en'), "
	 "   s2.s_item, "
	 "   (SELECT l_text FROM label WHERE s2.s_item = l_id AND l_lang = 'en'), "
	 "   s2rank.s_item_value, "
	 "   (SELECT l_text FROM label WHERE s2rank.s_item_value = l_id AND l_lang = 'en'), "
	 "   s1.s_string_value "
	 "FROM statement s1 "
	 "JOIN statement s2 "
	 "ON s1.s_property = 225 "
	 "   AND s2.s_property = 225 "
	 "   AND s1.s_string_value = s2.s_string_value "
	 "   AND s1.s_item < s2.s_item "
	 "LEFT JOIN statement s1rank "
	 "   ON s1.s_item = s1rank.s_item AND s1rank.s_property = 105 "
	 "LEFT JOIN statement s2rank "
	 "   ON s2.s_item = s2rank.s_item AND s2rank.s_property = 105 "
	 "WHERE s1rank.s_item IS NULL OR s2rank.s_item IS NULL "
	 "   OR s1rank.s_item_value = s2rank.s_item_value "
	 "   OR TRUE ");

   printf ("{| class=wikitable\n");
   printf ("! Item 1\n");
   printf ("! taxon rank\n");
   printf ("! Item 2\n");
   printf ("! taxon rank\n");
   printf ("! taxon name\n");
   int count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *i1     = row[0];
	const char *l1     = row[1];
	const char *ranki1 = row[2];
	const char *rankl1 = row[3];
	const char *i2     = row[4];
	const char *l2     = row[5];
	const char *ranki2 = row[6];
	const char *rankl2 = row[7];
	const char *taxon  = row[8];
	++ count;

	printf ("|-\n");
	printf ("| [[Q%s]] (%s)", i1, l1 ? l1 : "?");
	if (ranki1)
	  printf (" || [[Q%s]] (%s)", ranki1, rankl1 ? rankl1 : "?");
	else
	  printf (" || na");
	printf (" || [[Q%s]] (%s)", i2, l2 ? l2 : "?");
	if (ranki2)
	  printf (" || [[Q%s]] (%s)", ranki2, rankl2 ? rankl2 : "?");
	else
	  printf (" || na");
	printf (" || %s\n", taxon);
     }
   printf ("|}\n");
   printf ("Found %d.\n", count);

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
