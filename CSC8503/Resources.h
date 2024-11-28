#pragma once

#include <string>
#include <map>

#include "GameTechRenderer.h"
#include "Mesh.h"
#include "TextureLoader.h"
#include "Shader.h"

namespace NCL::CSC8503 {
    // Basic resource manager
    // Everything will be lazy-loaded from disk
    class Resources {
    public:
        // Maximum length of a path string
        // NOTE: This will be used for networking, so keep it small
        const static constexpr int MaxPathLength = 32;

        struct NetworkChunk {
            char meshName[MaxPathLength];
            char textureName[MaxPathLength];
            char vertexName[MaxPathLength];
            char fragmentName[MaxPathLength];

            NetworkChunk(std::string meshName, std::string textureName, std::string vertexName, std::string fragmentName) {
                strncpy_s(this->meshName, meshName.c_str(), MaxPathLength);
                strncpy_s(this->textureName, textureName.c_str(), MaxPathLength);
                strncpy_s(this->vertexName, vertexName.c_str(), MaxPathLength);
                strncpy_s(this->fragmentName, fragmentName.c_str(), MaxPathLength);
			}
        };

        struct ResourceSet {
            Mesh* mesh;
            Texture* texture;
            Shader* shader;

            ResourceSet(Resources* resources, const NetworkChunk& chunk) {
                mesh = resources->getMesh(chunk.meshName);
                texture = resources->getTexture(chunk.textureName);
                shader = resources->getShader(chunk.vertexName, chunk.fragmentName);
            }
        };

        Resources(GameTechRenderer* renderer) : renderer(renderer) {}
        ~Resources();


        Mesh* getMesh(const std::string& name);
        Texture* getTexture(const std::string& name);
        Shader* getShader(const std::string& vertex, const std::string& fragment);
    private:
        void checkLength(const std::string& name) const;

        GameTechRenderer* renderer;

        std::map<std::string, Mesh*> meshes;
        std::map<std::string, Texture*> textures;
        std::map<std::pair<std::string, std::string>, Shader*> shaders;
    };
}
