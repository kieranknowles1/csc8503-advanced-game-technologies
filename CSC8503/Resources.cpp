#include "Resources.h"

#include <stdexcept>

namespace NCL::CSC8503 {
    Resources::~Resources() {
        for (auto& [name, mesh] : meshes) {
            delete mesh;
        }
        for (auto& [name, texture] : textures) {
            delete texture;
        }
        for (auto& [name, shader] : shaders) {
            delete shader;
        }
    }

    Mesh* Resources::getMesh(const std::string& name) {
        checkLength(name);

        auto it = meshes.find(name);
        if (it != meshes.end()) {
            return it->second;
        }
        Mesh* mesh = renderer->LoadMesh(name);
        meshes[name] = mesh;
        return mesh;
    }

    Texture* Resources::getTexture(const std::string& name) {
        checkLength(name);

        auto it = textures.find(name);
        if (it != textures.end()) {
            return it->second;
        }
        Texture* texture = renderer->LoadTexture(name);
        textures[name] = texture;
        return texture;
    }

    Shader* Resources::getShader(const std::string& vertex, const std::string& fragment) {
        checkLength(vertex);
        checkLength(fragment);

        auto key = std::make_pair(vertex, fragment);
        auto it = shaders.find(key);
        if (it != shaders.end()) {
            return it->second;
        }
        Shader* shader = renderer->LoadShader(vertex, fragment);
        shaders[key] = shader;
        return shader;
    }

    void Resources::checkLength(const std::string& name) const {
        if ((name.length() + 1) > MaxPathLength) { // +1 for null terminator
            throw std::runtime_error("Path too long");
        }
    }
}
