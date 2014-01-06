#include "wikidatalib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

int main (int argc, const char *argv[])
{
   if (argc != 4)
   {
      printf ("Usage: %s language-code wp-database wikidata-database\n", argv[0]);
      return 2;
   }
   const char *lang = argv[1];
   const char *wp_db = argv[2];
   const char *wd_db = argv[3];

   setlocale (LC_CTYPE, "");

   MYSQL *mysql = open_named_database (wp_db);

   do_query (mysql,
	     "LOCK TABLE page WRITE, %s.link WRITE",
	     wd_db);

   printf ("Dropping key on page_item ...\n", wp_db);
   fflush (stdout);
   do_query (mysql,
	     "ALTER TABLE page DROP KEY page_item");

   printf ("Clearing %s.page_item ...\n", wp_db);
   fflush (stdout);
   do_query (mysql,
	     "UPDATE page "
	     "SET page_item = 0");
	     
   printf ("Updating %s.page_item for connected pages ...\n", wp_db);
   fflush (stdout);
   do_query (mysql,
	     "UPDATE page "
	     "JOIN %s.link "
	     "  ON k_lang = '%s' AND "
	     "     page_namespace = k_ns AND "
	     "     page_title = k_text "
	     "SET page_item = k_id ",
	     wd_db, lang);

   add_index ("page", mysql, "ADD INDEX(page_item)");

   do_query (mysql, "UNLOCK TABLE");

   close_database (mysql);
   return 0;
}
