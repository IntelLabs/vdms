/*
 * This test checks Jarvis iterator filters
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "jarvis.h"
#include "util.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */
#include "neighbor.h"

class QueryEngine
{

private: 

	uint32_t port;
	uint32_t portCounter;
	uint32_t taskIdSeed;

public: 

	QueryEngine();
	~QueryEngine();

	void init();
	void processQuery(std::string query);

};
