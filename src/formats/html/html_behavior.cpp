#include "html_behavior.h"

void HTMLBehavior::_bind_methods() {
	GDVIRTUAL_BIND(_attach, "label");
	GDVIRTUAL_BIND(_detach);
}
