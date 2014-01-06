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
   if (argc < 2)
     {
	printf ("Usage: %s language [database]\n", argv[0]);
	return 1;
     }
   const char *lang = argv[1];
   const char *database;
   if (argc < 3)
     database = "wikidatawiki";
   else
     database = argv[2];

   MYSQL *mysql = open_named_database (database);
   MYSQL_ROW row;
   
   printf ("== Items with most aliases in language %s ==\n",
	   lang);

   MYSQL_RES *result = do_query_use
     (mysql,
	 "/* __PRINT_QUERY__ */ "
	 "SELECT a_id, COUNT(*), k_text, k_ns "
	 "FROM alias "
	 "LEFT JOIN link ON k_id = a_id AND k_lang = '%s' "
	 "WHERE a_lang = '%s' "
	 "GROUP BY a_id "
	 "ORDER BY 2 DESC "
	 "LIMIT 500",
	 lang, lang);
   
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	int count = atoi (row[1]);
	const char *link = row[2];
	printf ("# [[Q%s]]", item);
	if (link)
	  {
	     int ns = atoi (row[3]);
	     printf (" (");
	     print_link (lang, ns, link);
	     printf (")");
	  }
	printf (": %d alias%s\n",
		count, count > 1 ? "es" : "");
     }
   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
