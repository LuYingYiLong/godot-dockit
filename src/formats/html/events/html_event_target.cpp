#include "html_event_target.h"

#include "html_event.h"

using namespace godot;

void HTMLEventTarget::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_valid"), &HTMLEventTarget::is_valid);
	ClassDB::bind_method(D_METHOD("add_event_listener", "type", "callable", "options"),
		&HTMLEventTarget::add_event_listener, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("remove_event_listener", "type", "callable", "options"),
		&HTMLEventTarget::remove_event_listener, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("dispatch_event", "event"), &HTMLEventTarget::dispatch_event);
	ClassDB::bind_method(D_METHOD("has_event_listener", "type"), &HTMLEventTarget::has_event_listener);
}
