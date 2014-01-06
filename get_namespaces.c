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

#define WIKILIST "list-of-wikis.txt"
#define WIKIPEDIA_LIST_URL   "http://noc.wikimedia.org/conf/wikipedia.dblist"
#define WIKIVOYAGE_LIST_URL  "http://noc.wikimedia.org/conf/wikivoyage.dblist"
#define NAMESPACEFILE "ns.xml"
#define API_FORMAT "http://%s%s/w/api.php?action=query&meta=siteinfo&siprop=namespaces|namespacealiases&format=xml"

void get_namespaces (MYSQL *mysql, const char *list_url, const char *list_suffix,
		     const char *site_suffix, const char *table);

int main (int argc, char *argv[])
{
   (void) argc;
   (void) argv;
   MYSQL *mysql = open_named_database ("wiki");
   get_namespaces (mysql, WIKIPEDIA_LIST_URL, "wiki\n", ".wikipedia.org", "w_namespace");
   get_namespaces (mysql, WIKIVOYAGE_LIST_URL, "wikivoyage\n", ".wikivoyage.org", "y_namespace");
   close_database (mysql);
}

void read_to_char (FILE *fp, int ch)
{
   int read_char;
   while ((read_char = fgetc (fp)) != ch && read_char != EOF)
     ;
}

char *copy_and_escape (char *dest, const char *src)
{
   while (*src)
     {
	if (*src == '\\' || *src == '\'')
	  {
	     *(dest ++) = '\\';
	  }
	*(dest ++) = *(src ++);
     }
   *dest = '\0';
   return dest;
}

void check_canonical_name (FILE *fp_ns, int ns, const char *wiki)
{
   for (;;)
     // We return when the canonical attribute is found.
     // If there is no canocinal attribue, we stop with an error message.
     {
	char attribute[80];
	char value[80];
	int ret = fscanf (fp_ns, " %[a-z]=\"%[^\"]\"",
			  &attribute[0], &value[0]);
	if (ret == 1 && strcmp (attribute, "subpages") == 0)
	  {
	     // The value for the subpages attribute is empty
	     fgetc (fp_ns); // eat '"'
	     continue;
	  }
	if (ret != 2)
	  {
	     printf ("Read of attribute and value failed,"
		     " ret=%d, next charcaters in stream:\n",
		     ret);
	     for (int i = 0 ; i < 25 ; i ++)
	       printf ("%c", fgetc (fp_ns));
	     exit (1);
	  }
	if (strcmp (attribute, "canonical") == 0)
	  {
	     switch (ns)
	       {
		case -2: if (strcmp (value, "Media") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case -1: if (strcmp (value, "Special") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 1: if (strcmp (value, "Talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 2: if (strcmp (value, "User") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 3: if (strcmp (value, "User talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 4: if (strcmp (value, "Project") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 5: if (strcmp (value, "Project talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 6: if (strcmp (value, "File") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 7: if (strcmp (value, "File talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 8: if (strcmp (value, "MediaWiki") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 9: if (strcmp (value, "MediaWiki talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 10: if (strcmp (value, "Template") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 11: if (strcmp (value, "Template talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 12: if (strcmp (value, "Help") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 13: if (strcmp (value, "Help talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 14: if (strcmp (value, "Category") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 15: if (strcmp (value, "Category talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 828: if (strcmp (value, "Module") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
		case 829: if (strcmp (value, "Module talk") != 0)
		    printf ("Note: %s namespace %d have canonical name %s\n",
			    wiki, ns, value);
		  break;
	       }
	     return;
	  }
     }
}

void get_namespaces (MYSQL *mysql, const char *list_url, const char *list_suffix,
		     const char *site_suffix, const char *table)
{
   printf ("Reading namespace names for %s", list_suffix);

   // Get list of Wikis
   char command[200];
   sprintf (command, "curl --silent --show-error --output " WIKILIST " \"%s\"", list_url);
   int ret = system (command);
   if (ret == -1)
     {
	printf ("system() failed.\n");
	exit (1);
     }

   if (table_exists (mysql, table))
     do_query (mysql,
	       "DROP TABLE %s",
	       table);
   do_query (mysql,
	     "CREATE TABLE IF NOT EXISTS %s ("
	     "lang VARCHAR(12) NOT NULL,"
	     "ns SMALLINT NOT NULL,"
	     "name VARCHAR(96) NOT NULL,"
	     "alias BOOL NOT NULL,"
	     "INDEX (lang)) "
	     "ENGINE=MyISAM "
	     "CHARSET binary",
	     table);

   FILE *fp = fopen (WIKILIST, "r");
   if (! fp)
     {
	printf ("fopen() failed.\n");
	exit (1);
     }
   
   int wiki_count = 0;
   size_t max_wiki_len = 0;
   int name_count = 0;
   size_t max_name_len = 0;
   const int list_suffix_len = strlen (list_suffix);
   
   while (1)
     {
	char wiki[60];
	char *p = wiki;
	while ((*p = fgetc (fp)) != '\n' && *p != EOF)
	  {
	     if (*p == '_')
	       *p = '-';
	     ++ p;
	  }
	if (*p == EOF)
	  {
	     if (p != wiki)
	       {
		  printf ("Unexpected FEOF\n");
		  exit (1);
	       }
	     break;
	  }
	if (p - wiki < list_suffix_len + 1)
	  {
	     printf ("Unexpected short line: p - wiki = %td\n", p - wiki);
	     exit (1);
	  }
	if (strncmp (p - list_suffix_len + 1, list_suffix, list_suffix_len) != 0)
	  {
	     printf ("Line does not end in %s", list_suffix);
	     exit (1);
	  }
	wiki[p - wiki - list_suffix_len + 1] = '\0';
	++ wiki_count;
	if (strlen (wiki) > max_wiki_len)
	  max_wiki_len = strlen (wiki);
	printf ("Read \"%s\"\n", wiki);

	char command[200];
	sprintf (command,
		 "curl --silent --show-error --output \"" NAMESPACEFILE
		 "\" \"" API_FORMAT "\"", wiki, site_suffix);
	int ret = system (command);
	if (ret == -1)
	  {
	     printf ("system() failed.\n");
	     exit (1);
	  }
	
	FILE *fp_ns = fopen (NAMESPACEFILE, "r");
	if (! fp_ns)
	  {
	     printf ("fopen() failed.\n");
	     exit (1);
	  }

	enum state { not_started, reading_namespaces, reading_aliases } state = not_started;
	while (! feof (fp_ns))
	  {
	     read_to_char (fp_ns, '<');
	     if (fgetc (fp_ns) != 'n')
	       continue;
	     int char2 = fgetc (fp_ns);
	     int char3 = fgetc (fp_ns);
	     if (char2 == 'a' && char3 == 'm')
	       {
		  if (state == not_started)
		    state = reading_namespaces;
		  else if (state == reading_namespaces)
		    state = reading_aliases;
		  else
		    {
		       printf ("Found unexpected tag starting with <nam\n");
		       exit (1);
		    }
		  continue;
	       }
	     else if (char2  != 's' || char3 != ' ')
	       {
		  printf ("Found unexpected tag starting with <n%c%c\n",
			  char2, char3);
		  exit (1);
	       }
	     if (state == not_started)
	       {
		  printf ("Found ns-tag before namespaces tag\n");
		  exit (1);
	       }
	     int ns;
	     int ret = fscanf (fp_ns, "id=\"%d\"", &ns);
	     if (ret != 1)
	       {
		  printf ("Read of id attribute failed.\n");
		  exit (1);
	       }
	     if (ns == 0)
	       continue;

	     if (state == reading_namespaces)
	       {
		  char casespec[80];
		  ret = fscanf (fp_ns, " case=\"%[^\"]\"", &casespec[0]);
		  if (ret != 1)
		    {
		       printf ("Read of case attribute failed.\n");
		       exit (1);
		    }
		  if (strcmp (casespec, "first-letter") != 0)
		    printf ("Note: %s namespace %d is %s\n",
			    wiki, ns, casespec);

		  check_canonical_name (fp_ns, ns, wiki);
	       }

	     read_to_char (fp_ns, '>');
	     char name[200];
	     int ret2 = fscanf (fp_ns, "%[^<]", name);
	     if (ret2 != 1)
	       {
		  printf ("Read namespace title failed.\n");
		  exit (1);
	       }
	     // printf ("ns=%d: \"%s\"\n", ns, name);
	     char name_escaped[200];
	     copy_and_escape (name_escaped, name);
	     ++ name_count;
	     if (strlen (name) > max_name_len)
	       max_name_len = strlen (name);
	     do_query (mysql,
		       "INSERT %s "
		       "VALUES ('%s', %d, '%s', %d)",
		       table,
		       wiki, ns, name_escaped, state == reading_aliases);
	  }
	fclose (fp_ns);
     }
   fclose (fp);
   printf ("Got %d names for %d wikis\n", name_count, wiki_count);
   printf ("Max wiki length = %zd, max namespace length = %zd\n",
	   max_wiki_len, max_name_len);
}
