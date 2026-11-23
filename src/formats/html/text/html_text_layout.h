#pragma once

#include <godot_cpp/classes/text_line.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <memory>
#include <vector>

namespace godot {
	class TextServer;
}

namespace dockit {
	class HTMLTextSpacing;
	class HTMLTextSlice;

	struct HTMLTextCluster {
		int64_t start;
		int64_t end;
		float width;
	};

	struct HTMLTextRunMetrics {
		int64_t start;
		int64_t end;
		float ascent;
		float descent;
	};

	class HTMLTextLayout {
		struct LayoutMetrics {
			float ascent;
			float descent;
			std::vector<HTMLTextRunMetrics> runs;
		};
		godot::Ref<godot::TextLine> line;
		std::shared_ptr<const HTMLTextSlice> slice;
		std::shared_ptr<const LayoutMetrics> layout_metrics;
		godot::Vector2 inline_trim;
		int64_t source_prefix = 0;
		int64_t source_suffix = 0;

	public:
		float ascent = 0;
		float descent = 0;
		std::shared_ptr<const HTMLTextSpacing> spacing;

		HTMLTextLayout() = default;
		explicit HTMLTextLayout(const godot::Ref<godot::TextLine>& p_line, float p_ascent = 0,
			int64_t p_source_prefix = 0, int64_t p_source_suffix = 0);
		bool is_valid() const { return line.is_valid(); }										// 检查当前对象是否有效
		bool is_subrange() const { return bool(slice); }										// 检查子范围
		godot::RID get_rid() const;																// 获取RID
		int64_t text_start(const godot::TextServer& p_server) const;							// 获取文本片段在 TextServer 中的起始位置
		godot::Vector2 size() const;															// 获取文本布局的二维尺寸
		float width() const;																	// 获取文本布局的可用宽度
		float native_width() const;																// 获取 TextServer 中对应文本布局的宽度
		float trim_start() const { return inline_trim.x; }										// 获取行内修整的起始偏移
		void trim_inline_advance(float p_start, float p_end);									// 按指定起止位置修整行内文本推进量
		float height() const;																	// 获取文本布局的高度
		float line_ascent() const;																// 获取行基线上方的上升高度
		float line_descent() const;																// 获取行基线下方的下降高度
		void set_run_metrics(std::vector<HTMLTextRunMetrics> p_runs);							// 设置片段度量信息
		godot::Vector2 metrics_in_range(int64_t p_start, int64_t p_end) const;					// 获取指定文本范围的宽度和位置
		float underline_position() const;														// 获取字体下划线相对基线的位置
		float underline_thickness() const;														// 获取字体下划线的厚度
		std::vector<HTMLTextCluster> clusters(const godot::String& p_text) const;				// 将文本拆分为字素簇并映射到字符范围
		HTMLTextLayout subrange(int64_t p_start, int64_t p_end) const;							// 创建覆盖指定字符范围的文本布局
		godot::PackedVector2Array selection(
			const godot::TextServer& p_server,
			int64_t p_from,
			int64_t p_to
		) const;																				// 获取文本选区对应的绘制矩形
		int64_t hit_test(double p_x) const;														// 获取指定横坐标对应的文本偏移
		void draw(const godot::TextServer* p_server, const godot::RID& p_canvas, const godot::Vector2& p_position,
			const godot::Color& p_color) const;													// 绘制已排版的文本
	};
}
