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
   int p;

   if (argc < 3)
     database = "wikidatawiki";
   else
     database = argv[2];
   if (argc < 2 || ! (p = atoi(argv[1])) > 0)
     {	
	printf ("Usage: %s Property-number [database]\n", argv[0]);
	return 1;
     }

   MYSQL *mysql = open_named_database (database);
   MYSQL_ROW row;   
   print_dumpdate (mysql);

   MYSQL_RES *result;
   printf ("Statistics for property {{P|%d}}.\n",
	   p);

   printf ("== {{P|%d}} used in claims ==\n", p);

   // TO FIX: Use of somevalue, novalue
   result  = do_query_use
     (mysql,
	 "SELECT count(*), s_datatype, s_item_value, "
	 "  (SELECT l_text FROM label WHERE l_lang = 'en' AND l_id = s_item_value) "
	 "FROM statement "
	 "WHERE s_property = %d "
	 "GROUP BY s_datatype, s_item_value",
	 p);
   printf ("{| class=wikitable\n");
   printf ("! Value\n");
   printf ("! Count\n");
   int sum1 = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int count = atoi(row[0]);
	sum1 += count;
	const char *datatype = row[1];
	const char *item = row[2];
	const char *litem = row[3];
	printf ("|-\n");
	if (strcmp (datatype, "item") == 0)
	  printf ("| [[Q%s]] (%s)", item, litem ? litem : "?");
	else
	  printf ("| %s", datatype);
	printf ("|| %d\n", count);
     }
   printf ("|-\n! Sum || %d\n", sum1);
   printf ("|}\n");
   mysql_free_result (result);

   printf ("== Number of claims with {{P|%d}} per item ==\n", p);

   result  = do_query_use
     (mysql,
	 "SELECT count(*), s_item_property_count "
	 "FROM item "
	 "LEFT JOIN statement "
	 "ON i_id = s_item AND s_property = %d "
	 "GROUP BY s_item_property_count",
	 p);
   printf ("{| class=wikitable\n");
   printf ("! # of claims\n");
   printf ("! Count\n");
   int sum_claims_pr_item = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int count = atoi(row[0]);
	int claims = row[1] ? atoi(row[1]) : 0;
	if (claims > 1)
	  count /= claims;
	sum_claims_pr_item += count;
	printf ("|-\n");
	printf ("| %d || %d\n", claims, count);
     }
   printf ("|-\n! Sum || %d\n", sum_claims_pr_item);
   printf ("|}\n");
   mysql_free_result (result);

   printf ("== Qualifiers used in claims with {{P|%d}} ==\n", p);

   result  = do_query_use
     (mysql,
	 "SELECT count(*), q_property, "
	 "  (SELECT pl_text FROM plabel WHERE pl_lang = 'en' AND pl_id = q_property) "
	 "FROM statement "
	 "LEFT JOIN qualifier ON q_statement = s_statement "
	 "WHERE s_property = %d "
	 "GROUP BY q_property",
	 p);
   printf ("{| class=wikitable\n");
   printf ("! Property\n");
   printf ("! Count\n");
   int sum2 = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int count = atoi(row[0]);
	sum2 += count;
	const char *prop = row[1];
	const char *lprop = row[2];
	printf ("|-\n");
	if (prop)
	  printf ("| [[P:P%s]] (%s)", prop, lprop ? lprop : "?");
	else
	  printf ("| none");
	printf ("|| %d\n", count);
     }
   printf ("|-\n! Sum || %d\n", sum2);
   printf ("|}\n");
   if (sum2 > sum1)
     printf ("Note: The sum can be greater than the number of claims with P%d as one claim "
	     "can have several qualifiers.\n", p);
   mysql_free_result (result);

   printf ("== List of claims with {{P|%d}} having qualifiers ==\n", p);

   result  = do_query_use
     (mysql,
	 "SELECT s_item, s_qualifiers, "
	 "  (SELECT l_text FROM label WHERE l_lang = 'en' AND l_id = s_item) "
	 "FROM statement "
	 "WHERE s_property = %d AND s_qualifiers > 0",
	 p);
   printf ("{| class=wikitable\n");
   printf ("! Item\n");
   printf ("! # of qualifiers\n");
   const int limit = 25;
   int count = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	++ count;
	if (count > limit)
	  continue;
	const char *item = row[0];
	const char *qs = row[1];
	const char *l = row[2];
	printf ("|-\n");
	printf ("| [[Q%s]] (%s)", item, l ? l : "?");
	printf ("|| %s\n", qs);
     }
   printf ("|}\n");
   printf ("%d statements found.", count);
   if (count > limit)
     printf ("(Output limited to %d statements.)", limit);
   printf ("\n");
   mysql_free_result (result);

   printf ("== Reference claims used in claims with {{P|%d}} ==\n", p);

   result  = do_query_use
     (mysql,
	 "SELECT count(*), rc_property, "
	 "  (SELECT pl_text FROM plabel WHERE pl_lang = 'en' AND pl_id = rc_property), "
	 "  rc_item_value, "
	 "  (SELECT l_text FROM label WHERE l_lang = 'en' AND l_id = rc_item_value) "
	 "FROM statement "
	 "LEFT JOIN refclaim ON rc_statement = s_statement "
	 "WHERE s_property = %d "
	 "GROUP BY rc_property, rc_item_value",
	 p);
   printf ("{| class=wikitable\n");
   printf ("! Property: value\n");
   printf ("! Count\n");
   int sum3 = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int count = atoi(row[0]);
	sum3 += count;
	const char *prop = row[1];
	const char *lprop = row[2];
	const char *value = row[3];
	const char *lvalue = row[4];
	printf ("|-\n");
	if (prop)
	  {
	     printf ("| [[P:P%s]] (%s)", prop, lprop ? lprop : "?");
	     if (value && *value != '0')
	       printf (": [[Q%s]] (%s)", value, lvalue ? lvalue : "?");
	  }
	else
	  printf ("| none");
	printf ("|| %d\n", count);
     }
   printf ("|-\n! Sum || %d\n", sum3);
   printf ("|}\n");
   if (sum3 > sum1)
     printf ("Note: The sum can be greater than the number of claims with P%d as one claim "
	     "can have several sources and several claims for each source.\n", p);
   mysql_free_result (result);

   printf ("== {{P|%d}} used in qualifiers ==\n", p);

   result  = do_query_use
     (mysql,
	 "SELECT count(*), q_datatype, q_item_value, "
	 "  (SELECT l_text FROM label WHERE l_lang = 'en' AND l_id = q_item_value) "
	 "FROM qualifier "
	 "WHERE q_property = %d "
	 "GROUP BY q_datatype, q_item_value",
	 p);
   printf ("{| class=wikitable\n");
   printf ("! Value\n");
   printf ("! Count\n");
   int sumq = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int count = atoi(row[0]);
	sumq += count;
	const char *datatype = row[1];
	const char *value = row[2];
	const char *lvalue = row[3];
	printf ("|-\n");
	if (strcmp (datatype, "item") == 0)
	  printf ("| [[Q%s]] (%s)", value, lvalue ? lvalue : "?");
	else
	  printf ("| %s", datatype);
	printf ("|| %d\n", count);
     }
   printf ("|-\n! Sum || %d\n", sumq);
   printf ("|}\n");
   mysql_free_result (result);

   printf ("== {{P|%d}} used in references ==\n", p);

   result  = do_query_use
     (mysql,
	 "SELECT count(*), rc_datatype, rc_item_value, "
	 "  (SELECT l_text FROM label WHERE l_lang = 'en' AND l_id = rc_item_value) "
	 "FROM refclaim "
	 "WHERE rc_property = %d "
	 "GROUP BY rc_datatype, rc_item_value",
	 p);
   printf ("{| class=wikitable\n");
   printf ("! Value\n");
   printf ("! Count\n");
   int sumrc = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int count = atoi(row[0]);
	sumrc += count;
	const char *datatype = row[1];
	const char *value = row[2];
	const char *lvalue = row[3];
	printf ("|-\n");
	if (strcmp (datatype, "item") == 0)
	  printf ("| [[Q%s]] (%s)", value, lvalue ? lvalue : "?");
	else
	  printf ("| %s", datatype);
	printf ("|| %d\n", count);
     }
   printf ("|-\n! Sum || %d\n", sumrc);
   printf ("|}\n");
   mysql_free_result (result);

   close_database (mysql);
   return 0;
}
