#pragma once

#include <godot_cpp/classes/http_client.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <functional>
#include <cstdint>

namespace dockit {
	struct HTTPResponse {
		godot::String url;
		int status_code = 0;
		godot::PackedStringArray headers;
		godot::PackedByteArray body;
		godot::String error;
	};

	struct HTTPRequestOptions {
		godot::PackedStringArray headers;
		godot::HTTPClient::Method method = godot::HTTPClient::METHOD_GET;
		godot::String body;
		int max_body = 8 * 1024 * 1024;
		double timeout = 12.0;
		bool track_redirects = false;
	};

	// 管理 HTTP 请求
	godot::String http_response_header(
		const godot::PackedStringArray& p_headers,
		const godot::String& p_name
	);																							// 按名称查找 HTTP 响应头
	uint64_t queue_http_request(const godot::String& p_url, const HTTPRequestOptions& p_options,
		std::function<void(const HTTPResponse&)> p_complete);									// 发起 HTTP 请求并在完成后调用回调
	uint64_t queue_http_request(const godot::String& p_url, int p_max_body, bool p_track_redirects,
		std::function<void(const HTTPResponse&)> p_complete);									// 发起 HTTP 请求并在完成后调用回调
	void cancel_http_request(uint64_t p_request_id);											// 取消HTTP请求
	void shutdown_http_requests();																// 取消未完成的 HTTP 请求并释放请求服务
}
