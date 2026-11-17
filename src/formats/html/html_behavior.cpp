#include "html_behavior.h"

using namespace godot;

void HTMLBehavior::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_document"), &HTMLBehavior::get_document);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "document", PROPERTY_HINT_RESOURCE_TYPE, "HTMLDocumentNode", PROPERTY_USAGE_NONE),
		"", "get_document");
	GDVIRTUAL_BIND(_attach_document, "document");
	GDVIRTUAL_BIND(_attach, "label");
	GDVIRTUAL_BIND(_detach);
}

Ref<HTMLDocumentNode> HTMLBehavior::get_document() const {
	return document;
}
