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
   
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT p_id, pl_text, p_datatype, "
	 "(SELECT COUNT(*) FROM plabel WHERE pl_id = p_id), "
	 "(SELECT COUNT(*) FROM pdescr WHERE pd_id = p_id), "
	 "(SELECT COUNT(*) FROM palias WHERE pa_id = p_id), "
	 "(SELECT COUNT(DISTINCT pa_lang) FROM palias WHERE pa_id = p_id), "
	 "(SELECT COUNT(*) FROM statement WHERE s_property = p_id), "
	 "(SELECT COUNT(DISTINCT s_item) FROM statement WHERE s_property = p_id), "
	 "(SELECT COUNT(*) FROM qualifier WHERE q_property = p_id), "
	 "(SELECT COUNT(*) FROM refclaim WHERE rc_property = p_id), "
	 "(SELECT COUNT(DISTINCT rc_statement, rc_refno) "
	 "        FROM refclaim WHERE rc_property = p_id) "
	 "FROM property "
	 "LEFT JOIN plabel ON p_id = pl_id AND pl_lang = 'en' "
	 "ORDER BY 1");

   printf ("{| class=\"wikitable sortable\"\n");
   printf ("|-\n");
   printf ("! Property\n");
   printf ("! Label (English)\n");
   printf ("! Datatype\n");
   printf ("! data-sort-type=\"number\"|# of labels\n");
   printf ("! data-sort-type=\"number\"|# of descriptions\n");
   printf ("! data-sort-type=\"number\"|# of aliases\n");
   printf ("! data-sort-type=\"number\"|# of languages with aliases\n");
   printf ("! data-sort-type=\"number\"|# of claims\n");
   printf ("! data-sort-type=\"number\"|# of items with claims\n");
   printf ("! data-sort-type=\"number\"|# of qualifiers\n");
   printf ("! data-sort-type=\"number\"|# of uses in sources\n");
   printf ("! data-sort-type=\"number\"|# of sources\n");

   MYSQL_ROW row;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *p = row[0];
	const char *en = row[1];
	const char *datatype = row[2];
	const char *labels = row[3];
	const char *descriptions = row[4];
	const char *aliases = row[5];
	const char *alias_langs = row[6];
	const char *claims = row[7];
	const char *claim_items = row[8];
	const char *qualifiers = row[9];
	const char *source_uses = row[10];
	const char *sources = row[11];
	printf ("|-\n"
		"| data-sort-value='%s' | [[Property:P%s|P:P%s]]\n"
		"| %s || %s || %s || %s || %s || %s || %s || %s || %s || %s || %s\n",
		p, p, p,
		en, datatype, labels, descriptions, aliases, alias_langs,
		claims, claim_items, qualifiers, source_uses, sources);
     }
   printf ("|}\n");

   mysql_free_result (result);
   close_database (mysql);
   return 0;
}
