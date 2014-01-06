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

   printf ("== Items which may be pair of persons ==\n\n");
   printf ("These items have GND type person and " 
	   "contains the word \"and\" in the English label.\n");

   MYSQL_ROW row;
   MYSQL_RES *result = do_query_store
     (mysql,
	 "SELECT l_id, l_text "
	 "FROM label "
	 "JOIN statement ON l_id = s_item "
	 "WHERE s_property = 107 AND s_item_value = 215627 "
	 "  AND l_text LIKE '%% and %%' AND l_lang = 'en'");

   printf ("{| class=wikitable\n");
   printf ("! Item\n");
   printf ("! Other properties\n");

   // count properties
   int personal = 0;
   int other = 0;
   int unlisted = 0;
   
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *label= row[1];

	const char *pos_and = strstr (label, " and ");
	const char *pos_of = strstr (label, " of ");
	if (pos_of && (pos_of < pos_and))
	  {
	     continue;
	  }
	
	const char *pos = label;	
	for ( ; pos < pos_and ; ++pos)
	  {
	     if (*pos == ',' || *pos == '(')
	       break;
	  }
	if (pos < pos_and)
	  continue; // Also cut off cases like "one, two, and three" - to be handled later

	printf ("|-\n");
	printf ("| [[Q%s]] (%s) ||", item, label);

	MYSQL_RES *res2 = do_query_use
	  (mysql,
	      "SELECT s_property "
	      "FROM statement "
	      "WHERE s_item = %s AND s_property != 107 "
	      "ORDER BY 1",
	      item);
	MYSQL_ROW row2;
	int found = 0;
	while (NULL != (row2 = mysql_fetch_row (res2)))
	  {
	     int property = atoi (row2[0]);
	     ++ found;
	     switch (property)
	       {
		case 7: // brother
		case 19: // place of birth
		case 20: // place of death
		case 21: // sex
		case 22: // father
		case 25: // mother
		case 27: // country of citizenship
		case 39: // office held
		case 69: // alma mater
		case 91: // sexual orientation
		case 106: // occupation
		case 119: // place of burial
		case 412: // voice type
		case 509: // cause of death
		case 535: // Find a Grave
		case 536: // ATP id
		case 564: // singles record
		case 569: // date of birth
		case 570: // date of death
		case 625: // coordinate location (not for persons, nor groups)
		  ++ personal; break;
		case 18: // image
		case 31: // instance of
		case 213: // ISNI
		case 214: // VIAF identifier
		case 244: // LCCN identifier
		case 227: // GND identifier
		case 268: // BNF
		case 269: // SUDOC identifier
		case 279: // subclass of
		case 345: // IMDb identifier
		case 349: // NDL identifier
		case 358: // dicography
		case 373: // Commons category
		case 434: // MusizBrainz artist ID
		case 360: // list of
		case 361: // part of	
		case 527: // consists of
		case 555: // doubles record
		case 691: // AUT NKC (Czech authory ID)
		  ++ other; break;
		default:
		  ++ unlisted;
		  // printf ("Missing property: %d\n", property);
		  break;
	       }

	     printf ("%s {{P|%d}}", found > 1 ? "," : "", property);
	  }
	printf ("\n");
	mysql_free_result (res2);
     }
   printf ("|}\n");
   printf ("* Found %d properties which groups of people should not have.\n", personal);
   printf ("* Found %d properties which groups of people maybe can have.\n", other);
   if (unlisted)
     printf ("* Found %d properties unknown to the program.\n", unlisted);
   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
