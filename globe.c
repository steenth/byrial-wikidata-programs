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
   printf ("== Globes used in values for coordinate properties ==\n");

   MYSQL_ROW row;
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT co_globe, COUNT(*), "
	 "  (SELECT l_text FROM label WHERE l_id = co_globe AND l_lang = 'en') "
	 "FROM coordinate "
	 "GROUP BY co_globe "
	 "ORDER BY 2 DESC");
   printf ("{| class=\"wikitable sortable\"\n"
	   "|-\n"
	   "! Globe !! Uses\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int globe = atoi (row[0]);
	const char *count = row[1];
	const char *label = row[2];

	if (globe > 0)
	  printf ("|-\n| [[Q%d]] (%s) || %s\n",
		  globe, label ? label : "?", count);
	else if (globe == 0)
	  printf ("|-\n| Not specified || %s\n", count);
	else if (globe == -1)
	  printf ("|-\n| \"earth\" || %s\n", count);
	else
	  printf ("|-\n| something else || %s\n", count);
     }
   printf ("|}\n");
   mysql_free_result (result);

   printf ("== Coordinate values with impossible (too high) latitude or longitude values ==\n");
   result = do_query_use
     (mysql,
	 "SELECT co_item, co_latitude, co_longitude, co_precision, co_globe, "
	 "  (SELECT l_text FROM label WHERE l_id = co_globe AND l_lang = 'en'), "
	 "  (SELECT l_text FROM label WHERE l_id = co_item AND l_lang = 'en') "
	 "FROM coordinate "
	 "WHERE co_latitude > 90 OR co_latitude < -90 OR "
	 "      co_longitude > 180 OR co_longitude < -180 ");
   printf ("{| class=\"wikitable sortable\"\n"
	   "|-\n"
	   "! Item !! Latitude !! Longitude !! Precision !! Globe\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *latitude = row[1];
	const char *longitude = row[2];
	const char *precision = row[3];
	int globe = atoi (row[4]);
	const char *lglobe = row[5];
	const char *litem = row[6];

	printf ("|-\n| [[Q%s]] (%s) || %s || %s || %s || ",
		item, litem ? litem : "?", latitude, longitude,
		precision ? precision : "Not specified");
	if (globe > 0)
	  printf ("[[Q%d]] (%s)\n",
		  globe, lglobe ? lglobe : "?");
	else if (globe == 0)
	  printf ("Not specified\n");
	else if (globe == -1)
	  printf ("\"earth\"\n");
	else
	  printf ("something else\n");
     }
   printf ("|}\n");
   close_database (mysql);
   return 0;
}
