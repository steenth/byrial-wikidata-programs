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
   
   printf ("== Number of labels, descriptions, aliases and sitelinks "
	   "for items per language ==\n");
   
   MYSQL_RES *item_result = do_query_store
     (mysql,
	 "SELECT COUNT(*) FROM item");
   MYSQL_ROW row = mysql_fetch_row (item_result);
   const int items = atoi (row[0]);

   do_query (mysql,
	     "CREATE TEMPORARY TABLE t1 ("
	     "t1_lang VARCHAR(32) NOT NULL,"
	     "t1_labels MEDIUMINT UNSIGNED NOT NULL,"
	     "t1_descs MEDIUMINT UNSIGNED NOT NULL,"
	     "t1_aliases MEDIUMINT UNSIGNED NOT NULL,"
	     "t1_items_with_aliases MEDIUMINT UNSIGNED NOT NULL,"
	     "t1_w_links MEDIUMINT UNSIGNED NOT NULL,"
	     "t1_y_links MEDIUMINT UNSIGNED NOT NULL,"
	     "UNIQUE KEY (t1_lang))"
	     "CHARSET binary");
   do_query (mysql,
	     "INSERT INTO t1 (t1_lang, t1_labels) "
	     "SELECT l_lang, COUNT(l_id) "
	     "FROM item "
	     "JOIN label ON i_id = l_id "
	     "GROUP BY l_lang");

   do_query (mysql,
	     "CREATE TEMPORARY TABLE t2 ("
	     "t2_lang VARCHAR(32) NOT NULL,"
	     "t2_descs MEDIUMINT UNSIGNED NOT NULL)"
	     "CHARSET binary");
   do_query (mysql,
	     "INSERT INTO t2 (t2_lang, t2_descs) "
	     "SELECT d_lang, COUNT(d_id) "
	     "FROM item "
	     "JOIN descr ON i_id = d_id "
	     "GROUP BY d_lang");
   do_query (mysql,
	     "INSERT INTO t1 (t1_lang, t1_descs) "
	     "SELECT t2_lang, t2_descs "
	     "FROM t2 "
	     "ON DUPLICATE KEY UPDATE "
	     "t1_descs = t2_descs");

   do_query (mysql,
	     "CREATE TEMPORARY TABLE t3 ("
	     "t3_lang VARCHAR(32) NOT NULL,"
	     "t3_aliases MEDIUMINT UNSIGNED NOT NULL)"
	     "CHARSET binary");
   do_query (mysql,
	     "INSERT INTO t3 (t3_lang, t3_aliases) "
	     "SELECT a_lang, COUNT(a_id) "
	     "FROM item "
	     "JOIN alias ON i_id = a_id "
	     "GROUP BY a_lang");
   do_query (mysql,
	     "INSERT INTO t1 (t1_lang, t1_aliases) "
	     "SELECT t3_lang, t3_aliases "
	     "FROM t3 "
	     "ON DUPLICATE KEY UPDATE "
	     "t1_aliases = t3_aliases");

   do_query (mysql,
	     "CREATE TEMPORARY TABLE t4 ("
	     "t4_lang VARCHAR(32) NOT NULL,"
	     "t4_items_with_aliases MEDIUMINT UNSIGNED NOT NULL)"
	     "CHARSET binary");
   do_query (mysql,
	     "INSERT INTO t4 (t4_lang, t4_items_with_aliases) "
	     "SELECT a_lang, COUNT(DISTINCT a_id) "
	     "FROM item "
	     "JOIN alias ON i_id = a_id "
	     "GROUP BY a_lang");
   do_query (mysql,
	     "INSERT INTO t1 (t1_lang, t1_items_with_aliases) "
	     "SELECT t4_lang, t4_items_with_aliases "
	     "FROM t4 "
	     "ON DUPLICATE KEY UPDATE "
	     "t1_items_with_aliases = t4_items_with_aliases");

   do_query (mysql,
	     "CREATE TEMPORARY TABLE t5 ("
	     "t5_lang VARCHAR(32) NOT NULL,"
	     "t5_project CHAR(1) NOT NULL,"
	     "t5_links MEDIUMINT UNSIGNED NOT NULL)"
	     "CHARSET binary");
   do_query (mysql,
	     "INSERT INTO t5 (t5_lang, t5_project, t5_links) "
	     "SELECT k_lang, k_project, COUNT(k_id) "
	     "FROM link "
	     "GROUP BY k_lang, k_project");
   do_query (mysql,
	     "INSERT INTO t1 (t1_lang, t1_w_links) "
	     "SELECT t5_lang, t5_links "
	     "FROM t5 "
	     "WHERE t5_project = 'w' "
	     "ON DUPLICATE KEY UPDATE "
	     "t1_w_links = t5_links");
   do_query (mysql,
	     "INSERT INTO t1 (t1_lang, t1_y_links) "
	     "SELECT t5_lang, t5_links "
	     "FROM t5 "
	     "WHERE t5_project = 'y' "
	     "ON DUPLICATE KEY UPDATE "
	     "t1_y_links = t5_links");

   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT t1_lang, t1_labels, t1_descs, t1_aliases, t1_items_with_aliases, "
	 "  t1_w_links, t1_y_links "
	 "FROM t1 "
	 "ORDER BY 2 DESC, 1 ASC");

   printf ("{| class=\"wikitable sortable\"\n");
   printf ("|-\n");
   printf ("! Language code\n");
   printf ("! Language (English)\n");
   printf ("! Language (native)\n");
   printf ("! data-sort-type=\"number\"|# of labels\n");
   printf ("! data-sort-type=\"number\"|# of descriptions\n");
   printf ("! data-sort-type=\"number\"|# of aliases\n");
   printf ("! data-sort-type=\"number\"|# of items with aliases\n");
   printf ("! data-sort-type=\"number\"|# of WP links\n");
   printf ("! data-sort-type=\"number\"|# of Voy links\n");

   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *lang = row[0];
	const char *labels = row[1];
	const char *descs = row[2];
	const char *aliases = row[3];
	const char *items_with_aliases = row[4];
	const char *w_links = row[5];
	const char *y_links = row[6];
	printf ("|-\n"
		"| %s || {{#language:%s|en}} || {{#language:%s}}\n"
		"| %s (%.1f%%) || %s (%.1f%%)\n"
		"| %s || %s || %s || %s \n",
		lang, lang, lang,
		labels, 100.0*atoi(labels)/items,
		descs, 100.0*atoi(descs)/items,
		aliases, items_with_aliases,
		*w_links == '0' ? "&nbsp;" : w_links,
		*y_links == '0' ? "&nbsp;" : y_links);
     }

   printf ("|}\n");

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
