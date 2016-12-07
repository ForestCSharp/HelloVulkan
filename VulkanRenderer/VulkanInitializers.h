#pragma once

#include "vulkan\vulkan.h"
#include "GLFW\glfw3.h"
#include <vector>
#include <tuple>

namespace VulkanCore
{
	//Encapsulates a Logical Device, Queue (with Index) and the Physical Device that will perform work
	struct GraphicsDevice
	{
		VkDevice Device = VK_NULL_HANDLE;
		VkQueue GraphicsQueue = VK_NULL_HANDLE;
		int GraphicsQueueIndex = VK_NULL_HANDLE;
		VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	};

	struct SwapchainData
	{
		VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
		VkFormat Format = VK_FORMAT_UNDEFINED;
	};

	//Creates a Vulkan Instance
	VkInstance CreateInstance();

	//Creates a device and graphis queue
	GraphicsDevice CreateDevice(VkInstance Instance);

	//Creates a surface for rendering output
	VkSurfaceKHR CreateGLFWSurface(VkInstance Instance, GLFWwindow* Window);

	//Creates a swapchain used to present images to the surface
	SwapchainData CreateSwapchain(GraphicsDevice& GFXDevice, VkSurfaceKHR& Surface, const int& BackBufferCount, const int& Width, const int& Height);

	//Fetches the swapchain images for use
	std::vector<VkImage> GetSwapchainImages(GraphicsDevice& GFXDevice, VkSwapchainKHR& Swapchain);

	//Creates the image views for our swapchain images
	std::vector<VkImageView> CreateSwapchainImageViews(GraphicsDevice& GFXDevice, VkFormat format, const std::vector<VkImage> Images);

	//Creates a minimal forward-rendering reanderpass
	VkRenderPass CreateForwardRenderpass(GraphicsDevice& GFXDevice, VkFormat SwapFormat);

	//Creates framebuffers and their image views for the input render pass
	std::vector<VkFramebuffer> CreateFrameBuffers(GraphicsDevice& GFXDevice, VkRenderPass RenderPass, std::vector<VkImageView>& SwapchainViews, const int& width, const int& height);

	//Creates a command pool from which command buffers can be created
	VkCommandPool CreateCommandPool(GraphicsDevice& GFXDevice);

	//Creates "Count" command buffers of type primary
	std::vector<VkCommandBuffer> AllocateCommandBuffers(GraphicsDevice& GFXDevice, VkCommandPool& CommandPool, const uint32_t& Count);

	//Creates a fence that is signaled if requested
	VkFence CreateFence(GraphicsDevice& GFXDevice, bool bSignaled);

	//Load a Spir-V shader
	VkShaderModule LoadShader(GraphicsDevice& GFXDevice, const void* ShaderContents, const size_t Size);

	//Create the VkPipeline
	VkPipeline CreatePipeline(GraphicsDevice& GFXDevice, VkRenderPass& RenderPass, VkShaderModule& VertexShader, VkShaderModule& FragmentShader, VkExtent2D& Extent);

	struct MemoryTypeInfo
	{
		bool deviceLocal = false;
		bool hostVisible = false;
		bool hostCoherent = false;
		bool hostCached = false;
		bool lazilyAllocated = false;

		struct Heap
		{
			uint64_t size = 0;
			bool deviceLocal = false;
		};

		Heap heap;
		int index;
	};

	//Get info about memory heaps
	std::vector<MemoryTypeInfo> EnumerateHeaps(VkPhysicalDevice Device);

	//GPU Memory Alloc helper
	VkDeviceMemory AllocateMemory(VkDevice Device, std::vector<MemoryTypeInfo>& MemoryInfos, const int Size);

	//GPU Buffer alloc helper
	VkBuffer AllocateBuffer(VkDevice Device, const int Size, const VkBufferUsageFlagBits UsageFlags);

	struct TestMesh
	{
		VkBuffer VertexBuffer;
		VkBuffer IndexBuffer;
		VkDeviceMemory DeviceMemory;
	};

	//Creates some testing Mesh buffers
	TestMesh CreateMeshBuffers(GraphicsDevice& GFXDevice, VkCommandBuffer UploadCommandBuffer);

	template <typename T>
	T RoundToNextMultiple(const T a, const T multiple)
	{
		return ((a + multiple - 1) / multiple) * multiple;
	}
}
