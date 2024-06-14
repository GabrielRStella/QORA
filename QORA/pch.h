#ifndef PCH_H
#define PCH_H

#define _CRT_SECURE_NO_WARNINGS

//c headers
#include <cassert>

//data structures
#include <map>
#include <queue>
#include <set>
#include <vector>

//misc
#include <chrono>
#include <functional>
#include <fstream>
#include <iostream> //for the getline
#include <random>
#include <string>
#include <sstream>

//nlohmann json lib
//https://github.com/nlohmann/json/blob/develop/LICENSE.MIT
#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include "json.hpp"
using json = nlohmann::json;

//sigh
#define NOMINMAX
#include <Windows.h>

#endif //PCH_H
