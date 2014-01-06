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

#include <mysql.h>
#include <stdbool.h>
#include <stdio.h>
/*
 * wikidatalib.h
 */

#define DATABASE_HOST   NULL
#define DATABASE_USER   NULL
#define DATABASE_PASSWD NULL
#define DATABASE_NAME   "wikidatawiki"
#define DATABASE_PORT   0
#define DATABASE_SOCKET NULL

/*
 * Get the dumpdate for the indicated database.
 * Returns a string to memory allocated with malloc(3). 
 */
char *get_dumpdate (MYSQL *mysql, const char *database);
  
/*
 * Print the dumpdate of the opened database. Exit on error.
 */
void print_dumpdate (MYSQL *mysql);

/*
 * Return the namespace number if prefix is a namespace name
 * for the language lang, otherwise return 0.
 * Note: prefix must be escaped for injection to MySQL.
 */
int find_namespace (const char *prefix, char project,
		    const char *lang, int len, bool *is_alias);

/*
 * Return the local name for a namespace in a static string
 */
const char *get_namespace_name (const char *lang, int ns);

/*
 * Print an interwiki link to stdout
 */
void print_link (const char *lang, int ns, const char *title);

/*
 * Read from a file until the searched text is reached.
 * Doesn't work if the first character in str isn't unique.
 */
void read_to (FILE *fp, const char *str);

/*
 * Read a varchar value from a MySQL dump file, and return it in a user supplied variable.
 * Convert '_' to space
 */
const char *read_varchar (FILE *fp, char *value, size_t len);

/*
 * Pass n apostrophes in a MySQL dump file
 */
void pass_n_apostrophes (FILE *fp, int n);

/*
 * Add index indexes and analyze and optimize a table
 */
void add_index (const char *table, MYSQL *mysql, const char *indexes);

/*
 * Open named database
 */
MYSQL *open_named_database (const char *name);

/*
 * Open database
 */
MYSQL *open_database (void);

/*
 * Close database
 */
void close_database (MYSQL *mysql);

/*
 * return true if the table exists
 */
bool table_exists (MYSQL *mysql, const char *table);

/*
 * return true if the table have no rows
 */
bool table_empty (MYSQL *mysql, const char *table);

  /*
 * Do a SQL query build with a format and arguments like printf arguments
 * The caller is responsible to read data returned from the query, if any.
 * Exits on error
 */
void do_query (MYSQL *mysql, const char *fmt, ...)
 __attribute__ ((format (printf, 2, 3)));

/*
 * Do a SQL query build with a format and arguments like printf arguments
 * Discard all returned data from the query
 * Exits on error
 */
void do_query_res (MYSQL *mysql, const char *fmt, ...)
 __attribute__ ((format (printf, 2, 3)));


/*
 * Do a SQL query build with a format and arguments like printf arguments
 * Returns a pointer to a result object obtained from mysql_use_result()
 * Exits on error
 */
MYSQL_RES *do_query_use (MYSQL *mysql, const char *fmt, ...)
 __attribute__ ((format (printf, 2, 3)));

/*
 * Do a SQL query build with a format and arguments like printf arguments
 * Returns a pointer to a result object obtained from mysql_store_result()
 * Exits on error
 */
MYSQL_RES *do_query_store (MYSQL *mysql, const char *fmt, ...)
 __attribute__ ((format (printf, 2, 3)));
