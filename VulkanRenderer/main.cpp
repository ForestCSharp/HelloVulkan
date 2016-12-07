// VulkanRenderer.cpp : Defines the entry point for the console application.
//

//Libraries
#pragma comment(lib, "glfw3dll.lib")

#include "vulkan\vulkan.h"
#include "GLFW\glfw3.h"
#include "VulkanInitializers.h"
#include "BasicShaders.h"

#include <iostream>
#include <vector>
using namespace std;

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void window_size_callback(GLFWwindow* window, int width, int height)
{

}

int main(void)
{
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	if (!glfwVulkanSupported())
	{
		cout << "Vulkan Not supported" << endl;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	static const uint32_t Width = 1280;
	static const uint32_t Height = 720;

	window = glfwCreateWindow(Width, Height, "Vulkan Renderer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	//Vulkan initial setup
	VkInstance Instance = VulkanCore::CreateInstance();
	VulkanCore::GraphicsDevice GFXDevice = VulkanCore::CreateDevice(Instance);
	VkSurfaceKHR Surface = VulkanCore::CreateGLFWSurface(Instance, window);
	const int BackBufferCount(2);
	VulkanCore::SwapchainData SwapchainData = VulkanCore::CreateSwapchain(GFXDevice, Surface, BackBufferCount, Width, Height);
	vector<VkImage> SwapchainImages = VulkanCore::GetSwapchainImages(GFXDevice, SwapchainData.Swapchain);
	vector<VkImageView> SwapchainImageViews = VulkanCore::CreateSwapchainImageViews(GFXDevice, SwapchainData.Format, SwapchainImages);
	VkRenderPass RenderPass = VulkanCore::CreateForwardRenderpass(GFXDevice, SwapchainData.Format);
	vector<VkFramebuffer> Framebuffers = VulkanCore::CreateFrameBuffers(GFXDevice, RenderPass, SwapchainImageViews, Width, Height);
	VkCommandPool CommandPool = VulkanCore::CreateCommandPool(GFXDevice);

	//Create our setup and queue command buffers
	vector<VkCommandBuffer> CommandBuffers = VulkanCore::AllocateCommandBuffers(GFXDevice, CommandPool, BackBufferCount + 1);

	//For ease of use, splitting up our setup command buffer from the frame command buffers
	VkCommandBuffer SetupCommandBuffer = CommandBuffers[BackBufferCount];
	vector<VkCommandBuffer> FrameCommandBuffers(CommandBuffers.begin(), CommandBuffers.end() - 1);

	//Frame Fences
	vector<VkFence> FrameFences;
	for (int i = 0; i < BackBufferCount; ++i)
	{
		FrameFences.push_back(VulkanCore::CreateFence(GFXDevice, true));
	}

	//Pre-Render setup
	vkResetFences(GFXDevice.Device, 1, &FrameFences[0]);
	//Begin a command buffer, record setup steps, End the buffer, and submit it to the queue

	VkCommandBufferBeginInfo BeginInfo = {};
	BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(SetupCommandBuffer, &BeginInfo);

	VkShaderModule VertexShader = VulkanCore::LoadShader(GFXDevice, BasicVertexShader, sizeof(BasicVertexShader));
	VkShaderModule FragmentShader = VulkanCore::LoadShader(GFXDevice, BasicFragmentShader, sizeof(BasicFragmentShader));
	VkExtent2D ScreenExtent = {};
	ScreenExtent.height = Height;
	ScreenExtent.width = Width;

	VkPipeline Pipeline = VulkanCore::CreatePipeline(GFXDevice, RenderPass, VertexShader, FragmentShader, ScreenExtent);
	VulkanCore::TestMesh Mesh = VulkanCore::CreateMeshBuffers(GFXDevice, SetupCommandBuffer);

	vkEndCommandBuffer(SetupCommandBuffer);
	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &SetupCommandBuffer;
	//The frame fence here will later be used for frames, but we can use it here initially
	vkQueueSubmit(GFXDevice.GraphicsQueue, 1, &SubmitInfo, FrameFences[0]);

	//Ensure setup is done
	vkWaitForFences(GFXDevice.Device, 1, &FrameFences[0], VK_TRUE, UINT64_MAX);

	//Semaphore create info used twice below
	//Signal: Rendering completed within queue submit (when queue finishes work)
	//Wait: presenting image
	VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
	SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//Used for waiting on image acquisition
	//Signal: vkAcquireNextImageKHR
	//Wait: Queue Submit
	VkSemaphore ImageAcquiredSemaphore;
	vkCreateSemaphore(GFXDevice.Device, &SemaphoreCreateInfo,
		nullptr, &ImageAcquiredSemaphore);

	//Used for waiting on render completion
	VkSemaphore RenderingCompleteSemaphore;
	vkCreateSemaphore(GFXDevice.Device, &SemaphoreCreateInfo,
		nullptr, &RenderingCompleteSemaphore);

	uint32_t CurrentBackBuffer = 0;

	while (!glfwWindowShouldClose(window))
	{
		vkAcquireNextImageKHR(GFXDevice.Device, SwapchainData.Swapchain, UINT64_MAX, ImageAcquiredSemaphore, VK_NULL_HANDLE, &CurrentBackBuffer);

		vkWaitForFences(GFXDevice.Device, 1, &FrameFences[CurrentBackBuffer], VK_TRUE, UINT64_MAX);
		vkResetFences(GFXDevice.Device, 1, &FrameFences[CurrentBackBuffer]);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkBeginCommandBuffer(CommandBuffers[CurrentBackBuffer], &beginInfo);

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.framebuffer = Framebuffers[CurrentBackBuffer];
		renderPassBeginInfo.renderArea.extent.width = Width;
		renderPassBeginInfo.renderArea.extent.height = Height;
		renderPassBeginInfo.renderPass = RenderPass;

		VkClearValue clearValue = {};

		clearValue.color.float32[0] = 0.042f;
		clearValue.color.float32[1] = 0.042f;
		clearValue.color.float32[2] = 0.042f;
		clearValue.color.float32[3] = 1.0f;

		renderPassBeginInfo.pClearValues = &clearValue;
		renderPassBeginInfo.clearValueCount = 1;

		vkCmdBeginRenderPass(CommandBuffers[CurrentBackBuffer], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		//Render Impl
		vkCmdBindPipeline(CommandBuffers[CurrentBackBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindIndexBuffer(CommandBuffers[CurrentBackBuffer], Mesh.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(CommandBuffers[CurrentBackBuffer], 0, 1, &Mesh.VertexBuffer, offsets);
		vkCmdDrawIndexed(CommandBuffers[CurrentBackBuffer], 6, 1, 0, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentBackBuffer]);
		vkEndCommandBuffer(CommandBuffers[CurrentBackBuffer]);

		// Submit rendering work to the graphics queue
		const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &ImageAcquiredSemaphore;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &CommandBuffers[CurrentBackBuffer];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &RenderingCompleteSemaphore;
		vkQueueSubmit(GFXDevice.GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

		// Submit present operation to present queue
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &RenderingCompleteSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &SwapchainData.Swapchain;
		presentInfo.pImageIndices = &CurrentBackBuffer;
		vkQueuePresentKHR(GFXDevice.GraphicsQueue, &presentInfo);

		vkQueueSubmit(GFXDevice.GraphicsQueue, 0, nullptr, FrameFences[CurrentBackBuffer]);

		glfwPollEvents();
	}

	//VULKAN SHUTDOWN ///////////////////////////////////////////////////////////////////////
	vkDestroyPipeline(GFXDevice.Device, Pipeline, nullptr);

	//Need to store pipeline layout
	//vkDestroyPipelineLayout(GFXDevice.Device, pipelineLayout_, nullptr);

	vkDestroyBuffer(GFXDevice.Device, Mesh.VertexBuffer, nullptr);
	vkDestroyBuffer(GFXDevice.Device, Mesh.IndexBuffer, nullptr);
	vkFreeMemory(GFXDevice.Device, Mesh.DeviceMemory, nullptr);

	vkDestroyShaderModule(GFXDevice.Device, VertexShader, nullptr);
	vkDestroyShaderModule(GFXDevice.Device, FragmentShader, nullptr);

	vkDestroySemaphore(GFXDevice.Device, ImageAcquiredSemaphore, nullptr);
	vkDestroySemaphore(GFXDevice.Device, RenderingCompleteSemaphore, nullptr);

	for (int i = 0; i < FrameFences.size(); ++i)
	{
		vkDestroyFence(GFXDevice.Device, FrameFences[i], nullptr);
	}

	vkDestroyRenderPass(GFXDevice.Device, RenderPass, nullptr);

	for (int i = 0; i < Framebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(GFXDevice.Device, Framebuffers[i], nullptr);
		vkDestroyImageView(GFXDevice.Device, SwapchainImageViews[i], nullptr);
	}

	vkDestroyCommandPool(GFXDevice.Device, CommandPool, nullptr);

	vkDestroySwapchainKHR(GFXDevice.Device, SwapchainData.Swapchain, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);

	vkDestroyDevice(GFXDevice.Device, nullptr);
	vkDestroyInstance(Instance, nullptr);

	std::cout << "Vulkan Shutdown Complete" << std::endl;

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
