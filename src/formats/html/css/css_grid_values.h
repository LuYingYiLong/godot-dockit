#pragma once

#include "css_grid_lines.h"
#include "css_length.h"

#include <array>
#include <optional>

namespace litehtml {

enum class grid_breadth_kind { automatic, length, fraction, min_content, max_content, fit_content };

struct grid_breadth {
	grid_breadth_kind kind = grid_breadth_kind::automatic;
	css_length length;
	double fraction = 0;

	bool has_length() const {
		return kind == grid_breadth_kind::length || kind == grid_breadth_kind::fit_content;
	}																							// 检查网格轨道定义是否包含长度值
};

struct grid_track {
	grid_breadth minimum;
	grid_breadth maximum;
};

using grid_track_list = std::vector<grid_track>;

struct grid_track_segment {
	grid_track_list tracks;
	grid_line_indices names;
};

struct grid_auto_repeat {
	grid_track_segment pattern;
	bool fit;
};

struct grid_subgrid_lines {
	using name_list = std::vector<std::vector<std::string>>;
	name_list prefix;
	name_list automatic;
	name_list suffix;
};

struct grid_track_template {
	grid_track_segment prefix;
	std::optional<grid_auto_repeat> repeat;
	grid_track_segment suffix;
	std::optional<grid_subgrid_lines> subgrid;
};

struct grid_area_template {
	std::size_t rows = 0;
	std::size_t columns = 0;
	std::unordered_map<std::string, grid_item_placement> areas;
};

struct grid_definition {
	grid_track_template rows;
	grid_track_template columns;
	grid_area_template areas;
	grid_track_list auto_rows;
	grid_track_list auto_columns;
	grid_auto_flow flow;
};

// 解析 CSS Grid 值
bool parse_grid_tracks(const css_token_vector& tokens, grid_track_template& tracks);			// 解析 CSS Grid 轨道列表
bool parse_grid_auto_tracks(const css_token_vector& tokens, grid_track_list& tracks);			// 解析 CSS Grid 自动轨道列表
bool parse_grid_auto_flow(const css_token_vector& tokens, grid_auto_flow& flow);				// 解析 CSS Grid 自动放置方向
bool parse_grid_template_areas(const css_token_vector& tokens, grid_area_template& areas);		// 解析 CSS Grid 模板区域
bool parse_grid_definition(
	const css_token_vector& tokens,
	bool allow_auto_flow,
	grid_definition& definition
);																								// 解析 CSS Grid 行列定义
bool parse_grid_line(const css_token_vector& tokens, grid_line& line);							// 解析 CSS Grid 网格线标识
bool parse_grid_axis_placement(const css_token_vector& tokens, grid_axis_placement& placement);	// 解析 CSS Grid 单轴放置范围
bool parse_grid_area(const css_token_vector& tokens, std::array<grid_line, 4>& lines);			// 解析 CSS Grid 区域边界
void append_grid_segment(grid_track_segment& to, const grid_track_segment& from);				// 追加网格线段
void add_subgrid_line_names(
	const grid_subgrid_lines& value,
	std::size_t track_count,
	grid_line_indices& names
);																								// 添加子网格行名称

}
