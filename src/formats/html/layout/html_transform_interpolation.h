#pragma once

#include <godot_cpp/variant/projection.hpp>

#include <optional>

namespace dockit {
	// 如果任一端点无法分解，则不返回矩阵
	std::optional<godot::Projection> interpolate_transform_matrices(const godot::Projection& p_from,
		const godot::Projection& p_to, double p_progress);
}
