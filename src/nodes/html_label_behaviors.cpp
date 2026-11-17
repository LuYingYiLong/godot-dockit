#include "html_label.h"

#include "formats/html/elements/html_metadata_elements.h"
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
}

HTMLLabel::~HTMLLabel() = default;

void HTMLLabel::set_behaviors_enabled(bool p_enabled) {
	if (behaviors_enabled == p_enabled) return;
	behaviors_enabled = p_enabled;
	request_behavior_reset();
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
		if (!ObjectDB::get_instance(id)) return;
	}
	dispatching_behavior = false;
	if (std::exchange(behavior_source_pending, false)) {
		reset_document_source();
	} else {
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
		label->attached_behaviors.push_back(instance);
		GDVIRTUAL_CALL_PTR(instance.ptr(), _attach, label);
		if (!ObjectDB::get_instance(id)) return;
		if (label->behavior_version != p_version || label->is_queued_for_deletion()) break;
	}
	label->dispatching_behavior = false;
	if (label->behavior_version != p_version) label->apply_behavior_reset();
}
