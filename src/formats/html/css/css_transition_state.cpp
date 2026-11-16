#include "css_transition_state.h"

#include "css_math.h"

#include <litehtml/element.h>
#include <litehtml/html.h>
#include <litehtml/html_tag.h>

#include <algorithm>
#include <cmath>

namespace docrender::css {
	using namespace litehtml;

	namespace {
		constexpr std::array property_names{"opacity", "color", "background-color",
			"border-top-color", "border-right-color", "border-bottom-color", "border-left-color",
			"width", "height", "transform"};
		using Kind = css_transform_function::kind;

		bool equal_length(const css_length& a, const css_length& b) {
			if (a.is_predefined() != b.is_predefined() || a.intrinsic_size() != b.intrinsic_size()) return false;
			if (a.is_predefined()) return a.predef() == b.predef();
			if (a.expression() || b.expression()) return a.expression() == b.expression() || a.to_string() == b.to_string();
			return (a.val() == 0 && b.val() == 0) || (a.units() == b.units() && a.val() == b.val());
		}

		bool equal_function(const css_transform_function& a, const css_transform_function& b) {
			return a.type == b.type && equal_length(a.x, b.x) && equal_length(a.y, b.y) && equal_length(a.z, b.z) &&
				a.first == b.first && a.second == b.second && a.third == b.third && a.axis == b.axis &&
				a.matrix == b.matrix && a.blend == b.blend;
		}

		css_transform_function identity(const css_transform_function& model) {
			css_transform_function result{model.type};
			result.axis = model.axis;
			if (model.type == Kind::scale) result.first = result.second = 1;
			if (model.type == Kind::perspective) result.z = css_length::predef_value();
			if (model.type == Kind::matrix) result.matrix[0] = result.matrix[3] = 1;
			if (model.type == Kind::matrix3d) result.matrix[0] = result.matrix[5] = result.matrix[10] = result.matrix[15] = 1;
			return result;
		}

		css_transform_function blend_matrices(css_transform_list from, css_transform_list to, double progress,
				bool allow_discrete) {
			css_transform_function result{Kind::blend};
			result.blend = std::make_shared<css_transform_blend>(
				css_transform_blend{std::move(from), std::move(to), progress, allow_discrete});
			return result;
		}

		css_transform_list interpolate_transforms(const css_transform_list& a, const css_transform_list& b,
				double progress, bool allow_discrete) {
			css_transform_list result;
			result.reserve(std::max(a.size(), b.size()));
			for (std::size_t index = 0; index < std::max(a.size(), b.size()); ++index) {
				if ((index < a.size() && a[index].type == Kind::blend) ||
						(index < b.size() && b[index].type == Kind::blend) ||
						(index < a.size() && index < b.size() && a[index].type != b[index].type)) {
					result.push_back(blend_matrices(
						{a.begin() + std::min(index, a.size()), a.end()},
						{b.begin() + std::min(index, b.size()), b.end()}, progress, allow_discrete));
					break;
				}
				auto first = index < a.size() ? a[index] : identity(b[index]);
				const auto second = index < b.size() ? b[index] : identity(a[index]);
				if (equal_function(first, second)) {
					result.push_back(std::move(first));
					continue;
				}
				if (first.type == Kind::matrix || first.type == Kind::matrix3d || first.type == Kind::perspective ||
						(first.type == Kind::rotate && first.axis != second.axis && first.first != 0 && second.first != 0)) {
					result.push_back(blend_matrices({std::move(first)}, {second}, progress, allow_discrete));
					continue;
				}
				if (first.type == Kind::translate) {
					first.x = interpolate_css_length(first.x, second.x, progress);
					first.y = interpolate_css_length(first.y, second.y, progress);
					first.z = interpolate_css_length(first.z, second.z, progress);
				} else {
					if (first.type == Kind::rotate && first.first == 0) first.axis = second.axis;
					first.first = std::lerp(first.first, second.first, progress);
					first.second = std::lerp(first.second, second.second, progress);
					first.third = std::lerp(first.third, second.third, progress);
				}
				result.push_back(std::move(first));
			}
			return result;
		}

		template<class Visitor>
		void visit_styled(const element::ptr& node, const Visitor& visit) {
			if (!node || node->css().get_display() == display_none || is_generated_tag(node->tag())) return;
			if (dynamic_cast<const html_tag*>(node.get())) visit(node);
			if (node->css().hides_contents()) return;
			for (const auto& child : node->children()) visit_styled(child, visit);
		}
	}

	TransitionState::Values TransitionState::values(const element& element) {
		const auto& css = element.css();
		const auto color = css.get_color();
		const auto& borders = css.get_borders();
		return {static_cast<float>(css.get_opacity()), color, css.get_bg().m_color.resolve(color),
			borders.top.color.resolve(color), borders.right.color.resolve(color),
			borders.bottom.color.resolve(color), borders.left.color.resolve(color),
			css.get_width(), css.get_height(), css.get_transforms()};
	}

	bool TransitionState::equal(const Value& a, const Value& b) {
		if (a.index() != b.index()) return false;
		if (const auto* value = std::get_if<float>(&a)) return *value == std::get<float>(b);
		if (const auto* value = std::get_if<web_color>(&a)) return equal_css_colors(*value, std::get<web_color>(b));
		if (const auto* value = std::get_if<css_length>(&a)) return equal_length(*value, std::get<css_length>(b));
		const auto& first = std::get<css_transform_list>(a);
		const auto& second = std::get<css_transform_list>(b);
		return first.size() == second.size() && std::equal(first.begin(), first.end(), second.begin(), equal_function);
	}

	bool TransitionState::interpolable(const Value& a, const Value& b) {
		if (a.index() != b.index()) return false;
		if (const auto* from = std::get_if<css_length>(&a)) {
			const auto& to = std::get<css_length>(b);
			return !from->is_predefined() && !to.is_predefined() &&
				from->intrinsic_size() == css_intrinsic_size::none && to.intrinsic_size() == css_intrinsic_size::none;
		}
		return true;
	}

	TransitionState::Value TransitionState::interpolate(const Value& a, const Value& b, double progress, bool allow_discrete) {
		if (const auto* value = std::get_if<float>(&a)) return static_cast<float>(
			std::clamp(*value + (std::get<float>(b) - *value) * progress, 0.0, 1.0));
		if (const auto* value = std::get_if<web_color>(&a)) return interpolate_css_color(*value, std::get<web_color>(b), progress);
		if (const auto* value = std::get_if<css_length>(&a)) return interpolate_css_length(*value, std::get<css_length>(b), progress, true);
		return interpolate_transforms(std::get<css_transform_list>(a), std::get<css_transform_list>(b), progress, allow_discrete);
	}

	double TransitionState::Track::progress(double now) const {
		return now < start ? 0 : duration == 0 || now >= start + duration ? 1 : easing.sample((now - start) / duration);
	}

	TransitionState::Value TransitionState::Track::sample(double now) const {
		if (now < start) return from;
		if (now >= start + duration) return to;
		const auto amount = progress(now);
		if (discrete) return amount < 0.5 ? from : to;
		return interpolate(from, to, amount, allow_discrete);
	}

	TransitionState::Snapshot TransitionState::capture(const std::shared_ptr<element>& root) {
		Snapshot before;
		visit_styled(root, [&](const element::ptr& node) {
			if (node->get_render_item() || node->css().get_display() == display_contents)
				before.emplace(node, values(*node));
		});
		return before;
	}

	void TransitionState::commit(const std::shared_ptr<element>& root, const Snapshot& before, double now) {
		decltype(running) next;
		visit_styled(root, [&](const element::ptr& node) {
			const auto* styles = node->css().get_transitions();
			if (!styles || (!node->get_render_item() && node->css().get_display() != display_contents)) return;
			const auto old = before.find(node);
			if (old == before.end()) return;
			const auto previous = running.find(node);
			const auto targets = values(*node);
			Running updated;
			for (std::size_t index = 0; index < property_count; ++index) {
				const auto config = styles->match(property_names[index]);
				if (!config) continue;
				const Track* current = previous != running.end() && previous->second[index] ? &*previous->second[index] : nullptr;
				const auto& to = targets[index];
				if (current && equal(current->to, to)) {
					updated[index] = *current;
					continue;
				}
				const auto from = current ? current->sample(now) : old->second[index];
				if (equal(from, to) || config->duration + config->delay <= 0) continue;
				const bool discrete = !interpolable(from, to);
				if (discrete && !config->allow_discrete) continue;
				const bool reversing = current && now < current->start + current->duration &&
					equal(current->reversing_start, to);
				const double factor = reversing ? std::clamp(std::abs(current->progress(now) * current->shortening +
					(1 - current->shortening)), 0.0, 1.0) : 1.0;
				const auto delay = config->delay < 0 ? config->delay * factor : config->delay;
				updated[index] = Track{from, to, reversing ? current->to : from, now + delay,
					config->duration * factor, factor, config->easing, discrete, config->allow_discrete};
			}
			if (std::any_of(updated.begin(), updated.end(), [](const auto& track) { return track.has_value(); }))
				next.emplace(node, std::move(updated));
		});
		running.swap(next);
		sample(now);
	}

	void TransitionState::sample(double now) {
		overrides.clear();
		for (auto it = running.begin(); it != running.end();) {
			if (const auto node = it->first.lock()) {
				for (std::size_t index = 0; index < property_count; ++index) {
					auto& track = it->second[index];
					if (!track) continue;
					if (now >= track->start + track->duration) {
						track.reset();
						continue;
					}
					const auto sample = track->sample(now);
					overrides[node.get()].emplace(_id(property_names[index]), std::visit(
						[](const auto& value) { return property_value(value, false); }, sample));
				}
				if (overrides.count(node.get())) {
					++it;
					continue;
				}
			}
			it = running.erase(it);
		}
	}

	const property_value* TransitionState::find(const element* element, string_id name) const {
		const auto found = overrides.find(element);
		if (found == overrides.end()) return nullptr;
		const auto value = found->second.find(name);
		return value == found->second.end() ? nullptr : &value->second;
	}
}
