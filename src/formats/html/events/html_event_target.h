#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

class HTMLEvent;

// DOM EventTarget。HTMLNode 与 HTMLWindow 的共同基类：监听器由所属文档的 HTMLDOM
// 统一持有，子类只负责把调用转发到自己的槽位（元素 / 文档 / 窗口）。
class HTMLEventTarget : public godot::RefCounted {
	GDCLASS(HTMLEventTarget, godot::RefCounted)

protected:
	static void _bind_methods();

public:
	virtual bool is_valid() const = 0; // 检查目标是否仍属于当前文档
	virtual godot::Error add_event_listener(const godot::StringName& p_type, const godot::Callable& p_callable,
		const godot::Dictionary& p_options = {}) = 0; // 注册监听器
	virtual godot::Error remove_event_listener(const godot::StringName& p_type, const godot::Callable& p_callable,
		const godot::Dictionary& p_options = {}) = 0; // 移除之前注册的监听器
	virtual bool dispatch_event(const godot::Ref<HTMLEvent>& p_event) = 0; // 从此目标派发事件，返回默认动作是否未被取消
	virtual bool has_event_listener(const godot::StringName& p_type) const = 0; // 检查是否注册了该类型的监听器
};
