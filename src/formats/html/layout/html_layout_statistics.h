#pragma once

#include <godot_cpp/classes/os.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/string.hpp>

namespace dockit {

// 统计由宿主布局持有，关闭时不读取时钟
struct HTMLLayoutStatistics {
	enum Stage { REFRESH_REQUEST, REFRESH, STYLE, RENDER_TREE, MEASUREMENT, MEASUREMENT_HIT,
		LAYOUT, GEOMETRY, PAINT, TEXT_INDEX, ANCHORS, TAB_ORDER, SCROLLBAR, INTERACTION, HOVER, COUNT };
	// 仅调试构建提供完整计算参考路径，用于对照缓存结果
	static bool reference_mode() {
#ifdef DEBUG_ENABLED
		static const bool reference = godot::OS::get_singleton()->get_environment("DOCKIT_HTML_LAYOUT_REFERENCE") == "1";
		return reference;
#else
		return false;
#endif
	}
	bool enabled = false;
	uint64_t epoch = 0;
	std::array<uint64_t, COUNT> counts{};
	std::array<uint64_t, COUNT> usec{};
	void reset() { ++epoch; counts.fill(0); usec.fill(0); }
	void count(Stage p_stage) { if (enabled) ++counts[p_stage]; }
	godot::Dictionary snapshot() const {
		static constexpr std::array names{ "refresh_requests", "refresh", "style", "render_tree", "measurement",
			"measurement_cache_hits", "layout", "geometry", "paint", "text_index", "anchors", "tab_order",
			"scrollbar", "interaction", "hover" };
		godot::Dictionary result;
		for (size_t i = 0; i < COUNT; ++i) {
			result[godot::String(names[i]) + "_count"] = static_cast<int64_t>(counts[i]);
			result[godot::String(names[i]) + "_usec"] = static_cast<int64_t>(usec[i]);
		}
		return result;
	}
	class Scope {
		using Clock = std::chrono::steady_clock;
		HTMLLayoutStatistics* owner;
		Stage stage;
		uint64_t epoch = 0;
		Clock::time_point start;
	public:
		Scope(HTMLLayoutStatistics* p_owner, Stage p_stage) : owner(p_owner && p_owner->enabled ? p_owner : nullptr), stage(p_stage) {
			if (owner) { epoch = owner->epoch; ++owner->counts[stage]; start = Clock::now(); }
		}
		~Scope() {
			if (owner && owner->enabled && owner->epoch == epoch) owner->usec[stage] +=
				std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
		}
	};
};

} // namespace dockit
