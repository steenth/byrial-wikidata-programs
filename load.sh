. /data/sat/databasedump/wikidata/arkiv/wikidata.mk
./set_dumpdate $WIKIDATADATO
./read_items wikidatawiki drop < /data/sat/databasedump/wikidata/wikidatawiki-$WIKIDATADATO-pages-articles.xml
