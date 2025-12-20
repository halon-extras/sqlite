#ifndef ARGUMENTS_HPP_
#define ARGUMENTS_HPP_

#include <string>
#include <vector>
#include <variant>
#include <HalonMTA.h>

using Param = std::variant<std::nullptr_t, std::string, double, bool>;

bool parse_hsl_argument_as_bool(HalonHSLContext* hhc, HalonHSLArguments* args, size_t index, bool required, bool& value);
bool parse_hsl_argument_as_double(HalonHSLContext* hhc, HalonHSLArguments* args, size_t index, bool required, double& value);
bool parse_hsl_argument_as_string(HalonHSLContext* hhc, HalonHSLArguments* args, size_t index, bool required, std::string& value);
bool parse_hsl_argument_as_params(HalonHSLContext* hhc, HalonHSLArguments* args, size_t index, bool required, std::vector<Param>& value);

#endif
