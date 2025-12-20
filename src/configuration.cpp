#include "configuration.hpp"
#include <syslog.h>
#include <sstream>
#include "string.h"

bool parse_config(HalonConfig* cfg, ParsedConfig* parsed_cfg)
{
	const char* default_profile = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "default_profile"), nullptr);
	if (default_profile)
		parsed_cfg->default_profile = default_profile;

	auto profiles = HalonMTA_config_object_get(cfg, "profiles");
	if (profiles)
	{
		size_t x = 0;
		HalonConfig* profile;
		while ((profile = HalonMTA_config_array_get(profiles, x++)))
		{
			auto sqlite_profile = std::make_shared<SQLiteProfile>();

			const char* id = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "id"), nullptr);
			if (!id)
			{
				syslog(LOG_CRIT, "sqlite: Missing id for profile");
				return false;
			}

			const char* filename = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "filename"), nullptr);
			if (!filename)
			{
				syslog(LOG_CRIT, "sqlite: Missing filename for profile");
				return false;
			}
			sqlite_profile->filename = filename;

			const char* pool_size = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "pool_size"), nullptr);
			if (pool_size)
				sqlite_profile->pool_size = (unsigned int)strtoul(pool_size, nullptr, 10);

			const char* foreign_keys = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "foreign_keys"), nullptr);
			if (foreign_keys)
				sqlite_profile->foreign_keys = (strcmp(foreign_keys, "true") == 0 || strcmp(foreign_keys, "TRUE") == 0) ? true : false;

			const char* readonly = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "readonly"), nullptr);
			if (readonly)
				sqlite_profile->readonly = (strcmp(readonly, "true") == 0 || strcmp(readonly, "TRUE") == 0) ? true : false;

			const char* busy_timeout = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "busy_timeout"), nullptr);
			if (busy_timeout)
				sqlite_profile->busy_timeout = (unsigned int)strtoul(busy_timeout, nullptr, 10);

			parsed_cfg->profiles[id] = sqlite_profile;
		}
	}

	return true;
}
