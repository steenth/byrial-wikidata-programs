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
   
   MYSQL_RES *result = do_query_use
     (mysql,
	 "/* __PRINT_QUERY__ */ "
	 "SELECT s_item, s_string_value, s_datatype, "
	 "  (SELECT l_text FROM label WHERE l_id = s_item AND l_lang = 'en') "
	 "FROM statement "
	 "WHERE s_property = 486 "
	 "ORDER BY s_string_value");
   
   printf ("== Items with {{P|486}} ==\n");
   int count = 0;
   char last[80] = "";
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *meshid = row[1];
	const char *datatype = row[2];
	const char *label = row[3];
	if (meshid)
	  {
	     if (strcmp (meshid, last) == 0)
	       printf ("*");
	     else
	       strcpy (last, meshid);
	  }	
	printf ("* [[Q%s]] (%s) MeSH ID = %s\n",
		item, label ? label : "?",
		strcmp (datatype, "string") == 0 ? meshid : datatype);
	++ count;
     }
   printf ("Total: %d\n", count);

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
