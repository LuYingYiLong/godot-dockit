#include "html_dom.h"

#include "formats/html/events/html_event.h"
#include "formats/html/html_layout.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <litehtml/document.h>

#include <algorithm>
#include <cstddef>
#include <utility>

using namespace godot;
using docrender::HTMLDOM;

namespace {
	// 防御监听器内部再次派发同一事件造成的无限递归
	constexpr int MAX_EVENT_DISPATCH_DEPTH = 32;
	constexpr std::size_t MAX_EVENT_PATH_LENGTH = 1024;

	class EventDispatchScope {
		int& depth;
		Ref<HTMLEvent> event;

	public:
		EventDispatchScope(int& p_depth, const Ref<HTMLEvent>& p_event) : depth(p_depth), event(p_event) {
			++depth;
		}
		EventDispatchScope(const EventDispatchScope&) = delete;
		EventDispatchScope& operator=(const EventDispatchScope&) = delete;

		~EventDispatchScope() {
			--depth;
			event->end_dispatch();
		}
	};
}

Error HTMLDOM::register_event_listener(ListenerStore& r_store, const StringName& p_type,
	const Callable& p_callable, bool p_capture, bool p_once, bool p_passive) {
	if (p_callable.is_null()) return ERR_INVALID_PARAMETER;
	if (p_type == StringName()) return ERR_INVALID_PARAMETER;
	auto& list = r_store.by_type[p_type];
	for (const auto& entry : list) {
		// 与 DOM 一致：同一 (类型, 回调, capture) 只注册一次
		if (entry.capture == p_capture && entry.callable == p_callable) return OK;
	}
	EventListener listener;
	listener.callable = p_callable;
	const Object* object = p_callable.get_object();
	listener.owner = object ? ObjectID(object->get_instance_id()) : ObjectID();
	listener.id = allocate_event_listener_id();
	listener.capture = p_capture;
	listener.once = p_once;
	listener.passive = p_passive;
	list.push_back(listener);
	++r_store.count;
	++event_listener_count;
	++event_counts_by_type[p_type];
	return OK;
}

uint64_t HTMLDOM::allocate_event_listener_id() {
	const uint64_t id = next_event_listener_id++;
	if (next_event_listener_id == 0) next_event_listener_id = 1;
	return id;
}

Error HTMLDOM::unregister_event_listener(ListenerStore& r_store, const StringName& p_type,
	const Callable& p_callable, bool p_capture) {
	auto* list_ptr = r_store.by_type.getptr(p_type);
	if (!list_ptr) return OK;
	auto& list = *list_ptr;
	const std::size_t before = list.size();
	list.erase(std::remove_if(list.begin(), list.end(), [&](const EventListener& p_entry) {
		return p_entry.capture == p_capture && p_entry.callable == p_callable;
		}), list.end());
	const std::size_t removed = before - list.size();
	if (removed == 0) return OK;
	r_store.count -= removed;
	event_listener_count -= removed;
	if (auto* count = event_counts_by_type.getptr(p_type)) {
		if (*count > removed) *count -= removed;
		else event_counts_by_type.erase(p_type);
	}
	if (list.empty()) r_store.by_type.erase(p_type);
	return OK;
}

bool HTMLDOM::store_has_event_listener(const ListenerStore& p_store, const StringName& p_type) {
	const auto* list_ptr = p_store.by_type.getptr(p_type);
	return list_ptr && !list_ptr->empty();
}

Error HTMLDOM::add_event_listener(const litehtml::element::ptr& p_element, const StringName& p_type,
	const Callable& p_callable, bool p_capture, bool p_once, bool p_passive) {
	if (!p_element) return ERR_INVALID_PARAMETER;
	if (p_callable.is_null() || p_type == StringName()) return ERR_INVALID_PARAMETER;
	return register_event_listener(event_listeners[p_element.get()], p_type, p_callable, p_capture, p_once, p_passive);
}

Error HTMLDOM::add_document_event_listener(const StringName& p_type, const Callable& p_callable,
	bool p_capture, bool p_once, bool p_passive) {
	return register_event_listener(document_event_listeners, p_type, p_callable, p_capture, p_once, p_passive);
}

Error HTMLDOM::add_window_event_listener(const StringName& p_type, const Callable& p_callable,
	bool p_capture, bool p_once, bool p_passive) {
	return register_event_listener(window_event_listeners, p_type, p_callable, p_capture, p_once, p_passive);
}

Error HTMLDOM::remove_event_listener(const litehtml::element::ptr& p_element, const StringName& p_type,
	const Callable& p_callable, bool p_capture) {
	if (!p_element) return ERR_INVALID_PARAMETER;
	auto* store = event_listeners.getptr(p_element.get());
	if (!store) return OK;
	const Error result = unregister_event_listener(*store, p_type, p_callable, p_capture);
	if (store->count == 0) event_listeners.erase(p_element.get());
	return result;
}

Error HTMLDOM::remove_document_event_listener(const StringName& p_type, const Callable& p_callable, bool p_capture) {
	return unregister_event_listener(document_event_listeners, p_type, p_callable, p_capture);
}

Error HTMLDOM::remove_window_event_listener(const StringName& p_type, const Callable& p_callable, bool p_capture) {
	return unregister_event_listener(window_event_listeners, p_type, p_callable, p_capture);
}

bool HTMLDOM::has_event_listener(const litehtml::element& p_element, const StringName& p_type) {
	const auto* store = event_listeners.getptr(&p_element);
	return store && store_has_event_listener(*store, p_type);
}

bool HTMLDOM::has_document_event_listener(const StringName& p_type) const {
	return store_has_event_listener(document_event_listeners, p_type);
}

bool HTMLDOM::has_window_event_listener(const StringName& p_type) const {
	return store_has_event_listener(window_event_listeners, p_type);
}

bool HTMLDOM::has_event_listener_for_type(const StringName& p_type) const {
	const auto* count = event_counts_by_type.getptr(p_type);
	return count && *count > 0;
}

Ref<HTMLWindow> HTMLDOM::get_window() const {
	if (!document || !document->root()) return Ref<HTMLWindow>();
	if (auto* handle = Object::cast_to<HTMLWindow>(ObjectDB::get_instance(window_handle))) {
		return Ref<HTMLWindow>(handle);
	}
	Ref<HTMLWindow> handle;
	handle.instantiate();
	handle->layout_id = owner_id;
	handle->parse_version = generation;
	window_handle = ObjectID(handle->get_instance_id());
	return handle;
}

void HTMLDOM::clear_event_listeners() {
	event_listeners.clear();
	document_event_listeners.by_type.clear();
	document_event_listeners.count = 0;
	window_event_listeners.by_type.clear();
	window_event_listeners.count = 0;
	event_counts_by_type.clear();
	event_listener_count = 0;
	window_handle = ObjectID();
	pending_scroll_targets.clear();
	scroll_delivery_queued = false;
	++scroll_delivery_epoch;
}

void HTMLDOM::forget_subtree_listeners(const litehtml::element& p_element) {
	if (event_listener_count == 0) return;
	const auto visit = [&](const auto& p_self, const litehtml::element& p_node) -> void {
		if (auto* store = event_listeners.getptr(&p_node)) {
			for (const auto& entry : store->by_type) {
				event_listener_count -= entry.value.size();
				if (auto* count = event_counts_by_type.getptr(entry.key)) {
					if (*count > entry.value.size()) *count -= entry.value.size();
					else event_counts_by_type.erase(entry.key);
				}
			}
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
	ListenerStore moved = std::move(*source_ptr);
	p_source.event_listeners.erase(&p_from);
	p_source.event_listener_count -= moved.count;
	auto& target = event_listeners[p_to.get()];
	for (auto& entry : moved.by_type) {
		auto& list = target.by_type[entry.key];
		for (auto& listener : entry.value) {
			listener.id = allocate_event_listener_id();
			list.push_back(listener);
		}
		if (auto* count = p_source.event_counts_by_type.getptr(entry.key)) {
			if (*count > entry.value.size()) *count -= entry.value.size();
			else p_source.event_counts_by_type.erase(entry.key);
		}
		event_counts_by_type[entry.key] += entry.value.size();
	}
	target.count += moved.count;
	event_listener_count += moved.count;
}

void HTMLDOM::invoke_event_listeners(const EventPathEntry& p_entry, const Ref<HTMLEvent>& p_event, int p_phase) {
	if (p_event->is_immediate_stopped()) return;
	Ref<HTMLEventTarget> current;
	const auto get_store = [this, &p_entry, &current]() -> ListenerStore* {
		switch (p_entry.slot) {
		case EventTargetSlot::ELEMENT:
			if (p_entry.node.is_null() || !p_entry.node->is_valid() || !p_entry.element) return nullptr;
			current = p_entry.node;
			return event_listeners.getptr(p_entry.element);
		case EventTargetSlot::DOCUMENT:
			if (p_entry.node.is_null() || !p_entry.node->is_valid()) return nullptr;
			current = p_entry.node;
			return &document_event_listeners;
		case EventTargetSlot::WINDOW:
			if (p_entry.window.is_null() || !p_entry.window->is_valid()) return nullptr;
			current = p_entry.window;
			return &window_event_listeners;
		}
		return nullptr;
		};
	ListenerStore* store = get_store();
	if (!store) return;
	const StringName type = p_event->get_type();
	auto* list_ptr = store->by_type.getptr(type);
	if (!list_ptr) return;
	// 派发期间允许增删监听器，因此按快照迭代
	const std::vector<EventListener> snapshot = *list_ptr;
	if (snapshot.empty()) return;
	p_event->set_dispatch_phase(current, p_phase);
	for (const EventListener& entry : snapshot) {
		if (p_event->is_immediate_stopped()) break;
		if (p_phase == HTMLEvent::PHASE_CAPTURING && !entry.capture) continue;
		if (p_phase == HTMLEvent::PHASE_BUBBLING && entry.capture) continue;
		// 回调可能移除、清空或迁移监听器，因此每次调用前都重新获取存储并检查注册 ID
		ListenerStore* active_store = get_store();
		if (!active_store) continue;
		const auto* active_list = active_store->by_type.getptr(type);
		if (!active_list || std::none_of(active_list->begin(), active_list->end(), [&](const EventListener& p_active) {
			return p_active.id == entry.id;
			})) continue;
		if (entry.once) unregister_event_listener(*active_store, type, entry.callable, entry.capture);
		if (!entry.callable.is_valid()) {
			if (ListenerStore* invalid_store = get_store()) {
				unregister_event_listener(*invalid_store, type, entry.callable, entry.capture);
			}
			continue;
		}
		p_event->set_passive_context(entry.passive);
		entry.callable.call(p_event);
		p_event->set_passive_context(false);
	}
}

bool HTMLDOM::dispatch_event(const Ref<HTMLEventTarget>& p_target, const Ref<HTMLEvent>& p_event) {
	if (p_event.is_null()) return true;
	if (event_dispatch_depth >= MAX_EVENT_DISPATCH_DEPTH) {
		ERR_PRINT("HTML event dispatch exceeded the maximum depth and was ignored.");
		return !p_event->is_default_prevented();
	}
	if (p_target.is_null() || !p_target->is_valid()) return !p_event->is_default_prevented();

	std::vector<EventPathEntry> entries;
	std::vector<Ref<HTMLEventTarget>> targets;
	const Ref<HTMLNode> node = p_target;
	if (node.is_valid()) {
		for (Ref<HTMLNode> current = node; current.is_valid(); current = current->get_parent_node()) {
			EventPathEntry entry;
			entry.slot = current->get_node_type() == HTMLNode::DOCUMENT_NODE ?
				EventTargetSlot::DOCUMENT : EventTargetSlot::ELEMENT;
			entry.node = current;
			const auto element = current->lock_element();
			entry.element = element.get();
			if (entry.slot == EventTargetSlot::ELEMENT && !entry.element) break;
			entries.push_back(entry);
			targets.push_back(Ref<HTMLEventTarget>(current));
			if (entries.size() >= MAX_EVENT_PATH_LENGTH) break;
		}
	}
	else if (const Ref<HTMLWindow> window = p_target; window.is_valid()) {
		EventPathEntry entry;
		entry.slot = EventTargetSlot::WINDOW;
		entry.window = window;
		entries.push_back(entry);
		targets.push_back(Ref<HTMLEventTarget>(window));
	}
	if (entries.empty()) return !p_event->is_default_prevented();

	// 元素路径末尾是文档节点时，把 window 追加为最后一项：捕获最先进入，冒泡最后离开
	if (entries.back().slot == EventTargetSlot::DOCUMENT) {
		if (const Ref<HTMLWindow> window = get_window(); window.is_valid()) {
			EventPathEntry entry;
			entry.slot = EventTargetSlot::WINDOW;
			entry.window = window;
			entries.push_back(entry);
			targets.push_back(Ref<HTMLEventTarget>(window));
		}
	}

	if (!p_event->begin_dispatch(p_target, targets)) {
		ERR_PRINT("The event is already being dispatched.");
		return !p_event->is_default_prevented();
	}
	EventDispatchScope dispatch_scope(event_dispatch_depth, p_event);

	for (std::size_t index = entries.size(); index > 1; --index) {
		if (p_event->is_propagation_stopped()) break;
		invoke_event_listeners(entries[index - 1], p_event, HTMLEvent::PHASE_CAPTURING);
	}
	if (!p_event->is_propagation_stopped()) {
		// 目标阶段：捕获与非捕获监听器按注册顺序合并执行一次
		invoke_event_listeners(entries[0], p_event, HTMLEvent::PHASE_AT_TARGET);
	}
	if (!p_event->is_propagation_stopped() && p_event->get_bubbles()) {
		for (std::size_t index = 1; index < entries.size(); ++index) {
			invoke_event_listeners(entries[index], p_event, HTMLEvent::PHASE_BUBBLING);
			if (p_event->is_propagation_stopped()) break;
		}
	}

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
		dom->dispatch_event(Ref<HTMLEventTarget>(node), docrender::make_dom_event(StringName("scroll"), false, false));
	}
}
