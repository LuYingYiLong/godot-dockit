#pragma once

#include "css_cascade_order.h"
#include "css_property_value.h"
#include "css_variables.h"

namespace litehtml { struct css_parsed_declaration; }

namespace docrender::css {

class Cascade {
	using Property = litehtml::string_id;
	using Value = litehtml::property_value;
	litehtml::props_map winners;
	litehtml::props_map defaults;
	std::map<Property, std::map<int, Value>> layer_candidates;
	litehtml::props_map computed;
	litehtml::css_computed_variables variables;
	vector<litehtml::css_diagnostic> diagnostics;
	std::uint64_t next_declaration = 0;

	void insert(Property name, Value value, CascadePriority priority);
	const Value& rollback(Property name, Property alias, int layer, bool layer_only, bool has_author_layers) const;

public:
	using ParseDeferred = std::function<litehtml::css_parsed_declaration(
		const Value::deferred_declaration&, const css_token_vector&, bool important)>;

	void add(Property name, Value value, CascadePriority priority);
	void merge(const Cascade& source, CascadePriority priority);
	void snapshot_defaults();
	void clear(bool keep_variables = false);
	void resolve(bool has_author_layers, bool inherited_rtl, const litehtml::css_inherited_variable& inherited,
		const ParseDeferred& parse_deferred);

	const Value& get(Property name) const;
	void collect_property_names(std::vector<Property>& names) const;
	const css_token_vector* get_custom_property(Property name, bool& local) const;
	const vector<litehtml::css_diagnostic>& get_diagnostics() const { return diagnostics; }
};

} // namespace docrender::css
