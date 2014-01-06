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
   print_dumpdate (mysql);
   printf ("Disambiguation items used by items in statements as value "
	   "for claims, qualifiers or sources. A disambiguation item is "
	   "here defined as an item with the claim {{P|107}}: {{Q|11651459}}.\n");
   
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT "
	 "  i.s_item, "
	 "  (SELECT l_text FROM label WHERE i.s_item = l_id AND l_lang = 'en'), "
	 "  i.s_property, "
	 "  (SELECT pl_text FROM plabel WHERE i.s_property = pl_id AND pl_lang = 'en'), "
	 "  i.s_item_value, "
	 "  (SELECT l_text FROM label WHERE i.s_item_value = l_id AND l_lang = 'en') "
	 "FROM statement i "
	 "JOIN statement dab "
	 "  ON i.s_item_value = dab.s_item "
	 "WHERE dab.s_property = 107 AND dab.s_item_value = 11651459 "
	 "/*LIMIT 7500*/"); // Seems much faster with a limit

   int count = 0;
   printf ("=== Disambiguation items used as values in claims ===\n\n");
   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Property\n");
   printf ("! Value\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	++ count;
	if (count > 1000)
	  continue;
	const char *item = row[0];
	const char *litem = row[1];
	const char *property = row[2];
	const char *lproperty = row[3];
	const char *value = row[4];
	const char *lvalue = row[5];
	printf ("|-\n");
	printf ("| [[Q%s]] (%s)", item, litem ? litem : "?");
	printf (" || [[P:P%s]] (%s)", property, lproperty ? lproperty : "?");
	printf (" || [[Q%s]] (%s)\n", value, lvalue ? lvalue : "?");
     }
   printf ("|}\n");
   printf ("In total: %d.", count);
   if (count > 1000)
     printf (" (Output limited to 1000 rows. Ask if you want to see more.)");
   printf ("\n");

   count = 0;
   result = do_query_use
     (mysql,
	 "SELECT "
	 "  q_item, "
	 "  (SELECT l_text FROM label WHERE q_item = l_id AND l_lang = 'en'), "
	 "  q_property, "
	 "  (SELECT pl_text FROM plabel WHERE q_property = pl_id AND pl_lang = 'en'), "
	 "  q_item_value, "
	 "  (SELECT l_text FROM label WHERE q_item_value = l_id AND l_lang = 'en') "
	 "FROM qualifier "
	 "JOIN statement dab "
	 "  ON q_item_value = dab.s_item "
	 "WHERE dab.s_property = 107 AND dab.s_item_value = 11651459 "
	 "/*LIMIT 100*/");

   printf ("=== Disambiguation items used as values in qualifiers ===\n\n");
   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Property\n");
   printf ("! Value\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	++ count;
	const char *item = row[0];
	const char *litem = row[1];
	const char *property = row[2];
	const char *lproperty = row[3];
	const char *value = row[4];
	const char *lvalue = row[5];
	printf ("|-\n");
	printf ("| [[Q%s]] (%s)", item, litem ? litem : "?");
	printf (" || [[P:P%s]] (%s)", property, lproperty ? lproperty : "?");
	printf (" || [[Q%s]] (%s)\n", value, lvalue ? lvalue : "?");
     }
   printf ("|}\n");
   printf ("In total: %d.\n", count);

   count = 0;
   result = do_query_use
     (mysql,
	 "SELECT "
	 "  rc_item, "
	 "  (SELECT l_text FROM label WHERE rc_item = l_id AND l_lang = 'en'), "
	 "  rc_property, "
	 "  (SELECT pl_text FROM plabel WHERE rc_property = pl_id AND pl_lang = 'en'), "
	 "  rc_item_value, "
	 "  (SELECT l_text FROM label WHERE rc_item_value = l_id AND l_lang = 'en') "
	 "FROM refclaim "
	 "JOIN statement dab "
	 "  ON rc_item_value = dab.s_item "
	 "WHERE dab.s_property = 107 AND dab.s_item_value = 11651459 "
	 "/*LIMIT 100*/");

   printf ("=== Disambiguation items used as values in sources ===\n\n");
   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Property\n");
   printf ("! Value\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	++ count;
	const char *item = row[0];
	const char *litem = row[1];
	const char *property = row[2];
	const char *lproperty = row[3];
	const char *value = row[4];
	const char *lvalue = row[5];
	printf ("|-\n");
	printf ("| [[Q%s]] (%s)", item, litem ? litem : "?");
	printf (" || [[P:P%s]] (%s)", property, lproperty ? lproperty : "?");
	printf (" || [[Q%s]] (%s)\n", value, lvalue ? lvalue : "?");
     }
   printf ("|}\n");
   printf ("In total: %d.\n", count);

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
