#pragma once

#include "formats/html/css/css_style_sheet.h"

#include <litehtml/html_tag.h>
#include <godot_cpp/classes/ref.hpp>

#include <cstdint>

namespace dockit {
	class HTMLStylesheetNode : public litehtml::html_tag {
	public:
		using html_tag::html_tag;
		uint64_t source_version = 0;
		bool style_disabled = false;
		litehtml::string linked_request_path;
		litehtml::string linked_path;
		litehtml::string linked_text;
		godot::Ref<CSSStyleSheet> linked_resource;

		bool accepts_style_text() const;														// 检查样式元素是否接受指定文本内容
		void parse_attributes() override;														// 解析属性
		void compute_styles(bool p_recursive) override;											// 计算样式
		bool removeChild(const litehtml::element::ptr& p_child) override;						// 从当前节点移除指定子节点
	};
}
