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

   printf ("== Items which are a [[P:P279|subclass]], "
	   "but not of same [[P:P107|GND type]] as the superclass ==\n\n");

   MYSQL_ROW row;   
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT "
	 "  t1.s_item, "
	 "  (SELECT l_text FROM label WHERE t1.s_item = l_id AND l_lang = 'en'), "
	 "  t1.s_item_value, t1.s_datatype, "
	 "  (SELECT l_text FROM label WHERE t1.s_item_value = l_id AND l_lang = 'en'), "
	 "  t2.s_item, "
	 "  (SELECT l_text FROM label WHERE t2.s_item = l_id AND l_lang = 'en'), "
	 "  t2.s_item_value, t2.s_datatype, "
	 "  (SELECT l_text FROM label WHERE t2.s_item_value = l_id AND l_lang = 'en') "
	 "FROM statement sc " // sc = subclass
	 "JOIN statement t1 " // type of subclass item
	 "  ON t1.s_property = 107 AND sc.s_item = t1.s_item "
	 "JOIN statement t2 " // type of superclass item
	 "  ON t2.s_property = 107 AND sc.s_item_value = t2.s_item "
	 "WHERE sc.s_property = 279 "
	 "  AND t1.s_item_value != t2.s_item_value ");

   printf ("{| class=wikitable\n");
   printf ("! Subclass item\n");
   printf ("! Type\n");
   printf ("! Superclass item\n");
   printf ("! Type\n");

   int count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item1 = row[0];
	const char *litem1 = row[1];
	const char *type1 = row[2];
	const char *datatype1 = row[3];
	const char *ltype1 = row[4];
	const char *item2 = row[5];
	const char *litem2 = row[6];
	const char *type2 = row[7];
	const char *datatype2 = row[8];
	const char *ltype2 = row[9];
	
	++ count;
	printf ("|-\n");
	printf ("| [[Q%s]] (%s)", item1, litem1 ? litem1 : "?");
	if (*type1 == '0')
	  printf (" || %s", datatype1);
	else
	  printf (" || [[Q%s]] (%s)", type1, ltype1 ? ltype1 : "?");
	printf (" || [[Q%s]] (%s)", item2, litem2 ? litem2 : "?");
	if (*type2 == '0')
	  printf (" || %s\n", datatype2);
	else
	printf (" || [[Q%s]] (%s)\n", type2, ltype2 ? ltype2 : "?");
     }
   printf ("|}\n");
   printf ("Found %d.\n", count);

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
