#pragma once

#include "css_animated_property.h"

#include <memory>
#include <unordered_map>

namespace litehtml { class element; class document; class html_tag; }

namespace docrender::css {
	class AnimationState {
		using Sample = std::map<std::size_t, AnimatedValue>;
		struct Stop {
			double offset;
			AnimatedValue value;
			std::optional<litehtml::css_easing> easing;
			bool implicit = false;
		};

		struct Track {
			std::string name;
			litehtml::css_animation animation;
			double elapsed = 0;
			double updated_at = 0;
			std::map<std::size_t, std::vector<Stop>> stops;

			double playback_time(double now) const;
			void sample(double now, Sample& output, bool& needs_frames) const;
			void resolve(const litehtml::html_tag& node, const std::shared_ptr<litehtml::document>& doc,
				const litehtml::css_keyframes& keyframes);
		};

		using ElementKey = std::weak_ptr<litehtml::element>;
		using ElementTracks = std::map<litehtml::string_id, std::vector<Track>>;
		std::map<ElementKey, ElementTracks, std::owner_less<>> tracks;
		std::unordered_map<const litehtml::element*, std::map<litehtml::string_id, litehtml::props_map>> overrides;
		bool playing = false;

	public:
		void commit(const std::shared_ptr<litehtml::document>& doc, double now);
		void sample(double now);
		bool active() const { return playing; }
		bool has_overrides() const { return !overrides.empty(); }
		const litehtml::property_value* find(const litehtml::element* element, litehtml::string_id name) const;
	};
}
