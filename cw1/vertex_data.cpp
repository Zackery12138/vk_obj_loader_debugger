
#include "vertex_data.hpp"

#include <limits>

#include <cstring> // for std::memcpy()
#include <tuple>
#include "../labutils/error.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/to_string.hpp"
namespace lut = labutils;


ModelPack set_up_model(lut::VulkanWindow const& aWindow, lut::Allocator const& aAllocator, SimpleModel const& aModel, VkCommandPool& aLoadCmdPool,
	VkDescriptorPool& aDesPool, VkSampler& aSampler, VkDescriptorSetLayout& aLayout)
{
	ModelPack ret;
	//extract data from SimpleModel
	for (auto mesh : aModel.meshes)
	{
		std::vector<float> vertexData;
		std::vector<uint32_t> indexData;
		std::uint32_t index = 0;
		auto& dataPos = mesh.textured ? aModel.dataTextured.positions : aModel.dataUntextured.positions;
		auto& dataTexcoords = aModel.dataTextured.texcoords;
		auto& dataCol = aModel.materials[mesh.materialIndex].diffuseColor;
		for (std::size_t i = mesh.vertexStartIndex; i < mesh.vertexStartIndex + mesh.vertexCount; ++i)
		{
			vertexData.emplace_back(dataPos[i].x);
			vertexData.emplace_back(dataPos[i].y);
			vertexData.emplace_back(dataPos[i].z);
			vertexData.emplace_back(dataCol.x);
			vertexData.emplace_back(dataCol.y);
			vertexData.emplace_back(dataCol.z);
			if (mesh.textured)
			{
				vertexData.emplace_back(dataTexcoords[i].x);
				vertexData.emplace_back(dataTexcoords[i].y);
			}
			else
			{
				vertexData.emplace_back(0.0f);
				vertexData.emplace_back(0.0f);
			}
			indexData.emplace_back(index);
			index++;
		}

		//create_buffers

		lut::Buffer vertexGPU = lut::create_buffer(aAllocator, vertexData.size() * sizeof(float),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		lut::Buffer vertexStaging = lut::create_buffer(aAllocator, vertexData.size() * sizeof(float),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		lut::Buffer indexGPU = lut::create_buffer(aAllocator, indexData.size() * sizeof(uint32_t),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		lut::Buffer indexStaging = lut::create_buffer(aAllocator, indexData.size() * sizeof(uint32_t),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);


		void* vertPtr = nullptr;
		if (auto const res = vmaMapMemory(aAllocator.allocator, vertexStaging.allocation, &vertPtr); VK_SUCCESS != res)
		{
			throw lut::Error("Mapping memory for writing\n""vmaMapMemory() returned %s", lut::to_string(res).c_str());

		}
		std::memcpy(vertPtr, vertexData.data(), vertexData.size() * sizeof(float));
		vmaUnmapMemory(aAllocator.allocator, vertexStaging.allocation);

		void* indexPtr = nullptr;
		if (auto const res = vmaMapMemory(aAllocator.allocator, indexStaging.allocation, &indexPtr); VK_SUCCESS != res)
		{
			throw lut::Error("Mapping memory for writing\n""vmaMapMemory() returned %s", lut::to_string(res).c_str());

		}
		std::memcpy(indexPtr, indexData.data(), indexData.size() * sizeof(uint32_t));
		vmaUnmapMemory(aAllocator.allocator, indexStaging.allocation);

		// We need to ensure that the Vulkan resources are alive until all the
		// transfers have completed. For simplicity, we will just wait for the
		// operations to complete with a fence. A more complex solution might want
		// to queue transfers, let these take place in the background while
		// performing other tasks
		lut::Fence uploadComplete = create_fence(aWindow);

		// Queue data uploads from staging buffers to the final buffers
		// This uses a separate command pool for simplicity.
		lut::CommandPool uploadPool = create_command_pool(aWindow);
		VkCommandBuffer uploadCmd = alloc_command_buffer(aWindow, uploadPool.handle);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (auto const res = vkBeginCommandBuffer(uploadCmd, &beginInfo); VK_SUCCESS != res)
		{
			throw lut::Error("Beginning command buffer recording\n" "vkBeginCommandBuffer() returned %s", lut::to_string(res).c_str());
		}

		VkBufferCopy vcopy{};
		vcopy.size = vertexData.size() * sizeof(float);

		vkCmdCopyBuffer(uploadCmd, vertexStaging.buffer, vertexGPU.buffer, 1, &vcopy);

		lut::buffer_barrier(uploadCmd, vertexGPU.buffer,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

		VkBufferCopy icopy{};
		icopy.size = indexData.size() * sizeof(uint32_t);

		vkCmdCopyBuffer(uploadCmd, indexStaging.buffer, indexGPU.buffer, 1, &icopy);

		lut::buffer_barrier(uploadCmd, indexGPU.buffer,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

		if (auto const res = vkEndCommandBuffer(uploadCmd); VK_SUCCESS != res)
		{
			throw lut::Error("Ennding command buffer recording\n""vkEndCommandBuffer() returned %s", lut::to_string(res).c_str());
		}

		// Submit transfer commands
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &uploadCmd;

		if (auto const res = vkQueueSubmit(aWindow.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS != res)
		{
			throw lut::Error("Submitting commands\n" "vkQueueSubmit() returned %s", lut::to_string(res));
		}

		// Wait for commands to finish before we destroy the temporary resources
		// required for the transfers (staging buffers, command pool, ...)
		//
		// The code doesn¡¯t destory the resources implicitly ¨C the resources are
		// destroyed by the destructors of the labutils wrappers for the various
		// objects once we leave the function¡¯s scope.
		if (auto const res = vkWaitForFences(aWindow.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res)
		{
			throw lut::Error("Waiting for upload to complete\n" "vkWaitForFences() returned %s", lut::to_string(res).c_str());
		}

		Mesh meshData;
		meshData.vertices = std::move(vertexGPU);
		meshData.indices = std::move(indexGPU);
		meshData.indexCount = static_cast<uint32_t>(indexData.size());
		if (mesh.textured)
		{
			auto texPath = aModel.materials[mesh.materialIndex].diffuseTexturePath.c_str();
			std::tie(meshData.materialDescriptor, meshData.image, meshData.view) = create_descriptor_for_textured_mesh(aWindow,
				texPath, aLoadCmdPool, aDesPool, aAllocator, aSampler, aLayout);
		}

		ret.Meshes.emplace_back(std::move(meshData));

	}
	return ret;
}

std::tuple<VkDescriptorSet, lut::Image, lut::ImageView> create_descriptor_for_textured_mesh(lut::VulkanWindow const& aWindow, const char* aPath, VkCommandPool& aLoadCmdPool,
	VkDescriptorPool& aDesPool, lut::Allocator const& aAllocator, VkSampler& aSampler, VkDescriptorSetLayout& aLayout)
{

	VkDescriptorSet descriptors = lut::alloc_desc_set(aWindow, aDesPool, aLayout);
	lut::Image image = lut::load_image_texture2d(aPath, aWindow, aLoadCmdPool, aAllocator);

	lut::ImageView imageView = lut::create_image_view_texture2d(aWindow, image.image, VK_FORMAT_R8G8B8A8_SRGB);

	VkDescriptorImageInfo textureInfo{};
	textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureInfo.imageView = imageView.handle;
	textureInfo.sampler = aSampler;

	VkWriteDescriptorSet desc[1]{};
	desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc[0].dstSet = descriptors;
	desc[0].dstBinding = 0;
	desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc[0].descriptorCount = 1;
	desc[0].pImageInfo = &textureInfo;

	constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
	vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);

	return { std::move(descriptors), std::move(image), std::move(imageView) };

}



