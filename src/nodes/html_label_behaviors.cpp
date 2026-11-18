#include "html_label.h"

#include "formats/html/elements/html_metadata_elements.h"
#include "formats/html/events/html_event.h"
#include "formats/html/html_behavior.h"
#include "formats/html/resources/html_resource_paths.h"
#include "formats/http_url.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>
#include <utility>

using namespace godot;

namespace {
	String behavior_script_error(const Ref<GDScript>& p_script) {
		if (p_script.is_null()) return "resource_not_found";
		if (p_script->get_instance_base_type() != HTMLBehavior::get_class_static()) return "invalid_base_type";
		if (p_script->is_abstract()) return "abstract_script";
		if (!p_script->can_instantiate()) return "cannot_instantiate";

		// Godot lists script methods from the most-derived script to its base.
		for (const Dictionary& method : p_script->get_script_method_list()) {
			if (StringName(method["name"]) != StringName("_init")) continue;
			const Array arguments = method["args"];
			const Array defaults = method["default_args"];
			return arguments.size() == defaults.size() ? String() : String("required_constructor_arguments");
		}
		return {};
	}

	String resolve_behavior_path(HTMLLabel* p_label, const String& p_src) {
		if (p_src.begins_with("res://") || p_src.begins_with("user://") || p_src.begins_with("uid://")) {
			const docrender::HTMLResourceBase local_base{ p_label->get_base_dir(), p_label->get_base_dir() };
			return docrender::resolve_html_link_href(p_src, local_base);
		}
		const String base_uri = p_label->get_base_uri();
		if (docrender::is_http_url(base_uri)) return {};
		const String fallback = p_label->get_base_dir();
		const String directory = base_uri.is_empty() ? fallback :
			(base_uri.ends_with("/") ? base_uri : base_uri.get_base_dir());
		const docrender::HTMLResourceBase base{ base_uri.is_empty() ? directory : base_uri, directory };
		return docrender::resolve_html_link_href(p_src, base);
	}

	Ref<GDScript> load_behavior_script(ResourceLoader* p_loader, const String& p_path, bool p_reload, String& r_error) {
		if (p_path.begins_with("user://")) {
			if (!FileAccess::file_exists(p_path)) {
				r_error = "script_resource_not_found";
				return {};
			}
			const String source = FileAccess::get_file_as_string(p_path);
			if (source.is_empty()) {
				r_error = "script_source_empty";
				return {};
			}
			Ref<GDScript> script;
			script.instantiate();
			script->set_source_code(source);
			script->set_path(p_path);
			if (script->reload() == OK) return script;
			r_error = "script_compile_failed";
			return {};
		}
		Ref<GDScript> script = p_loader->load(p_path, "GDScript",
			p_reload ? ResourceLoader::CACHE_MODE_REPLACE : ResourceLoader::CACHE_MODE_REUSE);
		if (script.is_null()) r_error = p_reload ? String("script_reload_failed") : String("script_load_failed");
		return script;
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

void HTMLLabel::reload_behaviors() {
	behavior_reload_pending = true;
	request_behavior_reset();
}

TypedArray<Dictionary> HTMLLabel::get_behavior_diagnostics() const {
	TypedArray<Dictionary> diagnostics;
	for (const Dictionary& entry : behavior_diagnostics) diagnostics.push_back(entry);
	return diagnostics;
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
	if (p_source_changed) {
		behavior_source_error_reported = false;
		behavior_diagnostics.clear();
	}
	if (!dispatching_behavior) apply_behavior_reset();
}

void HTMLLabel::add_behavior_diagnostic(const String& p_src, const String& p_path, const String& p_reason) {
	Dictionary diagnostic;
	diagnostic["src"] = p_src;
	diagnostic["path"] = p_path;
	diagnostic["reason"] = p_reason;
	behavior_diagnostics.push_back(diagnostic);
	UtilityFunctions::push_warning(String("HTMLLabel behavior '") + p_src + "' failed: " + p_reason +
		(p_path.is_empty() ? String() : String(" (") + p_path + ")"));
}

void HTMLLabel::apply_behavior_reset() {
	const ObjectID id(get_instance_id());
	auto instances = std::exchange(attached_behaviors, {});
	dispatching_behavior = true;
	for (auto it = instances.rbegin(); it != instances.rend(); ++it) {
		GDVIRTUAL_CALL_PTR(it->ptr(), _detach);
		(*it)->cancel_async_work();
		(*it)->document.unref();
		(*it)->local_storage.unref();
		(*it)->session_storage.unref();
		(*it)->label_id = ObjectID();
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
	return behaviors_enabled && behavior_host_in_tree &&
		!Engine::get_singleton()->is_editor_hint() && !is_queued_for_deletion() && parsed &&
		!source_dirty && !behavior_source_pending && !refresh_queued && !view_update_pending &&
		viewport_size.x > 0 && viewport_size.y > 0 && render_error.is_empty();
}

void HTMLLabel::queue_behavior_mount() {
	if (behavior_mount_scheduled || dispatching_behavior || !behaviors_enabled || !behavior_host_in_tree ||
		Engine::get_singleton()->is_editor_hint() || is_queued_for_deletion()) return;
	if (!render_error.is_empty()) {
		if (!behavior_source_error_reported) {
			behavior_source_error_reported = true;
			add_behavior_diagnostic({}, {}, String("document_render_failed: ") + render_error);
		}
		return;
	}
	if (!can_mount_behaviors()) return;
	behavior_mount_scheduled = true;
	callable_mp_static(&HTMLLabel::mount_behaviors).bind(get_instance_id(), behavior_version).call_deferred();
}

void HTMLLabel::mount_behaviors(uint64_t p_id, uint64_t p_version) {
	const ObjectID id(p_id);
	auto* label = Object::cast_to<HTMLLabel>(ObjectDB::get_instance(id));
	if (!label || label->behavior_version != p_version) return;
	if (!label->can_mount_behaviors()) {
		label->behavior_mount_scheduled = false;
		if (!label->render_error.is_empty() && !label->behavior_source_error_reported) {
			label->behavior_source_error_reported = true;
			label->add_behavior_diagnostic({}, {}, String("document_render_failed: ") + label->render_error);
		}
		return;
	}

	// Capture the declarations before invoking project code that may edit the DOM.
	const auto document = label->layout->get_document_node();
	struct BehaviorScript {
		Ref<GDScript> script;
		String src;
		String path;
	};
	std::vector<BehaviorScript> scripts;
	label->behavior_diagnostics.clear();
	label->behavior_source_error_reported = false;
	const bool reload_scripts = std::exchange(label->behavior_reload_pending, false);
	const auto diagnose = [label](const String& p_src, const String& p_path, const String& p_reason) {
		label->add_behavior_diagnostic(p_src, p_path, p_reason);
	};
	ResourceLoader* loader = ResourceLoader::get_singleton();
	for (const Ref<HTMLElement>& element : label->layout->query_selector_all("script")) {
		const Ref<HTMLScriptElement> declaration = element;
		if (declaration.is_null()) continue;
		const String type = declaration->get_type().strip_edges().get_slice(";", 0).strip_edges();
		if (type.nocasecmp_to("text/gdscript") != 0) continue;
		const String src = declaration->get_src().strip_edges();
		if (src.is_empty()) {
			diagnose(src, {}, "missing_src");
			continue;
		}
		String path;
		Ref<GDScript> script = label->behavior_scripts.get(StringName(src), Variant());
		if (script.is_valid() && reload_scripts && !script->get_path().is_empty()) {
			path = script->get_path();
			String load_error;
			script = load_behavior_script(loader, path, true, load_error);
			if (script.is_null()) {
				diagnose(src, path, load_error.is_empty() ? String("script_reload_failed") : load_error);
				continue;
			}
		}
		if (script.is_null()) {
			if (docrender::is_http_url(src) || src.begins_with("//")) {
				diagnose(src, {}, "remote_scripts_are_not_allowed");
				continue;
			}
			path = resolve_behavior_path(label, src);
			const auto normalized = docrender::normalize_html_resource_path(path);
			if (!normalized || normalized->get_extension().nocasecmp_to("gd") != 0) {
				diagnose(src, path, "invalid_local_script_path");
				continue;
			}
			path = *normalized;
			if (path.begins_with("res://") && !loader->exists(path, "GDScript")) {
				diagnose(src, path, "script_resource_not_found");
				continue;
			}
			String load_error;
			script = load_behavior_script(loader, path, reload_scripts, load_error);
			if (script.is_null()) {
				diagnose(src, path, load_error.is_empty() ? String("script_load_failed") : load_error);
				continue;
			}
		}
		const String script_error = behavior_script_error(script);
		if (!script_error.is_empty()) {
			diagnose(src, path.is_empty() && script.is_valid() ? script->get_path() : path, script_error);
			continue;
		}
		scripts.push_back({ script, src, path });
	}
	label->dispatching_behavior = true;
	for (const auto& entry : scripts) {
		const Variant created = entry.script->call("new");
		if (!ObjectDB::get_instance(id)) return;
		if (label->behavior_version != p_version || label->is_queued_for_deletion()) break;
		const Ref<HTMLBehavior> instance = created;
		if (instance.is_null()) {
			diagnose(entry.src, entry.path, "instance_creation_failed");
			continue;
		}
		instance->document = document;
		instance->label_id = ObjectID(id);
		instance->local_storage.instantiate();
		instance->local_storage->configure_persistent(String("user://docrender/html_storage/") +
			label->get_storage_namespace().sha256_text() + ".cfg");
		instance->session_storage = label->session_storage;
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

void HTMLLabel::set_storage_namespace(const String& p_namespace) {
	if (storage_namespace == p_namespace) return;
	storage_namespace = p_namespace.strip_edges();
	const String path = String("user://docrender/html_storage/") + get_storage_namespace().sha256_text() + ".cfg";
	for (const Ref<HTMLBehavior>& behavior : attached_behaviors) {
		if (behavior.is_valid() && behavior->local_storage.is_valid()) behavior->local_storage->configure_persistent(path);
	}
}

String HTMLLabel::get_storage_namespace() const {
	if (!storage_namespace.is_empty()) return storage_namespace;
	if (!current_location.is_empty()) {
		const int64_t query = current_location.find("?");
		const int64_t fragment = current_location.find("#");
		const int64_t suffix = query < 0 ? fragment : fragment < 0 ? query : std::min(query, fragment);
		return suffix < 0 ? current_location : current_location.left(suffix);
	}
	if (document.is_valid() && !document->get_path().is_empty()) return document->get_path();
	if (!source_path.is_empty()) return source_path;
	if (is_inside_tree()) return String(get_path());
	return String("instance://") + String::num_uint64(get_instance_id());
}
