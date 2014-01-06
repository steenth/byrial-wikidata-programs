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

void make_list (MYSQL *mysql, const char *filename, const char *type, int property);

#define FILENAME_BIRTH "birth-label.list"
#define FILENAME_DEATH "death-label.list"
int main (int argc, char *argv[])
{
   const char *database;
   if (argc < 2)
     database = "wikidatawiki";
   else
     database = argv[1];

   MYSQL *mysql = open_named_database (database);
   print_dumpdate (mysql);
   
   make_list (mysql, FILENAME_BIRTH, "birth", 569);
   make_list (mysql, FILENAME_DEATH, "death", 570);
   close_database (mysql);
}

void make_list (MYSQL *mysql, const char *filename, const char *type, int property)
{
   printf ("== Different item with same date of %s, and at least one common label ==\n",
	   type);
   
   fprintf (stderr, "Writing %s ...", filename);
   fflush (stderr);
   FILE *fp = fopen (filename, "w");
   if (!fp)
     {
	printf ("Opening of %s failed: %m\n", filename);
	exit (1);
     }

   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT l_id, l_text, tv_year, tv_month, tv_day "
	 "FROM timevalue "
	 "JOIN statement ON s_statement = tv_statement "
	 "JOIN label ON l_id = s_item "
	 "WHERE s_property = %d",
	 property);
   MYSQL_ROW row;
   int count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *label = row[1];
	const char *year = row[2];
	const char *month = row[3];
	const char *day = row[4];
	fprintf (fp, "%s#%s-%s-%s-%s\n",
		 item, year, month, day, label);
	++ count;
     }
   mysql_free_result (result);
   fclose (fp);
   
   fprintf (stderr, "wrote %d lines.\n", count);
   fprintf (stderr, "Sorting %s ...\n", filename);
   fflush (stderr);
   char command[200];
   sprintf (command, "sort --field-separator=# --key=2 < %s | uniq > %s.sort",
	    filename, filename);
   if (0 < system (command))
     {
	printf ("system(%s) failed: %m\n", command);
	exit (1);
     }

   char filenamesort[50];
   sprintf (filenamesort, "%s.sort", filename);
   fp = fopen (filenamesort, "r");
   if (!fp)
     {
	printf ("Opening of %s failed: %m\n", filenamesort);
	exit (1);
     }
   int item;
   int year;
   int month;
   int day;
   char label[300];
   
   int last_item = 0;
   int last_year = 0;
   int last_month = 0;
   int last_day = 0;
   char last_label[300] = "";

   int lines = 0;
   while (5 == fscanf (fp, "%d#%d-%d-%d-%[^\n]\n",
		       &item, &year, &month, &day, label))
     {
	++lines;
	
	if (item != last_item &&
	    year == last_year && month == last_month && day == last_day &&
	    strcmp (label, last_label) == 0)
	  printf ("* %s, %s %d-%.2d-%.2d: [[Q%d]], [[Q%d]]\n",
		 label, type, year, month, day, last_item, item);
	else
	  {
	     last_item = item;
	     last_year = year;
	     last_month = month;
	     last_day = day;
	     strcpy (last_label, label);
	  }
     }
   fclose (fp);
   fprintf (stderr, "read %d lines\n", lines);
}
