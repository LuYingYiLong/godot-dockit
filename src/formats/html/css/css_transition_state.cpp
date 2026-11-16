#include "css_transition_state.h"

#include <litehtml/element.h>
#include <litehtml/html.h>
#include <litehtml/html_tag.h>

#include <algorithm>
#include <cmath>

namespace docrender::css {
	using namespace litehtml;

	namespace {
		template<class Visitor>
		void visit_styled(const element::ptr& node, const Visitor& visit) {
			if (!node || node->css().get_display() == display_none || is_generated_tag(node->tag())) return;
			if (dynamic_cast<const html_tag*>(node.get())) visit(node);
			if (node->css().hides_contents()) return;
			for (const auto& child : node->children()) visit_styled(child, visit);
		}
	}

	double TransitionState::Track::progress(double now) const {
		return now < start ? 0 : duration == 0 || now >= start + duration ? 1 : easing.sample((now - start) / duration);
	}

	TransitionState::Value TransitionState::Track::sample(double now, const AnimatedProperty& property) const {
		if (now < start) return from;
		if (now >= start + duration) return to;
		const auto amount = progress(now);
		if (discrete) return amount < 0.5 ? from : to;
		return interpolate_animated_value(property, from, to, amount, allow_discrete);
	}

	TransitionState::Snapshot TransitionState::capture(const std::shared_ptr<element>& root) {
		Snapshot before;
		visit_styled(root, [&](const element::ptr& node) {
			if (node->get_render_item() || node->css().get_display() == display_contents)
				before.emplace(node, read_animated_values(node->css()));
		});
		return before;
	}

	void TransitionState::commit(const std::shared_ptr<element>& root, const Snapshot& before, double now,
			const TransformInterpolationCheck& can_interpolate_transform) {
		decltype(running) next;
		visit_styled(root, [&](const element::ptr& node) {
			const auto* styles = node->css().get_transitions();
			if (!styles || (!node->get_render_item() && node->css().get_display() != display_contents)) return;
			const auto old = before.find(node);
			if (old == before.end()) return;
			const auto previous = running.find(node);
			const auto& properties = animated_properties();
			const bool rtl = node->css().is_right_to_left();
			Running updated;
			for (std::size_t index = 0; index < properties.size(); ++index) {
				const auto& property = properties[index];
				const auto config = styles->match(property.name, rtl);
				if (!config) continue;
				const Track* current = nullptr;
				if (previous != running.end()) {
					const auto track = previous->second.find(index);
					if (track != previous->second.end()) current = &track->second;
				}
				auto to = property.read(node->css());
				if (current && equal_animated_value(property, current->to, to)) {
					updated.emplace(index, *current);
					continue;
				}
				const auto from = current ? current->sample(now, property) : old->second[index];
				if (equal_animated_value(property, from, to) || config->duration + config->delay <= 0) continue;
				bool discrete = !can_interpolate_animated_value(property, from, to);
				if (std::holds_alternative<css_transform_list>(from)) {
					const auto sample = std::get<css_transform_list>(interpolate_animated_value(property, from, to, 0.5, false));
					discrete = !can_interpolate_transform(*node, sample);
				}
				if (discrete && !config->allow_discrete) continue;
				const bool reversing = current && now < current->start + current->duration &&
					equal_animated_value(property, current->reversing_start, to);
				const double factor = reversing ? std::clamp(std::abs(current->progress(now) * current->shortening +
					(1 - current->shortening)), 0.0, 1.0) : 1.0;
				const auto delay = config->delay < 0 ? config->delay * factor : config->delay;
				updated.emplace(index, Track{from, std::move(to), reversing ? current->to : from, now + delay,
					config->duration * factor, factor, config->easing, discrete, config->allow_discrete});
			}
			if (!updated.empty())
				next.emplace(node, std::move(updated));
		});
		running.swap(next);
		sample(now);
	}

	void TransitionState::sample(double now) {
		overrides.clear();
		const auto& properties = animated_properties();
		for (auto it = running.begin(); it != running.end();) {
			if (const auto node = it->first.lock()) {
				props_map values;
				for (auto current = it->second.begin(); current != it->second.end();) {
					const auto& [index, track] = *current;
					if (now >= track.start + track.duration) {
						current = it->second.erase(current);
						continue;
					}
					const auto& property = properties[index];
					const auto sample = track.sample(now, property);
					values.emplace(property.name, std::visit(
						[](const auto& value) { return property_value(value, false); }, sample));
					++current;
				}
				if (!values.empty()) {
					overrides.emplace(node.get(), std::move(values));
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
