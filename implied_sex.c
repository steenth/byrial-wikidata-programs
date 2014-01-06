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

   printf ("== Conflicting or missing sex of items used as value "
	   "for a sex specific property ==\n");

   MYSQL_ROW row;
   
   // P7: brother
   // p22: father
   // p43: stepfather
   // p29: uncle

   // P9: sister
   // p25: mother
   // p44: stepmother
   // p139: aunt

   // p21: sex

   // q6581072: female
   // q6581097: male
   // q1097630: intersex
   // q44148: male animal
   // q43445: female animal
   
   MYSQL_RES *result = do_query_use
     (mysql,
	 //  "/* __PRINT_QUERY__*/"
	 // All in one select or a union??
	 /*
	 "SELECT "
	 "  s1.s_item, "
	 "  (SELECT l_text FROM label WHERE s1.s_item = l_id AND l_lang = 'en'), "
	 "  (SELECT pl_text FROM plabel WHERE s1.s_property = pl_id AND pl_lang = 'en'), "
	 "  s1.s_item_value, "
	 "  (SELECT l_text FROM label WHERE s1.s_item_value = l_id AND l_lang = 'en'), "
	 "  s2.s_item_value "
	 "FROM statement s1 "
	 "LEFT JOIN statement s2 "
	 "ON "
	 "  s1.s_property in (7,22,43,29, 9,25,44,139) AND "
	 "  s1.s_item_value = s2.s_item AND "
	 "  s2.s_property = 21 "
	 "WHERE "
	 "  s1.s_property in (7,22,43,29, 9,25,44,139) AND "
	 "  s1.s_item_value != 0 AND "
	 "  s2.s_item_value IS NULL OR s2.s_item_value != "
	 "    IF (s1.s_property in (7,22,43,29), 6581097, 6581072)"); */
	 "SELECT "
	 "  s1.s_item, "
	 "  (SELECT l_text FROM label WHERE s1.s_item = l_id AND l_lang = 'en'), "
	 "  (SELECT pl_text FROM plabel WHERE s1.s_property = pl_id AND pl_lang = 'en'), "
	 "  s1.s_item_value, "
	 "  (SELECT l_text FROM label WHERE s1.s_item_value = l_id AND l_lang = 'en'), "
	 "  s2.s_item_value "
	 "FROM statement s1 "
	 "LEFT JOIN statement s2 "
	 "ON "
	 "  s1.s_property in (7,22,43,29) AND "
	 "  s1.s_item_value = s2.s_item AND "
	 "  s2.s_property = 21 "
	 "WHERE "
	 "  s1.s_property in (7,22,43,29) AND "
	 "  s1.s_item_value != 0 AND "
	 "  s2.s_item_value IS NULL OR s2.s_item_value != 6581097 "
	 "UNION "
	 "SELECT "
	 "  s1.s_item, "
	 "  (SELECT l_text FROM label WHERE s1.s_item = l_id AND l_lang = 'en'), "
	 "  (SELECT pl_text FROM plabel WHERE s1.s_property = pl_id AND pl_lang = 'en'), "
	 "  s1.s_item_value, "
	 "  (SELECT l_text FROM label WHERE s1.s_item_value = l_id AND l_lang = 'en'), "
	 "  s2.s_item_value "
	 "FROM statement s1 "
	 "LEFT JOIN statement s2 "
	 "ON "
	 "  s1.s_property in (9,25,44,139) AND "
	 "  s1.s_item_value = s2.s_item AND "
	 "  s2.s_property = 21 "
	 "WHERE "
	 "  s1.s_property in (9,25,44,139) AND "
	 "  s1.s_item_value != 0 AND "
	 "  s2.s_item_value IS NULL OR s2.s_item_value != 6581072");

   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Label (English)\n");
   printf ("! Property\n");
   printf ("! Value\n");
   printf ("! Label (English)\n");
   printf ("! Value's sex\n");
   int conflict = 0;
   int missing = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item1 = row[0];
	const char *label1 = row[1];
	const char *property = row[2];
	const char *item2 = row[3];
	const char *label2 = row[4];
	const char *value = row[5];
	const char *value_text;
	if (! value)
	  {
	     ++ missing;
	     value_text = "not set";
	  }	
	else
	  {
	     ++ conflict;
	     switch (atoi (value))
	       {
		case 6581072: value_text = "female"; break;
		case 6581097: value_text = "male"; break;
		case 1097630: value_text = "intersex"; break;
		case 44148: value_text = "male animal"; break;
		case 43445: value_text = "female animal"; break;
		default: value_text = value;
	       }
	  }

	printf ("|-\n");
	printf ("| data-sort-type=\"number\" | [[Q%s]] || %s || %s\n"
		"| data-sort-type=\"number\" | [[Q%s]] || %s || %s\n",
		item1, label1 ? label1 : "",
		property,
		item2, label2 ? label2 : "",
		value_text);
     }
   printf ("|}\n");
   printf ("* %d cases with missing sex.\n", missing);
   printf ("* %d cases with conflicting sex.\n", conflict);
   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
