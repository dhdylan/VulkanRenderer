#pragma once
#include "main.cpp"

class TextureImage
{
public:
    TextureImage(const char* path);


private:
	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;
	VkFormat format;
	uint32_t mipLevels;

	void createTextureImage(const char* path);

	void generateMipmaps(int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
};

