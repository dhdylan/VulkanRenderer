#pragma once
#include "main.cpp"

namespace Utility
{
    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Error reading file: " + filename);
        }

        size_t filesize = (size_t)file.tellg();
        std::vector<char> buffer(filesize);

        file.seekg(0);
        file.read(buffer.data(), filesize);

        file.close();
        return buffer;
    }
}
