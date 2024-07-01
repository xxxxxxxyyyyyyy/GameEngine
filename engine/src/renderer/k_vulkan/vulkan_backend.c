#include "vulkan_backend.h"

#include "vulkan_types.inl"
#include "vulkan_platform.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "vulkan_command_buffer.h"
#include "vulkan_utils.h"
#include "vulkan_buffer.h"
#include "vulkan_image.h"

#include "core/logger.h"
#include "core/kstring.h"
#include "core/kmemory.h"
#include "core/application.h"

#include "containers/darray.h"

#include "math/math_types.h"

#include "platform/platform.h"

// Shaders
#include "shaders/vulkan_material_shader.h"
#include "shaders/vulkan_ui_shader.h"

// systems
#include "systems/material_system.h"

// static vulkan context
static vulkan_context context;
static u32 cached_framebuffer_width = 0;
static u32 cached_framebuffer_height = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);

i32 find_memory_index(u32 type_filter, u32 property_flags);
b8 create_buffers(vulkan_context* context);

void create_command_buffers(renderer_backend* backend);
void regenerate_framebuffers();
b8 recreate_swapchain(renderer_backend* backend);

void upload_data_range(vulkan_context* context, VkCommandPool pool, VkFence fence, VkQueue queue, vulkan_buffer* buffer, u64 offset, u64 size, const void* data) {
    // Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer staging;
    vulkan_buffer_create(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, &staging);

    // Load the data into the staging buffer.
    vulkan_buffer_load_data(context, &staging, 0, size, 0, data);

    // Perform the copy from staging to the device local buffer.
    vulkan_buffer_copy_to(context, pool, fence, queue, staging.handle, 0, buffer->handle, offset, size);

    // Clean up the staging buffer.
    vulkan_buffer_destroy(context, &staging);
}

void free_data_range(vulkan_buffer* buffer, u64 offset, u64 size) {
    // TODO: Free this in the buffer.
    // TODO: update free list with this range being free.
}

b8 vulkan_renderer_backend_initialize(struct renderer_backend* backend, const char* application_name) {
    // Function pointers
    context.find_memory_index = find_memory_index;

    // TODO: custom allocator
    context.allocator = 0;

    application_get_framebuffer_size(&cached_framebuffer_width, &cached_framebuffer_height);
    context.framebuffer_width = (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;
    context.framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    // setup vulkan instance
    VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.apiVersion = VK_API_VERSION_1_2;
    app_info.pApplicationName = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "DOD ENGINE";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create_info.pApplicationInfo = &app_info;

    // obtain a list of required extensions
    const char** required_extensions = darray_create(const char*);
    darray_push(required_extensions, &VK_KHR_SURFACE_EXTENSION_NAME);  // generic surface extension
    platform_get_required_extension_names(&required_extensions);       // platform-specific extensions
#if defined(_DEBUG)
    darray_push(required_extensions, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);  // debug utilities

    DEBUG("Required extensions:");
    u32 length = darray_length(required_extensions);
    for (u32 i = 0; i < length; ++i) {
        DEBUG(required_extensions[i]);
    }
#endif

    create_info.enabledExtensionCount = darray_length(required_extensions);
    create_info.ppEnabledExtensionNames = required_extensions;

    // validation layers
    const char** required_validation_layer_names = 0;
    u32 required_validation_layer_count = 0;

    // if validation should be done. get a list of the required validation layer names.
    // and make sure they exist. validation layers should only be enabled on non-release builds.
#if defined(_DEBUG)
    INFO("Validation layers enabled. Enumerating...");

    // the list of validation layers required.
    required_validation_layer_names = darray_create(const char*);
    darray_push(required_validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    required_validation_layer_count = darray_length(required_validation_layer_names);

    // obtain a list of available validation layers
    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, 0));
    VkLayerProperties* available_layers = darray_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    // verify all required layers are available
    for (u32 i = 0; i < required_validation_layer_count; ++i) {
        INFO("Searching for layer: %s...", required_validation_layer_names[i]);
        b8 found = false;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (strings_equal(required_validation_layer_names[i], available_layers[j].layerName)) {
                found = true;
                INFO("Found");
                break;
            }
        }

        if (!found) {
            FATAL("Required validation layer is missing: %s", required_validation_layer_names[i]);
            return false;
        }
    }
    INFO("All required validation layers are present");
#endif
    create_info.enabledLayerCount = required_validation_layer_count;
    create_info.ppEnabledLayerNames = required_validation_layer_names;

    VK_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
    INFO("vulkan instance created");

    // Debugger
#if defined(_DEBUG)
    DEBUG("Creating Vulkan debugger...");
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;  // |
                                                                         //    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;  //|
                                                                         //    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug_create_info.messageSeverity = log_severity;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_callback;
    debug_create_info.pUserData = 0;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    KASSERT_MSG(func, "Failed to create debug messenger!");
    VK_CHECK(func(context.instance, &debug_create_info, context.allocator, &context.debug_messenger));
    DEBUG("Vulkan debugger created.");
#endif

    // surface
    DEBUG("creating vulkan surface");
    if (!platform_create_vulkan_surface(&context)) {
        ERROR("Failed to create platform surface");
        return false;
    }
    DEBUG("vulkan surface created");

    // Device creation
    if (!vulkan_device_create(&context)) {
        ERROR("failed to create device.");
        return false;
    }

    vulkan_swapchain_create(
        &context,
        context.framebuffer_width,
        context.framebuffer_height,
        &context.swapchain);

    // World render pass
    vulkan_renderpass_create(
        &context,
        &context.main_renderpass,
        (vec4){0, 0, context.framebuffer_width, context.framebuffer_height},
        (vec4){0.0f, 0.0f, 0.2f, 1.0f},
        1.0f,
        0,
        RENDERPASS_CLEAR_COLOUR_BUFFER_FLAG | RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG | RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG,
        false, true);

    // UI renderpass
    vulkan_renderpass_create(
        &context,
        &context.ui_renderpass,
        (vec4){0, 0, context.framebuffer_width, context.framebuffer_height},
        (vec4){0.0f, 0.0f, 0.0f, 0.0f},
        1.0f,
        0,
        RENDERPASS_CLEAR_NONE_FLAG,
        true, false);

    // Regenerate swapchain and world framebuffers
    regenerate_framebuffers();

    // create command buffers
    create_command_buffers(backend);

    // Create sync objects.
    context.image_available_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);
    context.queue_complete_semaphores = darray_reserve(VkSemaphore, context.swapchain.max_frames_in_flight);

    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);

        // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
        // This will prevent the application from waiting indefinitely for the first frame to render since it
        // cannot be rendered until a frame is "rendered" before it.
        VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(context.device.logical_device, &fence_create_info, context.allocator, &context.in_flight_fences[i]));
    }

    // In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
    // because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
    // by this list.
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        context.images_in_flight[i] = 0;
    }

    // Create builtin shaders
    if (!vulkan_material_shader_create(&context, &context.material_shader)) {
        ERROR("Error loading built-in basic_lighting shader.");
        return false;
    }
    if (!vulkan_ui_shader_create(&context, &context.ui_shader)) {
        ERROR("Error loading built-in ui shader.");
        return false;
    }

    create_buffers(&context);

    // Mark all geometries as invalid
    for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
        context.geometries[i].id = INVALID_ID;
    }

    INFO("vulkan renderer initialized successfully.");

    return true;
}

void vulkan_renderer_backend_shutdown(struct renderer_backend* backend) {
    // wait until device is not doing anything at all.
    vkDeviceWaitIdle(context.device.logical_device);

    vulkan_buffer_destroy(&context, &context.object_vertex_buffer);
    vulkan_buffer_destroy(&context, &context.object_index_buffer);

    vulkan_ui_shader_destroy(&context, &context.ui_shader);
    vulkan_material_shader_destroy(&context, &context.material_shader);

    // Sync objects
    for (u8 i = 0; i < context.swapchain.max_frames_in_flight; ++i) {
        if (context.image_available_semaphores[i]) {
            vkDestroySemaphore(
                context.device.logical_device,
                context.image_available_semaphores[i],
                context.allocator);
            context.image_available_semaphores[i] = 0;
        }
        if (context.queue_complete_semaphores[i]) {
            vkDestroySemaphore(
                context.device.logical_device,
                context.queue_complete_semaphores[i],
                context.allocator);
            context.queue_complete_semaphores[i] = 0;
        }
        vkDestroyFence(context.device.logical_device, context.in_flight_fences[i], context.allocator);
    }
    darray_destroy(context.image_available_semaphores);
    context.image_available_semaphores = 0;

    darray_destroy(context.queue_complete_semaphores);
    context.queue_complete_semaphores = 0;

    // Command buffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        if (context.graphics_command_buffers[i].handle) {
            vulkan_command_buffer_free(
                &context,
                context.device.graphics_command_pool,
                &context.graphics_command_buffers[i]);
            context.graphics_command_buffers[i].handle = 0;
        }
    }
    darray_destroy(context.graphics_command_buffers);
    context.graphics_command_buffers = 0;

    // Destroy framebuffers
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vkDestroyFramebuffer(context.device.logical_device, context.world_framebuffers[i], context.allocator);
        vkDestroyFramebuffer(context.device.logical_device, context.swapchain.framebuffers[i], context.allocator);
    }

    // Renderpasses
    vulkan_renderpass_destroy(&context, &context.ui_renderpass);
    vulkan_renderpass_destroy(&context, &context.main_renderpass);

    // swapchain
    vulkan_swapchain_destroy(&context, &context.swapchain);

    DEBUG("destroying vulkan device...");
    vulkan_device_destroy(&context);

    DEBUG("destroying vulkan surface...");
    if (context.surface) {
        vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
        context.surface = 0;
    }

#if defined(_DEBUG)
    DEBUG("Destroying Vulkan debugger...");
    if (context.debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(context.instance, context.debug_messenger, context.allocator);
    }
#endif

    DEBUG("Destroying Vulkan instance...");
    vkDestroyInstance(context.instance, context.allocator);
}

void vulkan_renderer_backend_on_resized(struct renderer_backend* backend, u16 width, u16 height) {
    // Update the "framebuffer size generation", a counter which indicates when the
    // framebuffer size has been updated.
    cached_framebuffer_width = width;
    cached_framebuffer_height = height;
    context.framebuffer_size_generation++;

    INFO("Vulkan renderer backend->resized: w/h/gen: %i/%i/%llu", width, height, context.framebuffer_size_generation);
}

b8 vulkan_renderer_backend_begin_frame(struct renderer_backend* backend, f32 delta_time) {
    context.frame_delta_time = delta_time;
    vulkan_device* device = &context.device;

    // Check if recreating swap chain and boot out.
    if (context.recreating_swapchain) {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_success(result)) {
            ERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", vulkan_result_string(result, true));
            return false;
        }
        INFO("Recreating swapchain, booting.");
        return false;
    }

    // Check if the framebuffer has been resized. If so, a new swapchain must be created.
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation) {
        VkResult result = vkDeviceWaitIdle(device->logical_device);
        if (!vulkan_result_is_success(result)) {
            ERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", vulkan_result_string(result, true));
            return false;
        }

        // If the swapchain recreation failed (because, for example, the window was minimized),
        // boot out before unsetting the flag.
        if (!recreate_swapchain(backend)) {
            return false;
        }

        INFO("Resized, booting.");
        return false;
    }

    // Wait for the execution of the current frame to complete. The fence being free will allow this one to move on.
    VkResult result = vkWaitForFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame], true, UINT64_MAX);
    if (!vulkan_result_is_success(result)) {
        ERROR("In-flight fence wait failure! error: %s", vulkan_result_string(result, true));
        return false;
    }

    // Acquire the next image from the swap chain. Pass along the semaphore that should signaled when this completes.
    // This same semaphore will later be waited on by the queue submission to ensure this image is available
    if (!vulkan_swapchain_acquire_next_image_index(
            &context,
            &context.swapchain,
            UINT64_MAX,
            context.image_available_semaphores[context.current_frame],
            0,
            &context.image_index)) {
        return false;
    }

    // Begin recording commands.
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];
    vulkan_command_buffer_reset(command_buffer);
    vulkan_command_buffer_begin(command_buffer, false, false, false);

    // Dynamic state
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)context.framebuffer_height;
    viewport.width = (f32)context.framebuffer_width;
    viewport.height = -(f32)context.framebuffer_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = context.framebuffer_width;
    scissor.extent.height = context.framebuffer_height;

    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    context.main_renderpass.render_area.z = context.framebuffer_width;
    context.main_renderpass.render_area.w = context.framebuffer_height;

    return true;
}

void vulkan_renderer_update_global_world_state(matrix4 projection, matrix4 view, vec3 view_position, vec4 ambient_colour, i32 mode) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    vulkan_material_shader_use(&context, &context.material_shader);

    context.material_shader.global_ubo.projection = projection;
    context.material_shader.global_ubo.view = view;

    // TODO: other ubo properties

    vulkan_material_shader_update_global_state(&context, &context.material_shader, context.frame_delta_time);
}

void vulkan_renderer_update_global_ui_state(matrix4 projection, matrix4 view, i32 mode) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    vulkan_ui_shader_use(&context, &context.ui_shader);

    context.ui_shader.global_ubo.projection = projection;
    context.ui_shader.global_ubo.view = view;

    // TODO: other ubo properties

    vulkan_ui_shader_update_global_state(&context, &context.ui_shader, context.frame_delta_time);
}

b8 vulkan_renderer_backend_end_frame(struct renderer_backend* backend, f32 delta_time) {
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    vulkan_command_buffer_end(command_buffer);

    // Make sure the previous frame is not using this image (i.e. its fence is being waited on)
    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE) {  // was frame
        VkResult result = vkWaitForFences(context.device.logical_device, 1, context.images_in_flight[context.image_index], true, UINT64_MAX);
        if (!vulkan_result_is_success(result)) {
            FATAL("vkWaitForFences error: %s", vulkan_result_string(result, true));
        }
    }

    // Mark the image fence as in-use by this frame.
    context.images_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];

    // Reset the fence for use on the next frame
    VK_CHECK(vkResetFences(context.device.logical_device, 1, &context.in_flight_fences[context.current_frame]));

    // Submit the queue and wait for the operation to complete.
    // Begin queue submission
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    // Command buffer(s) to be executed.
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;

    // The semaphore(s) to be signaled when the queue is complete.
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.current_frame];

    // Wait semaphore ensures that the operation cannot begin until the image is available.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];

    // Each semaphore waits on the corresponding pipeline stage to complete. 1:1 ratio.
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent colour attachment
    // writes from executing until the semaphore signals (i.e. one frame is presented at a time)
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = flags;

    VkResult result = vkQueueSubmit(
        context.device.graphics_queue,
        1,
        &submit_info,
        context.in_flight_fences[context.current_frame]);

    if (result != VK_SUCCESS) {
        ERROR("vkQueueSubmit failed with result: %s", vulkan_result_string(result, true));
        return false;
    }

    vulkan_command_buffer_update_submitted(command_buffer);
    // End queue submission

    // Give the image back to the swapchain.
    vulkan_swapchain_present(
        &context,
        &context.swapchain,
        context.device.graphics_queue,
        context.device.present_queue,
        context.queue_complete_semaphores[context.current_frame],
        context.image_index);

    return true;
}

b8 vulkan_renderer_begin_renderpass(struct renderer_backend* backend, u8 renderpass_id) {
    vulkan_renderpass* renderpass = 0;
    VkFramebuffer framebuffer = 0;
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    // Choose a renderpass based on ID.
    switch (renderpass_id) {
        case BUILTIN_RENDERPASS_WORLD:
            renderpass = &context.main_renderpass;
            framebuffer = context.world_framebuffers[context.image_index];
            break;
        case BUILTIN_RENDERPASS_UI:
            renderpass = &context.ui_renderpass;
            framebuffer = context.swapchain.framebuffers[context.image_index];
            break;
        default:
            ERROR("vulkan_renderer_begin_renderpass called on unrecognized renderpass id: %#02x", renderpass_id);
            return false;
    }

    // Begin the render pass.
    vulkan_renderpass_begin(command_buffer, renderpass, framebuffer);

    // Use the appropriate shader.
    switch (renderpass_id) {
        case BUILTIN_RENDERPASS_WORLD:
            vulkan_material_shader_use(&context, &context.material_shader);
            break;
        case BUILTIN_RENDERPASS_UI:
            vulkan_ui_shader_use(&context, &context.ui_shader);
            break;
    }

    return true;
}

b8 vulkan_renderer_end_renderpass(struct renderer_backend* backend, u8 renderpass_id) {
    vulkan_renderpass* renderpass = 0;
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    // Choose a renderpass based on ID.
    switch (renderpass_id) {
        case BUILTIN_RENDERPASS_WORLD:
            renderpass = &context.main_renderpass;
            break;
        case BUILTIN_RENDERPASS_UI:
            renderpass = &context.ui_renderpass;
            break;
        default:
            ERROR("vulkan_renderer_end_renderpass called on unrecognized renderpass id:  %#02x", renderpass_id);
            return false;
    }

    vulkan_renderpass_end(command_buffer, renderpass);
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            ERROR(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            WARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            INFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            TRACE(callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}

i32 find_memory_index(u32 type_filter, u32 property_flags) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);

    for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
        // check each memory type to see if it's bit is set to 1
        if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags) == property_flags) {
            return i;
        }
    }

    WARN("Unable to find suitable memory type");
    return -1;
}

void create_command_buffers(renderer_backend* backend) {
    if (!context.graphics_command_buffers) {
        context.graphics_command_buffers = darray_reserve(vulkan_command_buffer, context.swapchain.image_count);
        for (u32 i = 0; i < context.swapchain.image_count; ++i) {
            kzero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        }
    }

    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        if (context.graphics_command_buffers[i].handle) {
            vulkan_command_buffer_free(
                &context,
                context.device.graphics_command_pool,
                &context.graphics_command_buffers[i]);
        }
        kzero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
        vulkan_command_buffer_allocate(
            &context,
            context.device.graphics_command_pool,
            true,
            &context.graphics_command_buffers[i]);
    }

    DEBUG("Vulkan command buffers created.");
}

void regenerate_framebuffers() {
    u32 image_count = context.swapchain.image_count;
    for (u32 i = 0; i < image_count; ++i) {
        VkImageView world_attachments[2] = {context.swapchain.views[i], context.swapchain.depth_attachment.view};
        VkFramebufferCreateInfo framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_create_info.renderPass = context.main_renderpass.handle;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments = world_attachments;
        framebuffer_create_info.width = context.framebuffer_width;
        framebuffer_create_info.height = context.framebuffer_height;
        framebuffer_create_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(context.device.logical_device, &framebuffer_create_info, context.allocator, &context.world_framebuffers[i]));

        // Swapchain framebuffers (UI pass). Outputs to swapchain images
        VkImageView ui_attachments[1] = {context.swapchain.views[i]};
        VkFramebufferCreateInfo sc_framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        sc_framebuffer_create_info.renderPass = context.ui_renderpass.handle;
        sc_framebuffer_create_info.attachmentCount = 1;
        sc_framebuffer_create_info.pAttachments = ui_attachments;
        sc_framebuffer_create_info.width = context.framebuffer_width;
        sc_framebuffer_create_info.height = context.framebuffer_height;
        sc_framebuffer_create_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(context.device.logical_device, &sc_framebuffer_create_info, context.allocator, &context.swapchain.framebuffers[i]));
    }
}

b8 recreate_swapchain(renderer_backend* backend) {
    // If already being recreated, do not try again.
    if (context.recreating_swapchain) {
        DEBUG("recreate_swapchain called when already recreating. Booting.");
        return false;
    }

    // Detect if the window is too small to be drawn to
    if (context.framebuffer_width == 0 || context.framebuffer_height == 0) {
        DEBUG("recreate_swapchain called when window is < 1 in a dimension. Booting.");
        return false;
    }

    // Mark as recreating if the dimensions are valid.
    context.recreating_swapchain = true;

    // Wait for any operations to complete.
    vkDeviceWaitIdle(context.device.logical_device);

    // Clear these out just in case.
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        context.images_in_flight[i] = 0;
    }

    // Requery support
    vulkan_device_query_swapchain_support(
        context.device.physical_device,
        context.surface,
        &context.device.swapchain_support);
    vulkan_device_detect_depth_format(&context.device);

    vulkan_swapchain_recreate(
        &context,
        cached_framebuffer_width,
        cached_framebuffer_height,
        &context.swapchain);

    // Sync the framebuffer size with the cached sizes.
    context.framebuffer_width = cached_framebuffer_width;
    context.framebuffer_height = cached_framebuffer_height;
    context.main_renderpass.render_area.z = context.framebuffer_width;
    context.main_renderpass.render_area.w = context.framebuffer_height;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    // Update framebuffer size generation.
    context.framebuffer_size_last_generation = context.framebuffer_size_generation;

    // cleanup swapchain command buffer
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vulkan_command_buffer_free(&context, context.device.graphics_command_pool, &context.graphics_command_buffers[i]);
    }

    // Framebuffers.
    for (u32 i = 0; i < context.swapchain.image_count; ++i) {
        vkDestroyFramebuffer(context.device.logical_device, context.world_framebuffers[i], context.allocator);
        vkDestroyFramebuffer(context.device.logical_device, context.swapchain.framebuffers[i], context.allocator);
    }

    context.main_renderpass.render_area.x = 0;
    context.main_renderpass.render_area.y = 0;
    context.main_renderpass.render_area.z = context.framebuffer_width;
    context.main_renderpass.render_area.w = context.framebuffer_height;

    // regenerate swapchain and world framebuffers.
    regenerate_framebuffers();

    create_command_buffers(backend);

    // Clear the recreating flag.
    context.recreating_swapchain = false;

    return true;
}

b8 create_buffers(vulkan_context* context) {
    VkMemoryPropertyFlagBits memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // geometry vertex buffer
    const u64 vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
    if (!vulkan_buffer_create(
            context,
            vertex_buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,
            true,
            &context->object_vertex_buffer)) {
        ERROR("Error creating vertex buffer.");
        return false;
    }
    context->geometry_vertex_offset = 0;

    // geometry index buffer
    const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
    if (!vulkan_buffer_create(
            context,
            index_buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memory_property_flags,
            true,
            &context->object_index_buffer)) {
        ERROR("Error creating vertex buffer.");
        return false;
    }
    context->geometry_index_offset = 0;

    return true;
}

void vulkan_renderer_create_texture(const u8* pixels, texture* texture) {
    // Internal data creation.
    // TODO: Use an allocator for this.
    texture->internal_data = (vulkan_texture_data*)kallocate(sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);
    vulkan_texture_data* data = (vulkan_texture_data*)texture->internal_data;
    VkDeviceSize image_size = texture->width * texture->height * texture->channel_count;

    // NOTE: Assumes 8 bits per channel.
    VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

    // Create a staging buffer and load data into it.
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memory_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkan_buffer staging;
    vulkan_buffer_create(&context, image_size, usage, memory_prop_flags, true, &staging);

    vulkan_buffer_load_data(&context, &staging, 0, image_size, 0, pixels);

    // NOTE: Lots of assumptions here, different texture types will require
    // different options here.
    vulkan_image_create(
        &context,
        VK_IMAGE_TYPE_2D,
        texture->width,
        texture->height,
        image_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &data->image);

    vulkan_command_buffer temp_buffer;
    VkCommandPool pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;
    vulkan_command_buffer_allocate_and_begin_single_use(&context, pool, &temp_buffer);

    // Transition the layout from whatever it is currently to optimal for recieving data.
    vulkan_image_transition_layout(
        &context,
        &temp_buffer,
        &data->image,
        image_format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy the data from the buffer.
    vulkan_image_copy_from_buffer(&context, &data->image, staging.handle, &temp_buffer);

    // Transition from optimal for data reciept to shader-read-only optimal layout.
    vulkan_image_transition_layout(
        &context,
        &temp_buffer,
        &data->image,
        image_format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vulkan_command_buffer_end_single_use(&context, pool, &temp_buffer, queue);

    vulkan_buffer_destroy(&context, &staging);

    // Create a sampler for the texture
    VkSamplerCreateInfo sampler_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    // TODO: These filters should be configurable.
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VkResult result = vkCreateSampler(context.device.logical_device, &sampler_info, context.allocator, &data->sampler);
    if (!vulkan_result_is_success(VK_SUCCESS)) {
        ERROR("Error creating texture sampler: %s", vulkan_result_string(result, true));
        return;
    }

    texture->generation++;
}

void vulkan_renderer_destroy_texture(struct texture* texture) {
    vkDeviceWaitIdle(context.device.logical_device);

    vulkan_texture_data* data = (vulkan_texture_data*)texture->internal_data;
    if (data) {
        vulkan_image_destroy(&context, &data->image);
        kzero_memory(&data->image, sizeof(vulkan_image));
        vkDestroySampler(context.device.logical_device, data->sampler, context.allocator);
        data->sampler = 0;

        kfree(texture->internal_data, sizeof(vulkan_texture_data), MEMORY_TAG_TEXTURE);
    }
    kzero_memory(texture, sizeof(struct texture));
}

b8 vulkan_renderer_create_material(struct material* material) {
    if (material) {
        switch (material->type) {
            case MATERIAL_TYPE_WORLD:
                if (!vulkan_material_shader_acquire_resources(&context, &context.material_shader, material)) {
                    ERROR("vulkan_renderer_create_material - Failed to acquire world shader resources.");
                    return false;
                }
                break;
            case MATERIAL_TYPE_UI:
                if (!vulkan_ui_shader_acquire_resources(&context, &context.ui_shader, material)) {
                    ERROR("vulkan_renderer_create_material - Failed to acquire UI shader resources.");
                    return false;
                }
                break;
            default:
                ERROR("vulkan_renderer_create_material - unknown material type");
                return false;
        }
    }

    TRACE("Renderer: Material created.");
    return true;
}

void vulkan_renderer_destroy_material(struct material* material) {
    if (material) {
        if (material->internal_id != INVALID_ID) {
            switch (material->type) {
                case MATERIAL_TYPE_WORLD:
                    vulkan_material_shader_release_resources(&context, &context.material_shader, material);
                    break;
                case MATERIAL_TYPE_UI:
                    vulkan_ui_shader_release_resources(&context, &context.ui_shader, material);
                    break;
                default:
                    ERROR("vulkan_renderer_destroy_material - unknown material type");
                    break;
            }
        } else {
            WARN("vulkan_renderer_destroy_material called with internal_id=INVALID_ID. Nothing was done.");
        }
    } else {
        WARN("vulkan_renderer_destroy_material called with nullptr. Nothing was done.");
    }
}

b8 vulkan_renderer_create_geometry(geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count, const void* indices) {
    if (!vertex_count || !vertices) {
        ERROR("vulkan_renderer_create_geometry requires vertex data, and none was supplied. vertex_count=%d, vertices=%p", vertex_count, vertices);
        return false;
    }

    // Check if this is a re-upload. If it is, need to free old data afterward.
    b8 is_reupload = geometry->internal_id != INVALID_ID;
    vulkan_geometry_data old_range;

    vulkan_geometry_data* internal_data = 0;
    if (is_reupload) {
        internal_data = &context.geometries[geometry->internal_id];

        // Take a copy of the old range.
        old_range.index_buffer_offset = internal_data->index_buffer_offset;
        old_range.index_count = internal_data->index_count;
        old_range.index_element_size = internal_data->index_element_size;
        old_range.vertex_buffer_offset = internal_data->vertex_buffer_offset;
        old_range.vertex_count = internal_data->vertex_count;
        old_range.vertex_element_size = internal_data->vertex_element_size;
    } else {
        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
            if (context.geometries[i].id == INVALID_ID) {
                // Found a free index.
                geometry->internal_id = i;
                context.geometries[i].id = i;
                internal_data = &context.geometries[i];
                break;
            }
        }
    }
    if (!internal_data) {
        FATAL("vulkan_renderer_create_geometry failed to find a free index for a new geometry upload. Adjust config to allow for more.");
        return false;
    }

    VkCommandPool pool = context.device.graphics_command_pool;
    VkQueue queue = context.device.graphics_queue;

    // Vertex data.
    internal_data->vertex_buffer_offset = context.geometry_vertex_offset;
    internal_data->vertex_count = vertex_count;
    internal_data->vertex_element_size = sizeof(vertex_3d);
    u32 total_size = vertex_count * vertex_size;
    upload_data_range(
        &context,
        pool,
        0,
        queue,
        &context.object_vertex_buffer,
        internal_data->vertex_buffer_offset,
        total_size,
        vertices);

    // TODO: should maintain a free list instead of this.
    context.geometry_vertex_offset += total_size;

    // Index data, if applicable
    if (index_count && indices) {
        internal_data->index_buffer_offset = context.geometry_index_offset;
        internal_data->index_count = index_count;
        internal_data->index_element_size = sizeof(u32);
        total_size = index_count * index_size;
        upload_data_range(
            &context, 
            pool, 
            0, 
            queue, 
            &context.object_index_buffer, 
            internal_data->index_buffer_offset, 
            total_size, 
            indices);

        // TODO: should maintain a free list instead of this.
        context.geometry_index_offset += total_size;
    }

    if (internal_data->generation == INVALID_ID) {
        internal_data->generation = 0;
    } else {
        internal_data->generation++;
    }

    if (is_reupload) {
        // Free vertex data
        free_data_range(&context.object_vertex_buffer, old_range.vertex_buffer_offset, old_range.vertex_element_size * old_range.vertex_count);
        // Free index data, if applicable
        if (old_range.index_element_size > 0) {
            free_data_range(&context.object_index_buffer, old_range.index_buffer_offset, old_range.index_element_size * old_range.index_count);
        }
    }

    return true;
}

void vulkan_renderer_destroy_geometry(geometry* geometry) {
    if (geometry && geometry->internal_id != INVALID_ID) {
        vkDeviceWaitIdle(context.device.logical_device);
        vulkan_geometry_data* internal_data = &context.geometries[geometry->internal_id];

        // Free vertex data
        free_data_range(&context.object_vertex_buffer, internal_data->vertex_buffer_offset, internal_data->vertex_element_size * internal_data->vertex_count);

        // Free index data, if applicable
        if (internal_data->index_element_size > 0) {
            free_data_range(&context.object_index_buffer, internal_data->index_buffer_offset, internal_data->index_element_size * internal_data->index_count);
        }

        // Clean up data.
        kzero_memory(internal_data, sizeof(vulkan_geometry_data));
        internal_data->id = INVALID_ID;
        internal_data->generation = INVALID_ID;
    }
}

void vulkan_renderer_draw_geometry(geometry_render_data data) {
    // Ignore non-uploaded geometries.
    if (data.geometry && data.geometry->internal_id == INVALID_ID) {
        return;
    }

    vulkan_geometry_data* buffer_data = &context.geometries[data.geometry->internal_id];
    vulkan_command_buffer* command_buffer = &context.graphics_command_buffers[context.image_index];

    material* m = 0;
    if (data.geometry->material) {
        m = data.geometry->material;
    } else {
        m = material_system_get_default();
    }

    switch (m->type) {
        case MATERIAL_TYPE_WORLD:
            vulkan_material_shader_set_model(&context, &context.material_shader, data.model);
            vulkan_material_shader_apply_material(&context, &context.material_shader, m);
            break;
        case MATERIAL_TYPE_UI:
            vulkan_ui_shader_set_model(&context, &context.ui_shader, data.model);
            vulkan_ui_shader_apply_material(&context, &context.ui_shader, m);
            break;
        default:
            ERROR("vulkan_renderer_draw_geometry - unknown material type: %i", m->type);
            return;
    }

    // Bind vertex buffer at offset.
    VkDeviceSize offsets[1] = {buffer_data->vertex_buffer_offset};
    vkCmdBindVertexBuffers(command_buffer->handle, 0, 1, &context.object_vertex_buffer.handle, (VkDeviceSize*)offsets);

    // Draw indexed or non-indexed.
    if (buffer_data->index_count > 0) {
        // Bind index buffer at offset.
        vkCmdBindIndexBuffer(command_buffer->handle, context.object_index_buffer.handle, buffer_data->index_buffer_offset, VK_INDEX_TYPE_UINT32);

        // Issue the draw.
        vkCmdDrawIndexed(command_buffer->handle, buffer_data->index_count, 1, 0, 0, 0);
    } else {
        vkCmdDraw(command_buffer->handle, buffer_data->vertex_count, 1, 0, 0);
    }
}
