#include "MProjectInternal.h"

namespace simdjson {
	template <typename builder_type>
	void tag_invoke(serialize_tag, builder_type& builder, const MProjectInternal& proj) {
		builder.start_object();
		builder.append_key_value("version", proj.version);
		builder.append_key_value("name", proj.name);
		builder.end_object();
	}
}

void MProjectInternal::Serialize(simdjson::builder::string_builder& sb)
{
	sb.append_key_value("project", *this);
	//simdjson::tag_invoke(simdjson::serialize_tag(), sb, *this);
}