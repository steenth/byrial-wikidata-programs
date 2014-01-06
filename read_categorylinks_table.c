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

   do_query (mysql, "DROP TABLE IF EXISTS categorylinks");
   if (! table_exists (mysql, "categorylinks"))
   {
      printf ("Creating table %s.categorylinks\n", database);
      do_query (mysql,
		"CREATE TABLE categorylinks ("
		"cl_from INT UNSIGNED NOT NULL,"
		"cl_to VARCHAR(255) NOT NULL,"
		"cl_to_id INT UNSIGNED NOT NULL)"
		"ENGINE=MyISAM "
		"CHARSET binary");	  
      do_query (mysql,
		"LOCK TABLE categorylinks WRITE");
      
      FILE *fp = stdin;
      
      unsigned int inserts = 0;
      unsigned int rows = 0;
      for (;;)
      {
	 read_to (fp, "VALUES");
	 if (feof (fp))
	 {
	    break;
	 }
	 
	 // We only want the two first columns:
	 //   cl_from int(8) unsigned NOT NULL DEFAULT '0',
	 //   cl_to varbinary(255) NOT NULL DEFAULT '',
	 ++ inserts;
	 int delimiter = ',';
	 for ( ; delimiter == ',' ; )
	 {
	    unsigned int cl_from;
	    int n = fscanf (fp, " (%u,'",
			    &cl_from);
	    if (n != 1)
	    {
	       printf ("fscanf() failed, n=%d\n", n);
	       return 1;
	    }
	    char cl_to[255];
	    read_varchar (fp, cl_to, sizeof cl_to);
	    
	    do_query (mysql,
		      "INSERT INTO categorylinks "
		      "VALUES (%d, '%s', 0)",
		      cl_from, cl_to);
	    ++ rows;
	    pass_n_apostrophes (fp, 10);
	    delimiter = getc (fp);
	    if (delimiter != ')') 
	      {
		 printf ("Expected ) to finish a row, got '%c'\n", delimiter);
		 return 1;
	      }	     
	    delimiter = getc (fp);
	    if (rows % 1000 == 0)
	    {
	       printf ("\r%u rows in %u insert statements",
		       rows, inserts);
	       fflush (0);
	    }
	 }
	 if (delimiter != ';')
	 {
	    printf ("Unexpected delimiter value: '%c'\n", delimiter);
	    return 1;
	 }	     
      }	
      printf ("\rTotal: %u rows in %u insert statements\n",
	      rows, inserts);

      do_query (mysql, "UNLOCK TABLE");	

      add_index ("categorylinks", mysql, "ADD INDEX(cl_from), ADD INDEX(cl_to)");

      puts ("Fill in categorylinks.cl_to_id");
      do_query (mysql,
		"UPDATE categorylinks, page "
		"SET cl_to_id = page_id "
		"WHERE cl_to = page_title AND page_namespace = 14");

      add_index ("categorylinks", mysql, "ADD INDEX(cl_to_id)");
   }
   
   close_database (mysql);
   return 0;
}
