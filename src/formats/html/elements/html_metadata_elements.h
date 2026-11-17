#pragma once

#include "html_element.h"

class HTMLTitleElement : public HTMLElement {
	GDCLASS(HTMLTitleElement, HTMLElement)

protected:
	static void _bind_methods();
};

class HTMLMetaElement : public HTMLElement {
	GDCLASS(HTMLMetaElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::String get_name() const; // 读取元数据名称
	void set_name(const godot::String& p_value); // 设置元数据名称
	godot::String get_content() const; // 读取元数据内容
	void set_content(const godot::String& p_value); // 设置元数据内容
	godot::String get_http_equiv() const; // 读取HTTP 指令
	void set_http_equiv(const godot::String& p_value); // 设置HTTP 指令
	godot::String get_media() const; // 读取适用的媒体条件
	void set_media(const godot::String& p_value); // 设置适用的媒体条件
	godot::String get_charset() const; // 读取字符编码声明
	void set_charset(const godot::String& p_value); // 设置字符编码声明
};

class HTMLBaseElement : public HTMLElement {
	GDCLASS(HTMLBaseElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::String get_href() const; // 读取原始基准地址
	void set_href(const godot::String& p_value); // 设置原始基准地址
	godot::String get_target() const; // 读取默认导航目标
	void set_target(const godot::String& p_value); // 设置默认导航目标
	godot::String get_resolved_href() const; // 以来源文档的基准解析此元素的 href
};

class HTMLScriptElement : public HTMLElement {
	GDCLASS(HTMLScriptElement, HTMLElement)

protected:
	static void _bind_methods();

public:
	godot::String get_src() const; // 读取原始脚本地址
	void set_src(const godot::String& p_value); // 设置原始脚本地址
	godot::String get_type() const; // 读取脚本内容类型
	void set_type(const godot::String& p_value); // 设置脚本内容类型
	bool is_async() const; // 读取异步属性
	void set_async(bool p_value); // 设置异步属性
	bool is_defer() const; // 读取延迟属性
	void set_defer(bool p_value); // 设置延迟属性
	bool is_no_module() const; // 读取非模块属性
	void set_no_module(bool p_value); // 设置非模块属性
};
