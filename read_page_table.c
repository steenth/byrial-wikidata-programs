#include "wikidatalib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

int main (int argc, const char *argv[])
{
   if (argc != 2)
   {
      printf ("Usage: %s DATABASE\n", argv[0]);
      return 2;
   }
   const char *database = argv[1];

   setlocale (LC_CTYPE, "");

   MYSQL *mysql = open_named_database (database);

   do_query (mysql, "DROP TABLE IF EXISTS page");
   if (! table_exists (mysql, "page"))
     {
	printf ("Creating table %s.page\n", database);
	do_query (mysql,
		  "CREATE TABLE page ("
		  "page_id INT UNSIGNED NOT NULL, "
		  "page_namespace SMALLINT NOT NULL, "
		  "page_title VARCHAR(255) NOT NULL, "
		  "page_is_redirect INT(1) NOT NULL, "
		  "page_item INT UNSIGNED NOT NULL) "
		  "ENGINE=MyISAM "
		  "CHARSET binary");	  
	do_query (mysql,
		  "LOCK TABLE page WRITE");
      
	FILE *fp = stdin;
      
	unsigned int inserts = 0;
	unsigned int rows = 0;
	unsigned int skipped = 0;
	unsigned int redirects = 0;
	for (;;)
	  {
	     read_to (fp, "VALUES");
	     if (feof (fp))
	       {
		  break;
	       }
	 
	     // We only want the first columns: id, namespace, title
	     // And the 6. column: is_redirect
	     ++ inserts;
	     int delimiter = ',';
	     for ( ; delimiter == ',' ; )
	       {
		  unsigned int page_id;
		  int page_namespace;
		  int n = fscanf (fp, " (%u,%d,'",
				  &page_id, &page_namespace);
		  if (n != 2)
		    {
		       printf ("fscanf() failed, n=%d\n", n);
		       return 1;
		    }

		  char page_title[512];
		  read_varchar (fp, page_title, sizeof page_title);
		  // printf ("Found: %u, %d:%s\n", page_id, page_namespace, page_title); fflush (0);

		  bool skip_this =
		    (page_namespace % 2 == 1) || // Skip all talk pages
		    (page_namespace == 2); // Skip user pages

		  if (getc (fp) != ',' || getc (fp) != '\'')
		    {
		       printf ("Didn't find ,' after page_title\n");
		       return 1;
		    }

		  char page_restrictions[512];
		  read_varchar (fp, page_restrictions, sizeof page_restrictions);
		  if (* page_restrictions)
		    {
		       // printf ("Restrictions for %s: %s\n", page_title, page_restrictions);
		    }

		  int is_redirect;
		  int res = fscanf (fp, ",%*d,%d", &is_redirect);
		  if (res != 1)
		    {
		       printf ("fscanf() for is_redirect failed, pageid=%d, ns=%d\n",
			       page_id, page_namespace);
		       for (int i = 0 ; i < 20 ; ++i ) putchar(fgetc(fp));
		       return 1;
		    }
		  if (is_redirect != 0 && is_redirect != 1)
		    {
		       printf ("is_redirect has unexpected value %d\n", is_redirect);
		       return 1;
		    }
		  redirects += is_redirect;
		  
		  if (skip_this)
		    ++ skipped;
		  else
		    do_query (mysql,
			      "INSERT INTO page "
			      "VALUES (%d, %d, '%s', %d, 0)",
			      page_id, page_namespace, page_title, is_redirect);
		  ++ rows;

		  read_to (fp, ")");
		  delimiter = getc (fp);
		  if (rows % 1000 == 0)
		    {
		       printf ("\r%u rows (%u skipped, %u redirects) in %u insert statements",
			       rows, skipped, redirects, inserts);
		       fflush (0);
		    }
	       }
	     if (delimiter != ';')
	       {
		  printf ("Unexpected delimiter value: '%c'\n", delimiter);
		  return 1;
	       }
	  }
	printf ("\rTotal: %u rows (%u skipped, %u redirects) in %u insert statements\n",
		rows, skipped, redirects, inserts);

	do_query (mysql, "UNLOCK TABLE");	

	add_index ("page", mysql,
		   "ADD INDEX(page_id),"
		   "ADD INDEX(page_namespace,page_title),"
		   "ADD INDEX(page_item)");
     }

   close_database (mysql);
   return 0;
}
