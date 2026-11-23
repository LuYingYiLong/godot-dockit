#pragma once

#include "html_text_control_node.h"
#include "html_node_copy_state.h"
#include "html_input_element.h"
#include "html_input_time.h"

#include <optional>
#include <string_view>

namespace dockit {
	class HTMLInputNode final : public HTMLTextControlNode, public HTMLNodeCopyState {
		bool checked = false;
		bool dirty_checked = false;
		godot::Rect2 range_box;
		godot::Rect2 decoration_box;
		godot::Rect2 suggestion_decoration_box;
		godot::Rect2 image_box;
		std::optional<godot::Color> preview_color;
		std::optional<godot::String> preview_time;
		bool image_available = false;
		bool indeterminate = false;
		godot::TypedArray<HTMLInputFile> files;

		godot::String button_text() const;
		void paint_contents(LiteHTMLContainer& p_container, const godot::Rect2& p_box);

	public:
		enum class Type {
			TEXT,
			SEARCH,
			PASSWORD,
			EMAIL,
			URL,
			TEL,
			NUMBER,
			RANGE,
			CHECKBOX,
			RADIO,
			HIDDEN,
			BUTTON,
			SUBMIT,
			RESET,
			DATE,
			MONTH,
			WEEK,
			TIME,
			LOCAL,
			COLOR,
			FILE,
			IMAGE
		};

	private:
		Type previous_type = Type::TEXT;

	public:

		explicit HTMLInputNode(const std::shared_ptr<litehtml::document>& p_document);
		void set_attr(const char* p_name, const char* p_value) override;						// 设置属性
		void parse_attributes() override;														// 解析属性
		bool is_replaced() const override;														// 检查替换元素
		void get_content_size(litehtml::size& r_size, litehtml::pixel_t p_width) override;		// 获取内容尺寸
		std::shared_ptr<litehtml::render_item> create_render_item(
			const std::shared_ptr<litehtml::render_item>& p_parent
		) override;																				// 创建 input 控件的布局渲染项
		void draw(
			litehtml::uint_ptr p_hdc,
			litehtml::pixel_t p_x,
			litehtml::pixel_t p_y,
			const litehtml::position* p_clip,
			const std::shared_ptr<litehtml::render_item>& p_item
		) override;																				// 绘制 input 控件及其当前值
		void attribute_changed(std::string_view p_name);										// 响应 HTML 属性变化并更新控件状态
		Type type() const;																		// 获取 input 控件类型
		godot::String type_name() const;														// 获取 input 类型的字符串名称
		bool is_text_control() const override;													// 检查文本控件
		bool is_button() const;																	// 检查按钮
		bool supports_selection() const;														// 检查是否支持选区
		const char* maxlength() const;															// 获取允许输入的最大字符数
		bool supports_readonly() const;															// 检查当前 input 类型是否支持 readonly 属性
		bool supports_required() const;															// 检查当前 input 类型是否支持 required 属性
		bool has_picker() const;																// 检查是否包含选择器
		double step_scale() const;																// 获取当前日期或时间类型的步长换算比例
		double step_base() const;																// 获取当前控件的步长基准值
		input_time::Kind time_kind() const;														// 获取当前日期或时间输入类型
		bool is_password() const { return type() == Type::PASSWORD; }							// 检查 input 类型是否为 password
		bool accepts_suggestions() const;														// 检查控件是否接受建议值
		bool is_suggestion_valid(const godot::String& p_value) const;							// 检查建议项有效性
		godot::Dictionary validity() const;														// 获取控件的约束验证结果
		double value_as_number() const;															// 将当前控件值转换为数值
		godot::String format_number(double p_value) const;										// 格式化数字
		godot::String sanitize_value(const godot::String& p_value) const;						// 按 input 类型规范化控件值
		godot::Error step_number(
			int64_t p_step,
			bool& r_changed,
			std::optional<bool> p_endpoint = {}
		);																						// 根据指定方向调整控件数值并应用步长
		bool set_range_point(const godot::Vector2& p_point);									// 设置范围坐标
		bool set_numeric_from_user(double p_value);												// 应用用户输入的数字值并更新编辑状态
		bool update_pseudos() override;															// 更新伪类
		godot::String get_value() const;														// 获取值
		bool is_checked() const { return checked; }												// 检查选中状态
		bool set_checked(bool p_checked);														// 设置选中状态
		void clear_group_checked();																// 清除组选中状态
		void reset();																			// 将控件值和交互状态重置为初始值
		bool is_indeterminate() const { return indeterminate; }									// 检查不确定状态
		bool set_indeterminate(bool p_value);													// 设置不确定状态
		const godot::TypedArray<HTMLInputFile>& get_files() const { return files; }				// 获取文件
		bool set_files(const godot::TypedArray<HTMLInputFile>& p_files);						// 设置文件
		static godot::Color color_preview(const godot::String& p_value);						// 将颜色值转换为预览颜色
		void set_color_preview(std::optional<godot::Color> p_color) { preview_color = p_color; }// 设置颜色预览
		void set_time_preview(
			std::optional<godot::String> p_value
		) {
			preview_time = std::move(p_value);
		}																						// 设置时间预览
		int decoration_at(const godot::Vector2& p_point) const;									// 获取指定坐标命中的文本装饰部件
		godot::Vector2i image_coordinates(const godot::Vector2& p_point) const;					// 获取图像输入控件内图像坐标对应的值
		bool edit_time_segment(int p_direction, bool p_step);									// 按方向调整当前选中的日期或时间字段
		bool is_toggle() const {
			const auto kind = type(); return kind == Type::CHECKBOX || kind == Type::RADIO;
		}																						// 检查输入类型是否为复选框或单选框
		bool copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const override;// 将当前控件状态复制到目标节点
	};
}
