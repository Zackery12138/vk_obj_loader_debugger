#pragma once

#include <cstdint>

#include "../labutils/vulkan_context.hpp"

#include "../labutils/vkbuffer.hpp"
#include "../labutils/allocator.hpp" 
#include "simple_model.hpp"
#include "../labutils/vkimage.hpp"
namespace lut = labutils;

//For objects without textures, I chose to emplace 0,0 as texcoords into vertices, 
//and then simply left empty for image, imageview and descriptor
struct Mesh
{
	lut::Buffer vertices; //positions, colors, texcoords
	lut::Buffer indices;
	std::uint32_t indexCount = 0;
	lut::Image image;
	lut::ImageView view;
	VkDescriptorSet materialDescriptor = VK_NULL_HANDLE;
};

struct ModelPack
{
	std::vector<Mesh> Meshes;
};



ModelPack set_up_model(lut::VulkanWindow const& aWindow, lut::Allocator const& aAllocator, SimpleModel const& aModel, VkCommandPool& aLoadCmdPool,
	VkDescriptorPool& aDesPool, VkSampler& aSampler, VkDescriptorSetLayout& aLayout);

std::tuple<VkDescriptorSet, lut::Image, lut::ImageView> create_descriptor_for_textured_mesh(lut::VulkanWindow const& aWindow, const char* aPath, VkCommandPool& aLoadCmdPool,
	VkDescriptorPool& aDesPool, lut::Allocator const& aAllocator, VkSampler& aSampler, VkDescriptorSetLayout& aLayout);




