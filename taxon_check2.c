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
   MYSQL_RES *result = 0;

   typedef struct  {
      int property;
      const char *allowed_values;
      const char *name;
   } taxon;
     
   taxon taxon_list[] = {
      { 89, "7432", "species" },
      { 74, "34740", "genus" },
      { 71, "35409", "family"  },
      { 70, "36602, 10861678", "order"  },
      { 77, "37517, 713623", "class"  },
      { 76, "38348", "phylum" },
      { 75, "36732", "kingdom" },
      { 273, "146481", "domain" },
      { 0, "", ""   }, // end of list
   };
   
   for (taxon *t = taxon_list ; t->property ; ++t)
     {
	result = do_query_use
	  (mysql,
	      "SELECT "
	      " s1.s_item, "
	      " (SELECT l_text FROM label WHERE s1.s_item = l_id AND l_lang = 'en'), "
	      " s2.s_item, "
	      " (SELECT l_text FROM label WHERE s2.s_item = l_id AND l_lang = 'en'), "
	      " s2.s_item_value, "
	      " (SELECT l_text FROM label WHERE s2.s_item_value = l_id AND l_lang = 'en') "
	      "FROM statement s1 " // s1 : use of taxon property
	      "JOIN statement s2 " // s2 : the taxon rank property for the value of s1  
	      "ON s1.s_property = %d "
	      "   AND s1.s_item_value = s2.s_item "
	      "   AND s2.s_property = 105 " // P105 = taxon rank
	      "   AND s2.s_item_value NOT IN (%s) "
	      // "LIMIT 100"
	      ,
	      t->property, t->allowed_values);

	printf ("\n== Conflicting %s values ==\n\n", t->name);
	
	printf ("Items used as value for the [[P:P%d|%s property]] which have "
		"a conflicting [[P:P105|taxon rank property]].\n\n",
		t->property, t->name);
	printf ("{| class=wikitable\n");
	printf ("! Item \n");
	printf ("! %s value\n", t->name);
	printf ("! the value's taxon rank value\n");
	int count = 0;
	while (NULL != (row = mysql_fetch_row (result)))
	  {
	     ++ count;
	     if (count > 100)
	       continue;
	     const char *item1 = row[0];
	     const char *label1 = row[1];
	     const char *item2 = row[2];
	     const char *label2 = row[3];
	     const char *rank = row[4];
	     const char *ranklabel = row[5];
	     printf ("|-\n");
	     printf ("| [[Q%s]] (%s)\n", item1, label1 ? label1 : "?");
	     printf ("| [[Q%s]] (%s)\n", item2, label2 ? label2 : "?");
	     printf ("| [[Q%s]] (%s)\n", rank, ranklabel ? ranklabel : "?");
	  }
	printf ("|}\n");
	printf ("Found %d cases.", count);
	if (count > 100)
	  printf (" (Output limited to 100 cases. Ask if more is wanted.)\n");
	printf ("\n");
     }
   
   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
