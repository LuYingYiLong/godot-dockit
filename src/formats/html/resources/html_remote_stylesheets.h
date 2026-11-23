#pragma once

#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <optional>

namespace dockit {
	inline constexpr int MAX_HTML_SOURCE_BYTES = 1024 * 1024;

	enum class RemoteStylesheetStatus { loading, ready, failed };

	struct RemoteStylesheet {
		RemoteStylesheetStatus status = RemoteStylesheetStatus::failed;
		godot::String url;
		godot::String text;
	};

	// 加载远程样式表
	std::optional<godot::String> decode_html_source_text(const godot::PackedByteArray& p_bytes);// 根据响应字节和字符集解码 HTML 源文本
	RemoteStylesheet request_remote_stylesheet(
		const godot::String& p_url,
		const godot::Callable& p_completed
	);																							// 发起远程样式表请求
	void shutdown_remote_stylesheets();															// 取消远程样式表请求并释放相关资源
}
