#pragma once

#include "css_selectors.h"

#include <functional>
#include <optional>
#include <string_view>

namespace dockit::css {

	// The DOM owner keeps nodes and borrowed strings alive during a synchronous match.
	using SelectorNode = const void*;
	enum class SelectorNodeKind { element, generated, text, whitespace, other };

	class SelectorDOM {
	public:
		virtual ~SelectorDOM() = default;
		virtual SelectorNode parent(SelectorNode node) const = 0;								// 获取选择器节点的父节点
		// A true result stops traversal.
		virtual void visit_children(SelectorNode node, bool reverse,
			const std::function<bool(SelectorNode)>& visit) const = 0;
		virtual SelectorNodeKind kind(SelectorNode node) const = 0;								// 获取选择器节点的类型
		virtual bool is_matchable(SelectorNode node) const = 0;									// 检查节点是否可参与选择器匹配
		virtual std::string_view tag(SelectorNode node) const = 0;								// 获取选择器节点的标签名
		virtual bool same_tag(SelectorNode left, SelectorNode right) const = 0;					// 检查两个节点的标签名是否相同
		virtual std::string_view id(SelectorNode node) const = 0;								// 获取选择器节点的 id 属性
		virtual bool has_class(SelectorNode node, std::string_view name) const = 0;				// 检查选择器节点是否具有指定 class
		virtual std::optional<std::string_view> attribute(
			SelectorNode node,
			const std::string& name
		) const = 0;																			// 获取选择器节点的指定属性值
		virtual bool matches_direction(SelectorNode node, std::string_view direction) const = 0;// 检查节点方向是否匹配指定值
		virtual bool has_pseudo_class(SelectorNode node, std::string_view name) const = 0;		// 检查节点是否具有指定伪类
	};

	enum class MatchMode { candidate, complete };
	struct NestingMatchCache;

	struct MatchContext {
		SelectorNode scope = nullptr;
		MatchMode mode = MatchMode::complete;
		bool virtual_scope = false;
		NestingMatchCache* nesting_cache = nullptr;
	};

	enum MatchFlag : unsigned {
		match_none = 0,
		match_element = 1 << 0,
		match_deferred_pseudo_class = 1 << 1,
		match_before = 1 << 2,
		match_after = 1 << 3,
		match_marker = 1 << 4,
		match_placeholder = 1 << 5,
		match_selection = 1 << 6,
		match_backdrop = 1 << 7,
	};

	struct ScopeRule {
		std::shared_ptr<const ScopeRule> parent;
		Selector::vector start;
		Selector::vector end;
		SelectorNode implicit_root = nullptr;
		unsigned state_dependencies() const;													// 获取作用域匹配依赖的动态状态标记
	};

	struct SelectorMatch {
		unsigned flags = match_none;
		int proximity = std::numeric_limits<int>::max();
	};

// 匹配 CSS 选择器
	unsigned match_selector(const SelectorDOM& dom, SelectorNode node, const Selector& selector,
		const MatchContext& context = {});														// 匹配单个 CSS 选择器
	unsigned match_selector_list(const SelectorDOM& dom, SelectorNode node, const Selector::vector& selectors,
		const MatchContext& context = {});														// 匹配 CSS 选择器列表
	SelectorMatch match_scoped_selector(const SelectorDOM& dom, SelectorNode node, const Selector& selector,
		const ScopeRule& scope, MatchMode mode);												// 在指定作用域内匹配 CSS 选择器
	std::optional<std::string_view> inherited_language(
		const SelectorDOM& dom,
		SelectorNode node
	);																							// 查找节点继承得到的语言标记
	bool matches_language(
		const SelectorDOM& dom,
		SelectorNode node,
		const vector<string>& ranges
	);																							// 检查元素语言是否匹配指定范围

} // namespace dockit::css
