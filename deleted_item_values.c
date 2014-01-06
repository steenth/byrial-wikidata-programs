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
   printf ("Fixed entries should be removed.\n");

   printf ("=== Deleted items used as value for claims ===\n");
   MYSQL_ROW row;
   MYSQL_RES *result = do_query_use
     (mysql,
	 // "/* __PRINT_QUERY__ */"
	 "SELECT COUNT(s_item_value), s_item_value, "
	 "  GROUP_CONCAT(DISTINCT s_property SEPARATOR ']], [[P:P'), "
	 "  GROUP_CONCAT(DISTINCT pl_text SEPARATOR ', '), "
	 "  GROUP_CONCAT(DISTINCT s_item SEPARATOR ']], [[Q') "
	 "FROM statement "
	 "LEFT JOIN item ON s_item_value = i_id "
	 "LEFT JOIN plabel ON s_property = pl_id AND pl_lang = 'en' "
	 "WHERE s_item_value AND i_id IS NULL "
	 "GROUP BY s_item_value "
	 "/* ORDER BY 1 DESC, 2 */");

   printf ("{| class='wikitable sortable'\n");
   printf ("! # of uses\n");
   printf ("! Item\n");
   printf ("! Properties\n");
   printf ("! Used by\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *count = row[0];
	const char *item = row[1];
	const char *ps = row[2];
	const char *plabels = row[3];
	const char *users = row[4];
	printf ("|-\n");
	printf ("| %s || [[Q%s]] || [[P:P%s]] (%s) || [[Q%s]]\n",
		count, item, ps, plabels, users);
     }
   printf ("|}\n");

   printf ("\n=== Deleted items used as value for qualifiers ===\n\n");
   result = do_query_use
     (mysql,
	 "SELECT COUNT(q_item_value), q_item_value, q_property, pl_text, "
	 "  GROUP_CONCAT(q_item SEPARATOR ']], [[Q') "
	 "FROM qualifier "
	 "LEFT JOIN item ON q_item_value = i_id "
	 "LEFT JOIN plabel ON q_property = pl_id AND pl_lang = 'en' "
	 "WHERE q_item_value AND i_id IS NULL "
	 "GROUP BY q_item_value "
	 "ORDER BY 1 DESC, 2");

   printf ("{| class='wikitable sortable'\n");
   printf ("! # of uses\n");
   printf ("! Item\n");
   printf ("! Property (example)\n");
   printf ("! Used by\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *count = row[0];
	const char *item = row[1];
	const char *p = row[2];
	const char *plabel = row[3];
	const char *users = row[4];
	printf ("|-\n");
	printf ("| %s || [[Q%s]] || [[P:P%s|%s]] || [[Q%s]]\n",
		count, item, p, plabel ? plabel : p, users);
     }
   printf ("|}\n");

   printf ("\n=== Deleted items used as value for references ===\n\n");
   result = do_query_use
     (mysql,
	 "SELECT COUNT(rc_item_value), rc_item_value, rc_property, pl_text, "
	 "  GROUP_CONCAT(rc_item SEPARATOR ']], [[Q') "
	 "FROM refclaim "
	 "LEFT JOIN item ON rc_item_value = i_id "
	 "LEFT JOIN plabel ON rc_property = pl_id AND pl_lang = 'en' "
	 "WHERE rc_item_value AND i_id IS NULL "
	 "GROUP BY rc_item_value "
	 "ORDER BY 1 DESC, 2");

   printf ("{| class='wikitable sortable'\n");
   printf ("! # of uses\n");
   printf ("! Item\n");
   printf ("! Property (example)\n");
   printf ("! Used by\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *count = row[0];
	const char *item = row[1];
	const char *p = row[2];
	const char *plabel = row[3];
	const char *users = row[4];
	printf ("|-\n");
	printf ("| %s || [[Q%s]] || [[P:P%s|%s]] || [[Q%s]]\n",
		count, item, p, plabel ? plabel : p, users);
     }
   printf ("|}\n");

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
