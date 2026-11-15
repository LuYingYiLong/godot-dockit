#pragma once

#include <godot_cpp/variant/char_string.hpp>

#include <litehtml/document.h>
#include <litehtml/element.h>

namespace docrender {
	class LiteHTMLContainer;
}

namespace litehtml {
	struct box_shadow;
	// 分阶段的 litehtml 头文件让这个适配器可以访问原生的文档和元素状态
	struct docrender_editor {
		static document::ptr parse(
			const godot::CharString& p_html,
			docrender::LiteHTMLContainer& p_container,
			const string& p_default_styles,
			const string& p_user_styles
		);																			// 解析 UTF-8 文档并应用初始样式
		static bool media_changed(document& p_document);							// 检查媒体条件的匹配结果是否改变
		static void compute_styles(document& p_document);							// 按文档顺序更新计算样式与计数器
		static void restyle(
			const document::ptr& p_document,
			docrender::LiteHTMLContainer& p_container,
			const string& p_default_styles,
			const string& p_user_styles
		);																			// 在原有元素树上重算样式并重建渲染树
		static bool has_hover_styles(const document& p_document);					// 检查已解析的样式表是否引用悬停伪类
		static void set_hover(element& p_element, bool p_hovered);					// 更新元素的悬停伪类状态
		static bool is_command_button(const element& p_element);					// 判断元素是否有按钮激活行为
		static const std::vector<box_shadow>* get_box_shadows(
			const html_tag& p_element
		);																			// 读取层叠后的阴影列表

		static bool set_attribute(
			element& p_element,
			const char* p_name,
			const char* p_value
		);																			// 更新属性，空指针表示移除属性
		static elements_list parse_fragment(
			document& p_document,
			const element& p_parent,
			const godot::CharString& p_html
		);																			// 按父标签的上下文解析 HTML 片段
		static elements_list create_text(
			const document::ptr& p_document,
			const char* p_text,
			bool p_split
		);																			// 创建纯文本节点，可按排版需要拆分文本
		static void replace_children(element& p_parent, elements_list p_children);	// 替换父元素的子节点
		enum class child_edit_result { invalid, unchanged, changed };
		static child_edit_result place_child(const element::ptr& p_child, element& p_parent,
			const element::ptr& p_reference, bool p_replace_reference);			// 插入或替换子节点，保留节点身份并报告是否改变顺序
		static bool insert_children(element& p_source, element& p_parent,
			const element::ptr& p_reference);									// 按顺序移动来源的全部子节点

	private:
		static document::ptr parse_tree(const godot::CharString& p_html, document_container& p_container);
		static void apply_author_styles(const document::ptr& p_document, docrender::LiteHTMLContainer& p_container,
			const string& p_additional_styles);
		static void remove_attribute(html_tag& p_element, const char* p_name);
		static void reset_tree(element& p_element);
		static void trim_white_space(render_item& p_root);
	};
}
