#pragma once

#include "css_selectors.h"

#include <limits>
#include <map>
#include <cstdint>
#include <memory>
#include <string>

namespace dockit::css {
	// Layer ranks follow normal precedence. Special ranks represent the baseline, unlayered rules and inline declarations.
	inline constexpr int css_default_layer = -1;
	inline constexpr int css_unlayered = std::numeric_limits<int>::max() - 1;
	inline constexpr int css_inline_layer = std::numeric_limits<int>::max();

	enum class CSSCascadeOrigin : int {
		user_agent,
		author,
		author_extra,
	};

	struct CSSCascadeSource {
		CSSCascadeOrigin origin = CSSCascadeOrigin::author;
		std::string selector;
		std::string path;
		const void* stylesheet_owner = nullptr;
	};

	struct CascadePriority {
		int layer = css_default_layer;
		selector_specificity specificity;
		int source_order = 0;
		int proximity = std::numeric_limits<int>::max();
		std::uint64_t declaration_order = 0;
		std::shared_ptr<const CSSCascadeSource> source;
	};

	bool takes_cascade_precedence(
		const CascadePriority& candidate,
		bool candidate_important,
		const CascadePriority& current,
		bool current_important
	);																							// 比较两个声明的 CSS 层叠优先级

	class CascadeLayers {
		struct Layer {
			vector<size_t> children;
			std::map<string, size_t> named_children;
			int rank = css_unlayered;
		};
		vector<Layer> layers{ Layer{} };

		size_t append_child(size_t parent);

	public:
		size_t register_layer(size_t parent, const vector<string>& names);						// 注册 CSS 层并返回其内部索引
		void assign_ranks();																	// 根据层顺序分配层叠优先级
		int rank(size_t layer) const { return layers[layer].rank; }								// 获取指定层的层叠顺序等级
		bool has_layers() const { return layers.size() > 1; }									// 检查是否包含层
	};
} // namespace dockit::css
