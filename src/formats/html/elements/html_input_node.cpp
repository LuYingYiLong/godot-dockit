#include "html_input_node.h"
#include "html_gauge_elements.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "formats/html/backend/litehtml_container.h"

#include <litehtml/document.h>
#include <litehtml/render_item.h>

using namespace godot;

namespace docrender {
	HTMLInputNode::HTMLInputNode(const std::shared_ptr<litehtml::document>& p_document) : HTMLTextControlNode(p_document, false) {
		update_pseudos();
	}

	namespace {
		using Type = HTMLInputNode::Type;
		constexpr std::pair<const char*, Type> types[] = {
			{"text", Type::TEXT}, {"search", Type::SEARCH}, {"password", Type::PASSWORD}, {"email", Type::EMAIL},
			{"url", Type::URL}, {"tel", Type::TEL}, {"number", Type::NUMBER}, {"range", Type::RANGE},
			{"checkbox", Type::CHECKBOX}, {"radio", Type::RADIO}, {"hidden", Type::HIDDEN},
			{"button", Type::BUTTON}, {"submit", Type::SUBMIT}, {"reset", Type::RESET},
		};

		struct NumberLimits {
			std::optional<double> min;
			std::optional<double> max;
			std::optional<double> step;
			double base;
		};

		NumberLimits number_limits(const HTMLInputNode& p_node) {
			auto min = html_number(p_node.get_attr("min"), true);
			auto max = html_number(p_node.get_attr("max"), true);
			auto step = html_number(p_node.get_attr("step"), true);
			if (std::string_view(p_node.get_attr("step", "")) == "any") step.reset();
			else if (!step || *step <= 0) step = 1;
			if (p_node.type() == Type::RANGE) {
				min = min.value_or(0);
				max = std::max(*min, max.value_or(100));
			}
			const double base = min.value_or(html_number(p_node.get_attr("value"), true).value_or(0));
			return { min, max, step, base };
		}

		double constrain_number(double p_value, const NumberLimits& p_limits) {
			if (p_limits.min) p_value = std::max(*p_limits.min, p_value);
			if (p_limits.max) p_value = std::min(*p_limits.max, p_value);
			if (!p_limits.step) return p_value;
			const double steps = (p_value - p_limits.base) / *p_limits.step;
			if (!std::isfinite(steps)) return p_value;
			double candidate = std::fma(std::floor(steps + 0.5), *p_limits.step, p_limits.base);
			if (p_limits.max && candidate > *p_limits.max) {
				candidate = std::fma(std::floor((*p_limits.max - p_limits.base) / *p_limits.step), *p_limits.step, p_limits.base);
			}
			return std::isfinite(candidate) && (!p_limits.min || candidate >= *p_limits.min) ? candidate : p_value;
		}

		bool ascii_alnum(char32_t p_char) {
			return (p_char >= '0' && p_char <= '9') || (p_char >= 'a' && p_char <= 'z') || (p_char >= 'A' && p_char <= 'Z');
		}

		bool valid_email(const String& p_value) {
			const int64_t at = p_value.find("@");
			if (at <= 0 || at == p_value.length() - 1 || p_value.find("@", at + 1) >= 0) return false;
			const std::string_view special(".!#$%&'*+-/=?^_{}|~");
			for (int64_t i = 0; i < at; ++i) {
				const auto c = p_value[i];
				if (!ascii_alnum(c) && c != 0x60 && (c > 127 || special.find(static_cast<char>(c)) == std::string_view::npos)) return false;
			}
			for (const String& part : p_value.substr(at + 1).split(".", true)) {
				if (part.is_empty() || part.length() > 63 || !ascii_alnum(part[0]) || !ascii_alnum(part[part.length() - 1])) return false;
				for (int64_t i = 0; i < part.length(); ++i) if (!ascii_alnum(part[i]) && part[i] != '-') return false;
			}
			return true;
		}

		bool valid_absolute_url(const String& p_value) {
			const auto colon = p_value.find(":");
			if (colon <= 0) return false;
			const auto scheme = p_value.substr(0, colon).to_lower();
			if (scheme[0] < 'a' || scheme[0] > 'z') return false;
			for (int64_t i = 0; i < scheme.length(); ++i) {
				const auto c = scheme[i];
				if (!ascii_alnum(c) && c != '+' && c != '-' && c != '.') return false;
			}
			for (int64_t i = colon + 1; i < p_value.length(); ++i) if (p_value[i] <= ' ') return false;
			if (scheme == "http" || scheme == "https" || scheme == "ftp") {
				const auto rest = p_value.substr(colon + 1).lstrip("/");
				return !rest.is_empty() && rest[0] != '?' && rest[0] != '#';
			}
			return true;
		}
	}

	HTMLInputNode::Type HTMLInputNode::type() const {
		const String name = String::utf8(get_attr("type", "")).to_lower();
		for (const auto& [label, kind] : types) if (name == label) return kind;
		if (name == "date" || name == "datetime-local" || name == "month" || name == "week" || name == "time" ||
			name == "color" || name == "file" || name == "image") return Type::OTHER;
		return Type::TEXT;
	}

	String HTMLInputNode::type_name() const {
		const auto current = type();
		for (const auto& [label, kind] : types) if (current == kind) return label;
		return String::utf8(get_attr("type", "")).to_lower();
	}

	bool HTMLInputNode::is_text_control() const {
		const auto kind = type();
		return kind == Type::TEXT || kind == Type::SEARCH || kind == Type::PASSWORD || kind == Type::EMAIL ||
			kind == Type::URL || kind == Type::TEL || kind == Type::NUMBER;
	}

	bool HTMLInputNode::is_button() const {
		const auto kind = type();
		return kind == Type::BUTTON || kind == Type::SUBMIT || kind == Type::RESET;
	}

	bool HTMLInputNode::accepts_suggestions() const { return is_text_control() && !is_password(); }

	bool HTMLInputNode::is_suggestion_valid(const String& p_value) const {
		switch (type()) {
		case Type::EMAIL: return valid_email(p_value.strip_edges());
		case Type::URL: return valid_absolute_url(p_value.strip_edges());
		case Type::NUMBER: return html_number(p_value.utf8().get_data(), true).has_value();
		default: return true;
		}
	}

	bool HTMLInputNode::has_type_violation() const {
		const auto actual = editor.get_value();
		if (actual.is_empty()) return false;
		if (type() == Type::EMAIL && get_attr("multiple")) {
			for (const String& item : actual.split(",", true)) if (!valid_email(item.strip_edges())) return true;
			return false;
		}
		if (!is_suggestion_valid(actual)) return true;
		if (type() != Type::NUMBER) return false;
		const double value = value_as_number();
		const auto limits = number_limits(*this);
		if ((limits.min && value < *limits.min) || (limits.max && value > *limits.max)) return true;
		if (!limits.step) return false;
		const double steps = (value - limits.base) / *limits.step;
		return !std::isfinite(steps) || std::abs(steps - std::round(steps)) > 1e-7;
	}

	String HTMLInputNode::sanitize_value(const String& p_value) const {
		const auto kind = type();
		if (kind == Type::NUMBER && !html_number(p_value.utf8().get_data(), true)) return {};
		if (kind == Type::RANGE) {
			const auto limits = number_limits(*this);
			const double average = *limits.min / 2 + *limits.max / 2;
			const double value = constrain_number(html_number(p_value.utf8().get_data(), true).value_or(average), limits);
			return String::num_real(value, false);
		}
		if (kind == Type::EMAIL || kind == Type::URL) return p_value.replace("\r", "").replace("\n", "").strip_edges();
		return p_value;
	}

	double HTMLInputNode::value_as_number() const {
		const double missing = std::numeric_limits<double>::quiet_NaN();
		if (type() != Type::NUMBER && type() != Type::RANGE) return missing;
		return html_number(get_value().utf8().get_data(), true).value_or(missing);
	}

	bool HTMLInputNode::set_numeric_from_user(double p_value) {
		if (!std::isfinite(p_value)) return false;
		const auto value = String::num_real(constrain_number(p_value, number_limits(*this)), false);
		if (html_number(value.utf8().get_data(), true).value_or(p_value) == value_as_number()) return false;
		editor.set_selection(0, editor.get_value().length());
		return editor.insert(value);
	}

	bool HTMLInputNode::step_number(int p_step, std::optional<bool> p_endpoint) {
		const auto limits = number_limits(*this);
		if (p_endpoint) {
			const auto limit = *p_endpoint ? limits.max : limits.min;
			return limit && set_numeric_from_user(*limit);
		}
		const auto current = value_as_number();
		const auto step = limits.step.value_or(1);
		const double steps = std::isfinite(current) ? (current - limits.base) / step : 0;
		const double index = p_step < 0 ? std::ceil(steps - 1e-9) : std::floor(steps + 1e-9);
		return set_numeric_from_user(std::fma(index + p_step, step, limits.base));
	}

	bool HTMLInputNode::set_range_point(const Vector2& p_point) {
		if (!range_box.has_area()) return false;
		const auto limits = number_limits(*this);
		double progress = std::clamp<double>((p_point.x - range_box.position.x) / range_box.size.x, 0, 1);
		if (css().is_right_to_left()) progress = 1 - progress;
		return set_numeric_from_user((1 - progress) * *limits.min + progress * *limits.max);
	}

	void HTMLInputNode::set_attr(const char* p_name, const char* p_value) {
		html_tag::set_attr(p_name, p_value);
		attribute_changed(p_name);
	}

	void HTMLInputNode::attribute_changed(std::string_view p_name) {
		if (p_name == "value" || p_name == "type") {
			editor.use_default(sanitize_value(String::utf8(get_attr("value", ""))));
		}
		if ((p_name == "checked" || p_name == "type") && !dirty_checked) checked = get_attr("checked");
		update_pseudos();
	}

	bool HTMLInputNode::update_pseudos() {
		const Type kind = type();
		const bool supported = kind != Type::OTHER;
		const bool disabled = supported && forms::is_disabled(*this);
		bool changed = set_pseudo_class(litehtml::_id("disabled"), disabled);
		changed |= set_pseudo_class(litehtml::_id("enabled"), supported && !disabled);
		changed |= set_pseudo_class(litehtml::_id("checked"), is_toggle() && checked);
		changed |= set_pseudo_class(litehtml::_id("placeholder-shown"),
			is_text_control() && editor.get_value().is_empty() && get_attr("placeholder"));
		return changed;
	}

	String HTMLInputNode::get_value() const {
		if (is_toggle()) return String::utf8(get_attr("value", "on"));
		if (type() == Type::HIDDEN || is_button()) return String::utf8(get_attr("value", ""));
		return sanitize_value(editor.get_value());
	}

	bool HTMLInputNode::set_checked(bool p_checked) {
		dirty_checked = true;
		bool changed = checked != p_checked;
		checked = p_checked;
		if (p_checked && type() == Type::RADIO) {
			for (const auto& peer : forms::radio_group(*this)) {
				auto* input = static_cast<HTMLInputNode*>(peer.get());
				if (input != this && input->checked) {
					input->clear_group_checked();
					changed = true;
				}
			}
		}
		return changed;
	}

	void HTMLInputNode::clear_group_checked() {
		checked = false;
		update_pseudos();
	}

	void HTMLInputNode::reset() {
		editor.reset_value(sanitize_value(String::utf8(get_attr("value", ""))));
		dirty_checked = false;
		checked = get_attr("checked") != nullptr;
	}

	bool HTMLInputNode::copy_state_to(litehtml::element& p_target, HTMLNodeCopyMode p_mode) const {
		auto* target = dynamic_cast<HTMLInputNode*>(&p_target);
		if (!target) {
			return false;
		}
		target->editor.copy_value_state(editor, p_mode == HTMLNodeCopyMode::ADOPT);
		target->checked = checked;
		target->dirty_checked = dirty_checked;
		target->update_pseudos();
		return true;
	}

	void HTMLInputNode::draw(litehtml::uint_ptr p_hdc, litehtml::pixel_t p_x, litehtml::pixel_t p_y,
		const litehtml::position* p_clip, const std::shared_ptr<litehtml::render_item>& p_item) {
		html_tag::draw(p_hdc, p_x, p_y, p_clip, p_item);
		const auto pos = p_item->pos();
		auto* container = static_cast<LiteHTMLContainer*>(get_document()->container());
		paint_contents(*container, Rect2(pos.x + p_x, pos.y + p_y, pos.width, pos.height));
	}

	std::shared_ptr<litehtml::render_item> HTMLInputNode::create_render_item(const std::shared_ptr<litehtml::render_item>& p_parent) {
		if (type() == Type::HIDDEN) return {};
		return html_tag::create_render_item(p_parent);
	}

	void HTMLInputNode::paint_contents(LiteHTMLContainer& p_container, const Rect2& p_box) {
		const auto kind = type();
		if (kind == Type::OTHER || kind == Type::HIDDEN || !p_box.has_area()) return;
		const auto color = css().get_color();
		if (is_button()) {
			const char* fallback = kind == Type::RESET ? "Reset" : kind == Type::SUBMIT ? "Submit" : "";
			const auto line = p_container.shape_input(css().get_font(), String::utf8(get_attr("value", fallback)));
			if (line.is_valid()) p_container.draw_input(p_box, line,
				Vector2(p_box.position.x + std::max<real_t>(0, (p_box.size.x - line.width()) / 2),
					p_box.get_center().y + (line.line_ascent() - line.line_descent()) / 2), color, {}, {}, {});
			return;
		}
		if (kind == Type::RANGE) {
			range_box = p_box;
			if (css().get_appearance() == litehtml::appearance_none) return;
			const auto limits = number_limits(*this);
			const double span = *limits.max - *limits.min;
			double ratio = span > 0 ? (value_as_number() - *limits.min) / span : 0;
			if (!std::isfinite(ratio)) ratio = 0;
			if (css().is_right_to_left()) ratio = 1 - ratio;
			const auto accent = css().get_accent_color().to_srgb(color);
			const Color ink(accent.red / 255.0, accent.green / 255.0, accent.blue / 255.0, accent.alpha / 255.0);
			const real_t thumb = std::min<real_t>(p_box.size.x, std::min<real_t>(p_box.size.y, 12));
			const real_t offset = (p_box.size.x - thumb) * std::clamp(ratio, 0.0, 1.0);
			p_container.draw_input_chrome(p_box, Rect2(p_box.position.x, p_box.get_center().y - 2, p_box.size.x, 4), Color(0.79, 0.83, 0.88));
			p_container.draw_input_chrome(p_box, Rect2(p_box.position.x + offset, p_box.get_center().y - thumb / 2, thumb, thumb), ink);
			return;
		}
		if (is_toggle()) {
			if (checked && css().get_appearance() != litehtml::appearance_none) {
				auto mark = p_container.shape_input(css().get_font(), String::chr(kind == Type::RADIO ? 0x25cf : 0x2713));
				if (mark.is_valid()) {
					const Vector2 origin(p_box.get_center().x - mark.width() * 0.5f,
						p_box.get_center().y + (mark.line_ascent() - mark.line_descent()) * 0.5f);
					auto ink = color;
					if (!has_author_property(litehtml::_color_) && !has_author_property(litehtml::_background_color_) &&
						!has_pseudo_class(litehtml::_id("disabled"))) {
						const auto accent = css().get_accent_color().to_srgb(color);
						const float lightness = (accent.red * 0.2126f + accent.green * 0.7152f + accent.blue * 0.0722f) / 255;
						ink = lightness > 0.55f ? litehtml::web_color::black : litehtml::web_color::white;
					}
					p_container.draw_input(p_box, mark, origin, ink, {}, {}, {});
				}
			}
			return;
		}

		editor.paint_single(p_container, p_box, *this, String::utf8(get_attr("placeholder", "")), focused, is_password());
	}
}
