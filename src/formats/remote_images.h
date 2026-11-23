#pragma once

#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace dockit {
	enum class RemoteImageStatus {
		loading,
		ready,
		failed
	};

	struct RemoteImage {
		RemoteImageStatus status = RemoteImageStatus::failed;
		godot::Ref<godot::Texture2D> texture;
		godot::String error;
	};

	// 管理远程图像资源
	bool is_remote_image_url(const godot::String& p_url);										// 检查 URL 是否指向可远程加载的图像
	godot::String remote_image_request_url(const godot::String& p_url);							// 生成远程图像请求使用的 URL
	RemoteImage request_remote_image(
		const godot::String& p_url,
		const godot::Callable& p_completed = {}
	);																							// 请求远程图像并在完成时调用回调
	void shutdown_remote_images();																// 取消远程图像请求并释放请求服务
}
