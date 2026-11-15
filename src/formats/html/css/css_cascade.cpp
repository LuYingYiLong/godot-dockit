#include "css_cascade.h"

#include "css_declarations.h"

namespace docrender::css {

namespace {

using namespace litehtml;

bool is_rollback(const property_value& value) {
	return value.is<revert_value>() || value.is<revert_layer_value>();
}

} // namespace

void Cascade::add(Property name, Value value, CascadePriority priority) {
	value.priority = priority;
	const auto wins = [&](const Value& current) {
		return takes_cascade_precedence(priority, value.m_important, current.priority, current.m_important);
	};
	if (priority.layer >= 0 && priority.layer != css_inline_layer) {
		auto [entry, inserted] = layer_candidates[name].try_emplace(priority.layer, value);
		if (!inserted && wins(entry->second)) entry->second = value;
	}
	auto [entry, inserted] = winners.try_emplace(name, std::move(value));
	if (!inserted && wins(entry->second)) entry->second = std::move(value);
}

void Cascade::merge(const Cascade& source, CascadePriority priority) {
	for (const auto& [name, value] : source.winners) add(name, value, priority);
}

void Cascade::snapshot_defaults() {
	defaults = winners;
}

void Cascade::clear() {
	winners.clear();
	defaults.clear();
	layer_candidates.clear();
	computed.clear();
	variables.clear();
	diagnostics.clear();
}

const Cascade::Value& Cascade::rollback(Property name, int layer, bool layer_only, bool has_author_layers) const {
	if (has_author_layers && layer_only) {
		if (const auto found = layer_candidates.find(name); found != layer_candidates.end()) {
			const Value* best = nullptr;
			for (const auto& [rank, candidate] : found->second) {
				// The fallback boundary follows normal layer order, including for important winners.
				if (rank >= layer) break;
				if (!best || takes_cascade_precedence(candidate.priority, candidate.m_important,
					best->priority, best->m_important)) best = &candidate;
			}
			if (best) return *best;
		}
	}
	if (layer != css_default_layer) {
		if (const auto found = defaults.find(name); found != defaults.end()) return found->second;
	}
	static const Value unset;
	return unset;
}

void Cascade::resolve(bool has_author_layers, const litehtml::css_inherited_variable& inherited,
	const ParseDeferred& parse_deferred) {
	using namespace litehtml;
	computed.clear();
	variables.clear();
	diagnostics.clear();

	const auto find_rollback = [&](Property name, int layer, bool layer_only) -> const Value& {
		return rollback(name, layer, layer_only, has_author_layers);
	};
	const auto restore = [&](Property name, const Value& entry) -> const Value& {
		const auto* effective = &entry;
		while (is_rollback(*effective)) {
			effective = &find_rollback(name, effective->priority.layer, effective->is<revert_layer_value>());
		}
		return *effective;
	};

	css_custom_properties local_variables;
	vector<std::pair<Property, const Value*>> deferred_winners;
	for (const auto& [name, winner] : winners) {
		const auto& value = restore(name, winner);
		if (value.deferred) deferred_winners.emplace_back(name, &value);
		else if (is_rollback(winner)) computed.emplace(name, value);
		if (value.is<css_token_vector>() || (value.is<initial_value>() && _s(name).compare(0, 2, "--") == 0)) {
			local_variables.emplace(name, &value);
		}
	}

	if (!local_variables.empty() || !deferred_winners.empty()) {
		const css_custom_rollback restore_custom = [&](Property name, const Value& value,
			css_wide_keyword keyword) -> const Value& {
			return restore(name, find_rollback(name, value.priority.layer, keyword == css_wide_keyword::revert_layer));
		};
		css_variable_resolver resolver(inherited, std::move(local_variables), restore_custom);
		std::map<const Value::deferred_declaration*, props_map> parsed;
		const auto evaluate = [&](const Value& value) -> const props_map& {
			const auto& declaration = *value.deferred;
			auto [entry, inserted] = parsed.try_emplace(&declaration);
			if (!inserted) return entry->second;
			if (const auto tokens = resolver.substitute(declaration.tokens)) {
				auto result = parse_deferred(declaration, *tokens, value.m_important);
				if (result.status == css_declaration_status::parsed) {
					entry->second = std::move(result.properties);
					return entry->second;
				}
			}
			if (auto diagnostic = check_css_declaration(css_declaration_status::invalid, declaration.name,
				declaration.tokens, declaration.source)) diagnostics.push_back(std::move(*diagnostic));
			return entry->second;
		};

		for (const auto& [name, winner] : deferred_winners) {
			const auto* effective = winner;
			while (effective->deferred) {
				const auto& properties = evaluate(*effective);
				const auto found = properties.find(name);
				if (found == properties.end()) {
					computed.insert_or_assign(name, Value{});
					break;
				}
				if (!is_rollback(found->second)) {
					computed.insert_or_assign(name, found->second);
					break;
				}
				effective = &restore(name, find_rollback(name, effective->priority.layer, found->second.is<revert_layer_value>()));
			}
			if (!effective->deferred) computed.insert_or_assign(name, *effective);
		}
		variables = resolver.take_computed();
	}

	for (const auto& [name, winner] : winners) {
		const auto found = computed.find(name);
		const auto& value = found == computed.end() ? winner : found->second;
		if (value.origin) {
			diagnostics.push_back({css_diagnostic_kind::not_rendered, _s(value.origin->name),
				value.origin->value, value.origin->source});
		}
	}
}

const Cascade::Value& Cascade::get(Property name) const {
	if (const auto found = computed.find(name); found != computed.end()) return found->second;
	if (const auto found = winners.find(name); found != winners.end()) return found->second;
	static const Value unset;
	return unset;
}

const css_token_vector* Cascade::get_custom_property(Property name, bool& local) const {
	const auto found = variables.find(name);
	local = found != variables.end();
	return local && found->second ? &*found->second : nullptr;
}

} // namespace docrender::css
