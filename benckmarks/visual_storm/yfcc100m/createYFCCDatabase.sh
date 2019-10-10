#!/bin/sh

PMGD_DB=yfcc100m_pmgd

rm -r $PMGD_DB

#./yfcc --locationTree data/extras/geolite2/cities.csv $JARVIS_DB
#./yfcc --cities data/extras/geolite2/latlongcities.csv $JARVIS_DB
#./yfcc --synonyms data/extras/dictionary-seed/db/wordnet_generics.tsv $JARVIS_DB
./yfcc --media data/yfcc100m_dataset_short_10k $PMGD_DB
