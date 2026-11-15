#pragma once

#include <litehtml/gradient.h>

#include <optional>

namespace litehtml {
	struct css_cross_fade {
		struct Layer {
			image value;
			double weight;
		};
		std::vector<Layer> layers;
	};

	std::optional<image> parse_css_image(const css_token& p_token, document_container* p_container); // 解析 CSS 图片值及其候选项
	void compute_css_image(image& r_image, const string& p_base, document& p_doc, const font_metrics& p_font,
		const css_image_resolution& p_resolution, css_image_orientation p_orientation); // 解析图片来源和长度计算值
	double resolve_image_dppx(const string& p_url, const string& p_base, document_container& p_container,
		const css_image_resolution& p_resolution, bool p_from_image_set = false,
		std::optional<double> p_candidate_density = {}); // 按声明、候选项和元数据求出位图密度
	size get_css_image_natural_size(const image& p_image, const string& p_base, document_container& p_container); // 计算图片的自然尺寸
}
