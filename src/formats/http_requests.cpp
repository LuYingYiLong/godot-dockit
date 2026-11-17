#include "http_requests.h"
#include "http_url.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <deque>
#include <map>
#include <utility>

using namespace godot;

namespace docrender {
	namespace {
		constexpr int MAX_ACTIVE = 6;
		constexpr int MAX_REDIRECTS = 8;

		struct Request {
			String url;
			int max_body;
			bool track_redirects;
			std::function<void(const HTTPResponse&)> complete;
			ObjectID request;
			int redirects = 0;
		};
		std::deque<Request> pending;
		std::map<uint64_t, Request> active;
		uint64_t next_id = 0;
		bool start_queued = false;
		bool stopping = false;

		void queue_start();

		void finish(std::map<uint64_t, Request>::iterator p_job, const HTTPResponse& p_response) {
			auto job = std::move(p_job->second);
			active.erase(p_job);
			if (auto* request = Object::cast_to<HTTPRequest>(ObjectDB::get_instance(job.request))) request->queue_free();
			job.complete(p_response);
			queue_start();
		}

		void completed(int p_result, int p_code, const PackedStringArray& p_headers, const PackedByteArray& p_body, uint64_t p_id) {
			const auto entry = active.find(p_id);
			if (entry == active.end()) return;
			auto& job = entry->second;
			if (job.track_redirects && p_result == HTTPRequest::RESULT_REDIRECT_LIMIT_REACHED &&
				job.redirects < MAX_REDIRECTS) {
				const String location = http_response_header(p_headers, "Location");
				const String next = location.is_empty() ? String() : resolve_http_url(location, job.url);
				auto* request = Object::cast_to<HTTPRequest>(ObjectDB::get_instance(job.request));
				if (!next.is_empty() && request) {
					++job.redirects;
					job.url = http_request_url(next);
					if (request->request(job.url) == OK) return;
				}
			}
			if (p_result != HTTPRequest::RESULT_SUCCESS || p_code < 200 || p_code >= 300) {
				finish(entry, {job.url, {}, {}, "request_failed"});
			} else {
				finish(entry, {job.url, p_headers, p_body, {}});
			}
		}

		void start_requests() {
			start_queued = false;
			auto* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
			auto* root = tree ? tree->get_root() : nullptr;
			while (!stopping && active.size() < MAX_ACTIVE && !pending.empty()) {
				auto job = std::move(pending.front());
				pending.pop_front();
				if (!root || !root->is_inside_tree()) {
					job.complete({job.url, {}, {}, "scene_tree_unavailable"});
					continue;
				}
				auto* request = memnew(HTTPRequest);
				request->set_use_threads(true);
				request->set_timeout(12.0);
				request->set_body_size_limit(job.max_body);
				request->set_max_redirects(job.track_redirects ? 0 : MAX_REDIRECTS);
				request->set_process_mode(Node::PROCESS_MODE_ALWAYS);
				job.request = ObjectID(request->get_instance_id());
				const auto token = ++next_id;
				const auto entry = active.emplace(token, std::move(job)).first;
				request->connect("request_completed", callable_mp_static(&completed).bind(token));
				root->add_child(request, false, Node::INTERNAL_MODE_BACK);
				if (request->request(entry->second.url) != OK) finish(entry, {entry->second.url, {}, {}, "request_start_failed"});
			}
		}

		void queue_start() {
			if (stopping || start_queued || active.size() >= MAX_ACTIVE || pending.empty()) return;
			start_queued = true;
			callable_mp_static(&start_requests).call_deferred();
		}
	}

	String http_response_header(const PackedStringArray& p_headers, const String& p_name) {
		for (const auto& header : p_headers) {
			const auto separator = header.find(":");
			if (separator >= 0 && header.left(separator).strip_edges().nocasecmp_to(p_name) == 0) {
				return header.substr(separator + 1).strip_edges();
			}
		}
		return {};
	}

	void queue_http_request(const String& p_url, int p_max_body, bool p_track_redirects,
		std::function<void(const HTTPResponse&)> p_complete) {
		if (stopping) return;
		pending.push_back({p_url, p_max_body, p_track_redirects, std::move(p_complete), {}});
		queue_start();
	}

	void shutdown_http_requests() {
		stopping = true;
		for (const auto& [id, job] : active) {
			if (auto* request = Object::cast_to<HTTPRequest>(ObjectDB::get_instance(job.request))) {
				request->disconnect("request_completed", callable_mp_static(&completed).bind(id));
				request->cancel_request();
				request->queue_free();
			}
		}
		active.clear();
		pending.clear();
	}
}
