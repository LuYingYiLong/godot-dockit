#pragma once

#include "css_animated_property.h"
#include "css_playback.h"

#include <functional>
#include <map>
#include <memory>
#include <unordered_map>

namespace litehtml { class element; }

namespace dockit::css {
	class TransitionState {
		using ElementKey = std::weak_ptr<litehtml::element>;
		using Value = AnimatedValue;
		using Values = AnimatedValues;

		struct Track {
			Value from;
			Value to;
			Value reversing_start;
			double delay;
			double duration;
			double shortening = 1.0;
			litehtml::css_easing easing;
			bool discrete = false;
			bool allow_discrete = false;
			Playback playback;

			double end() const { return std::max(0.0, delay + duration); }						// 获取动画结束时间
			double progress(double now) const;													// 计算过渡的当前进度
			Value sample(double now, const AnimatedProperty& property) const;					// 根据当前进度计算过渡属性值
		};
		using Running = std::map<std::size_t, Track>;
		std::map<ElementKey, Running, std::owner_less<>> running;
		std::unordered_map<const litehtml::element*, litehtml::props_map> overrides;
		uint64_t next_id = 1;
		bool playing = false;

	public:
		std::vector<uint64_t> list(const litehtml::element* target, double now) const;			// 获取元素当前过渡的标识列表
		std::optional<PlaybackInfo> info(uint64_t id, double now) const;						// 获取指定动画的播放信息
		bool control(uint64_t id, PlaybackCommand command, double value, double now);			// 控制指定过渡的播放状态
		using Snapshot = std::map<ElementKey, Values, std::owner_less<>>;
		using TransformInterpolationCheck = std::function<bool(const litehtml::element&, const litehtml::css_transform_list&)>;

		static Snapshot capture(const std::shared_ptr<litehtml::element>& root);				// 捕获根节点下各元素的当前样式
		void commit(
			const std::shared_ptr<litehtml::element>& root,
			const Snapshot& before,
			double now,
			const TransformInterpolationCheck& can_interpolate_transform
		);																						// 提交 CSS 动画当前帧的样式值
		void sample(double now);																// 更新过渡在指定时刻的属性值
		bool active() const { return playing; }													// 检查当前动画是否仍在播放
		bool has_overrides() const { return !overrides.empty(); }								// 检查当前动画是否具有样式覆盖项
		const litehtml::property_value* find(
			const litehtml::element* element,
			litehtml::string_id name
		) const;																				// 查找元素指定属性的过渡覆盖值
	};
}
