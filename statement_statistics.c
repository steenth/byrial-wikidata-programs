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

   printf ("== Basic numbers ==\n");

   MYSQL_RES *result_1 = do_query_store
     (mysql, "SELECT COUNT(*) FROM item");
   MYSQL_ROW row_1 = mysql_fetch_row (result_1);
   const int items = atoi (row_1[0]);
   printf ("* Number of items: {{formatnum:%d}}\n", items);

   MYSQL_RES *result_2 = do_query_store
     (mysql, "SELECT COUNT(*) FROM statement");
   MYSQL_ROW row_2 = mysql_fetch_row (result_2);
   const int statements = atoi (row_2[0]);
   printf ("* Number of statements: {{formatnum:%d}}\n", statements);

   MYSQL_RES *result_3 = do_query_store
     (mysql, "SELECT COUNT(DISTINCT s_item) FROM statement");
   MYSQL_ROW row_3 = mysql_fetch_row (result_3);
   const int items_with_statements = atoi (row_3[0]);
   printf ("* Number of items with statements: {{formatnum:%d}}\n", items_with_statements);

   MYSQL_RES *result_4 = do_query_store
     (mysql, "SELECT COUNT(*) FROM statement WHERE s_refs > 0");
   MYSQL_ROW row_4 = mysql_fetch_row (result_4);
   const int statements_with_sources = atoi (row_4[0]);
   printf ("* Number of statements with sources: {{formatnum:%d}}\n", statements_with_sources);

   MYSQL_RES *result_5 = do_query_store
     (mysql, "SELECT COUNT(*) FROM ref");
   MYSQL_ROW row_5 = mysql_fetch_row (result_5);
   const int sources = atoi (row_5[0]);
   printf ("* Number of sources: {{formatnum:%d}}\n", sources);

   MYSQL_RES *result_6 = do_query_store
     (mysql, "SELECT COUNT(*) FROM refclaim");
   MYSQL_ROW row_6 = mysql_fetch_row (result_6);
   const int source_properties = atoi (row_6[0]);
   printf ("* Number of properties used in sources: {{formatnum:%d}}\n", source_properties);

   // items fordelt efter antal statements

   printf ("== Items after their number of statements ==\n");
   MYSQL_ROW row;
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT COUNT(*), i_statements "
	 "FROM item "
	 "GROUP BY i_statements "
	 "ORDER BY i_statements ");
   int statements_11_100 [9] = { 0 };
   int statements_101_500 [20] = { 0 };
   printf ("{| class=wikitable\n"
	   "! Items !! # of statements\n");
   int sum_items = 0;
   int max_statements = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int items = atoi(row[0]);
	int statements = atoi(row[1]);

	sum_items += items;
	if (statements > max_statements)
	  max_statements = statements;

	if (statements <= 10)
	  printf ("|-\n"
		  "| %d || %d\n",
		  items, statements);
	else if (statements <= 100)
	  statements_11_100 [ (statements - 11) / 10 ] += items;
	else if (statements <= 500)
	  statements_101_500 [ (statements - 101) / 25 ] += items;
	else
	  // I guess will not happen - but let me see if it does
	  printf ("|-\n"
		  "| %d || %d\n",
		  items, statements);
     }

   for (int i = 0 ; i < 9 ; ++i)
     if (statements_11_100[i])
       printf ("|-\n"
	       "| %d || %d-%d\n",
	       statements_11_100[i],
	       i*10+11, i*10+20);

   for (int i = 0 ; i < 20 ; ++i)
     if (statements_101_500[i])
       printf ("|-\n"
	       "| %d || %d-%d\n",
	       statements_101_500[i],
	       i*25+101, i*25+125);
   printf ("|-\n"
	   "! Total %d !! 0-%d\n"
	   "|}\n",
	   sum_items, max_statements);

   // statements fordelt efter antal kilder

   printf ("== Statements after their number of sources ==\n");
   fflush (stdout);
   result = do_query_use
     (mysql,
	 "SELECT COUNT(*), s_refs "
	 "FROM  statement "
	 "GROUP BY s_refs "
	 "ORDER BY s_refs");
   printf ("{| class=wikitable\n"
	   "! Statements || # of sources\n");
   int sum_statements = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *statements = row[0];
	const char *refs = row[1];
	sum_statements += atoi (statements);
	printf ("|-\n"
		"| %s || %s\n",
		statements, refs);
     }
   printf ("|-\n"
	   "! Total %d !! \n"
	   "|}\n",
	   sum_statements);

   // kilder fordelt efter antal egenskaber

   printf ("== Sources after their number of properties ==\n");
   fflush (stdout);
   result = do_query_use
     (mysql,
	 "SELECT COUNT(*), r_claims "
	 "FROM  ref "
	 "GROUP BY r_claims "
	 "ORDER BY r_claims");
   printf ("{| class=wikitable\n"
	   "! Sources || # of properties\n");
   int sum_sources = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *sources = row[0];
	const char *claims = row[1];
	sum_sources += atoi (sources);
	printf ("|-\n"
		"| %s || %s\n",
		sources, claims);
     }
   printf ("|-\n"
	   "! Total %d !! \n"
	   "|}\n",
	   sum_sources);

   // statements fordelt efter antal kvalifikatorer

   printf ("== Statements after their number of qualifiers ==\n");
   fflush (stdout);
   result = do_query_use
     (mysql,
	 "SELECT COUNT(*), s_qualifiers "
	 "FROM  statement "
	 "GROUP BY s_qualifiers "
	 "ORDER BY s_qualifiers");
   printf ("{| class=wikitable\n"
	   "! Statements || # of qualifiers\n");
   int sum_statements2 = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *statements = row[0];
	const char *qualifiers = row[1];
	sum_statements2 += atoi (statements);
	printf ("|-\n"
		"| %s || %s\n",
		statements, qualifiers);
     }
   printf ("|-\n"
	   "! Total %d !! \n"
	   "|}\n",
	   sum_statements2);

   // egenskaber fordelt efter valuetype

   printf ("== Properties after their value type ==\n");
   fflush (stdout);
   result = do_query_use
     (mysql,
	 "SELECT s_datatype, "
	 "  COUNT(*), "
	 "  (SELECT COUNT(*) FROM refclaim WHERE rc_datatype = s_datatype), "
	 "  (SELECT COUNT(*) FROM qualifier WHERE q_datatype = s_datatype) "
	 "FROM statement "
	 "GROUP BY s_datatype "
	 "ORDER BY s_datatype");
   printf ("{| class=wikitable\n"
	   "! Value type !! # of claims !! # of reference properties !! # of qualifiers\n");
   int sum_claims = 0;
   int sum_refclaims = 0;
   int sum_qualifiers = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *datatype = row[0];
	const char *claims = row[1];
	const char *refclaims = row[2];
	const char *qualifiers = row[3];
	printf ("|-\n"
		"| %s || %s || %s || %s\n",
		datatype, claims, refclaims, qualifiers);
	sum_claims += atoi (claims);
	sum_refclaims += atoi (refclaims);
	sum_qualifiers += atoi (qualifiers);
     }
   printf ("|-\n"
	   "! Total || %d || %d || %d\n"
	   "|}\n",
	   sum_claims, sum_refclaims, sum_qualifiers);

/* Included in properties after their value type 
   // påstande fordelt efter valuetype

   printf ("== Claims after their value type ==\n");
   fflush (stdout);
   result = do_query_use
     (mysql,
	 "SELECT COUNT(*), s_datatype "
	 "FROM statement "
	 "GROUP BY s_datatype "
	 "ORDER BY s_datatype");
   printf ("{| class=wikitable\n"
	   "! Value type !! # of claims\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *claims = row[0];
	const char *datatype = row[1];
	printf ("|-\n"
		"| %s || %s\n",
		datatype, claims);
     }
   printf ("|}\n");
*/

/* Included in properties after their value type 
   // egenskaber i kilder fordelt efter valuetype

   printf ("== Properties in sources after their value type ==\n");
   fflush (stdout);
   result = do_query_use
     (mysql,
	 "SELECT COUNT(*), rc_datatype "
	 "FROM refclaim "
	 "GROUP BY rc_datatype "
	 "ORDER BY rc_datatype");
   printf ("{| class=wikitable\n"
	   "! Value type !! # of source properties\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *claims = row[0];
	const char *datatype = row[1];
	printf ("|-\n"
		"| %s || %s\n",
		datatype, claims);
     }
   printf ("|}\n");
*/

   // Anvendte egenskaber i kilder

   printf ("== Properties used in sources ==\n");
   fflush (stdout);
   result = do_query_use
     (mysql,
	 "SELECT COUNT(*), rc_property, "
	 "  (SELECT pl_text FROM plabel WHERE rc_property = pl_id AND pl_lang = 'en') "
	 "FROM refclaim "
	 "GROUP BY rc_property "
	 "ORDER BY 1 DESC");
   printf ("{| class=wikitable\n"
	   "! Property !! # of uses in sources\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *count = row[0];
	const char *property = row[1];
	const char *label = row[2];
	if (label)
	     printf ("|-\n"
		     "| [[P:P%s|%s]] || %s\n",
		     property, label, count);
	else
	     printf ("|-\n"
		     "| [[P:P%s]] || %s\n",
		     property, count);
     }
   printf ("|}\n");

   // mest anvendte items i kilder
   printf ("== Most used items in sources ==\n");
   fflush (stdout);
   result = do_query_use
     (mysql,
	 "SELECT COUNT(*), rc_item_value, "
	 "( SELECT l_text "
	 "  FROM label "
	 "  WHERE l_id = rc_item_value AND l_lang = 'en')"
	 "FROM refclaim "
	 "WHERE rc_item_value != 0 "
	 "GROUP BY rc_item_value "
	 "ORDER BY 1 DESC ");
   printf ("{| class=wikitable\n"
	   "! Item !! # of uses in sources\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int count = atoi(row[0]);
	if (count < 10)
	  break;
	const char *item = row[1];
	const char *label = row[2];
	if (label)
	  printf ("|-\n"
		  "| [[Q%s|%s]] || %d\n",
		  item, label, count);
	else
	  printf ("|-\n"
		  "| [[Q%s]] || %d\n",
		  item, count);
     }
   printf ("|}\n");

   close_database (mysql);
   return 0;
}
