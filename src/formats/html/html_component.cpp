#include "html_component.h"

void HTMLComponent::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("get_host"), &::HTMLComponent::get_host);
	godot::ClassDB::bind_method(godot::D_METHOD("get_shadow_root"), &::HTMLComponent::get_shadow_root);
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "host", godot::PROPERTY_HINT_NONE, "", godot::PROPERTY_USAGE_NONE, "HTMLElement"), "", "get_host");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "shadow_root", godot::PROPERTY_HINT_NONE, "", godot::PROPERTY_USAGE_NONE, "HTMLShadowRoot"), "", "get_shadow_root");
	GDVIRTUAL_BIND(_mount);
	GDVIRTUAL_BIND(_connected);
	GDVIRTUAL_BIND(_disconnected);
	GDVIRTUAL_BIND(_unmount);
}

godot::Ref<HTMLShadowRoot> HTMLComponent::get_shadow_root() const {
	return host.is_valid() && host->is_valid() ? host->get_shadow_root() : godot::Ref<HTMLShadowRoot>();
}
