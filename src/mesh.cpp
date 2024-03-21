#include "disable_all_warnings.h"
DISABLE_WARNINGS_PUSH()
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <gsl-lite/gsl-lite.hpp>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <numeric>
#include <stack>
#include "mesh.h"

static glm::mat4 assimpMatrix(const aiMatrix4x4 &m) {
    glm::mat4 matrix;
    matrix[0][0] = m.a1;
    matrix[0][1] = m.b1;
    matrix[0][2] = m.c1;
    matrix[0][3] = m.d1;
    matrix[1][0] = m.a2;
    matrix[1][1] = m.b2;
    matrix[1][2] = m.c2;
    matrix[1][3] = m.d2;
    matrix[2][0] = m.a3;
    matrix[2][1] = m.b3;
    matrix[2][2] = m.c3;
    matrix[2][3] = m.d3;
    matrix[3][0] = m.a4;
    matrix[3][1] = m.b4;
    matrix[3][2] = m.c4;
    matrix[3][3] = m.d4;
    return matrix;
}

static glm::vec3 assimpVec(const aiVector3D &v) {
    return glm::vec3(v.x, v.y, v.z);
}

static glm::vec3 assimpVec(const aiColor3D &c) {
    return glm::vec3(c.r, c.g, c.b);
}

static void centerAndScaleToUnitMesh(gsl::span<Mesh> meshes) {
    std::vector<glm::vec3> positions;
    for (const auto &mesh : meshes) {
        std::transform(std::begin(mesh.vertices), std::end(mesh.vertices), std::back_inserter(positions), [](const Vertex &v) { return v.position; });
    }
    const glm::vec3 center = std::accumulate(std::begin(positions), std::end(positions), glm::vec3(0.0F)) / static_cast<float>(positions.size());
    float maxD = 0.0F;
    for (const glm::vec3 &p : positions) {
        maxD = std::max(glm::length(p - center), maxD);
    }
    /*// REQUIRES A MODERN COMPILER
    const float maxD = std::transform_reduce(
        std::begin(vertices), std::end(vertices),
        0.0f,
        [](float lhs, float rhs) { return std::max(lhs, rhs); },
        [=](const Vertex& v) { return glm::length(v.pos - center); });*/

    for (Mesh &mesh : meshes) {
        std::transform(std::begin(mesh.vertices), std::end(mesh.vertices), std::begin(mesh.vertices),
            [=](Vertex v) {
                v.position = (v.position - center) / maxD;
                return v;
            });
    }
}

std::vector<Mesh> loadMesh(const std::filesystem::path& file, bool normalize) {
    if (!std::filesystem::exists(file)) {
        std::cerr << "File " << file << " does not exist." << std::endl;
        throw std::exception();
    }

    Assimp::Importer importer;
    const aiScene* pAssimpScene = importer.ReadFile(file.string().c_str(), aiProcess_GenNormals | aiProcess_Triangulate);

    if (pAssimpScene == nullptr || pAssimpScene->mRootNode == nullptr || pAssimpScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "Assimp failed to load mesh file " << file << std::endl;
        throw std::exception();
    }

    std::vector<Mesh> out;

    std::stack<std::tuple<aiNode*, glm::mat4>> stack;
    stack.push({ pAssimpScene->mRootNode, assimpMatrix(pAssimpScene->mRootNode->mTransformation) });
    while (!stack.empty()) {
        auto [node, matrix] = stack.top();
        stack.pop();

        matrix *= assimpMatrix(node->mTransformation);
        const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(matrix));

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            // Process sub mesh.
            const aiMesh* pAssimpMesh = pAssimpScene->mMeshes[node->mMeshes[i]];

            if (pAssimpMesh->mNumVertices == 0 || pAssimpMesh->mNumFaces == 0)
                std::cerr << "Empty mesh encountered" << std::endl;

            // Process triangles in sub mesh.
            Mesh mesh;
            for (unsigned int j = 0; j < pAssimpMesh->mNumFaces; j++) {
                const aiFace &face = pAssimpMesh->mFaces[j];
                if (face.mNumIndices != 3) {
                    std::cerr << "Found a face which is not a triangle, discarding!" << std::endl;
                }

                const unsigned int *aiIndices = face.mIndices;
                mesh.triangles.emplace_back(aiIndices[0], aiIndices[1], aiIndices[2]);
            }

            // Process vertices in sub mesh.
            for (unsigned int j = 0; j < pAssimpMesh->mNumVertices; j++) {
                const glm::vec3 pos = matrix * glm::vec4(assimpVec(pAssimpMesh->mVertices[j]), 1.0F);
                const glm::vec3 normal = normalMatrix * assimpVec(pAssimpMesh->mNormals[j]);
                mesh.vertices.push_back(Vertex { pos, normal });
            }

            // Read the material, more info can be found here:
            // http://assimp.sourceforge.net/lib_html/materials.html
            const aiMaterial* pAssimpMaterial = pAssimpScene->mMaterials[pAssimpMesh->mMaterialIndex];
            auto getMaterialColor = [&](const char* pKey, unsigned type, unsigned idx) {
                aiColor3D color(0.0F, 0.0F, 0.0F);
                pAssimpMaterial->Get(pKey, type, idx, color);
                return assimpVec(color);
            };
            auto getMaterialFloat = [&](const char* pKey, unsigned type, unsigned idx) {
                float value;
                pAssimpMaterial->Get(pKey, type, idx, value);
                return value;
            };

            mesh.material.kd = getMaterialColor(AI_MATKEY_COLOR_DIFFUSE);
            mesh.material.ks = getMaterialColor(AI_MATKEY_COLOR_SPECULAR);
            mesh.material.shininess = getMaterialFloat(AI_MATKEY_SHININESS);
            mesh.material.transparency = getMaterialFloat(AI_MATKEY_OPACITY);
            out.emplace_back(std::move(mesh));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            stack.push({ node->mChildren[i], matrix });
        }
    }

    importer.FreeScene();

    if (normalize) {
        centerAndScaleToUnitMesh(out);
    }

    return out;
}
