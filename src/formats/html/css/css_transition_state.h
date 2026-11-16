#pragma once

#include "css_property_value.h"

#include <array>
#include <map>
#include <memory>
#include <unordered_map>
#include <variant>

namespace litehtml { class element; }

namespace docrender::css {
	class TransitionState {
		using ElementKey = std::weak_ptr<litehtml::element>;
		using Value = std::variant<float, litehtml::web_color, litehtml::css_length, litehtml::css_transform_list>;
		static constexpr std::size_t property_count = 10;
		using Values = std::array<Value, property_count>;

		struct Track {
			Value from;
			Value to;
			Value reversing_start;
			double start;
			double duration;
			double shortening = 1.0;
			litehtml::css_easing easing;
			bool discrete = false;
			bool allow_discrete = false;

			double progress(double now) const;
			Value sample(double now) const;
		};
		using Running = std::array<std::optional<Track>, property_count>;
		std::map<ElementKey, Running, std::owner_less<>> running;
		std::unordered_map<const litehtml::element*, litehtml::props_map> overrides;

		static Values values(const litehtml::element& element);
		static bool equal(const Value& a, const Value& b);
		static bool interpolable(const Value& a, const Value& b);
		static Value interpolate(const Value& a, const Value& b, double progress, bool allow_discrete);

	public:
		using Snapshot = std::map<ElementKey, Values, std::owner_less<>>;

		static Snapshot capture(const std::shared_ptr<litehtml::element>& root);
		void commit(const std::shared_ptr<litehtml::element>& root, const Snapshot& before, double now);
		void sample(double now);
		bool active() const { return !running.empty(); }
		bool has_overrides() const { return !overrides.empty(); }
		const litehtml::property_value* find(const litehtml::element* element, litehtml::string_id name) const;
	};
}
