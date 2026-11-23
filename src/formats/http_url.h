#pragma once

#include <godot_cpp/variant/string.hpp>

namespace dockit {
// 解析 HTTP URL
	bool is_http_url(const godot::String& p_url);												// 检查字符串是否为 HTTP 或 HTTPS URL
	godot::String http_request_url(const godot::String& p_url);									// 将 URL 转换为发起 HTTP 请求时使用的地址
	godot::String resolve_http_url(
		const godot::String& p_reference,
		const godot::String& p_base = {}
	);																							// 根据基准 URL 解析相对 HTTP URL
}
