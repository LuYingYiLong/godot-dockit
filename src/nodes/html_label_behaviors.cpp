#include "html_label.h"

#include "formats/html/elements/html_metadata_elements.h"
#include "formats/html/events/html_event.h"
#include "formats/html/html_behavior.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <utility>

using namespace godot;

namespace {
	bool can_construct_behavior(const Ref<GDScript>& p_script) {
		if (p_script.is_null() || p_script->get_instance_base_type() != HTMLBehavior::get_class_static() ||
			p_script->is_abstract() || !p_script->can_instantiate()) return false;

		// Godot lists script methods from the most-derived script to its base.
		for (const Dictionary& method : p_script->get_script_method_list()) {
			if (StringName(method["name"]) != StringName("_init")) continue;
			const Array arguments = method["args"];
			const Array defaults = method["default_args"];
			return arguments.size() == defaults.size();
		}
		return true;
	}

	bool parse_inline_handler(const String& p_source, StringName& r_method, bool& r_with_event) {
		const String source = p_source.strip_edges();
		const int64_t open = source.find("(");
		if (open <= 0 || !source.ends_with(")")) return false;
		const String name = source.substr(0, open).strip_edges();
		const String argument = source.substr(open + 1, source.length() - open - 2).strip_edges();
		if (argument != "" && argument != "event") return false;
		for (int64_t index = 0; index < name.length(); ++index) {
			const char32_t ch = name[index];
			const bool valid = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' ||
				(index > 0 && ch >= '0' && ch <= '9');
			if (!valid) return false;
		}
		r_method = StringName(name);
		r_with_event = argument == "event";
		return true;
	}

	bool has_compatible_method(const Object& p_object, const StringName& p_method, int p_argument_count) {
		for (const Dictionary& method : p_object.get_method_list()) {
			if (StringName(method.get("name", StringName())) != p_method) continue;
			const Array arguments = method.get("args", Array());
			const Array defaults = method.get("default_args", Array());
			const int required = std::max(0, int(arguments.size() - defaults.size()));
			return p_argument_count >= required && p_argument_count <= arguments.size();
		}
		return false;
	}
}

HTMLLabel::~HTMLLabel() = default;

void HTMLLabel::set_behaviors_enabled(bool p_enabled) {
	if (behaviors_enabled == p_enabled) return;
	behaviors_enabled = p_enabled;
	request_behavior_reset();
}

bool HTMLLabel::invoke_inline_event_handler(const Ref<HTMLElement>& p_element, const Ref<HTMLEvent>& p_event) {
	if (p_element.is_null() || p_event.is_null() || !p_element->is_valid()) return false;
	const String attribute = String("on") + String(p_event->get_type());
	if (!p_element->has_attribute(attribute)) return false;
	StringName method;
	bool with_event = false;
	if (!parse_inline_handler(p_element->get_attribute(attribute), method, with_event)) return false;
	const int argument_count = with_event ? 1 : 0;
	for (const Ref<HTMLBehavior>& behavior : attached_behaviors) {
		if (behavior.is_null() || !has_compatible_method(*behavior.ptr(), method, argument_count)) continue;
		Array arguments;
		if (with_event) arguments.push_back(p_event);
		const Variant result = behavior->callv(method, arguments);
		if (result.get_type() == Variant::BOOL && !bool(result)) p_event->prevent_default();
		return true;
	}
	return false;
}

bool HTMLLabel::are_behaviors_enabled() const {
	return behaviors_enabled;
}

void HTMLLabel::set_behavior_scripts(const TypedDictionary<StringName, GDScript>& p_scripts) {
	if (behavior_scripts == p_scripts) return;
	behavior_scripts = p_scripts.duplicate();
	request_behavior_reset();
}

TypedDictionary<StringName, GDScript> HTMLLabel::get_behavior_scripts() const {
	return behavior_scripts.duplicate();
}

void HTMLLabel::request_behavior_reset(bool p_source_changed) {
	++behavior_version;
	behavior_mount_scheduled = false;
	behavior_source_pending |= p_source_changed;
	if (!dispatching_behavior) apply_behavior_reset();
}

void HTMLLabel::apply_behavior_reset() {
	const ObjectID id(get_instance_id());
	auto instances = std::exchange(attached_behaviors, {});
	dispatching_behavior = true;
	for (auto it = instances.rbegin(); it != instances.rend(); ++it) {
		GDVIRTUAL_CALL_PTR(it->ptr(), _detach);
		(*it)->document.unref();
		if (!ObjectDB::get_instance(id)) return;
	}
	dispatching_behavior = false;
	if (std::exchange(behavior_source_pending, false)) {
		reset_document_source();
	}
	else {
		queue_behavior_mount();
	}
}

bool HTMLLabel::can_mount_behaviors() const {
	return behaviors_enabled && !behavior_scripts.is_empty() && behavior_host_in_tree &&
		!Engine::get_singleton()->is_editor_hint() && !is_queued_for_deletion() && parsed &&
		!source_dirty && !behavior_source_pending && !refresh_queued && !view_update_pending &&
		viewport_size.x > 0 && viewport_size.y > 0 && render_error.is_empty();
}

void HTMLLabel::queue_behavior_mount() {
	if (behavior_mount_scheduled || dispatching_behavior || !can_mount_behaviors()) return;
	behavior_mount_scheduled = true;
	callable_mp_static(&HTMLLabel::mount_behaviors).bind(get_instance_id(), behavior_version).call_deferred();
}

void HTMLLabel::mount_behaviors(uint64_t p_id, uint64_t p_version) {
	const ObjectID id(p_id);
	auto* label = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id));
	if (!label || label->behavior_version != p_version) return;
	if (!label->can_mount_behaviors()) {
		label->behavior_mount_scheduled = false;
		return;
	}

	// Capture the declarations before invoking project code that may edit the DOM.
	const auto document = label->layout->get_document_node();
	std::vector<Ref<GDScript>> scripts;
	for (const Ref<HTMLElement>& element : label->layout->query_selector_all("script")) {
		const Ref<HTMLScriptElement> declaration = element;
		if (declaration.is_null() || declaration->get_type().strip_edges().nocasecmp_to("text/gdscript") != 0) continue;
		const String name = declaration->get_src().strip_edges();
		if (name.is_empty()) continue;
		const Ref<GDScript> script = label->behavior_scripts.get(StringName(name), Variant());
		if (can_construct_behavior(script)) scripts.push_back(script);
	}
	label->dispatching_behavior = true;
	for (const auto& script : scripts) {
		const Ref<HTMLBehavior> instance = script->call("new");
		if (!ObjectDB::get_instance(id)) return;
		if (label->behavior_version != p_version || label->is_queued_for_deletion()) break;
		if (instance.is_null()) continue;
		instance->document = document;
		label->attached_behaviors.push_back(instance);
		if (!GDVIRTUAL_CALL_PTR(instance.ptr(), _attach_document, document)) {
			GDVIRTUAL_CALL_PTR(instance.ptr(), _attach, label);
		}
		if (!ObjectDB::get_instance(id)) return;
		if (label->behavior_version != p_version || label->is_queued_for_deletion()) break;
	}
	label->dispatching_behavior = false;
	if (label->behavior_version != p_version) label->apply_behavior_reset();
}
