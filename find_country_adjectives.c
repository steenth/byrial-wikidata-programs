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

/*
 * This program uses ahocorasick, an LGPL implementation of the Aho-Corasick 
 * algorithm as a C library by Kamiar Kanani.
 * Get it from http://multifast.sourceforge.net/
 */

// For stpcpy()
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include "wikidatalib.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <locale.h>
#include "ahocorasick.h"

void create_country_adjective_table (MYSQL *mysql);
void fill_country_adjective_table (MYSQL *mysql, const char *lang,
				   const char *before, const char *after);
void create_country_adjective_words_table (MYSQL *mysql);
void create_country_adjective_approved_table (MYSQL *mysql);
void prune_approved_table (MYSQL *mysql);
void create_countrymerge_partof_table (MYSQL *mysql);

int main (int argc, char *argv[])
{
   setlocale (LC_CTYPE, "");

   const char *database;
   if (argc < 2)
     database = "wikidatawiki";
   else
     database = argv[1];

   MYSQL *mysql = open_named_database (database);
   create_country_adjective_table (mysql);
   fill_country_adjective_table (mysql, "da", "fra ", "");
   fill_country_adjective_table (mysql, "sv", "från ", "");
   // fill_country_adjective_table (mysql, "de", "(", ")");
   create_country_adjective_words_table (mysql);
   create_country_adjective_approved_table (mysql);
   prune_approved_table (mysql);

   create_countrymerge_partof_table (mysql);
   close_database (mysql);
}

void create_countrymerge_partof_table (MYSQL *mysql)
{
   // This has nothing to do with the adjectives, but this program is
   // a convenient place to create a table to mark the country
   // for subdivisions to be able to filter out false reports like
   // "Mexiko ≠ Yucatán: Q165204 links to de:Mérida (Mexiko) and en:Mérida, Yucatán"

   if (table_exists (mysql, "countrymerge_partof"))
     do_query (mysql, "DROP TABLE countrymerge_partof");
   do_query (mysql,
	     "CREATE TABLE countrymerge_partof ("
	     "country INT NOT NULL,"
	     "partof INT NOT NULL,"
	     "UNIQUE KEY (country))"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   do_query
     (mysql,
	 "INSERT countrymerge_partof "

	 "SELECT s_item, s_item "
	 "FROM statement "
	 "WHERE s_property = 31 AND s_item_value = 3624078 "
	 // p31=instance of, q3624078=sovereign state
	 
	 "UNION "
	 "SELECT i_id, i_id "
	 "FROM item "
	 "WHERE i_id IN (15,18,46,48,49,51,538, "
	 // q15=Africa, q18=South America, q46=Europe, q48=Asia,
	 // q49=North America, q51=Antarctica, q538=Oceania
	 "  16957,713750,180573,172640,36704,33946,28513) "
	 // q16957=East Germany, q713750=West Germany, q180573=South Vietnam
	 // q172640=North Vietnam, q36704=Yugoslavia, q33946=Czechoslovakia
	 // q28513=Austria-Hungary
	 
	 "UNION "
	 "SELECT s_item_value, s_item "
	 "FROM statement "
	 "WHERE s_property = 150 " // p150=subdivides into (administrative)
	 "  AND s_item IN (16,20,30,31,35,38,39,40,43,96,145,155,159,183,668,878,29999) "
	 // q16=Canada, q20=Norway, q30=USA, q31=Belgium, q35=Denmark, q38=Italy,
	 // q39=Switzerland, q40=Austria, q43=Turkey, q96=Mexico, q145=UK,
	 // q155=Brazil, q159=Russia, q183=Germany, q668=India, q878=UEA,
	 // q29999=Kingdom of the Netherlands
     );
}

void prune_approved_table (MYSQL *mysql)
{
   time_t start = time (NULL);
   printf ("Pruning words where many for a language/country combination are approved ...\n");
   MYSQL_RES *result = do_query_store
     (mysql,
	 "SELECT lang, country, word, count "
	 "FROM country_adjective_approved "
	 "ORDER BY lang, country, count DESC ");
   MYSQL_ROW row;
   char last_lang[13] = "";
   int last_country = 0;
   int last_count = 0;
   int first_count = 0;
   int lang_country_count = 0;
   int deleted = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *lang = row[0];
	int country = atoi (row[1]);
	const char *word = row[2];
	int count = atoi (row[3]);
	
	if (strcmp (last_lang, lang) != 0 || last_country != country)
	  {
	     strcpy (last_lang, lang);
	     last_country = country;
	     last_count = count;
	     first_count = count;
	     lang_country_count = 1;
	  }
	else
	  {
	     if ((lang_country_count > 3 && count < last_count) ||
		 (count * 10 < first_count))
	       {
		  printf ("Removing word %s for lang %s, country %d with count %d\n",
			  word, lang, country, count);
		  do_query (mysql,
			    "DELETE FROM country_adjective_approved "
			    "WHERE lang = '%s' AND country = %d AND word = \"%s\"",
			    lang, country, word);
		  ++ deleted;
	       }
	     else
	       {
		  ++ lang_country_count;
		  last_count = count;
	       }
	  }
     }
   mysql_free_result (result);
   printf ("Deleted %d words in %ld seconds.\n", deleted, time (NULL) - start);
}

void create_country_adjective_approved_table (MYSQL *mysql)
{
   time_t start = time (NULL);
   printf ("Approving candidate words ... \n");
   fflush (stdout);
   do_query (mysql,
	     "INSERT IGNORE country_adjective_words "
	     "VALUES ('zzzzz', 'sentinel', 0, 0)");
   if (table_exists (mysql, "country_adjective_approved"))
     do_query (mysql, "DROP TABLE country_adjective_approved");
   do_query (mysql,
	     "CREATE TABLE country_adjective_approved ("
	     "lang VARCHAR(12) NOT NULL,"
	     "word VARCHAR(200) NOT NULL,"
	     "country INT NOT NULL,"
	     "count INT NOT NULL,"
	     "KEY (lang))"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   MYSQL_RES *result =
     do_query_store (mysql,
		   "SELECT lang, word, country, count "
		   "FROM country_adjective_words "
		   "ORDER BY lang, word, count DESC");
   MYSQL_ROW row;
   char last_lang[13] = "";
   char last_word[201] = "";
   int last_country = 0;
   int last_count = 0;
   int number_of_countries = -1;
   
   int approved_words = 0;
   int one_time_words = 0;
   int maybe_words = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *lang = row[0];
	const char *word = row[1];
	int country = atoi (row[2]);
	int count = atoi (row[3]);

	if (strcmp (lang, last_lang) != 0 ||
	    strcmp (word, last_word) != 0)
	  {
	     // This is a new word. Check if last_word is OK
	     if (number_of_countries == 1)
	       {
		  if (last_count > 1)
		    {
		       do_query (mysql,
				 "INSERT country_adjective_approved "
				 "VALUES ('%s', \"%s\", %d, %d)",
				last_lang, last_word, last_country, last_count);
		       ++ approved_words;
		    }
		  else
		    ++ one_time_words;
	       }
	     strcpy (last_lang, lang);
	     strcpy (last_word, word);
	     last_country = country;
	     last_count = count;
	     number_of_countries = 1;
	  }
	else
	  {
	     // A repeat of word used in a link for another country
	     if (number_of_countries == 1 && count * 5 < last_count)
	       {
		  // Mostly used for one country. This may be caused by 
		  // wrong links for one language for an item.
		  do_query (mysql,
			    "INSERT country_adjective_approved "
			    "VALUES ('%s', \"%s\", %d, %d)",
			    last_lang, last_word, last_country, last_count);
		  printf ("Check %s for word %s: used %d times for country {{Q|%d}} "
			  "and %d time for country {{Q|%d}}\n",
			  lang, word, last_count, last_country, count, country);
		  MYSQL_RES *result = do_query_use
		    (mysql,
			"SELECT link_item "
			"FROM country_adjective "
			"JOIN link "
			"  ON k_id = link_item "
			"    AND k_lang = '%s' "
			"    AND k_text LIKE \"%%%s%%\" "
			"WHERE country_item = %d",
			lang, word, country);
		  MYSQL_ROW row = mysql_fetch_row (result);
		  if (! row)
		    {
		       printf ("Did not find the item to check.\n");
		    }
		  else
		    {
		       do
			 {			    
			    const char *item = row[0];
			    printf ("* check [[Q%s]]\n", item);
			    row = mysql_fetch_row (result);
			 }
		       while (row);
		       mysql_free_result (result);
		    }
		  ++ maybe_words;
	       }
	     ++ number_of_countries;
	  }
     }
   printf ("Approved %d words, found %d words once, "
	   "found %d words to check in %ld seconds.\n",
	   approved_words, one_time_words, maybe_words, time (NULL) - start);
}

void create_country_adjective_words_table (MYSQL *mysql)
{
   printf ("Finding candidate words for country adjectives ...\n");
   fflush (stdout);
   time_t start = time (NULL);
   if (table_exists (mysql, "country_adjective_words"))
     do_query (mysql, "DROP TABLE country_adjective_words");
   do_query (mysql,
	     "CREATE TABLE country_adjective_words ("
	     "lang VARCHAR(12) NOT NULL,"
	     "word VARCHAR(200) NOT NULL,"
	     "country INT NOT NULL,"
	     "count MEDIUMINT UNSIGNED NOT NULL, "
	     "UNIQUE KEY (lang, word, country))"
	     "ENGINE=MyISAM "
	     "CHARSET binary");

   // Find links to wiki pages with country adjectives
   MYSQL_RES *result = do_query_store
     (mysql,
	 "SELECT country_item, k1.k_lang, k1.k_text, k2.k_text "
	 "FROM country_adjective "
	 "JOIN link k1 "
	 "  ON link_item = k1.k_id "
	 "JOIN link k2 "
	 "  ON country_item = k2.k_id AND k2.k_lang = k1.k_lang "
	 "    AND k2.k_ns = 0 "
	 "    AND k2.k_project = 'w' " // Not sure about only using Wikipedia links
	 /* "WHERE k1.k_lang != 'da' " */);
   MYSQL_ROW row;
   int links = 0;
   int words = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	const char *country = row[0];
	const char *lang = row[1];
	const char *link = row[2];
	const char *country_link = row[3];
	++ links;

	const char *p = link;
	  {
	     // printf ("country: %s, lang: %s, link: '%s'\n", country, lang, link);
	     while (*p)
	       {
		  while (*p && ! isalpha (*p)
			 && ! (*p & '\x80'))
		    ++ p;
		  const char *word = p;
		  int prefix_count = 0;

		  // Follow the country link as long as possible. That way
		  // spaces in the country name may be transfered to the adjective
 		  const char *country_ptr = country_link;
		  while (*p && *country_ptr == *p)
		    {
		       ++ p;
		       ++ country_ptr;
		       if ((*p & '\x80') == '\x00' || // one byte character
			   (*p & '\xc0') == '\xc0') // Byte 1 of multibyte character 
			 ++ prefix_count;
		    }

		  // One back if the last matching characters was " " or " ("
		  // (Otherwise we would not find e.g. "Chech" as an independent word
		  //  with country_link == "Czech Replublic", or "Soviet" with
		  //  country_link == "Soviet Union" etc.)
		  if (country_ptr != country_link)
		    {
		       if (p[-1] == '(')
			 -- p;
		       if (p[-1] == ' ')
			 -- p;
		    }

		  // Now search for end of word
		  while (isalpha (*p)
			 || *p == '-' || *p == '\'' || *p & '\x80')
		    ++p;

		  while (prefix_count > 2 &&
			 NULL != (country_ptr = strchr (country_ptr, ' ')))
		    {
		       // The previoust word started as a prefix of a multiworded country name. 
		       // If also the next word starts as a prefix of the next word of the
		       // country, we guess that they should be kept together.
		       prefix_count = 0;
		       int byte_count = 0;
		       while (*p && *country_ptr == *p)
			 {
			    ++ p;
			    ++ country_ptr;
			    if ((*p & '\x80') == '\x00' || // one byte character
				(*p & '\xc0') == '\xc0') // Byte 1 of multibyte character 
			      ++ prefix_count;
			    ++ byte_count;
			 }
		       if (prefix_count <= 2)
			 {
			    // Prefix not long enough.
			    p -= byte_count;
			    break;
			 }
		       printf ("Will keep following word for %s together with start "
			       "(country %s, lang %s)\n",
			       word, country_link, lang);

		       // Search for end of word
		       while (isalpha (*p)
			      || *p == '-' || *p == '\'' || *p & '\x80')
			 ++p;
		    }

		  int len = p - word;
		  if (len > 1)
		    {
		       // Note: the word may contain ' , but not "
		       do_query (mysql,
				 "INSERT country_adjective_words "
				 "VALUES ('%s', \"%.*s\", %s, 1) "
				 "ON DUPLICATE KEY UPDATE count = count + 1",
				 lang, len, word, country);
		       // printf ("word: '%.*s'\n", len, word);
		       ++ words;
		    }
	       }
	  }
     }
   mysql_free_result (result);
   printf ("Found %d words in %d links in %ld seconds.\n",
	   words, links, time (NULL) - start);
}

struct match_handler_param 
{
   const char *item;
   const char *link;
   MYSQL *mysql;
};

static int matches = 0;

int match_handler (AC_MATCH_t *m, void *vp)
{
   struct match_handler_param *param = vp;

   // We need to check if the found match stops at a word boundary
   // E.g. "fra Indianapolis" should be found for "Indiana"
   const char next = param->link[m->position];
   if (isalpha (next) || (next & '\x80'))
     {
	printf ("match_handler: match for %s (link_item %s, country %lu) dropped.\n",
		param->link, param->item, m->patterns[0].rep.number);
	return 0; // continue search
     }
   
   do_query (param->mysql,
	     "INSERT country_adjective "
	     "VALUES (%s, %lu) "
	     "ON DUPLICATE KEY UPDATE country_item = %lu",
	     param->item, m->patterns[0].rep.number,
	     m->patterns[0].rep.number);
   ++ matches;
   return 0; // continue searching so e.g. Nigeria later can
   // override the first found Niger
}

void create_country_adjective_table (MYSQL *mysql)
{
   // Create table
   if (table_exists (mysql, "country_adjective"))
     do_query (mysql, "DROP TABLE country_adjective");
   do_query (mysql,
	     "CREATE TABLE country_adjective ("
	     "link_item INT NOT NULL,"
	     "country_item INT NOT NULL,"
	     "UNIQUE KEY (link_item),"
	     "KEY (country_item))"
	     "ENGINE=MyISAM "
	     "CHARSET binary");
}

void fill_country_adjective_table (MYSQL *mysql, const char *lang,
				   const char *before, const char *after)
{
   printf ("Finding links with \"<from> <country>\" in %s: ...\n", lang);
   fflush (stdout);
   time_t start = time (NULL);

   AC_AUTOMATA_t *acap = ac_automata_init (match_handler);
   
   // Find the search strings
   MYSQL_RES *result = do_query_use
     (mysql,
	 "SELECT s_item, "
	 "  (SELECT k_text FROM link "
	 "   WHERE k_id = s_item AND k_lang = '%s' AND k_project = 'w') "
	 "FROM statement "
	 "WHERE s_property = 31 AND s_item_value = 3624078 "
	 // p31=instance of, q3624078=sovereign state

	 "UNION "
	 "SELECT k_id, k_text "
	 "FROM link "
	 "WHERE k_lang = '%s' AND k_project = 'w' AND "
	 "  k_id IN (15,18,46,48,49,51,538, "
	 // q15=Africa, q18=South America, q46=Europe, q48=Asia,
	 // q49=North America, q51=Antarctica, q538=Oceania
	 "  16957,713750,180573,172640,36704,33946,28513) "
	 // q16957=East Germany, q713750=West Germany, q180573=South Vietnam
	 // q172640=North Vietnam, q36704=Yugoslavia, q33946=Czechoslovakia
	 // q28513=Austria-Hungary
   
	 "UNION "
	 "SELECT s_item_value, "
	 "  (SELECT l_text FROM label WHERE l_id = s_item_value AND l_lang = '%s') "
	 "FROM statement "
	 "WHERE s_property = 150 " // p150=subdivides into (administrative)
	 "  AND s_item IN (16,20,30,31,35,38,39,40,43,96,145,155,159,183,668,878,29999) "
	 // q16=Canada, q20=Norway, q30=USA, q31=Belgium, q35=Denmark, q38=Italy,
	 // q39=Switzerland, q40=Austria, q43=Turkey, q96=Mexico, q145=UK,
	 // q155=Brazil, q159=Russia, q183=Germany, q668=India, q878=UEA,
	 // q29999=Kingdom of the Netherlands
	 "  AND s_item_value != 1428 " // q1428=Georgia (US state)

	 , lang, lang, lang);

   MYSQL_ROW row;
   int patterns = 0;
   size_t extra_len = strlen (before) + strlen (after);
   while (NULL != (row = mysql_fetch_row (result)))
     {
	int item = atoi (row[0]);
	const char *link = row[1];
	if (link == NULL)
	  continue;
	size_t len = strlen (link) + extra_len;
	char *search_string = malloc (len + 1);
	if (!search_string)
	  {
	     printf ("malloc() failed: %m\n");
	     exit (1);
	  }
	char *dest = search_string;
	dest = stpcpy (dest, before);
	dest = stpcpy (dest, link);
	dest = stpcpy (dest, after);

	AC_PATTERN_t ac_pattern;
	ac_pattern.astring = search_string;
	ac_pattern.length = len;
	ac_pattern.rep.number = item;
	AC_ERROR_t ret = ac_automata_add (acap, &ac_pattern);
	if (ret == ACERR_SUCCESS)
	  {
	     printf ("New search string: %s\n", search_string);
	     ++ patterns;
	  }
     }
   mysql_free_result (result);
   ac_automata_finalize (acap);

   // Find links to dawiki with "<from> <country>"
   result = do_query_store
     (mysql,
	 "SELECT k_id, k_text "
	 "FROM link "
	 "WHERE k_lang = '%s'",
	 lang);
   int links = 0;
   matches = 0;
   while (NULL != (row = mysql_fetch_row (result)))
     {
	++ links;
	const char *item = row[0];
	const char *link = row[1];

	// Check if the link has a country as substring
	AC_TEXT_t ac_text;
	// ac_text.astring = link;
	//   libahocorasick will not modify the string, so it should have used const.
	ac_text.astring = (char *) link;
        ac_text.length = strlen (link);
	struct match_handler_param param;
	param.item = item;
	param.link = link;
	param.mysql = mysql;
	ac_automata_search (acap, &ac_text, &param);
	ac_automata_reset (acap);
     }
   printf ("searched %d %s links for %d patterns, found %d matches in %ld seconds.\n",
	   links, lang, patterns, matches, time (NULL) - start);
   ac_automata_release (acap);
   mysql_free_result (result);
}
