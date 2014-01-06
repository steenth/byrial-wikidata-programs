C_OPTIONS =  -g -Wall -Wextra -std=c99 -O2 $(shell mysql_config --include)
LINK_OPTIONS = -g -Wall -Wextra -O2

PROGRAMS = read_items language_statistics_for_items \
  property_statistics statement_statistics \
  taxon_check taxon_check2 used_taxons \
  deleted_item_values duplicate_link implied_sex sex_distribution \
  selflink alias-without-label empty-items \
  class-type-conflict person \
  read_species taxa-at-specieswiki \
  used-disambiguations pairs \
  catmerge catmerge2 \
  read_categorylinks_table read_page_table update_page_item \
  empty-items-repair \
  empty-but-used all-links duplicates list list2 list3 most_revisions \
  uncles no-sex no-sex_en read_pronouns read_pronouns_en \
  coordmerge countymerge disambigmerge \
  oneprop_stat iw_distribution \
  gu ml numbermerge numbermerge2 numbermerge3 commonsmerge \
  dates zh cs \
  get_namespaces ns_stats meshid catnamemerge ns_lists \
  most_aliases globe set_dumpdate ce link-label-compare \
  link_without_label
  
OBJECTS = $(addsuffix .o, $(PROGRAMS)) wikidatalib.o

PROGRAMS_WITH_AC = countrymerge bandmerge bandmerge2
OBJECTS_WITH_AC = $(addsuffix .o, $(PROGRAMS_WITH_AC))

.PHONY: all clean

all: $(PROGRAMS) $(PROGRAMS_WITH_AC)

clean:
	rm -f $(PROGRAMS) $(OBJECTS)

$(OBJECTS): %.o: %.c wikidatalib.h
	gcc -c $(C_OPTIONS) $< -o $@

$(PROGRAMS): %: %.o wikidatalib.o
	gcc $(LINK_OPTIONS) $^ $(shell mysql_config --libs) -o $@

$(OBJECTS_WITH_AC): %.o: %.c wikidatalib.h
	gcc -c $(C_OPTIONS) -Imultifast-v1.0.0/ahocorasick $< -o $@

$(PROGRAMS_WITH_AC): %: %.o wikidatalib.o
	gcc $(LINK_OPTIONS) $^ $(shell mysql_config --libs) -lahocorasick -Lmultifast-v1.0.0/ahocorasick -o $@
