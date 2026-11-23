#pragma once

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <array>
#include <optional>
#include <vector>

namespace dockit {
	// 平面之间的映射，最后一行计算齐次除数
	class HTMLPlaneTransform {
		godot::Basis matrix;
		std::vector<godot::Vector2> project_polygon(const godot::Vector2* p_points, std::size_t p_count) const;

	public:
		HTMLPlaneTransform() = default;
		HTMLPlaneTransform(const godot::Transform2D& p_affine);
		
		explicit HTMLPlaneTransform(const godot::Basis& p_matrix) : matrix(p_matrix) {}

		const godot::Vector3& row(int p_index) const { return matrix[p_index]; }				// 获取变换矩阵的指定行
		godot::real_t determinant() const { return matrix.determinant(); }						// 获取变换矩阵的指定行列式
		bool is_affine() const;																	// 检查平面变换是否可表示为二维仿射变换
		godot::Transform2D affine_at(const godot::Vector2& p_point) const;						// 在指定平面坐标处构造二维仿射变换
		std::optional<HTMLPlaneTransform> try_inverse() const;									// 尝试计算变换矩阵的逆矩阵
		HTMLPlaneTransform inverse() const;														// 计算变换矩阵的逆矩阵
		HTMLPlaneTransform operator*(const HTMLPlaneTransform& p_other) const;					// 将两个平面变换矩阵相乘
		
		bool operator==(
			const HTMLPlaneTransform& p_other
		) const { return matrix == p_other.matrix; }											// 检查两个平面变换矩阵是否相等
		bool operator!=(const HTMLPlaneTransform& p_other) const { return !(*this == p_other); }// 检查两个平面变换矩阵是否不相等

		godot::Vector2 xform(const godot::Vector2& p_point) const;								// 将二维坐标变换到目标平面
		godot::Rect2 xform(const godot::Rect2& p_rect) const;									// 将二维坐标变换到目标平面
		std::vector<godot::Vector2> project_polygon(const std::vector<godot::Vector2>& p_points) const {
			return project_polygon(p_points.data(), p_points.size());
		}																						// 将多边形投影到目标平面
		
		template<std::size_t Size>
		std::vector<godot::Vector2> project_polygon(const std::array<godot::Vector2, Size>& p_points) const {
			return project_polygon(p_points.data(), p_points.size());
		}
	};
}
