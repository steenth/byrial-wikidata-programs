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

   printf ("Are you sure you want to drop all merge tables in database %s? ",
	   database);
   int r;
   while ((r = getchar ()) != EOF && r != 'y' && r != 'n')
     ;
   printf ("\n");
   if (r != 'y')
     {
	printf ("Action not confirmed.\n");
	exit (1);
     }

   MYSQL *mysql = open_named_database (database);
   MYSQL_ROW row;
   MYSQL_RES *result;

   result = do_query_store
     (mysql,
	 "SHOW tables");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *table = row[0];
	
	if (strncmp (table, "numbermerge_", 12) == 0 ||
	    strncmp (table, "countrymerge_", 13) == 0 ||
	    strncmp (table, "commonsmerge_", 13) == 0 ||
	    strncmp (table, "bandmerge_", 10) == 0 ||
	    strncmp (table, "unique_property_merge_",
		     strlen ("unique_property_merge_")) == 0)
	  {
	     printf ("Dropping %s\n", table);
	     do_query (mysql, "DROP TABLE `%s`", table);
	  }
     }
   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
