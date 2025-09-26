#pragma once

#include <simdjson.h>

struct MProjectInternal
{
	int version;
	int name;

	void Serialize(simdjson::builder::string_builder& sb);
};