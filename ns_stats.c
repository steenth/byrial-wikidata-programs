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

void impossible (int ns, int links, int count)
{
   printf ("ns = %d, links = %d, count = %d, that should be impossible\n",
	   ns, links, count);
   exit (1);
}

void print_ns_field (int ns)
{
   printf ("|-\n");
   switch (ns)
     {
      case -10: printf ("| More than one"); break;
      case -2: printf ("| -2: Media"); break;
      case -1: printf ("| -1: Special"); break;
      case 0: printf ("| 0: (Article)"); break;
      case 1: printf ("| 1: Talk"); break;
      case 2: printf ("| 2: User"); break;
      case 3: printf ("| 3: User talk"); break;
      case 4: printf ("| 4: Project"); break;
      case 5: printf ("| 5: Project talk"); break;
      case 6: printf ("| 6: File"); break;
      case 7: printf ("| 7: File talk"); break;
      case 8: printf ("| 8: MediaWiki"); break;
      case 9: printf ("| 9: MediaWiki talk"); break;
      case 10: printf ("| 10: Template"); break;
      case 11: printf ("| 11: Template talk"); break;
      case 12: printf ("| 12: Help"); break;
      case 13: printf ("| 13: Help talk"); break;
      case 14: printf ("| 14: Category"); break;
      case 15: printf ("| 15: Category talk"); break;
      case 828: printf ("| 828: Module"); break;
      case 829: printf ("| 829: Module talk"); break;
      case 999: printf ("| ?: (Unknown)"); break;
      case 1000: printf ("! Sum"); break;
      default:
	if (ns % 2)
	  printf ("| %d: (Local defined talk)", ns);
	else
	  printf ("| %d: (Local defined)", ns);
	break;
     }
}

void print_row (int ns, int *counts, int *sums)
{
   print_ns_field (ns);
   int sum = 0;
   for (int i = 0 ; i < 9 ; ++ i)
     {
	sum += counts[i];
	sums[i] += counts[i];
	printf (" || %d", counts[i]);
     }
   printf ("|| %d\n", sum);
}

void item_stat (MYSQL *mysql);
void link_stat (MYSQL *mysql);

int main (int argc, char *argv[])
{
   const char *database;
   if (argc < 2)
     database = "wikidatawiki";
   else
     database = argv[1];

   MYSQL *mysql = open_named_database (database);

   print_dumpdate (mysql);
   item_stat (mysql);
   link_stat (mysql);
   close_database (mysql);
}

void link_stat (MYSQL *mysql)
{
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT k_ns, k_project, COUNT(*) "
	 "FROM link "
	 "GROUP BY k_ns, k_project "
	 "ORDER BY k_ns, k_project");

   printf ("{| class=wikitable\n");
   printf ("|+ Links after namespace and project\n"); 
   printf ("|-\n");
   printf ("! Namespace !! Wikipedia !! Wikivoyage !! Sum\n");

   int last_ns = -100;
   char last_project = ' ';
   int w_sum = 0;
   int y_sum = 0;
   int ns_sum = 0;
   MYSQL_ROW row;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int ns = atoi (row[0]);
	char project = *(row[1]);
	int count = atoi (row[2]);
	
	if (ns != last_ns)
	  {
	     if (last_project == 'w')
	       {
		  // Finish last row with an empty Wikivoyage field and sum
		  printf ("||&nbsp;||%d\n", ns_sum);
	       }
	     print_ns_field (ns);
	     if (project == 'w')
	       {
		  printf ("||%d", count);
		  w_sum += count;
	       }
	     else
	       {
		  printf ("||&nbsp;||%d||%d\n", count, count);
		  y_sum += count;
	       }
	     ns_sum = count;
	     last_ns = ns;
	     last_project = project;
	  }
	else
	  {
	     if (project == 'w')
	       {
		  printf ("\nThe Wikipedia count should be first for a namespace\n");
		  exit (1);
	       }
	     else
	       {
		  printf ("||%d||%d\n", count, ns_sum + count);
		  y_sum += count;
	       }
	     last_project = project;
	  }
     }
   if (last_project == 'w')
     printf ("||&nbsp;||%d\n", ns_sum);

   print_ns_field (1000); // The sum
   printf ("||%d||%d||%d\n", w_sum, y_sum, w_sum + y_sum);
   printf ("|}\n");
}

void item_stat (MYSQL *mysql)
{
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT i_ns, i_links, COUNT(*) "
	 "FROM item "
	 "GROUP BY i_ns, i_links "
	 "ORDER BY i_ns, i_links");

   printf ("{| class=wikitable\n");
   printf ("|+ Items after number of links and the namespace of the links\n"); 
   printf ("|-\n");
   printf ("! Namespace/links\n");
   printf ("! 1 !! 2 !! 3 !! 4 !! 5 !! 6-10 !! 11-25 !! 26-100 !! 101+ !! Sum\n");

   MYSQL_ROW row;
   int current_ns = -100; // Lowest value - meaning no links
   int no_links = 0;
   int count_array[9];
   int sum_array[9] = { 0,0,0, 0,0,0, 0,0,0 };
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int ns = atoi (row[0]);
	int links = atoi (row[1]);
	int count = atoi (row[2]);
	if (ns == -100)
	  {
	     if (links != 0)
	       impossible (ns, links, count);
	     no_links = count;
	     continue;
	  }
	
	if (ns != current_ns)
	  {
	     if (current_ns != -100)
	       {
		  print_row (current_ns, count_array, sum_array);
	       }
	     current_ns = ns;
	     for (int i = 0 ; i < 9 ; ++ i)
	       count_array[i] = 0;
	  }
	if (links < 1)
	  impossible (ns, links, count);
	else if (links <= 5)
	  count_array[links - 1] += count;
	else if (links <= 10)
	  count_array[5] += count;
	else if (links <= 25)
	  count_array[6] += count;
	else if (links <= 100)
	  count_array[7] += count;
	else
	  count_array[8] += count;
     }
   print_row (current_ns, count_array, sum_array);
   print_row (1000, sum_array, count_array);
   printf ("|}\n");
   printf ("Besides there are %d items with no links.\n", no_links);
   mysql_free_result (result);
}
