#include "asset.h"

enum class EBasicGeometryType {
    Plane,
    Box,
    SkyBox
};

void GeneratePlaneBufferData(
    const glm::vec3& center,
    const glm::vec3& normal,
    float width,
    float height,

    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr
) {
    if (!descPtr->isEmpty()) {
        std::cerr << "VAO desc must be empty" << std::endl;
        return;
    }
    glm::vec3 n = glm::normalize(normal);

    glm::vec3 up = std::fabs(n.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::vec3 tangent = glm::normalize(glm::cross(up, n));
    glm::vec3 bitangent = glm::normalize(glm::cross(n, tangent));

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    glm::vec3 p0 = center - hw * tangent - hh * bitangent; // 左下
    glm::vec3 p1 = center + hw * tangent - hh * bitangent; // 右下
    glm::vec3 p2 = center + hw * tangent + hh * bitangent; // 右上
    glm::vec3 p3 = center - hw * tangent + hh * bitangent; // 左上

    vertexPtr = {
        // ---- pos -------- uv ------ normal -----
        p0.x, p0.y, p0.z,   0.0f, 0.0f,   n.x, n.y, n.z,
        p1.x, p1.y, p1.z,   1.0f, 0.0f,   n.x, n.y, n.z,
        p2.x, p2.y, p2.z,   1.0f, 1.0f,   n.x, n.y, n.z,
        p3.x, p3.y, p3.z,   0.0f, 1.0f,   n.x, n.y, n.z
    };

    elePtr = {
        0, 1, 2,
        0, 2, 3
    };
    descPtr->addDesc(0, 3, GL_FLOAT, GL_FALSE, "vertex");
    descPtr->addDesc(1, 2, GL_FLOAT, GL_FALSE, "uv");
    descPtr->addDesc(2, 3, GL_FLOAT, GL_FALSE, "normal");
}

void GenerateBoxBufferData(
    const glm::vec3& center,
    const glm::vec3& size,
    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr
) {
    if (!descPtr->isEmpty()) {
        std::cerr << "VAO desc must be empty" << std::endl;
        return;
    }

    glm::vec3 half = size * 0.5f;

    // 八个角顶点
    glm::vec3 p[8] = {
        center + glm::vec3(-half.x, -half.y, -half.z), // 0: 左下后
        center + glm::vec3(half.x, -half.y, -half.z), // 1: 右下后
        center + glm::vec3(half.x,  half.y, -half.z), // 2: 右上后
        center + glm::vec3(-half.x,  half.y, -half.z), // 3: 左上后
        center + glm::vec3(-half.x, -half.y,  half.z), // 4: 左下前
        center + glm::vec3(half.x, -half.y,  half.z), // 5: 右下前
        center + glm::vec3(half.x,  half.y,  half.z), // 6: 右上前
        center + glm::vec3(-half.x,  half.y,  half.z)  // 7: 左上前
    };

    struct Face { int v0, v1, v2, v3; glm::vec3 normal; };
    Face faces[6] = {
        {4, 5, 6, 7, glm::vec3(0,0,1)},  // 前
        {0, 3, 2, 1, glm::vec3(0,0,-1)}, // 后
        {0, 4, 7, 3, glm::vec3(-1,0,0)}, // 左
        {1, 2, 6, 5, glm::vec3(1,0,0)},  // 右
        {3, 7, 6, 2, glm::vec3(0,1,0)},  // 上
        {0, 1, 5, 4, glm::vec3(0,-1,0)}  // 下
    };

    vertexPtr.clear();
    elePtr.clear();
    int vertexOffset = 0;

    for (int f = 0; f < 6; ++f) {
        Face face = faces[f];

        // 四个顶点，position + uv + normal
        glm::vec2 uv[4] = {
            {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
        };

        for (int i = 0; i < 4; ++i) {
            glm::vec3 pos;
            switch (i) {
            case 0: pos = p[face.v0]; break;
            case 1: pos = p[face.v1]; break;
            case 2: pos = p[face.v2]; break;
            case 3: pos = p[face.v3]; break;
            }
            vertexPtr.push_back(pos.x);
            vertexPtr.push_back(pos.y);
            vertexPtr.push_back(pos.z);
            vertexPtr.push_back(uv[i].x);
            vertexPtr.push_back(uv[i].y);
            vertexPtr.push_back(face.normal.x);
            vertexPtr.push_back(face.normal.y);
            vertexPtr.push_back(face.normal.z);
        }

        // 两个三角形索引
        elePtr.push_back(vertexOffset + 0);
        elePtr.push_back(vertexOffset + 1);
        elePtr.push_back(vertexOffset + 2);

        elePtr.push_back(vertexOffset + 0);
        elePtr.push_back(vertexOffset + 2);
        elePtr.push_back(vertexOffset + 3);

        vertexOffset += 4;
    }

    // VAO 描述
    descPtr->addDesc(0, 3, GL_FLOAT, GL_FALSE, "vertex");
    descPtr->addDesc(1, 2, GL_FLOAT, GL_FALSE, "uv");
    descPtr->addDesc(2, 3, GL_FLOAT, GL_FALSE, "normal");
}

void GenerateSkyboxCube(
    const glm::vec3& center,
    const glm::vec3& size,
    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr
) {
    if (!descPtr->isEmpty()) {
        std::cerr << "VAO desc must be empty" << std::endl;
        return;
    }

    glm::vec3 half = size * 0.5f;

    // 八个角顶点
    glm::vec3 p[8] = {
        center + glm::vec3(-half.x, -half.y, -half.z), // 0: 左下后
        center + glm::vec3(half.x, -half.y, -half.z), // 1: 右下后
        center + glm::vec3(half.x,  half.y, -half.z), // 2: 右上后
        center + glm::vec3(-half.x,  half.y, -half.z), // 3: 左上后
        center + glm::vec3(-half.x, -half.y,  half.z), // 4: 左下前
        center + glm::vec3(half.x, -half.y,  half.z), // 5: 右下前
        center + glm::vec3(half.x,  half.y,  half.z), // 6: 右上前
        center + glm::vec3(-half.x,  half.y,  half.z)  // 7: 左上前
    };

    struct Face { int v0, v1, v2, v3; glm::vec3 normal; };
    Face faces[6] = {
        {4, 5, 6, 7, glm::vec3(0,0,1)},  // 前
        {1, 0, 3, 2, glm::vec3(0,0,-1)}, // 后
        {0, 4, 7, 3, glm::vec3(-1,0,0)}, // 左
        {5, 1, 2, 6, glm::vec3(1,0,0)},  // 右
        {3, 7, 6, 2, glm::vec3(0,1,0)},  // 上
        {0, 1, 5, 4, glm::vec3(0,-1,0)}  // 下
    };

    vertexPtr.clear();
    elePtr.clear();
    int vertexOffset = 0;

    for (int f = 0; f < 6; ++f) {
        Face face = faces[f];

        // 6 张面独立 UV，局部从 (0,0) 到 (1,1)
        glm::vec2 uv[4] = {
            {0.0f, 0.0f}, // 左下
            {1.0f, 0.0f}, // 右下
            {1.0f, 1.0f}, // 右上
            {0.0f, 1.0f}  // 左上
        };

        for (int i = 0; i < 4; ++i) {
            glm::vec3 pos;
            switch (i) {
            case 0: pos = p[face.v0]; break;
            case 1: pos = p[face.v1]; break;
            case 2: pos = p[face.v2]; break;
            case 3: pos = p[face.v3]; break;
            }
            vertexPtr.push_back(pos.x);
            vertexPtr.push_back(pos.y);
            vertexPtr.push_back(pos.z);

            vertexPtr.push_back(uv[i].x);
            vertexPtr.push_back(uv[i].y);

            vertexPtr.push_back(-face.normal.x);
            vertexPtr.push_back(-face.normal.y);
            vertexPtr.push_back(-face.normal.z);
        }

        // 两个三角形索引
        elePtr.push_back(vertexOffset + 0);
        elePtr.push_back(vertexOffset + 1);
        elePtr.push_back(vertexOffset + 2);

        elePtr.push_back(vertexOffset + 0);
        elePtr.push_back(vertexOffset + 2);
        elePtr.push_back(vertexOffset + 3);

        vertexOffset += 4;
    }

    // VAO 描述
    descPtr->addDesc(0, 3, GL_FLOAT, GL_FALSE, "vertex");
    descPtr->addDesc(1, 2, GL_FLOAT, GL_FALSE, "uv");
    descPtr->addDesc(2, 3, GL_FLOAT, GL_FALSE, "normal");
}

void GenerateUVSphereBufferData(
    const glm::vec3& center,
    float radius,
    uint32_t longitudeSegments,
    uint32_t latitudeSegments,
    bool positionOnly,
    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr,
	bool reverseNormal
) {
    if (!descPtr->isEmpty()) {
        std::cerr << "VAO desc must be empty" << std::endl;
        return;
    }

    vertexPtr.clear();
    elePtr.clear();

    const float PI = 3.14159265359f;

    // =========================
    // Generate vertices
    // =========================
    for (uint32_t y = 0; y <= latitudeSegments; ++y) {
        float v = float(y) / float(latitudeSegments);
        float theta = v * PI;

        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (uint32_t x = 0; x <= longitudeSegments; ++x) {
            float u = float(x) / float(longitudeSegments);
            float phi = u * 2.0f * PI;

            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            glm::vec3 normal(
                sinTheta * cosPhi,
                sinTheta * sinPhi,
                cosTheta
            );
            if(reverseNormal) {
                normal = -normal;
			}
            glm::vec3 pos = center + radius * normal;

            if (positionOnly) {
                // ---- pos ----
                vertexPtr.push_back(pos.x);
                vertexPtr.push_back(pos.y);
                vertexPtr.push_back(pos.z);
            }
            else {
                // ---- pos -------- uv ------ normal -----
                vertexPtr.push_back(pos.x);
                vertexPtr.push_back(pos.y);
                vertexPtr.push_back(pos.z);

                vertexPtr.push_back(u);
                vertexPtr.push_back(1.0f - v);

                vertexPtr.push_back(normal.x);
                vertexPtr.push_back(normal.y);
                vertexPtr.push_back(normal.z);
            }
        }
    }

    // =========================
    // Generate indices
    // =========================
    uint32_t stride = longitudeSegments + 1;

    for (uint32_t y = 0; y < latitudeSegments; ++y) {
        for (uint32_t x = 0; x < longitudeSegments; ++x) {
            uint32_t i0 = y * stride + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + stride;
            uint32_t i3 = i2 + 1;

            // two triangles
            elePtr.push_back(i0);
            elePtr.push_back(i2);
            elePtr.push_back(i1);

            elePtr.push_back(i1);
            elePtr.push_back(i2);
            elePtr.push_back(i3);
        }
    }

    // =========================
    // VAO layout
    // =========================
    if (positionOnly) {
        descPtr->addDesc(0, 3, GL_FLOAT, GL_FALSE, "vertex");
    }
    else {
        descPtr->addDesc(0, 3, GL_FLOAT, GL_FALSE, "vertex");
        descPtr->addDesc(1, 2, GL_FLOAT, GL_FALSE, "uv");
        descPtr->addDesc(2, 3, GL_FLOAT, GL_FALSE, "normal");
    }
}

void GenerateLightBoxBufferData(
    const glm::vec3& center,
    float halfWidth,   // x
    float halfHeight,  // z
    float halfBias,    // y
    bool positionOnly,

    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr
) {
    if (!descPtr->isEmpty()) {
        std::cerr << "VAO desc must be empty" << std::endl;
        return;
    }

    vertexPtr.clear();
    elePtr.clear();

    // =========================
    // Corner points
    // =========================
    float x0 = center.x - halfWidth;
    float x1 = center.x + halfWidth;
    float y0 = center.y - halfBias;
    float y1 = center.y + halfBias;
    float z0 = center.z - halfHeight;
    float z1 = center.z + halfHeight;

    struct Face {
        glm::vec3 p0, p1, p2, p3;
        glm::vec3 n;
    };

    // =========================
    // 6 faces (CCW)
    // =========================
    Face faces[6] = {
        // +X
        { {x1,y0,z0}, {x1,y0,z1}, {x1,y1,z1}, {x1,y1,z0}, {1,0,0} },
        // -X
        { {x0,y0,z1}, {x0,y0,z0}, {x0,y1,z0}, {x0,y1,z1}, {-1,0,0} },

        // +Y
        { {x0,y1,z0}, {x1,y1,z0}, {x1,y1,z1}, {x0,y1,z1}, {0,1,0} },
        // -Y
        { {x0,y0,z1}, {x1,y0,z1}, {x1,y0,z0}, {x0,y0,z0}, {0,-1,0} },

        // +Z
        { {x1,y0,z1}, {x0,y0,z1}, {x0,y1,z1}, {x1,y1,z1}, {0,0,1} },
        // -Z
        { {x0,y0,z0}, {x1,y0,z0}, {x1,y1,z0}, {x0,y1,z0}, {0,0,-1} }
    };

    const glm::vec2 uvs[4] = {
        {0, 0},
        {1, 0},
        {1, 1},
        {0, 1}
    };

    // =========================
    // Fill vertex buffer
    // =========================
    for (int f = 0; f < 6; ++f) {
        const Face& face = faces[f];

        const glm::vec3 positions[4] = {
            face.p0, face.p1, face.p2, face.p3
        };

        for (int i = 0; i < 4; ++i) {
            if (positionOnly) {
                vertexPtr.push_back(positions[i].x);
                vertexPtr.push_back(positions[i].y);
                vertexPtr.push_back(positions[i].z);
            }
            else {
                // ---- pos -------- uv ------ normal -----
                vertexPtr.push_back(positions[i].x);
                vertexPtr.push_back(positions[i].y);
                vertexPtr.push_back(positions[i].z);

                vertexPtr.push_back(uvs[i].x);
                vertexPtr.push_back(uvs[i].y);

                vertexPtr.push_back(face.n.x);
                vertexPtr.push_back(face.n.y);
                vertexPtr.push_back(face.n.z);
            }
        }
    }

    // =========================
    // Indices
    // =========================
    for (uint32_t f = 0; f < 6; ++f) {
        uint32_t base = f * 4;

        elePtr.push_back(base + 0);
        elePtr.push_back(base + 1);
        elePtr.push_back(base + 2);

        elePtr.push_back(base + 0);
        elePtr.push_back(base + 2);
        elePtr.push_back(base + 3);
    }

    // =========================
    // VAO layout
    // =========================
    if (positionOnly) {
        descPtr->addDesc(0, 3, GL_FLOAT, GL_FALSE, "vertex");
    }
    else {
        descPtr->addDesc(0, 3, GL_FLOAT, GL_FALSE, "vertex");
        descPtr->addDesc(1, 2, GL_FLOAT, GL_FALSE, "uv");
        descPtr->addDesc(2, 3, GL_FLOAT, GL_FALSE, "normal");
    }
}
