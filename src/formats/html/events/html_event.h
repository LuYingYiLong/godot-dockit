#pragma once

#include "formats/html/elements/html_element.h"
#include "formats/html/events/html_event_target.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

// DOM 事件对象。脚本可以构造合成事件并交给 HTMLEventTarget.dispatch_event()；
// 引擎内部合成的真实输入事件由 dispatcher 设置 target/current_target/phase，
// 这些字段只保留 getter，脚本无法篡改派发路径。
// 事件路径为 window -> document -> 元素链，因此 target/current_target 是
// HTMLEventTarget（元素、文档节点或 window），而不是固定的 HTMLNode。
class HTMLEvent : public godot::RefCounted {
	GDCLASS(HTMLEvent, godot::RefCounted)

public:
	enum EventPhase {
		PHASE_NONE = 0,
		PHASE_CAPTURING = 1,
		PHASE_AT_TARGET = 2,
		PHASE_BUBBLING = 3,
	};

private:
	godot::StringName type;
	godot::Ref<HTMLEventTarget> target;
	godot::Ref<HTMLEventTarget> current_target;
	std::vector<godot::Ref<HTMLEventTarget>> path;
	int phase = PHASE_NONE;
	bool bubbles = false;
	bool cancelable = false;
	bool default_prevented = false;
	bool trusted = false;
	bool propagation_stopped = false;
	bool immediate_stopped = false;
	bool dispatching = false;
	bool passive_context = false;
	bool passive_warned = false;
	int64_t time_stamp = 0;

protected:
	static void _bind_methods();

public:
	// 由派发器使用：在派发期间接管 target / current_target / phase 与事件路径。
	// 同一事件在派发过程中再次派发会被拒绝，返回 false。
	bool begin_dispatch(const godot::Ref<HTMLEventTarget>& p_target,
		const std::vector<godot::Ref<HTMLEventTarget>>& p_path);
	void set_dispatch_phase(const godot::Ref<HTMLEventTarget>& p_node, int p_phase);
	void end_dispatch();
	void set_passive_context(bool p_passive) { passive_context = p_passive; }	// 由派发器在调用 passive 监听器前后切换
	void mark_trusted() { trusted = true; }										// 标记为引擎合成的真实输入事件
	bool is_dispatching() const { return dispatching; }							// 返回是否正在派发
	bool is_propagation_stopped() const { return propagation_stopped; }			// 返回本次派发是否已停止传播
	bool is_immediate_stopped() const { return immediate_stopped; }				// 返回是否已停止当前节点的其余监听器

	godot::StringName get_type() const { return type; }							// 返回事件类型
	void set_type(const godot::StringName& p_type);								// 设置事件类型，派发期间无效
	godot::Ref<HTMLEventTarget> get_target() const { return target; }			// 返回事件的目标
	godot::Ref<HTMLEventTarget> get_current_target() const { return current_target; } // 返回当前处理到的目标
	godot::TypedArray<HTMLEventTarget> composed_path() const;					// 返回派发时的事件路径（target 在前，window 在后）
	int get_event_phase() const { return phase; }								// 返回派发阶段
	bool get_bubbles() const { return bubbles; }								// 返回是否冒泡
	void set_bubbles(bool p_bubbles);											// 设置是否冒泡，派发期间无效
	bool get_cancelable() const { return cancelable; }							// 返回是否可取消
	void set_cancelable(bool p_cancelable);										// 设置是否可取消，派发期间无效
	bool is_default_prevented() const { return default_prevented; }				// 返回默认动作是否已被取消
	bool is_trusted() const { return trusted; }									// 返回是否由引擎合成
	int64_t get_time_stamp() const { return time_stamp; }						// 返回事件创建时间的毫秒时间戳
	void prevent_default();														// 取消此事件的默认动作（passive 监听器中无效）
	void stop_propagation();													// 阻止事件继续传播到后续节点
	void stop_immediate_propagation();											// 阻止当前节点上其余监听器并停止传播
};

VARIANT_ENUM_CAST(HTMLEvent::EventPhase);

class HTMLCustomEvent : public HTMLEvent {
	GDCLASS(HTMLCustomEvent, HTMLEvent)

	godot::Dictionary detail;

protected:
	static void _bind_methods();

public:
	godot::Dictionary get_detail() const { return detail; } // 返回自定义载荷
	void set_detail(const godot::Dictionary& p_detail); // 设置自定义载荷，派发期间无效
};

class HTMLMouseEvent : public HTMLEvent {
	GDCLASS(HTMLMouseEvent, HTMLEvent)

	int button = 0;
	int buttons = 0;
	int detail = 0;
	godot::Vector2 position;
	bool shift = false;
	bool ctrl = false;
	bool alt = false;
	bool meta = false;

protected:
	static void _bind_methods();

public:
	int get_button() const { return button; }								// 返回触发的鼠标键
	void set_button(int p_button) { button = p_button; }
	int get_buttons() const { return buttons; }								// 返回按键位掩码
	void set_buttons(int p_buttons) { buttons = p_buttons; }
	int get_detail() const { return detail; }								// 返回连续点击计数
	void set_detail(int p_detail) { detail = p_detail; }
	godot::Vector2 get_position() const { return position; }				// 返回标签局部坐标中的指针位置
	void set_position(const godot::Vector2& p_position) { position = p_position; }
	bool is_shift_pressed() const { return shift; }
	void set_shift_pressed(bool p_pressed) { shift = p_pressed; }
	bool is_ctrl_pressed() const { return ctrl; }
	void set_ctrl_pressed(bool p_pressed) { ctrl = p_pressed; }
	bool is_alt_pressed() const { return alt; }
	void set_alt_pressed(bool p_pressed) { alt = p_pressed; }
	bool is_meta_pressed() const { return meta; }
	void set_meta_pressed(bool p_pressed) { meta = p_pressed; }
};

class HTMLKeyboardEvent : public HTMLEvent {
	GDCLASS(HTMLKeyboardEvent, HTMLEvent)

	int keycode = 0;
	int physical_keycode = 0;
	int key_label = 0;
	int unicode = 0;
	bool shift = false;
	bool ctrl = false;
	bool alt = false;
	bool meta = false;
	bool echo = false;

protected:
	static void _bind_methods();

public:
	int get_keycode() const { return keycode; }
	void set_keycode(int p_keycode) { keycode = p_keycode; }
	int get_physical_keycode() const { return physical_keycode; }
	void set_physical_keycode(int p_physical_keycode) { physical_keycode = p_physical_keycode; }
	int get_key_label() const { return key_label; } // 返回按键在布局中的标签码
	void set_key_label(int p_key_label) { key_label = p_key_label; }
	int get_unicode() const { return unicode; }
	void set_unicode(int p_unicode) { unicode = p_unicode; }
	bool is_shift_pressed() const { return shift; }
	void set_shift_pressed(bool p_pressed) { shift = p_pressed; }
	bool is_ctrl_pressed() const { return ctrl; }
	void set_ctrl_pressed(bool p_pressed) { ctrl = p_pressed; }
	bool is_alt_pressed() const { return alt; }
	void set_alt_pressed(bool p_pressed) { alt = p_pressed; }
	bool is_meta_pressed() const { return meta; }
	void set_meta_pressed(bool p_pressed) { meta = p_pressed; }
	bool is_echo() const { return echo; } // 返回是否为按住产生的重复按键
	void set_echo(bool p_echo) { echo = p_echo; }
};

class HTMLFocusEvent : public HTMLEvent {
	GDCLASS(HTMLFocusEvent, HTMLEvent)

	godot::Ref<HTMLEventTarget> related_target;

protected:
	static void _bind_methods();

public:
	godot::Ref<HTMLEventTarget> get_related_target() const { return related_target; } // 返回焦点转移的对端目标
	void set_related_target(const godot::Ref<HTMLEventTarget>& p_target) { related_target = p_target; }
};

class HTMLSubmitEvent : public HTMLEvent {
	GDCLASS(HTMLSubmitEvent, HTMLEvent)

	godot::Ref<HTMLElement> submitter;

protected:
	static void _bind_methods();

public:
	godot::Ref<HTMLElement> get_submitter() const { return submitter; } // 返回触发提交的按钮，可能为空
	void set_submitter(const godot::Ref<HTMLElement>& p_submitter) { submitter = p_submitter; }
};

namespace docrender {
	// 构造一个已标记为 trusted 的空事件，按类型选择具体子类。
	godot::Ref<HTMLEvent> make_dom_event(const godot::StringName& p_type, bool p_bubbles, bool p_cancelable);
}
