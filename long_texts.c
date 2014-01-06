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
#include <locale.h>

#define BYTE_LIMIT "250"

void long_texts (MYSQL *mysql, const char *table, const char *letters, const char *name);

int main (int argc, char *argv[])
{
   const char *database;
   if (argc < 2)
     database = "wikidatawiki";
   else
     database = argv[1];

   setlocale (LC_CTYPE, "");   
   MYSQL *mysql = open_named_database (database);
   print_dumpdate (mysql);
   long_texts (mysql, "label", "l", "Item labels");
   long_texts (mysql, "descr", "d", "Item descriptions");
   long_texts (mysql, "alias", "a", "Item aliases");
   long_texts (mysql, "plabel", "pl", "Property labels");
   long_texts (mysql, "pdescr", "pd", "Property descriptions");
   long_texts (mysql, "palias", "pa", "Property aliases");
   close_database (mysql);
   return 0;
}

char *wiki_escape (const char *text)
{
   static char escaped_text[2000];
   char *dest = escaped_text;
   while (*text)
     {
	if (*text == '<')
	  {
	     *(dest ++) = '&';
	     *(dest ++) = 'l';
	     *(dest ++) = 't';
	     *(dest ++) = ';';
	  }
	else if (*text == '[')
	  {
	     *(dest ++) = '&';
	     *(dest ++) = '#';
	     *(dest ++) = '9';
	     *(dest ++) = '1';
	     *(dest ++) = ';';
	  }
	else if (*text == '{')
	  {
	     *(dest ++) = '&';
	     *(dest ++) = '#';
	     *(dest ++) = '1';
	     *(dest ++) = '2';
	     *(dest ++) = '3';
	     *(dest ++) = ';';
	  }
	else
	  *(dest ++) = *text;
	++text;
     }
   *dest = '\0';
   return escaped_text;
}

void long_texts (MYSQL *mysql, const char *table, const char *letters, const char *name)
{
   printf ("== %s with " BYTE_LIMIT " or more bytes.==\n", name);
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT %s_id, %s_lang, %s_text "
	 "FROM %s "
	 "WHERE LENGTH(%s_text) >= " BYTE_LIMIT " "
	 "ORDER BY LENGTH(%s_text) DESC",
	 letters, letters, letters, table, letters, letters);
   MYSQL_ROW row;
   printf ("{| class='wikitable sortable'\n");
   printf ("! Item !! language !! characters !! bytes !! text\n");
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *item = row[0];
	const char *lang = row[1];
	const char *text = row[2];
	wchar_t wc[2000];
	size_t len = mbstowcs (wc, text, 2000);
	if (len == (size_t) -1)
	  {
	     printf ("mbstowcs failed for Q%s, lang %s: %m\n", item, lang);
	     exit (1);
	  }
	printf ("|-\n");
	printf ("| [[%s%s]] || %s || %zu || %zu || %s\n",
		*letters == 'p' ? "P:P" : "Q",
		item, lang, len, strlen (text), wiki_escape (text));
     }
   printf ("|}\n");
   mysql_free_result (result);
}
