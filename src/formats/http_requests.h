#pragma once

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <functional>

namespace docrender {
	struct HTTPResponse {
		godot::String url;
		godot::PackedStringArray headers;
		godot::PackedByteArray body;
		godot::String error;
	};

	godot::String http_response_header(const godot::PackedStringArray& p_headers, const godot::String& p_name);
	void queue_http_request(const godot::String& p_url, int p_max_body, bool p_track_redirects,
		std::function<void(const HTTPResponse&)> p_complete);
	void shutdown_http_requests();
}
