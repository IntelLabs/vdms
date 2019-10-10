#define __STDC_FORMAT_MACROS
#include <string>

#include "pmgd.h"
#include "util.h"

int insertSynonyms(std::string file, PMGD::Graph& db, std::string tag, std::string prop);

int insertCountriesTree(std::string file, PMGD::Graph& db, std::string tag);

int insertCities(std::string file, PMGD::Graph& db, std::string tag);

