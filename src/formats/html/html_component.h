#pragma once
#include "html_behavior.h"
#include "dom/html_shadow_root.h"

class HTMLComponent : public HTMLBehavior {
	GDCLASS(HTMLComponent, HTMLBehavior)
	friend class HTMLLabel;
	godot::Ref<HTMLElement> host;
	bool connected = false;
	GDVIRTUAL0(_mount);
	GDVIRTUAL0(_connected);
	GDVIRTUAL0(_disconnected);
	GDVIRTUAL0(_unmount);
protected:
	static void _bind_methods();
public:
	godot::Ref<HTMLElement> get_host() const { return host; }									// 获取组件挂载的宿主元素
	godot::Ref<HTMLShadowRoot> get_shadow_root() const;											// 获取组件宿主关联的影子根节点
};
