#ifndef LH_EL_IMAGE_H
#define LH_EL_IMAGE_H

#include "html_tag.h"
#include "document.h"
#include "types.h"
#include "formats/html/html_image_sources.h"

namespace docrender { class HTMLImageAltText; }

namespace litehtml
{

	class el_image : public html_tag
	{
		string	m_src;
		string	m_baseurl;
		string m_current_src;
		std::optional<docrender::HTMLImageSelection> m_selection;
		std::shared_ptr<docrender::HTMLImageAltText> m_alt_text;
		bool m_decorative;
		const string& image_source() const;
		css_image_orientation image_orientation() const;
	public:
		el_image(const document::ptr& doc, const string& baseurl = {}, bool decorative = false);
		bool select_source(docrender::HTMLImageSelection p_selection); // 更新本轮选图并报告尺寸提示是否变化
		void commit_source(); // 发布最近一次成功排版的图片来源
		const string& get_current_src() const { return m_current_src; }
		docrender::HTMLImageAltText* get_alt_text() const { return m_alt_text.get(); }

		bool	is_replaced() const override;
		void	parse_attributes() override;
		void	compute_styles(bool recursive = true) override;
		void	draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri) override;
		void	get_content_size(size& sz, pixel_t max_width) override;
		string	dump_get_name() override;

		std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

	};
}

#endif  // LH_EL_IMAGE_H
