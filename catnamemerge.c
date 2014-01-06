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

// For open_memstream()
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include "wikidatalib.h"
#include <string.h>
#include <stdlib.h>

int cat_merges = 0;
int cat_no_item1 = 0;
int cat_no_item2 = 0;
int cat_same_item = 0;
int art_merges = 0;
int art_no_item1 = 0;
int art_no_item2 = 0;
int art_same_item = 0;
int other_ns = 0;
int cat_conflicts = 0;
int art_conflicts = 0;

void find_candidates (MYSQL *mysql, FILE *conflict_fp, const char *lang1, const char *lang2,
		      int level, const char *cat,
		      int page_id1, int page_id2, int item1, int item2)
{
   // Write merge proposal if different items
   if (item1 && item2 && item1 != item2)
     {
	// Check if the items have links for the same langauge
	MYSQL_RES *result = do_query_use
	  (mysql,
	      "SELECT COUNT(DISTINCT k_lang), "
	      " (SELECT i_links FROM item WHERE i_id = %d), "
	      " (SELECT i_links FROM item WHERE i_id = %d) "
	      "FROM link "
	      "WHERE k_id IN (%d, %d)",
	      item1, item2,
	      item1, item2);
	MYSQL_ROW row = mysql_fetch_row (result);
	int distinct_links = atoi (row[0]);
	int links1 = atoi (row[1]);
	int links2 = atoi (row[2]);
	row = mysql_fetch_row (result);
	if (row)
	  {
	     printf ("Unexpected row fetched from MySQL.\n");
	     exit (1);
	  }
	mysql_free_result (result);
	
	if (links1 + links2 > distinct_links)
	  {
	     ++ cat_conflicts;
	     fprintf (conflict_fp,
		      "* %d:Category:%s: [[Q%d]] (%d link%s), [[Q%d]] (%d link%s)\n",
		      level, cat,
		      item1, links1, links1 > 1 ? "s" : "",
		      item2, links2, links2 > 1 ? "s" : "");
	  }
	else
	  {
	     ++ cat_merges;
	     printf ("* %d:Category:%s: [[Q%d]] [[Q%d]]\n",
		     level, cat, item1, item2);
	  }
     }
   if (item1 == 0)
     ++ cat_no_item1;
   if (item2 == 0)
     ++ cat_no_item2;
   if (item1 && item1 == item2)
     ++ cat_same_item;

   // Find members of the category in lang1
   // where pages with same namespace and title in lang2 are member of the category in lang2 
   MYSQL_RES *result = do_query_store
     (mysql,
	 "SELECT p1.page_id, p1.page_item, p2.page_id, p2.page_item, p1.page_namespace, p1.page_title "
	 "FROM %swiki.categorylinks cl1 "
	 "JOIN %swiki.page p1 "
	 "  ON cl1.cl_from = p1.page_id "
	 "JOIN %swiki.page p2 "
	 "  ON p1.page_namespace = p2.page_namespace AND p1.page_title = p2.page_title "
	 "JOIN %swiki.categorylinks cl2 "
	 "  ON cl2.cl_from = p2.page_id "
	 "WHERE cl1.cl_to_id = %d AND cl2.cl_to_id = %d",
	 lang1, lang1,
	 lang2, lang2,
	 page_id1, page_id2);
   MYSQL_ROW row;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int page_id1 = atoi(row[0]);
	int item1 = atoi (row[1]);
	int page_id2 = atoi (row[2]);
	int item2 = atoi (row[3]);
	int ns = atoi (row[4]);
	const char *title = row[5];

	if (ns == 14)
	  find_candidates (mysql, conflict_fp, lang1, lang2, level + 1, title,
			   page_id1, page_id2, item1, item2);
	else if (ns == 0)
	  {
	     if (item1 && item2 && item1 != item2)
	       {
		  // Check if the items have links for the same langauge
		  MYSQL_RES *result = do_query_use
		    (mysql,
			"SELECT COUNT(DISTINCT k_lang), "
			" (SELECT i_links FROM item WHERE i_id = %d), "
			" (SELECT i_links FROM item WHERE i_id = %d) "
			"FROM link "
			"WHERE k_id IN (%d, %d)",
			item1, item2,
			item1, item2);
		  MYSQL_ROW row = mysql_fetch_row (result);
		  int distinct_links = atoi (row[0]);
		  int links1 = atoi (row[1]);
		  int links2 = atoi (row[2]);
		  row = mysql_fetch_row (result);
		  if (row)
		    {
		       printf ("Unexpected row fetched from MySQL.\n");
		                     exit (1);
		    }
		  mysql_free_result (result);

		  if (links1 + links2 > distinct_links)
		    {
		       ++ art_conflicts;
		       fprintf (conflict_fp,
				"* %d:Article:%s: [[Q%d]] (%d link%s), [[Q%d]] (%d link%s)\n",
				level, title,
				item1, links1, links1 > 1 ? "s" : "",
				item2, links2, links2 > 1 ? "s" : "");
		    }
		  else
		    {
		       ++ art_merges;
		       printf ("* %d:Article:%s: [[Q%d]] [[Q%d]]\n",
			       level + 1, title, item1, item2);
		    }
	       }
	     
	     if (item1 == 0)
	       ++ art_no_item1;
	     if (item2 == 0)
	       ++ art_no_item2;
	     if (item1 && item1 == item2)
	       ++ art_same_item;
	  }
	else
	  {
	     ++ other_ns;
	  }
     }
   mysql_free_result (result);
   return;
}

int main (int argc, char *argv[])
{
   if (argc < 4)
     {
	printf ("Usage: %s lang1 lang2 start-category [wikidata-database]\n",
		argv[0]);
	return 1;
     }
   const char *lang1 = argv[1];
   const char *lang2 = argv[2];
   const char *start_cat = argv[3];
   
   const char *database;
   if (argc < 5)
     database = "wikidatawiki";
   else
     database = argv[4];
   
   MYSQL *mysql = open_named_database (database);
   MYSQL_ROW row;

   // Find page_id and item for the start category in lang1
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT page_id, page_item "
	 "FROM %swiki.page "
	 "WHERE page_title = '%s' AND page_namespace = 14",
	 lang1, start_cat);
   row = mysql_fetch_row (result);
   if (! row)
     {
	// The category doesn't exist in lang1
	mysql_free_result (result);
	printf ("Could not find the start category [[:%s:Category:%s]]\n",
		lang1, start_cat);
	return 0;
     }
   int page_id1 = atoi (row[0]);
   int item1 = atoi (row[1]);
   mysql_fetch_row (result);
   mysql_free_result (result);

   // Find page_id and item for the start category in lang2
   result = do_query_use
     (mysql,
	 "SELECT page_id, page_item "
	 "FROM %swiki.page "
	 "WHERE page_title = '%s' AND page_namespace = 14",
	 lang2, start_cat);
   row = mysql_fetch_row (result);
   if (! row)
     {
	// The category doesn't exist in lang2
	mysql_free_result (result);
	printf ("Could not find the start category [[:%s:Category:%s]]\n",
		lang2, start_cat);
	return 0;
     }
   int page_id2 = atoi (row[0]);
   int item2 = atoi (row[1]);
   mysql_fetch_row (result);
   mysql_free_result (result);
   
   char *conflict_buf;
   size_t conflict_buf_size;
   FILE *conflict_fp = open_memstream (& conflict_buf, & conflict_buf_size);

   printf ("== Merge candidates based on pages with identical names in %s: and %s: "
	   "in the category trees starting at %s ==\n",
	   lang1, lang2, start_cat);

   int level = 1;
   find_candidates (mysql, conflict_fp, lang1, lang2, level, start_cat,
		    page_id1, page_id2, item1, item2);
   close_database (mysql);

   fclose (conflict_fp);
   printf ("== Merge conflicts ==\n"
	   "These items are merge candidates, but have links to the same language:\n"
	   "%s",
	   conflict_buf);

   printf ("== Statistics ==\n"
	   "Found:\n"
	   "* %d candidates for merging items for categories without link conflicts\n"
	   "* %d candidates for merging items for articles without link conflicts\n"
	   "* %d candidates for merging items for categories with link conflicts\n"
	   "* %d candidates for merging items for articles with link conflicts\n"
	   "* %d possible candidates in other namespaces (skipped)\n"
	   "* %d categories in same item\n"
	   "* %d articles in same item\n"
	   "* %d %s categories and %d %s categories with no item\n"
	   "* %d %s articles and %d %s articles with no item\n",
	   cat_merges, art_merges,
	   cat_conflicts, art_conflicts,
	   other_ns,
	   cat_same_item, art_same_item,
	   cat_no_item1, lang1, cat_no_item2, lang2,
	   art_no_item1, lang1, art_no_item2, lang2);
   return 0;
}
