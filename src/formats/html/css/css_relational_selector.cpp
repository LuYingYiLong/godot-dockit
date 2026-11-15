#include "css_relational_selector.h"

#include <litehtml/element.h>
#include <litehtml/html.h>

#include <algorithm>
#include <string_view>
#include <unordered_set>

namespace litehtml {
	namespace {
		thread_local const element* scope_root = nullptr;

		std::vector<std::string_view> language_subtags(std::string_view p_value) {
			std::vector<std::string_view> subtags;
			size_t start = 0;
			for (;;) {
				const auto end = p_value.find('-', start);
				subtags.push_back(p_value.substr(start, end == std::string_view::npos ? end : end - start));
				if (end == std::string_view::npos) return subtags;
				start = end + 1;
			}
		}

		// RFC 4647 extended filtering permits skipped subtags up to an extension singleton.
		bool matches_language_range(const string& p_language, const string& p_range) {
			if (p_language.empty() || p_range.empty()) return p_language.empty() && p_range.empty();
			if (p_language == p_range || p_range == "*") return true;
			const auto tag = language_subtags(p_language);
			const auto range = language_subtags(p_range);
			if (range[0] != "*" && range[0] != tag[0]) return false;

			size_t tag_index = 1;
			for (size_t range_index = 1; range_index < range.size(); ++range_index) {
				const auto subtag = range[range_index];
				if (subtag == "*") continue;
				while (tag_index < tag.size() && tag[tag_index] != subtag) {
					if (tag[tag_index++].size() <= 1) return false;
				}
				if (tag_index == tag.size()) return false;
				++tag_index;
			}
			return true;
		}

		bool is_dom_element(const element& p_element) {
			return p_element.tag() != empty_id && !is_generated_tag(p_element.tag());
		}

		// The parser stores a complex selector from right to left. Each step here
		// describes how to walk from the anchor or the previous matched element.
		struct RelativeStep {
			const css_element_selector* compound;
			css_combinator combinator;
		};

		using VisitedSteps = std::vector<std::unordered_set<const element*>>;

		struct PendingStep {
			const element* from;
			size_t index;
		};

		bool match_steps(const element& p_anchor, const std::vector<RelativeStep>& p_steps) {
			VisitedSteps visited(p_steps.size());
			std::vector<PendingStep> pending{ { &p_anchor, 0 } };
			while (!pending.empty()) {
				const auto [from, index] = pending.back();
				pending.pop_back();
				if (index == p_steps.size()) return true;
				// Multiple matches along an earlier axis can lead to the same remaining search.
				if (!visited[index].insert(from).second) continue;
				const auto& [compound, combinator] = p_steps[index];
				const auto enqueue_match = [&](element& p_candidate) {
					if (p_candidate.select(*compound) != select_no_match) {
						pending.push_back({ &p_candidate, index + 1 });
					}
				};
				const bool descendants = combinator == combinator_descendant;
				if (descendants || combinator == combinator_child) {
					for (const auto& child : from->children()) {
						if (!is_dom_element(*child)) continue;
						if (descendants) pending.push_back({ child.get(), index });
						enqueue_match(*child);
					}
					continue;
				}
				if (const auto parent = from->parent()) {
					bool after_anchor = false;
					for (const auto& sibling : parent->children()) {
						if (sibling.get() == from) {
							after_anchor = true;
						} else if (after_anchor && is_dom_element(*sibling)) {
							enqueue_match(*sibling);
							if (combinator == combinator_adjacent_sibling) break;
						}
					}
				}
			}
			return false;
		}
	}

	selector_scope::selector_scope(const element* p_root) : previous(scope_root) {
		scope_root = p_root;
	}

	selector_scope::~selector_scope() {
		scope_root = previous;
	}

	bool matches_selector_scope(const element& p_element) {
		return scope_root ? scope_root == &p_element : p_element.is_root();
	}

	const char* get_element_language(const element& p_element) {
		if (const char* value = p_element.get_attr("lang")) return value;
		for (auto parent = p_element.parent(); parent; parent = parent->parent()) {
			if (const char* value = parent->get_attr("lang")) return value;
		}
		return nullptr;
	}

	bool matches_element_language(const element& p_element, const string_vector& p_ranges) {
		const char* value = get_element_language(p_element);
		const auto language = lowcase(value ? value : "");
		return std::any_of(p_ranges.begin(), p_ranges.end(), [&](const string& p_range) {
			return matches_language_range(language, p_range);
		});
	}

	bool matches_relative_selector(const element& p_anchor, const css_selector::vector& p_selectors) {
		for (const auto& selector : p_selectors) {
			std::vector<RelativeStep> steps;
			for (const auto* part = selector.get(); part; part = part->m_left.get()) {
				steps.push_back({ &part->m_right, part->m_left ? part->m_combinator : part->m_anchor_combinator });
			}
			std::reverse(steps.begin(), steps.end());
			if (match_steps(p_anchor, steps)) return true;
		}
		return false;
	}
}
