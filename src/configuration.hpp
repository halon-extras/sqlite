#ifndef CONFIGURATION_HPP_
#define CONFIGURATION_HPP_

#include "sqlite/sqlite.hpp"
#include <string>
#include <map>
#include <HalonMTA.h>

struct ParsedConfig
{
	std::string default_profile;
	std::map<std::string, std::shared_ptr<SQLiteProfile>> profiles;
};

bool parse_config(HalonConfig* cfg, ParsedConfig* parsed_cfg);

#endif
