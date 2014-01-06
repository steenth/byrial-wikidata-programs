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

   MYSQL_ROW row;
   if (! table_exists (mysql, "birth_death"))
     {
	MYSQL *mysql2 = open_named_database (database);
	do_query (mysql,
	     "CREATE TABLE birth_death "
	     "(item INTEGER UNSIGNED NOT NULL,"
	     "birth INTEGER NOT NULL,"
	     "death INTEGER NOT NULL,"
	     "UNIQUE KEY (item),"
	     "KEY (birth,death))"
	     "ENGINE=MyISAM DEFAULT CHARSET=binary");
	MYSQL_RES *result = do_query_use
	  (mysql,
	      "SELECT tv_item, s_property, tv_year, tv_month, tv_day "
	      "FROM timevalue "
	      "JOIN statement "
	      "  ON tv_statement = s_statement AND s_property IN (569,570)"
	      "wHERE tv_precision = 11"); // precision 11 = day
	int birth_dates = 0;
	int death_dates = 0;
	while (NULL != (row = mysql_fetch_row (result)))
	  {
	     const char *item = row[0];
	     const char *property = row[1];
	     int year = atoi (row[2]);
	     int month = atoi (row[3]);
	     int day = atoi (row[4]);
	     
	     int date = ((year * 100) + month) * 100 + day;
	     bool is_birth = strcmp (property, "569") == 0;
	     if (is_birth)
	       ++ birth_dates;
	     else
	       ++ death_dates;
	     
	     do_query (mysql2,
		       "INSERT birth_death "
		       "SET item=%s, %s=%d "
		       "ON DUPLICATE KEY UPDATE %s=%d",
		       item,
		       is_birth ? "birth" : "death", date,
		       is_birth ? "birth" : "death", date);
	  }
	mysql_free_result (result);
	close_database (mysql2);
	fprintf (stderr, "Inserted %d birth dates and %d death dates into birth_death\n",
		 birth_dates, death_dates);
     }

   printf ("== Items with identical dates of birth and death ==\n");

   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT a.item, b.item, a.birth, a.death, "
	 "  (SELECT l_text FROM label WHERE l_id = a.item AND l_lang = 'en'), "
	 "  (SELECT l_text FROM label WHERE l_id = b.item AND l_lang = 'en') "
	 "FROM birth_death a "
	 "JOIN birth_death b "
	 "  ON a.birth = b.birth AND a.birth != 0 "
	 "  AND a.death = b.death AND a.death != 0 "
	 "  AND a.item < b.item");
   printf ("{| class=\"wikitable sortable\"\n"
	   "|-\n"
	   "! Item 1 !! Item 2 !! Birth !! Death\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item1 = row[0];
	const char *item2 = row[1];
	int birth = atoi (row[2]);
	int death = atoi (row[3]);
	const char *label1 = row[4];
	const char *label2 = row[5];

	printf ("|-\n| [[Q%s]] (%s) || [[Q%s]] (%s) || %d-%02d-%02d || %d-%02d-%02d\n",
		item1, label1 ? label1 : "?",
		item2, label2 ? label2 : "?",
		birth / 10000, (birth / 100) % 100, birth % 100,
		death / 10000, (death / 100) % 100, death % 100);		
     }
   printf ("|}\n");
   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
