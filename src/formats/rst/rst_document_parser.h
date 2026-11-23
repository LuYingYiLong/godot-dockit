#pragma once

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <vector>

using namespace godot;

class RestructuredtextLabel;

enum RSTBlockType {
	RST_BLOCK_PARAGRAPH,
	RST_BLOCK_HEADING,
	RST_BLOCK_BULLET_LIST,
	RST_BLOCK_ENUMERATED_LIST,
	RST_BLOCK_CODE,
	RST_BLOCK_ADMONITION,
	RST_BLOCK_TABLE,
	RST_BLOCK_IMAGE,
	RST_BLOCK_TOCTREE,
	RST_BLOCK_TRANSITION,
};

struct RSTBlock {
	RSTBlockType type = RST_BLOCK_PARAGRAPH;
	String text;
	String argument;
	String directive;
	String image_alt;
	String image_align;
	String image_width;
	String image_height;
	String image_scale;
	PackedStringArray items;
	std::vector<PackedStringArray> rows;
	int32_t level = 0;
	int32_t columns = 0;
	int32_t header_rows = 0;
	int32_t line = 0;
	String anchor;
};

struct RSTFootnoteEntry {
	String label;
	String display_label;
};

struct RSTFootnoteMap {
	std::vector<RSTFootnoteEntry> entries;
};

// 解析 reStructuredText
String format_footnote_label(const String& p_label, const RSTFootnoteMap* p_footnotes);			// 根据脚注定义格式化脚注标签
RSTFootnoteMap build_footnote_map(const std::vector<String>& p_lines);							// 从 RST 源文本构建脚注索引
std::vector<String> split_lines(const String& p_text);											// 按行拆分源文本
std::vector<RSTBlock> parse_rst_blocks(
	const String& p_text,
	RestructuredtextLabel* p_owner,
	const RSTFootnoteMap* p_footnotes = nullptr
);																								// 解析 reStructuredText 文档块
