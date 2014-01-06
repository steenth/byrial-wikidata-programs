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

int main (int argc, char *argv[])
{
   const char *database;
   if (argc < 2)
     {
	printf ("Usage: %s dump-date [database]\n", argv[0]);
	return 1;
     }
   const char *dumpdate = argv[1];
   if (argc < 3)
     database = "wikidatawiki";
   else
     database = argv[2];
   MYSQL *mysql = open_named_database (database);

   if (! table_exists (mysql, "info"))
     {
	do_query (mysql,
		  "CREATE TABLE info "
		  "(dumpdate DATE NOT NULL)"
		  "ENGINE=MyISAM DEFAULT CHARSET=binary");
	do_query (mysql,
		  "INSERT info "
		  "SET dumpdate='%s'",
		  dumpdate);
     }
   else
     {
	do_query (mysql,
		  "UPDATE info "
		  "SET dumpdate='%s'",
		  dumpdate);
     }
   close_database (mysql);
   return 0;
}
