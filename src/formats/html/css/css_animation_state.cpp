#include "css_animation_state.h"

#include <litehtml/document.h>
#include <litehtml/html.h>
#include <litehtml/html_tag.h>
#include <litehtml/style.h>

#include <algorithm>
#include <cmath>
#include <iterator>

namespace docrender::css {
	using namespace litehtml;

	namespace {
		const auto& animated_ids() {
			static const auto ids = [] {
				std::array<string_id, animated_property_names.size()> result;
				std::transform(animated_property_names.begin(), animated_property_names.end(), result.begin(),
					[](const char* name) { return _id(name); });
				return result;
			}();
			return ids;
		}

		std::optional<css_easing> keyframe_easing(const style& frame, const html_tag& node) {
			const auto& declaration = frame.get_property(css_animation_property_names()[3]);
			if (declaration.is<css_transition_easings>()) return declaration.get<css_transition_easings>().front();
			if (declaration.is<initial_value>()) return css_easing{};
			if (declaration.is<inherit>()) {
				const auto parent = node.parent();
				const auto* animations = parent ? parent->css().get_animations() : nullptr;
				return animations ? animations->easings.front() : css_easing{};
			}
			return std::nullopt;
		}
	}

	double AnimationState::Track::playback_time(double now) const {
		return elapsed + (animation.paused ? 0 : now - updated_at);
	}

	bool AnimationState::Track::resolve(const html_tag& node, const std::shared_ptr<document>& doc,
			const css_keyframes& keyframes) {
		const auto& ids = animated_ids();
		for (const auto& [offset, declarations] : keyframes.frames) {
			style frame = *declarations;
			frame.resolve_cascade(node, true);
			props_map properties;
			for (const auto id : ids) {
				const auto& value = frame.get_property(id);
				if (!value.is<invalid>() || value.declaration_property != empty_id)
					properties.emplace(id, value);
			}
			if (properties.empty()) continue;
			css_properties computed;
			computed.compute(&node, doc, &properties);
			const auto values = read_animated_values(computed);
			const auto easing = keyframe_easing(frame, node);
			for (std::size_t index = 0; index < ids.size(); ++index) {
				if (properties.count(ids[index])) stops[index].push_back({offset, values[index], easing});
			}
		}
		const auto underlying = read_animated_values(node.css());
		bool animates = false;
		for (std::size_t index = 0; index < stops.size(); ++index) {
			auto& series = stops[index];
			if (series.empty()) continue;
			animates = true;
			if (series.front().offset != 0) series.insert(series.begin(), {0, underlying[index], {}, true});
			if (series.back().offset != 1) series.push_back({1, underlying[index], {}, true});
		}
		return animates;
	}

	void AnimationState::Track::sample(double now, Sample& output, bool& needs_frames) const {
		const auto& settings = animation;
		const double local = playback_time(now) - settings.delay;
		const double duration = settings.duration == 0 ? 0 : settings.duration * settings.iterations;
		const bool before = local < 0;
		const bool after = local >= duration;
		needs_frames |= !settings.paused && !after;
		if ((before && !(settings.fill & 2)) || (after && !(settings.fill & 1))) return;

		const double overall = before ? 0 : after ? settings.iterations : local / settings.duration;
		double iteration = std::isfinite(overall) ? std::floor(overall) : 0;
		double progress = std::isfinite(overall) ? overall - iteration : 0;
		if (after && progress == 0 && iteration > 0) {
			--iteration;
			progress = 1;
		}
		const bool odd = std::fmod(iteration, 2.0) != 0;
		const bool reversed = ((settings.direction & 1) != 0) != (settings.direction >= 2 && odd);
		if (reversed) progress = 1 - progress;
		const bool before_flag = reversed ? after : before;
		for (std::size_t index = 0; index < stops.size(); ++index) {
			const auto& series = stops[index];
			if (series.empty()) continue;
			auto upper = std::upper_bound(series.begin(), series.end(), progress,
				[](double offset, const Stop& stop) { return offset < stop.offset; });
			if (upper == series.end()) --upper;
			const auto& start = *std::prev(upper);
			const auto& end = *upper;
			const auto& easing = start.easing ? *start.easing : settings.easing;
			const auto amount = easing.sample((progress - start.offset) / (end.offset - start.offset), before_flag);
			const auto& from = start.implicit && output[index] ? *output[index] : start.value;
			const auto& to = end.implicit && output[index] ? *output[index] : end.value;
			output[index] = can_interpolate_animated_value(from, to) ?
				interpolate_animated_value(from, to, amount, true) : amount < 0.5 ? from : to;
		}
	}

	void AnimationState::commit(const std::shared_ptr<document>& doc, double now) {
		decltype(tracks) updated;
		const auto visit = [&](const auto& visit, const element::ptr& node) -> void {
			if (!node || node->css().get_display() == display_none || is_generated_tag(node->tag())) return;
			const auto* settings = node->css().get_animations();
			const auto* subject = dynamic_cast<const html_tag*>(node.get());
			if (settings && subject) {
				const auto old = tracks.find(node);
				const auto old_count = old == tracks.end() ? 0 : old->second.size();
				std::vector<bool> matched(old_count);
				std::vector<Track> resolved;
				// The last occurrence of a repeated name keeps the last matching player.
				for (std::size_t index = settings->names.size(); index-- > 0;) {
					const auto& name = settings->names[index];
					const auto* rule = name ? doc->find_keyframes(*name) : nullptr;
					if (!rule) continue;
					Track track;
					track.name = *name;
					track.animation = settings->at(index);
					track.updated_at = now;
					for (std::size_t previous = old_count; previous-- > 0;) {
						const auto& prior = old->second[previous];
						if (matched[previous] || prior.name != *name) continue;
						matched[previous] = true;
						track.elapsed = prior.playback_time(now);
						break;
					}
					if (track.resolve(*subject, doc, *rule)) resolved.push_back(std::move(track));
				}
				if (!resolved.empty()) {
					std::reverse(resolved.begin(), resolved.end());
					updated.emplace(node, std::move(resolved));
				}
			}
			for (const auto& child : node->children()) visit(visit, child);
		};
		visit(visit, doc->root());
		tracks.swap(updated);
		sample(now);
	}

	void AnimationState::sample(double now) {
		playing = false;
		overrides.clear();
		for (auto it = tracks.begin(); it != tracks.end();) {
			const auto node = it->first.lock();
			if (!node) {
				it = tracks.erase(it);
				continue;
			}
			Sample sample;
			for (const auto& track : it->second) track.sample(now, sample, playing);
			props_map values;
			const auto& ids = animated_ids();
			for (std::size_t index = 0; index < sample.size(); ++index) {
				if (sample[index]) values.emplace(ids[index], std::visit(
					[](const auto& value) { return property_value(value, false); }, *sample[index]));
			}
			if (!values.empty()) overrides.emplace(node.get(), std::move(values));
			++it;
		}
	}

	const property_value* AnimationState::find(const element* element, string_id name) const {
		const auto subject = overrides.find(element);
		if (subject == overrides.end()) return nullptr;
		const auto value = subject->second.find(name);
		return value == subject->second.end() ? nullptr : &value->second;
	}
}
