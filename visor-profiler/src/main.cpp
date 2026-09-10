#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

#include "visor/shared_metrics.hpp"

namespace {

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    std::uint32_t queue_family = 0;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    ImGui_ImplVulkanH_Window window_data;
};

void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(operation) + " failed");
    }
}

class SharedMetricsReader {
public:
    SharedMetricsReader() {
#ifndef _WIN32
        file_descriptor_ = open(visor::kSharedMetricsPath, O_RDONLY);
        if (file_descriptor_ >= 0) {
            mapping_ = mmap(nullptr, sizeof(visor::SharedMetrics), PROT_READ, MAP_SHARED, file_descriptor_, 0);
            if (mapping_ == MAP_FAILED) mapping_ = nullptr;
        }
#endif
    }

    ~SharedMetricsReader() {
#ifndef _WIN32
        if (mapping_ != nullptr) munmap(mapping_, sizeof(visor::SharedMetrics));
        if (file_descriptor_ >= 0) close(file_descriptor_);
#endif
    }

    visor::SharedMetrics snapshot() const {
        if (mapping_ == nullptr) return {};
        return *static_cast<const visor::SharedMetrics*>(mapping_);
    }

private:
#ifndef _WIN32
    int file_descriptor_ = -1;
    void* mapping_ = nullptr;
#endif
};
VulkanContext create_vulkan_context(GLFWwindow* window) {
    VulkanContext context;
    std::uint32_t extension_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = "Visor Profiler";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "Visor";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> instance_extensions(extensions, extensions + extension_count);
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = static_cast<std::uint32_t>(instance_extensions.size());
    instance_info.ppEnabledExtensionNames = instance_extensions.data();
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    instance_info.enabledExtensionCount = static_cast<std::uint32_t>(instance_extensions.size());
    instance_info.ppEnabledExtensionNames = instance_extensions.data();
    instance_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    check(vkCreateInstance(&instance_info, nullptr, &context.instance), "vkCreateInstance");

    std::uint32_t device_count = 0;
    check(vkEnumeratePhysicalDevices(context.instance, &device_count, nullptr), "vkEnumeratePhysicalDevices");
    if (device_count == 0) {
        throw std::runtime_error("No Vulkan physical device found");
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    check(vkEnumeratePhysicalDevices(context.instance, &device_count, devices.data()), "vkEnumeratePhysicalDevices");
    context.physical_device = ImGui_ImplVulkanH_SelectPhysicalDevice(context.instance);
    if (context.physical_device == VK_NULL_HANDLE) {
        throw std::runtime_error("No Vulkan physical device with presentation support found");
    }

    context.queue_family = ImGui_ImplVulkanH_SelectQueueFamilyIndex(context.physical_device);

    constexpr float priority = 1.0F;
    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = context.queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    std::vector<const char*> device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif
    device_info.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size());
    device_info.ppEnabledExtensionNames = device_extensions.data();
    check(vkCreateDevice(context.physical_device, &device_info, nullptr, &context.device), "vkCreateDevice");
    vkGetDeviceQueue(context.device, context.queue_family, 0, &context.queue);

    std::array<VkDescriptorPoolSize, 1> pool_sizes{{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}}};
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    check(vkCreateDescriptorPool(context.device, &pool_info, nullptr, &context.descriptor_pool), "vkCreateDescriptorPool");

    check(glfwCreateWindowSurface(context.instance, window, nullptr, &context.surface), "glfwCreateWindowSurface");
    context.window_data.Surface = context.surface;
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    const VkFormat surface_formats[] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,
    };
    context.window_data.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        context.physical_device, context.surface, surface_formats, 4, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
    const VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
    context.window_data.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        context.physical_device, context.surface, present_modes, 1);
    ImGui_ImplVulkanH_CreateOrResizeWindow(context.instance, context.physical_device, context.device,
        &context.window_data, context.queue_family, nullptr, width, height, 2);
    return context;
}

void destroy_vulkan_context(VulkanContext& context) {
    if (context.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(context.device);
        ImGui_ImplVulkanH_DestroyWindow(context.instance, context.device, &context.window_data, nullptr);
        if (context.descriptor_pool != VK_NULL_HANDLE) vkDestroyDescriptorPool(context.device, context.descriptor_pool, nullptr);
        vkDestroyDevice(context.device, nullptr);
    }
    if (context.surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    if (context.instance != VK_NULL_HANDLE) vkDestroyInstance(context.instance, nullptr);
}

void draw_dashboard(const visor::SharedMetrics& metrics) {
    static std::array<float, 120> frame_times{};
    static std::size_t sample = 0;
    frame_times[sample++ % frame_times.size()] = 14.0F + static_cast<float>(std::rand() % 60) / 20.0F;

    ImGui::Begin("Visor Profiler");
    ImGui::TextUnformatted("FRAME HISTORY");
    if (ImPlot::BeginPlot("##frame-history", ImVec2(-1, 180))) {
        ImPlot::SetupAxes("Frame", "ms", ImPlotAxisFlags_NoMenus, ImPlotAxisFlags_NoMenus);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, static_cast<double>(frame_times.size()), ImGuiCond_Always);
        ImPlot::PlotBars("Frame time", frame_times.data(), static_cast<int>(frame_times.size()), 0.7, 0.0, ImPlotBarsFlags_None, 0, 0.0);
        ImPlot::EndPlot();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("GPU MEMORY");
    ImGui::Text("VRAM       2,048 / 8,192 MB");
    ImGui::Text("Allocations              128");
    ImGui::Text("PIPELINE METRICS");
    ImGui::Text("Draw calls                1,284");
    ImGui::Text("Pipeline switches           37");
    ImGui::Separator();
    ImGui::TextUnformatted("EXECUTION TIMELINE");
    ImGui::Text("VRAM       %4llu / %4llu MB", metrics.used_vram_mb, metrics.total_vram_mb);
    ImGui::Indent();
    ImGui::BulletText("Depth       1.2 ms");
    ImGui::BulletText("G-Buffer    5.8 ms");
    ImGui::BulletText("Lighting    6.7 ms");
    ImGui::Unindent();
    ImGui::End();
}

void render_frame(VulkanContext& context) {
    ImGui_ImplVulkanH_Window& window = context.window_data;
    VkSemaphore image_acquired = window.FrameSemaphores[window.SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete = window.FrameSemaphores[window.SemaphoreIndex].RenderCompleteSemaphore;
    check(vkAcquireNextImageKHR(context.device, window.Swapchain, UINT64_MAX, image_acquired, VK_NULL_HANDLE,
        &window.FrameIndex), "vkAcquireNextImageKHR");

    ImGui_ImplVulkanH_Frame& frame = window.Frames[window.FrameIndex];
    check(vkWaitForFences(context.device, 1, &frame.Fence, VK_TRUE, UINT64_MAX), "vkWaitForFences");
    check(vkResetFences(context.device, 1, &frame.Fence), "vkResetFences");
    check(vkResetCommandPool(context.device, frame.CommandPool, 0), "vkResetCommandPool");
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(frame.CommandBuffer, &begin_info), "vkBeginCommandBuffer");

    VkRenderPassBeginInfo pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass_info.renderPass = window.RenderPass;
    pass_info.framebuffer = frame.Framebuffer;
    pass_info.renderArea.extent = {static_cast<std::uint32_t>(window.Width), static_cast<std::uint32_t>(window.Height)};
    pass_info.clearValueCount = 1;
    pass_info.pClearValues = &window.ClearValue;
    vkCmdBeginRenderPass(frame.CommandBuffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.CommandBuffer);
    vkCmdEndRenderPass(frame.CommandBuffer);
    check(vkEndCommandBuffer(frame.CommandBuffer), "vkEndCommandBuffer");

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_acquired;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &frame.CommandBuffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_complete;
    check(vkQueueSubmit(context.queue, 1, &submit_info, frame.Fence), "vkQueueSubmit");

    VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_complete;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &window.Swapchain;
    present_info.pImageIndices = &window.FrameIndex;
    check(vkQueuePresentKHR(context.queue, &present_info), "vkQueuePresentKHR");
    window.SemaphoreIndex = (window.SemaphoreIndex + 1) % window.SemaphoreCount;
}

} // namespace

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Visor Profiler", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return 1;
    }

    try {
        VulkanContext context = create_vulkan_context(window);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = context.instance;
        init_info.PhysicalDevice = context.physical_device;
        init_info.Device = context.device;
        init_info.QueueFamily = context.queue_family;
        init_info.Queue = context.queue;
        init_info.DescriptorPool = context.descriptor_pool;
        init_info.RenderPass = context.window_data.RenderPass;
        init_info.MinImageCount = 2;
        init_info.ImageCount = context.window_data.ImageCount;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.CheckVkResultFn = [](VkResult result) { check(result, "ImGui Vulkan"); };
        if (!ImGui_ImplVulkan_Init(&init_info)) {
            throw std::runtime_error("ImGui Vulkan backend initialization failed");
        }
        SharedMetricsReader metrics_reader;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            const visor::SharedMetrics metrics = metrics_reader.snapshot();
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            draw_dashboard(metrics);
            ImGui::Render();
            render_frame(context);
        }

        vkDeviceWaitIdle(context.device);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
    } catch (const std::exception& error) {
        std::cerr << "[Visor Profiler] " << error.what() << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}