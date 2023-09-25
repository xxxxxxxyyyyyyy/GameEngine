#pragma once

#include "vulkan_types.inl"

void vulkan_swapchain_create(
	vulkan_context* context,
	u32 width,
	u32 height,
	vulkan_swapchain* out_swapchain
);

void vulkan_swapchain_recreate(
	vulkan_context* context,
	u32 width,
	u32 height,
	vulkan_swapchain* swapchain 
);

void vulkan_swapchain_destroy(
	vulkan_context* context,
	vulkan_swapchain* swapchain
);

void vulkan_swapchain_present(
	vulkan_context* context,
	vulkan_swapchain* swapchain,
	VkQueue graphics_queue,
	VkQueue present_queue,
	VkSemaphore render_complete_semaphore,
	u32 present_image_index
);