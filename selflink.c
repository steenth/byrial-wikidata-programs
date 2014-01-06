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

   printf ("=== Items using themselves as values for claims ===\n\n");

   MYSQL_ROW row;
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT s_item, s_property, "
	 "  (SELECT l_text FROM label WHERE s_item = l_id AND l_lang = 'en'), "
	 "  (SELECT pl_text FROM plabel WHERE s_property = pl_id AND pl_lang = 'en') "
	 "FROM statement "
	 "WHERE s_item_value = s_item "
	 "ORDER BY 2, 1");

   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Label (English)\n");
   printf ("! Property\n");
   int count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *p = row[1];
	const char *label = row[2];
	const char *plabel = row[3];
	printf ("|-\n");
	printf ("| [[Q%s]] || %s || [[P:P%s|%s]]\n",
		item, label ? label : "", p, plabel ? plabel : plabel);
	++ count;
     }
   printf ("|}\n"
	   "Count: %d.\n", count);

   printf ("=== Items using themselves as values for qualifiers ===\n\n");

   result = do_query_use
     (mysql,
	 "SELECT q_item, q_property, "
	 "  (SELECT l_text FROM label WHERE q_item = l_id AND l_lang = 'en'), "
	 "  (SELECT pl_text FROM plabel WHERE q_property = pl_id AND pl_lang = 'en') "
	 "FROM qualifier "
	 "WHERE q_item_value = q_item "
	 "ORDER BY 2, 1");

   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Label (English)\n");
   printf ("! Property\n");
   count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *p = row[1];
	const char *label = row[2];
	const char *plabel = row[3];
	printf ("|-\n");
	printf ("| [[Q%s]] || %s || [[P:P%s|%s]]\n",
		item, label ? label : "", p, plabel ? plabel : plabel);
	++ count;
     }
   printf ("|}\n"
	   "Count: %d.\n", count);

   printf ("=== Items using themselves as values for sources ===\n\n");

   result = do_query_use
     (mysql,
	 "SELECT rc_item, rc_property, "
	 "  (SELECT l_text FROM label WHERE rc_item = l_id AND l_lang = 'en'), "
	 "  (SELECT pl_text FROM plabel WHERE rc_property = pl_id AND pl_lang = 'en') "
	 "FROM refclaim "
	 "WHERE rc_item_value = rc_item "
	 "ORDER BY 2, 1");

   printf ("{| class='wikitable sortable'\n");
   printf ("! Item\n");
   printf ("! Label (English)\n");
   printf ("! Property\n");
   count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *p = row[1];
	const char *label = row[2];
	const char *plabel = row[3];
	printf ("|-\n");
	printf ("| [[Q%s]] || %s || [[P:P%s|%s]]\n",
		item, label ? label : "", p, plabel ? plabel : plabel);
	++ count;
     }
   printf ("|}\n"
	   "Count: %d.\n", count);

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
