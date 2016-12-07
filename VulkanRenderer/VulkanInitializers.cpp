#include "VulkanInitializers.h"
#include <memory.h>
#include "GLFW\glfw3.h"
#include <vector>
#include <iostream>
#include "VulkanFunctionPointers.h"
#include <cassert>


namespace VulkanCore
{

	VkInstance CreateInstance()
	{
		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		std::vector<const char*> instanceExtensions = { "VK_KHR_surface", "VK_KHR_win32_surface" };

		

#ifdef _DEBUG
		instanceExtensions.push_back("VK_EXT_debug_report");
#endif

		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t> (instanceExtensions.size());

		std::vector<const char*> instanceLayers;
		
#ifdef _DEBUG
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount,
			nullptr);

		std::vector<VkLayerProperties> desiredLayers{ layerCount };

		vkEnumerateInstanceLayerProperties(&layerCount,
			desiredLayers.data());

		for (const auto& p : desiredLayers)
		{
			std::cout << "EnumeratedLayer: " << p.layerName << std::endl;
			if (strcmp(p.layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
			{
				instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
				std::cout << "Standard Validation layers found" << std::endl;
			}
		}
#endif

		instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t> (instanceLayers.size());

		VkApplicationInfo applicationInfo = {};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.apiVersion = VK_API_VERSION_1_0;
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pApplicationName = "Vulkan Test";
		applicationInfo.pEngineName = "Vulkan Test Engine";

		instanceCreateInfo.pApplicationInfo = &applicationInfo;

		VkInstance Instance = VK_NULL_HANDLE;

		VkResult R = vkCreateInstance(&instanceCreateInfo, nullptr, &Instance);

		if (R == VK_SUCCESS)
		{
			std::cout << "Instance Created Successfully" << std::endl;
		}
		else
		{
			std::cout << "Failed to create instance: Error: " << R << std::endl;
		}

		return Instance;
	}

	GraphicsDevice CreateDevice(VkInstance Instance)
	{
		//Count the physical devices
		uint32_t PhysicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, nullptr);
		
		//Actually fetch the physical devices
		std::vector<VkPhysicalDevice> Devices{ PhysicalDeviceCount };
		vkEnumeratePhysicalDevices(Instance, &PhysicalDeviceCount, Devices.data());

		GraphicsDevice GFXDevice;
		
		//Find A device that supports graphics queue
		for (auto PhysicalDevice : Devices)
		{
			uint32_t QueueFamilyPropertyCount = 0;

			//Count Queue Family Properties
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, nullptr);

			//Actually Get Queue Fmaily Properties
			std::vector<VkQueueFamilyProperties> QueueFamilyProperties{ QueueFamilyPropertyCount };
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertyCount, QueueFamilyProperties.data());

			for (int i = 0; i < QueueFamilyProperties.size(); ++i)
			{
				if (QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					GFXDevice.PhysicalDevice = PhysicalDevice;
					GFXDevice.GraphicsQueueIndex = i;

					//Hacky Break Statement
					i = QueueFamilyProperties.size();
				}
			}
		}

		VkDeviceQueueCreateInfo DeviceQueueCreateInfo = {};
		DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		DeviceQueueCreateInfo.queueCount = 1;
		DeviceQueueCreateInfo.queueFamilyIndex = GFXDevice.GraphicsQueueIndex;

		static const float QueuePriorities[] = { 1.0f };
		DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;

		VkDeviceCreateInfo DeviceCreateInfo = {};
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount = 1;
		DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;

		std::vector<const char*> DeviceLayers;

#ifdef _DEBUG //TODO Validation
		//auto debugDeviceLayerNames = GetDebugDeviceLayerNames(physicalDevice);
		//deviceLayers.insert(deviceLayers.end(),
			//debugDeviceLayerNames.begin(), debugDeviceLayerNames.end());
#endif

		DeviceCreateInfo.ppEnabledLayerNames = DeviceLayers.data();
		DeviceCreateInfo.enabledLayerCount = static_cast<uint32_t> (DeviceLayers.size());

		std::vector<const char*> deviceExtensions =
		{
			"VK_KHR_swapchain"
		};

		DeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		DeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t> (deviceExtensions.size());
		VkResult R = VK_SUCCESS;
		R = vkCreateDevice(GFXDevice.PhysicalDevice, &DeviceCreateInfo, nullptr, &GFXDevice.Device);
		if (R == VK_SUCCESS)
		{
			std::cout << "Device Created Successfully" << std::endl;
			std::cout << "Graphics Queue Index: " << GFXDevice.GraphicsQueueIndex << std::endl;
		}
		else
		{
			std::cout << "Device Creation Failure: Error Code: " << R << std::endl;
		}

		vkGetDeviceQueue(GFXDevice.Device, GFXDevice.GraphicsQueueIndex, 0, &GFXDevice.GraphicsQueue);

		return GFXDevice;
	}

	VkSurfaceKHR CreateGLFWSurface(VkInstance Instance, GLFWwindow* Window)
	{
		//This function is currently dependent on GLFW 
		VkSurfaceKHR Surface;
		VkResult R = glfwCreateWindowSurface(Instance, Window, nullptr, &Surface);

		if (R == VK_SUCCESS)
		{
			std::cout << "Surface Created Successfully" << std::endl;
		}
		else
		{
			std::cout << "Failed to create Surface: Error: " << R << std::endl;
		}

		return Surface;
	}

	SwapchainData CreateSwapchain(GraphicsDevice& GFXDevice, VkSurfaceKHR& Surface, const int& BackBufferCount,const int& Width, const int& Height)
	{
		SwapchainData SwapData;

		//Check that present is supported
		VkBool32 bPresentSupported;
		vkGetPhysicalDeviceSurfaceSupportKHR(GFXDevice.PhysicalDevice, GFXDevice.GraphicsQueueIndex, Surface, &bPresentSupported);
		if (!bPresentSupported)
		{
			std::cout << "Error: Graphics Queue Does not support surface presentation" << std::endl;
		}
		
		//Surface capabilities
		VkSurfaceCapabilitiesKHR SurfaceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GFXDevice.PhysicalDevice, Surface, &SurfaceCapabilities);
		
		//Present Modes
		uint32_t PresentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(GFXDevice.PhysicalDevice, Surface, &PresentModeCount, nullptr);
		std::vector<VkPresentModeKHR> PresentModes{ PresentModeCount };
		vkGetPhysicalDeviceSurfacePresentModesKHR(GFXDevice.PhysicalDevice, Surface, &PresentModeCount, PresentModes.data());

		VkExtent2D SwapChainSize = {};
		SwapChainSize = SurfaceCapabilities.currentExtent;
		
		//FCS:: CHANGE THIS (weird requirement)
		//This is checking that the Window and surface sizes are exactly the same
		assert(static_cast<int> (SwapChainSize.width) == Width);
		assert(static_cast<int> (SwapChainSize.height) == Height);

		uint32_t SwapChainImageCount = BackBufferCount;
		assert(SwapChainImageCount >= SurfaceCapabilities.minImageCount);

		//0 means unlimited number of images
		if (SurfaceCapabilities.maxImageCount != 0)
		{
			assert(SwapChainImageCount < SurfaceCapabilities.maxImageCount);
		}

		VkSurfaceTransformFlagBitsKHR SurfaceTransformFlags;
		if (SurfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			SurfaceTransformFlags = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			SurfaceTransformFlags = SurfaceCapabilities.currentTransform;
		}

		VkColorSpaceKHR swapchainFormatColorSpace;

		//Color Space
		uint32_t SurfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(GFXDevice.PhysicalDevice, Surface, &SurfaceFormatCount, nullptr);
		std::vector<VkSurfaceFormatKHR> SurfaceFormats{ SurfaceFormatCount };
		vkGetPhysicalDeviceSurfaceFormatsKHR(GFXDevice.PhysicalDevice, Surface, &SurfaceFormatCount, SurfaceFormats.data());

		VkColorSpaceKHR ColorSpace;

		if (SurfaceFormatCount == 1 && SurfaceFormats.front().format == VK_FORMAT_UNDEFINED)
		{
			SwapData.Format = VK_FORMAT_R8G8B8A8_UNORM;
		}
		else
		{
			SwapData.Format = SurfaceFormats.front().format;
		}
		ColorSpace = SurfaceFormats.front().colorSpace;

		VkSwapchainCreateInfoKHR SwapchainCreateInfo = {};
		SwapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapchainCreateInfo.surface = Surface;
		SwapchainCreateInfo.minImageCount = SwapChainImageCount;
		SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		SwapchainCreateInfo.preTransform = SurfaceTransformFlags;
		SwapchainCreateInfo.imageColorSpace = ColorSpace;
		SwapchainCreateInfo.imageFormat = SwapData.Format;
		SwapchainCreateInfo.pQueueFamilyIndices = nullptr;
		SwapchainCreateInfo.queueFamilyIndexCount = 0;
		SwapchainCreateInfo.clipped = VK_TRUE;
		SwapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		SwapchainCreateInfo.imageExtent = SwapChainSize;
		SwapchainCreateInfo.imageArrayLayers = 1;
		SwapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

		VkResult R = vkCreateSwapchainKHR(GFXDevice.Device, &SwapchainCreateInfo, nullptr, &SwapData.Swapchain);

		if (R == VK_SUCCESS)
		{
			std::cout << "Swapchain Created Successfully" << std::endl;
		}
		else
		{
			std::cout << "Failed to create Surface: Error: " << R << std::endl;
		}

		return SwapData;
	}

	std::vector<VkImage> GetSwapchainImages(GraphicsDevice& GFXDevice, VkSwapchainKHR& Swapchain)
	{
		uint32_t SwapchainImageCount(0);
		vkGetSwapchainImagesKHR(GFXDevice.Device, Swapchain, &SwapchainImageCount, nullptr);
		std::vector<VkImage> SwapchainImages{ SwapchainImageCount };
		vkGetSwapchainImagesKHR(GFXDevice.Device, Swapchain, &SwapchainImageCount, SwapchainImages.data());

		std::cout << SwapchainImageCount << " Swapchain Images Fetched" << std::endl;

		return SwapchainImages;
	}

	std::vector<VkImageView> CreateSwapchainImageViews(GraphicsDevice& GFXDevice, VkFormat format, const std::vector<VkImage> Images)
	{
		std::vector<VkImageView> SwapchainImageViews(Images.size());

		VkResult R = VK_SUCCESS;
		bool bFailed = false;

		for (int i = 0; i < Images.size(); ++i)
		{
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = Images[i];
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = format;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			R = vkCreateImageView(GFXDevice.Device, &imageViewCreateInfo, nullptr,
				&SwapchainImageViews[i]);

			if (R != VK_SUCCESS)
			{
				bFailed = true;
				std::cout << "Swapchain image View creation Failed with error: " << R << std::endl;
			}
		}

		if (!bFailed)
		{
			std::cout << "All swapchain image views created" << std::endl;
		}

		return SwapchainImageViews;
	}

	VkRenderPass CreateForwardRenderpass(GraphicsDevice& GFXDevice, VkFormat SwapFormat)
	{
		//Describe Color Attachment (this is where the final result gets drawn)
		VkAttachmentDescription AttachmentDescription = {};
		AttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescription.format = SwapFormat;
		AttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		AttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		AttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		AttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//TODO: Depth attach

		//A reference to the above attachment, for use with the subpass 
		//This is how subpasses reference attachments set up in the overarching render pass
		VkAttachmentReference AttachmentReference = {};
		AttachmentReference.attachment = 0;
		AttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//Describe our single (the minimum required) subpass 
		//We pass in our attachment reference and tell it it is a graphics subpass
		VkSubpassDescription SubpassDescription = {};
		SubpassDescription.inputAttachmentCount = 0;
		SubpassDescription.pColorAttachments = &AttachmentReference;
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		//Hook up the attachment desc and subpass to our renderpass create info struct
		VkRenderPassCreateInfo RenderPassCreateInfo = {};
		RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.attachmentCount = 1;
		RenderPassCreateInfo.subpassCount = 1;
		RenderPassCreateInfo.pSubpasses = &SubpassDescription;
		RenderPassCreateInfo.pAttachments = &AttachmentDescription;

		VkRenderPass RenderPass;
		VkResult R = vkCreateRenderPass(GFXDevice.Device, &RenderPassCreateInfo, nullptr, &RenderPass);

		if (R == VK_SUCCESS)
		{
			std::cout << "Render pass created successfully" << std::endl;
		}
		else
		{
			std::cout << "Render pass creation failed with error: " << R << std::endl;
		}

		return RenderPass;	
	}

	std::vector<VkFramebuffer> CreateFrameBuffers(GraphicsDevice& GFXDevice, VkRenderPass RenderPass, std::vector<VkImageView>& SwapchainViews, const int& width, const int& height)
	{
		std::vector<VkFramebuffer> Framebuffers;

		VkResult R = VK_SUCCESS;
		bool bFailed = false;

		for (int i = 0; i < SwapchainViews.size(); ++i)
		{
			VkFramebufferCreateInfo FramebufferCreateInfo = {};
			FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			FramebufferCreateInfo.attachmentCount = 1;
			FramebufferCreateInfo.pAttachments = &SwapchainViews[i];
			FramebufferCreateInfo.height = height;
			FramebufferCreateInfo.width = width;
			FramebufferCreateInfo.layers = 1;
			FramebufferCreateInfo.renderPass = RenderPass;

			VkFramebuffer Framebuffer;
			R = vkCreateFramebuffer(GFXDevice.Device, &FramebufferCreateInfo, nullptr, &Framebuffer);
			if (R != VK_SUCCESS)
			{
				bFailed = true;
				std::cout << "Framebuffer creation Failed with error: " << R << std::endl;
			}
			else
			{
				Framebuffers.push_back(Framebuffer);
			}
		}

		if (!bFailed)
		{
			std::cout << "All Framebuffers created" << std::endl;
		}

		return Framebuffers;
	}

	VkCommandPool CreateCommandPool(GraphicsDevice& GFXDevice)
	{
		VkCommandPoolCreateInfo CommandPoolCreateInfo = {};
		CommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		CommandPoolCreateInfo.queueFamilyIndex = GFXDevice.GraphicsQueueIndex;
		CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkCommandPool CommandPool;
		VkResult R = vkCreateCommandPool(GFXDevice.Device, &CommandPoolCreateInfo, nullptr, &CommandPool);

		if (R != VK_SUCCESS)
		{
			std::cout << "Command Pool creation failed with error: " << R << std::endl;
		}
		else
		{
			std::cout << "Command pool created successfully " << std::endl;
		}

		return CommandPool;
	}

	std::vector<VkCommandBuffer> AllocateCommandBuffers(GraphicsDevice& GFXDevice, VkCommandPool& CommandPool, const uint32_t& Count)
	{
		VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
		CommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CommandBufferAllocateInfo.commandBufferCount = Count;
		CommandBufferAllocateInfo.commandPool = CommandPool;
		CommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		std::vector<VkCommandBuffer> CommandBuffers{ Count };

		VkResult R = vkAllocateCommandBuffers(GFXDevice.Device, &CommandBufferAllocateInfo, CommandBuffers.data());

		if (R == VK_SUCCESS)
		{
			std::cout << "Command buffers successfully allocated" << std::endl;
		}
		else
		{
			std::cout << "Command Buffer allocation failed with error: " << R << std::endl;
		}

		return CommandBuffers;
	}

	VkFence CreateFence(GraphicsDevice& GFXDevice, bool bSignaled)
	{
		VkFenceCreateInfo FenceCreateInfo = {};
		FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		if (bSignaled)
		{
			FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		}
		VkFence Fence;
		VkResult R = vkCreateFence(GFXDevice.Device, &FenceCreateInfo, nullptr, &Fence);

		if (R == VK_SUCCESS)
		{
			std::cout << "Fence created successfully" << std::endl;
		}
		else
		{
			std::cout << "Fence creation failed with error: " << R << std::endl;
		}

		return Fence;
	}

	VkShaderModule LoadShader(GraphicsDevice& GFXDevice, const void* ShaderContents, const size_t Size)
	{
		VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.pCode = static_cast<const uint32_t*> (ShaderContents);
		ShaderModuleCreateInfo.codeSize = Size;
		
		VkShaderModule Shader;
		VkResult R = vkCreateShaderModule(GFXDevice.Device, &ShaderModuleCreateInfo, nullptr, &Shader);
		if (R == VK_SUCCESS)
		{
			std::cout << "Shader created successfully" << std::endl;
		}
		else
		{
			std::cout << "Shader creation failed with error: " << R << std::endl;
		}

		return Shader;
	}

	VkPipeline CreatePipeline(GraphicsDevice& GFXDevice, VkRenderPass& RenderPass, VkShaderModule& VertexShader, VkShaderModule& FragmentShader, VkExtent2D& Extent)
	{
		VkPipelineLayoutCreateInfo LayoutCreateInfo = {};
		LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		VkPipelineLayout PipelineLayout;
		VkResult R = vkCreatePipelineLayout(GFXDevice.Device, &LayoutCreateInfo, nullptr, &PipelineLayout);
		if (R != VK_SUCCESS)
		{
			std::cout << "Pipeline layout creation failed" << std::endl;
		}

		//Describe per-vertex data
		VkVertexInputBindingDescription VertInputBindingDesc;
		VertInputBindingDesc.binding = 0;
		VertInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VertInputBindingDesc.stride = sizeof(float) * 5;

		VkVertexInputAttributeDescription InputAttrDescriptions[2] = {};
		InputAttrDescriptions[0].binding = VertInputBindingDesc.binding;
		InputAttrDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		InputAttrDescriptions[0].location = 0;
		InputAttrDescriptions[0].offset = 0;

		InputAttrDescriptions[1].binding = VertInputBindingDesc.binding;
		InputAttrDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		InputAttrDescriptions[1].location = 1;
		InputAttrDescriptions[1].offset = sizeof(float) * 3;

		VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo = {};
		PipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = std::extent<decltype(InputAttrDescriptions)>::value;
		PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = InputAttrDescriptions;
		PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &VertInputBindingDesc;

		//Setup Input Assembler
		VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo = {};
		InputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		InputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		//Render viewport
		VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo = {};
		PipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

		VkViewport Viewport;
		Viewport.height = static_cast<float> (Extent.height);
		Viewport.width = static_cast<float> (Extent.width);
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.minDepth = 0;
		Viewport.maxDepth = 1;

		PipelineViewportStateCreateInfo.viewportCount = 1;
		PipelineViewportStateCreateInfo.pViewports = &Viewport;

		VkRect2D ScissorRect;
		ScissorRect.extent = Extent;
		ScissorRect.offset.x = 0;
		ScissorRect.offset.y = 0;

		PipelineViewportStateCreateInfo.scissorCount = 1;
		PipelineViewportStateCreateInfo.pScissors = &ScissorRect;

		//Color blending
		VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState = {};
		PipelineColorBlendAttachmentState.colorWriteMask = 0xF;
		PipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo = {};
		PipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		
		PipelineColorBlendStateCreateInfo.attachmentCount = 1;
		PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;

		//Raster state: for example, Draw type (which face is front, whehter or not to fill polys, culling)
		VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo = {};
		PipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		PipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		PipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
		PipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		PipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		PipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		PipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

		//Depth stencil state
		VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo = {};
		PipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
		PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
		PipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
		PipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		PipelineDepthStencilStateCreateInfo.back.failOp = VK_STENCIL_OP_KEEP;
		PipelineDepthStencilStateCreateInfo.back.passOp = VK_STENCIL_OP_KEEP;
		PipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
		PipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
		PipelineDepthStencilStateCreateInfo.front = PipelineDepthStencilStateCreateInfo.back;

		//Rasterization multi-sampling
		VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo = {};
		PipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		//VERTEX STAGE
		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfos[2] = {};
		PipelineShaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		PipelineShaderStageCreateInfos[0].module = VertexShader;
		PipelineShaderStageCreateInfos[0].pName = "main"; //Entry Point
		PipelineShaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;

		//FRAGMENT STAGE
		PipelineShaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		PipelineShaderStageCreateInfos[1].module = FragmentShader;
		PipelineShaderStageCreateInfos[1].pName = "main"; //Entry Point
		PipelineShaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		//GFX Pipeline create-info super-struct (we hook up all of the above structs here)
		VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo = {};
		GraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		GraphicsPipelineCreateInfo.layout = PipelineLayout;
		GraphicsPipelineCreateInfo.pVertexInputState = &PipelineVertexInputStateCreateInfo;
		GraphicsPipelineCreateInfo.pInputAssemblyState = &InputAssemblyCreateInfo;
		GraphicsPipelineCreateInfo.renderPass = RenderPass;
		GraphicsPipelineCreateInfo.pViewportState = &PipelineViewportStateCreateInfo;
		GraphicsPipelineCreateInfo.pColorBlendState = &PipelineColorBlendStateCreateInfo;
		GraphicsPipelineCreateInfo.pRasterizationState = &PipelineRasterizationStateCreateInfo;
		GraphicsPipelineCreateInfo.pDepthStencilState = &PipelineDepthStencilStateCreateInfo;
		GraphicsPipelineCreateInfo.pMultisampleState = &PipelineMultisampleStateCreateInfo;
		GraphicsPipelineCreateInfo.pStages = PipelineShaderStageCreateInfos;
		GraphicsPipelineCreateInfo.stageCount = 2;

		VkPipeline Pipeline;
		R = vkCreateGraphicsPipelines(GFXDevice.Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo,
												nullptr, &Pipeline);
		if (R == VK_SUCCESS)
		{
			std::cout << "Pipeline Created Successfully" << std::endl;
		}
		else
		{
			std::cout << "Pipeline Creation Failed with error: " << R << std::endl;
		}

		return Pipeline;
	}

	std::vector<MemoryTypeInfo> EnumerateHeaps(VkPhysicalDevice Device)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties = {};
		//Pull memory properties from our physical device
		vkGetPhysicalDeviceMemoryProperties(Device, &memoryProperties);

		std::vector<MemoryTypeInfo::Heap> heaps;

		//Create a heap for each memory Heap found
		for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i)
		{
			MemoryTypeInfo::Heap info;
			info.size = memoryProperties.memoryHeaps[i].size;
			info.deviceLocal = (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;

			heaps.push_back(info);
		}

		std::vector<MemoryTypeInfo> result;

		//Iterate over each memory type from memory properties, and add it to our result vector (setting values appropriately)
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
		{
			MemoryTypeInfo typeInfo;

			typeInfo.deviceLocal = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;
			typeInfo.hostVisible = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
			typeInfo.hostCoherent = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
			typeInfo.hostCached = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0;
			typeInfo.lazilyAllocated = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0;

			//Extract heap index and use it to set the heap for this memory type
			typeInfo.heap = heaps[memoryProperties.memoryTypes[i].heapIndex];

			//Simply set index of this heap from our for loop "i"
			typeInfo.index = static_cast<int> (i);

			//add it to result
			result.push_back(typeInfo);
		}

		return result;
	}

	VkDeviceMemory AllocateMemory(VkDevice Device, std::vector<MemoryTypeInfo>& MemoryInfos, const int size)
	{
		// We take the first HOST_VISIBLE memory
		for (auto& memoryInfo : MemoryInfos)
		{
			if (memoryInfo.hostVisible)
			{
				VkMemoryAllocateInfo memoryAllocateInfo = {};
				memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memoryAllocateInfo.memoryTypeIndex = memoryInfo.index;
				memoryAllocateInfo.allocationSize = size;

				VkDeviceMemory deviceMemory;
				VkResult R = vkAllocateMemory(Device, &memoryAllocateInfo, nullptr, &deviceMemory);
				if (R == VK_SUCCESS)
				{
					std::cout << "GPU Memory successfully allocated" << std::endl;
					return deviceMemory;
				}
				else
				{
					std::cout << "memory alloc failed with error: " << R << std::endl;
				}
				
			}
		}

		return VK_NULL_HANDLE;
	}

	VkBuffer AllocateBuffer(VkDevice Device, const int Size, const VkBufferUsageFlagBits UsageFlags)
	{
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = static_cast<uint32_t> (Size);
		bufferCreateInfo.usage = UsageFlags;

		VkBuffer result;
		VkResult R = vkCreateBuffer(Device, &bufferCreateInfo, nullptr, &result);
		if (R == VK_SUCCESS)
		{
			std::cout << "Buffer successfully created" << std::endl;
		}
		else
		{
			std::cout << "Buffer creation failed with error: " << R << std::endl;
		}

		return result;
	}

	TestMesh CreateMeshBuffers(GraphicsDevice& GFXDevice, VkCommandBuffer UploadCommandBuffer)
	{
		struct Vertex
		{
			float position[3];
			float uv[2];
		};

		static const Vertex vertices[4] = {
			// Upper Left
			{ { -1.0f, 1.0f, 0 },{ 0, 0 } },
			// Upper Right
			{ { 1.0f, 1.0f, 0 },{ 1, 0 } },
			// Bottom right
			{ { 1.0f, -1.0f, 0 },{ 1, 1 } },
			// Bottom left
			{ { -1.0f, -1.0f, 0 },{ 0, 1 } }
		};

		static const int indices[6] = {
			0, 1, 2, 2, 3, 0
		};

		TestMesh RetVal;

		//Get info on physical device memory heaps
		std::vector<VulkanCore::MemoryTypeInfo> MemoryHeaps = EnumerateHeaps(GFXDevice.PhysicalDevice);

		//Allocate our buffers and fetch memory requirements
		RetVal.IndexBuffer = AllocateBuffer(GFXDevice.Device, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		RetVal.VertexBuffer = AllocateBuffer(GFXDevice.Device, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		VkMemoryRequirements vertexBufferMemoryRequirements = {};
		vkGetBufferMemoryRequirements(GFXDevice.Device, RetVal.VertexBuffer, &vertexBufferMemoryRequirements);
		VkMemoryRequirements indexBufferMemoryRequirements = {};
		vkGetBufferMemoryRequirements(GFXDevice.Device, RetVal.IndexBuffer, &indexBufferMemoryRequirements);

		VkDeviceSize bufferSize = vertexBufferMemoryRequirements.size;
		// We want to place the index buffer behind the vertex buffer. Need to take
		// the alignment into account to find the next suitable location
		VkDeviceSize indexBufferOffset = RoundToNextMultiple(bufferSize, indexBufferMemoryRequirements.alignment);

		bufferSize = indexBufferOffset + indexBufferMemoryRequirements.size;

		RetVal.DeviceMemory = AllocateMemory(GFXDevice.Device, MemoryHeaps, static_cast<int>(bufferSize));

		VkResult R = VK_SUCCESS;

		R = vkBindBufferMemory(GFXDevice.Device, RetVal.VertexBuffer, RetVal.DeviceMemory, 0);
		switch (R)
		{
		case VK_SUCCESS:
			std::cout << "Vertex Buffer memory bind successful" << std::endl; break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			std::cout << "VertexBuffer memory bind Error: Out of Host Memory" << std::endl; break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			std::cout << "VertexBuffer memory bind Error: Out of Device Memory" << std::endl; break;
		default:
			std::cout << "VertexBuffer memory bind Unknown error" << std::endl;
		}
		
		vkBindBufferMemory(GFXDevice.Device, RetVal.IndexBuffer, RetVal.DeviceMemory, indexBufferOffset);

		switch (R)
		{
		case VK_SUCCESS:
			std::cout << "IndexBuffer memory bind successful" << std::endl; break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			std::cout << "IndexBuffer memory bind Error: Out of Host Memory" << std::endl; break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			std::cout << "IndexBuffer memory bind Error: Out of Device Memory" << std::endl; break;
		default:
			std::cout << "IndexBuffer memory bind Unknown error" << std::endl;
		}

		void* mapping = nullptr;
		vkMapMemory(GFXDevice.Device, RetVal.DeviceMemory, 0, VK_WHOLE_SIZE,
			0, &mapping);
		::memcpy(mapping, vertices, sizeof(vertices));

		::memcpy(static_cast<uint8_t*> (mapping) + indexBufferOffset,
			indices, sizeof(indices));
		vkUnmapMemory(GFXDevice.Device, RetVal.DeviceMemory);

		return RetVal;
	}
}