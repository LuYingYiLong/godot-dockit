#include "html_dom.h"

#include "formats/html/events/html_event.h"
#include "formats/html/html_layout.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

using namespace godot;
using docrender::HTMLDOM;

namespace {
	// 防御监听器内部再次派发同一事件造成的无限递归。
	constexpr int MAX_EVENT_DISPATCH_DEPTH = 32;
	constexpr std::size_t MAX_EVENT_PATH_LENGTH = 1024;
}

Error HTMLDOM::add_event_listener(const litehtml::element::ptr& p_element, const StringName& p_type,
	const Callable& p_callable, bool p_capture, bool p_once) {
	if (!p_element) return ERR_INVALID_PARAMETER;
	if (p_callable.is_null()) return ERR_INVALID_PARAMETER;
	if (p_type == StringName()) return ERR_INVALID_PARAMETER;
	auto& list = event_listeners[p_element.get()][p_type];
	for (const auto& entry : list) {
		// 与 DOM 一致：同一 (类型, 回调, capture) 只注册一次。
		if (entry.capture == p_capture && entry.callable == p_callable) return OK;
	}
	EventListener listener;
	listener.callable = p_callable;
	const Object* object = p_callable.get_object();
	listener.owner = object ? ObjectID(object->get_instance_id()) : ObjectID();
	listener.capture = p_capture;
	listener.once = p_once;
	list.push_back(listener);
	++event_listener_count;
	return OK;
}

Error HTMLDOM::remove_event_listener(const litehtml::element::ptr& p_element, const StringName& p_type,
	const Callable& p_callable, bool p_capture) {
	if (!p_element) return ERR_INVALID_PARAMETER;
	auto* map_ptr = event_listeners.getptr(p_element.get());
	if (!map_ptr) return OK;
	auto* list_ptr = map_ptr->getptr(p_type);
	if (!list_ptr) return OK;
	auto& list = *list_ptr;
	const std::size_t before = list.size();
	list.erase(std::remove_if(list.begin(), list.end(), [&](const EventListener& p_entry) {
		return p_entry.capture == p_capture && p_entry.callable == p_callable;
	}), list.end());
	event_listener_count -= before - list.size();
	if (list.empty()) {
		map_ptr->erase(p_type);
		if (map_ptr->is_empty()) event_listeners.erase(p_element.get());
	}
	return OK;
}

bool HTMLDOM::has_event_listener(const litehtml::element& p_element, const StringName& p_type) {
	const auto* map_ptr = event_listeners.getptr(&p_element);
	if (!map_ptr) return false;
	const auto* list_ptr = map_ptr->getptr(p_type);
	return list_ptr && !list_ptr->empty();
}

void HTMLDOM::clear_event_listeners() {
	event_listeners.clear();
	event_listener_count = 0;
	event_dispatch_depth = 0;
	pending_scroll_targets.clear();
	scroll_delivery_queued = false;
	++scroll_delivery_epoch;
}

void HTMLDOM::forget_subtree_listeners(const litehtml::element& p_element) {
	if (event_listener_count == 0) return;
	const auto visit = [&](const auto& p_self, const litehtml::element& p_node) -> void {
		if (const auto* map_ptr = event_listeners.getptr(&p_node)) {
			for (const auto& entry : *map_ptr) event_listener_count -= entry.value.size();
			event_listeners.erase(&p_node);
		}
		for (const auto& child : p_node.children()) p_self(p_self, *child);
	};
	visit(visit, p_element);
}

void HTMLDOM::transfer_listeners(HTMLDOM& p_source, const litehtml::element& p_from,
	const litehtml::element::ptr& p_to) {
	if (&p_source == this || !p_to) return;
	auto* source_ptr = p_source.event_listeners.getptr(&p_from);
	if (!source_ptr) return;
	std::size_t moved_count = 0;
	for (const auto& entry : *source_ptr) moved_count += entry.value.size();
	auto moved = std::move(*source_ptr);
	p_source.event_listeners.erase(&p_from);
	p_source.event_listener_count -= moved_count;
	auto& target = event_listeners[p_to.get()];
	for (auto& entry : moved) {
		auto& list = target[entry.key];
		for (auto& listener : entry.value) list.push_back(listener);
	}
	event_listener_count += moved_count;
}

void HTMLDOM::invoke_event_listeners(const Ref<HTMLNode>& p_node, const Ref<HTMLEvent>& p_event, int p_phase) {
	if (p_event->is_immediate_stopped()) return;
	if (p_node.is_null() || !p_node.is_valid()) return;
	const auto element = p_node->lock_element();
	if (!element) return;
	auto* map_ptr = event_listeners.getptr(element.get());
	if (!map_ptr) return;
	auto* list_ptr = map_ptr->getptr(p_event->get_type());
	if (!list_ptr) return;
	// 派发期间允许增删监听器，因此按快照迭代。
	const std::vector<EventListener> snapshot = *list_ptr;
	if (snapshot.empty()) return;
	const StringName type = p_event->get_type();
	p_event->set_dispatch_phase(p_node, p_phase);
	for (const EventListener& entry : snapshot) {
		if (p_event->is_immediate_stopped()) break;
		if (p_phase == HTMLEvent::PHASE_CAPTURING && !entry.capture) continue;
		if (p_phase == HTMLEvent::PHASE_BUBBLING && entry.capture) continue;
		if (entry.once) remove_event_listener(element, type, entry.callable, entry.capture);
		if (!entry.callable.is_valid()) {
			remove_event_listener(element, type, entry.callable, entry.capture);
			continue;
		}
		entry.callable.call(p_event);
	}
}

bool HTMLDOM::dispatch_event(const Ref<HTMLNode>& p_target, const Ref<HTMLEvent>& p_event) {
	if (p_event.is_null()) return true;
	if (event_listener_count == 0) return !p_event->is_default_prevented();
	if (event_dispatch_depth >= MAX_EVENT_DISPATCH_DEPTH) {
		ERR_PRINT("HTML event dispatch exceeded the maximum depth and was ignored.");
		return !p_event->is_default_prevented();
	}
	if (p_target.is_null() || !p_target.is_valid()) return true;

	Ref<HTMLNode> target = p_target;
	std::vector<Ref<HTMLNode>> path;
	for (Ref<HTMLNode> node = target; node.is_valid(); node = node->get_parent_node()) {
		path.push_back(node);
		if (path.size() >= MAX_EVENT_PATH_LENGTH) break;
	}
	if (path.empty()) return true;

	p_event->begin_dispatch(target);
	++event_dispatch_depth;

	for (std::size_t index = path.size(); index > 1; --index) {
		if (p_event->is_propagation_stopped()) break;
		invoke_event_listeners(path[index - 1], p_event, HTMLEvent::PHASE_CAPTURING);
	}
	if (!p_event->is_propagation_stopped()) {
		// 目标阶段：捕获与非捕获监听器按注册顺序合并执行一次。
		invoke_event_listeners(path[0], p_event, HTMLEvent::PHASE_AT_TARGET);
	}
	if (!p_event->is_propagation_stopped() && p_event->get_bubbles()) {
		for (std::size_t index = 1; index < path.size(); ++index) {
			invoke_event_listeners(path[index], p_event, HTMLEvent::PHASE_BUBBLING);
			if (p_event->is_propagation_stopped()) break;
		}
	}

	--event_dispatch_depth;
	p_event->end_dispatch();
	return !p_event->is_default_prevented();
}

void HTMLDOM::queue_scroll_event(const Ref<HTMLNode>& p_node) {
	if (p_node.is_null() || event_listener_count == 0) return;
	for (const auto& entry : pending_scroll_targets) {
		if (entry.ptr() == p_node.ptr()) return;
	}
	pending_scroll_targets.push_back(p_node);
	if (scroll_delivery_queued) return;
	scroll_delivery_queued = true;
	callable_mp_static(&HTMLDOM::deliver_scroll_events).bind(
		static_cast<uint64_t>(owner_id), ++scroll_delivery_epoch, lifetime).call_deferred();
}

void HTMLDOM::deliver_scroll_events(uint64_t p_owner_id, uint64_t p_epoch, uint64_t p_lifetime) {
	auto* layout = Object::cast_to<HTMLLayout>(ObjectDB::get_instance(ObjectID(p_owner_id)));
	if (!layout) return;
	const auto dom = layout->get_dom().shared_from_this();
	if (dom->scroll_delivery_epoch != p_epoch || dom->lifetime != p_lifetime) return;
	dom->scroll_delivery_queued = false;
	std::vector<Ref<HTMLNode>> pending = std::move(dom->pending_scroll_targets);
	dom->pending_scroll_targets.clear();
	for (const Ref<HTMLNode>& node : pending) {
		if (dom->lifetime != p_lifetime) break;
		if (node.is_null() || !node.is_valid()) continue;
		dom->dispatch_event(node, docrender::make_dom_event(StringName("scroll"), false, false));
	}
}
