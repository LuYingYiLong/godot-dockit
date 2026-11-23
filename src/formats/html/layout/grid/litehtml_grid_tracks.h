#pragma once

#include "litehtml_grid_placement.h"
#include "formats/html/css/css_grid_values.h"

namespace litehtml {
	struct expanded_grid_tracks : grid_track_segment {
		std::size_t explicit_count = 0;
		std::optional<grid_track_range> fit_range;
		std::unordered_map<std::string, grid_track_range> areas;
	};

	// 用同样的方式计算排水沟，用于大小贡献和定位
	struct grid_track_gutters {
		std::size_t track_count;
		std::vector<std::size_t> active_before;

		std::size_t active_count() const {
			return active_before.empty() ? track_count : active_before.back();
		}																						// 获取当前有效网格轨道数量
		std::size_t count_inside(grid_track_range range) const;									// 统计位于网格轨道范围内的子项数量
		bool after(std::size_t track) const;													// 判断当前标点间距是否位于另一个标点之后
	};

	struct grid_track_contribution {
		grid_track_range range;
		pixel_t minimum = 0;
		pixel_t min_content = 0;
		pixel_t preferred = 0;
	};

	struct grid_track_sizes {
		std::vector<pixel_t> tracks;
		pixel_t minimum = 0;
		pixel_t extent = 0;
	};

	// 计算 CSS Grid 轨道
	expanded_grid_tracks expand_grid_tracks(
		const grid_track_template& tracks,
		const css_length& gap,
		std::optional<pixel_t> definite,
		std::optional<pixel_t> minimum,
		std::optional<pixel_t> maximum
	);																							// 展开 CSS Grid 轨道定义
	void apply_grid_template_areas(
		const grid_area_template& areas,
		expanded_grid_tracks& rows,
		expanded_grid_tracks& columns,
		bool shared_rows,
		bool shared_columns
	);																							// 将网格模板区域应用到行和列轨道
	grid_line_indices grid_placement_names(const expanded_grid_tracks& axis);					// 构建网格线名称到轨道索引的映射
	grid_track_gutters finish_grid_tracks(
		expanded_grid_tracks& axis,
		const grid_track_list& pattern,
		std::size_t offset,
		std::size_t count,
		const std::vector<grid_item_placement>& items,
		grid_track_range grid_item_placement::* dimension
	);																							// 根据轨道范围和子项确定 CSS Grid 轨道间距
	bool grid_tracks_use_percentage(const grid_track_list& tracks);								// 检查网格轨道定义是否使用百分比长度
	bool grid_range_needs_content(
		const grid_track_list& tracks,
		grid_track_range range,
		bool definite
	);																							// 检查网格轨道范围是否依赖内容尺寸
	std::optional<pixel_t> fixed_grid_span(
		const grid_track_list& tracks,
		grid_track_range range,
		const grid_track_gutters& gutters,
		const css_length& gap,
		std::optional<pixel_t> available
	);																							// 计算由固定轨道组成的网格跨度尺寸
	bool grid_range_has_auto_minimum(
		const grid_track_list& tracks,
		grid_track_range range,
		bool definite
	);																							// 检查网格跨轨道范围是否包含自动最小尺寸
	grid_track_sizes size_grid_tracks(
		const grid_track_list& tracks,
		const std::vector<grid_track_contribution>& contributions,
		const grid_track_gutters& gutters,
		pixel_t gap,
		std::optional<pixel_t> available,
		bool stretch
	);																							// 计算 CSS Grid 轨道尺寸

}
