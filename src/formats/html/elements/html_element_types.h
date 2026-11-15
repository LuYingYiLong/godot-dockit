#pragma once

#include "html_element.h"

class HTMLDivElement : public HTMLElement {
	GDCLASS(HTMLDivElement, HTMLElement)

protected:
	static void _bind_methods() {}
};

class HTMLParagraphElement : public HTMLElement {
	GDCLASS(HTMLParagraphElement, HTMLElement)

protected:
	static void _bind_methods() {}
};

class HTMLSpanElement : public HTMLElement {
	GDCLASS(HTMLSpanElement, HTMLElement)

protected:
	static void _bind_methods() {}
};
