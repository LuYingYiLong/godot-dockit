#pragma once

#include "css_animated_property.h"
#include "css_playback.h"

#include <memory>
#include <unordered_map>

namespace litehtml {
	class element;
	class document;
	class html_tag;
}

namespace dockit::css {
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
			Playback playback;
			std::shared_ptr<const litehtml::css_keyframes> scripted;
			std::map<std::size_t, std::vector<Stop>> stops;

			double duration() const;															// 获取动画持续时间
			double end() const;																	// 获取动画结束时间
			void sample(double now, Sample& output, bool& needs_frames) const;					// 计算动画在指定时刻的属性值
			void resolve(const litehtml::html_tag& node, const std::shared_ptr<litehtml::document>& doc,
				const litehtml::css_keyframes& keyframes);										// 根据节点、文档和关键帧解析 CSS 动画参数
		};

		using ElementKey = std::weak_ptr<litehtml::element>;
		using ElementTracks = std::map<litehtml::string_id, std::vector<Track>>;
		std::map<ElementKey, ElementTracks, std::owner_less<>> tracks;
		std::unordered_map<const litehtml::element*, std::map<litehtml::string_id, litehtml::props_map>> overrides;
		bool playing = false;
		uint64_t next_id = 1;

	public:
		uint64_t create(
			const std::shared_ptr<litehtml::element>& node,
			const std::shared_ptr<litehtml::document>& doc,
			std::shared_ptr<const litehtml::css_keyframes> frames,
			const litehtml::css_animation& settings, double now
		);																						// 创建元素的 CSS 动画轨道
		std::vector<uint64_t> list(const litehtml::element* target) const;						// 获取元素当前动画的标识列表
		std::optional<PlaybackInfo> info(uint64_t id, double now) const;						// 获取指定动画的播放信息
		bool control(uint64_t id, PlaybackCommand command, double value, double now);			// 控制指定动画的播放状态
		void commit(const std::shared_ptr<litehtml::document>& doc, double now);				// 提交 CSS 动画当前帧的样式值
		void sample(double now);																// 更新动画在指定时刻的属性值
		bool active() const { return playing; }													// 检查当前动画是否仍在播放
		bool has_overrides() const { return !overrides.empty(); }								// 检查当前动画是否具有样式覆盖项
		const litehtml::property_value* find(
			const litehtml::element* element,
			litehtml::string_id name
		) const;																				// 查找元素指定属性的动画覆盖值
	};
}
