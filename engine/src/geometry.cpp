#include "mercury_geometry.h"

using namespace mercury;
using namespace geometry;
using namespace glm;

void StaticMeshData::RecomputeTangents()
{
    const size_t vertexCount = vertices.size();
    std::vector<vec3> tan1(vertexCount, glm::vec3(0.0f));
    std::vector<vec3> tan2(vertexCount, glm::vec3(0.0f));

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        uint32_t i1 = indices[i];
        uint32_t i2 = indices[i + 1];
        uint32_t i3 = indices[i + 2];

        const vec3& p1 = vertices[i1].position;
        const vec3& p2 = vertices[i2].position;
        const vec3& p3 = vertices[i3].position;

        const vec2& uv1 = vertices[i1].uv0;
        const vec2& uv2 = vertices[i2].uv0;
        const vec2& uv3 = vertices[i3].uv0;

        vec3 e1 = p2 - p1;
        vec3 e2 = p3 - p1;

        vec2 duv1 = uv2 - uv1;
        vec2 duv2 = uv3 - uv1;

        float det = duv1.x * duv2.y - duv2.x * duv1.y;
        if (fabsf(det) < 1e-6f) continue;

        float invDet = 1.0f / det;

        vec3 tangent = (e1 * duv2.y - e2 * duv2.x) * invDet;
        vec3 bitangent = (e2 * duv1.x - e1 * duv1.y) * invDet;

        tan1[i1] += tangent;   tan1[i2] += tangent;   tan1[i3] += tangent;
        tan2[i1] += bitangent; tan2[i2] += bitangent; tan2[i3] += bitangent;
    }

    for (size_t i = 0; i < vertexCount; ++i)
    {
        vec3 n = normalize(vertices[i].normal);
        vec3 t = tan1[i];
        vec3 b = tan2[i];

        // Gram-Schmidt orthogonalize
        t = normalize(t - n * dot(n, t));

        // handedness
        if (dot(cross(n, t), b) < 0.0f)
        {
            t = -t;
        }

        b = cross(n, t); // recompute clean orthogonal bitangent

        vertices[i].tangent = t;
        vertices[i].binormal = b;
    }
}

// =============================================================================
// Plane (XZ plane, Y up)
// =============================================================================
StaticMeshData geometry::CreatePlaneMesh(float width, float height, u16 subdivisionsWidth, u16 subdivisionsHeight)
{
    
    StaticMeshData mesh;

    uint32_t w = subdivisionsWidth + 1;
    uint32_t h = subdivisionsHeight + 1;

    mesh.vertices.resize(w * h);

    for (uint32_t z = 0; z < h; ++z)
    {
        float zPos = (z / (float)subdivisionsHeight - 0.5f) * height;
        float v = z / (float)subdivisionsHeight;

        for (uint32_t x = 0; x < w; ++x)
        {
            float xPos = (x / (float)subdivisionsWidth - 0.5f) * width;
            float u = x / (float)subdivisionsWidth;

            size_t idx = z * w + x;

            mesh.vertices[idx].position = vec3(xPos, 0.0f, zPos);
            mesh.vertices[idx].normal = vec3(0.0f, 1.0f, 0.0f);
            mesh.vertices[idx].uv0 = vec2(u, 1.0f - v);   // V=0 at top if you prefer
            mesh.vertices[idx].uv1 = mesh.vertices[idx].uv0; // or different scale for lightmap
        }
    }

    for (uint32_t z = 0; z < subdivisionsWidth; ++z)
    {
        for (uint32_t x = 0; x < subdivisionsHeight; ++x)
        {
            uint32_t i00 = z * w + x;
            uint32_t i10 = i00 + 1;
            uint32_t i01 = (z + 1) * w + x;
            uint32_t i11 = i01 + 1;

            // CCW when looking down +Y
            mesh.indices.push_back(i00); mesh.indices.push_back(i10); mesh.indices.push_back(i11);
            mesh.indices.push_back(i00); mesh.indices.push_back(i11); mesh.indices.push_back(i01);
        }
    }

    mesh.RecomputeTangents();
    return mesh;
}

// =============================================================================
// Cube (centered at origin, flat shaded)
// =============================================================================
StaticMeshData geometry::CreateCubeMesh(float size)   // size = edge length, cube from -size/2 to +size/2
{
    StaticMeshData mesh;
    float h = size * 0.5f;

    auto addFace = [&](vec3 c, vec3 r, vec3 u, vec3 n, bool flipU = false)
        {
            vec2 uvBase = flipU ? vec2(1.0f, 0.0f) : vec2(0.0f, 0.0f);
            vec2 uvR = flipU ? vec2(-1.0f, 0.0f) : vec2(1.0f, 0.0f);
            vec2 uvU = vec2(0.0f, 1.0f);

            vec3 p0 = c - r - u;
            vec3 p1 = c + r - u;
            vec3 p2 = c + r + u;
            vec3 p3 = c - r + u;

            uint16_t base = static_cast<uint16_t>(mesh.vertices.size());

            mesh.vertices.emplace_back(); mesh.vertices.back().position = p0; mesh.vertices.back().normal = n; mesh.vertices.back().uv0 = uvBase;
            mesh.vertices.emplace_back(); mesh.vertices.back().position = p1; mesh.vertices.back().normal = n; mesh.vertices.back().uv0 = uvBase + uvR;
            mesh.vertices.emplace_back(); mesh.vertices.back().position = p2; mesh.vertices.back().normal = n; mesh.vertices.back().uv0 = uvBase + uvR + uvU;
            mesh.vertices.emplace_back(); mesh.vertices.back().position = p3; mesh.vertices.back().normal = n; mesh.vertices.back().uv0 = uvBase + uvU;

            mesh.indices.push_back(base + 0);
            mesh.indices.push_back(base + 1);
            mesh.indices.push_back(base + 2);

            mesh.indices.push_back(base + 0);
            mesh.indices.push_back(base + 2);
            mesh.indices.push_back(base + 3);
        };

    addFace(vec3(0, 0, h), vec3(h, 0, 0), vec3(0, h, 0), vec3(0, 0, 1));          // front
    addFace(vec3(0, 0, -h), vec3(-h, 0, 0), vec3(0, h, 0), vec3(0, 0, -1), true);  // back
    addFace(vec3(0, h, 0), vec3(h, 0, 0), vec3(0, 0, -h), vec3(0, 1, 0));        // top
    addFace(vec3(0, -h, 0), vec3(h, 0, 0), vec3(0, 0, h), vec3(0, -1, 0), true);  // bottom
    addFace(vec3(h, 0, 0), vec3(0, 0, -h), vec3(0, h, 0), vec3(1, 0, 0));        // right
    addFace(vec3(-h, 0, 0), vec3(0, 0, h), vec3(0, h, 0), vec3(-1, 0, 0), true);  // left

    for (auto& v : mesh.vertices)
    {
        v.color = vec4(1.0f);
        v.uv1 = v.uv0;   // duplicate or change scale if you want lightmap UVs
    }

    mesh.RecomputeTangents();    
    return mesh;
}

// =============================================================================
// Geosphere (subdivided icosahedron -> very even triangle distribution)
// =============================================================================
StaticMeshData geometry::CreateGeoSphereMesh(float radius, u8 subdivisions)
{
    StaticMeshData mesh;

    const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;

    std::vector<vec3> baseVerts = {
        vec3(-1,  phi,  0), vec3(1,  phi,  0),
        vec3(-1, -phi,  0), vec3(1, -phi,  0),
        vec3(0, -1,  phi), vec3(0,  1,  phi),
        vec3(0, -1, -phi), vec3(0,  1, -phi),
        vec3(phi,  0, -1), vec3(phi,  0,  1),
        vec3(-phi,  0, -1), vec3(-phi,  0,  1),
    };

    mesh.vertices.reserve(10000); // rough guess
    for (auto& v : baseVerts)
    {
        v = normalize(v) * radius;
        DedicatedStaticMeshVertex vert;
        vert.position = v;
        vert.normal = v / radius;
        vert.color = vec4(1.0f);
        mesh.vertices.push_back(vert);
    }

    mesh.indices = {
        0,11,5,  0,5,1,  0,1,7,  0,7,10, 0,10,11,
        1,5,9,   5,11,4,  11,10,2, 10,7,6,  7,1,8,
        3,9,4,   3,4,2,   3,2,6,   3,6,8,   3,8,9,
        4,9,5,   2,4,11,  6,2,10,  8,6,7,   9,8,1
    };

    std::unordered_map<uint64_t, uint16_t> midCache;

    auto getMidPoint = [&](uint16_t i1, uint16_t i2) -> uint16_t
        {
            uint64_t a = i1, b = i2;
            if (a > b) std::swap(a, b);
            uint64_t key = (a << 32) | b;

            auto found = midCache.find(key);
            if (found != midCache.end()) return found->second;

            vec3 mid = normalize(mesh.vertices[i1].position + mesh.vertices[i2].position) * radius;

            uint16_t idx = static_cast<uint16_t>(mesh.vertices.size());
            DedicatedStaticMeshVertex v;
            v.position = mid;
            v.normal = mid / radius;
            v.color = vec4(1.0f);
            mesh.vertices.push_back(v);

            midCache[key] = idx;
            return idx;
        };

    for (uint32_t level = 0; level < subdivisions; ++level)
    {
        std::vector<uint16_t> newIndices;
        newIndices.reserve(mesh.indices.size() * 4);
        midCache.clear();

        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            uint16_t v1 = mesh.indices[i];
            uint16_t v2 = mesh.indices[i + 1];
            uint16_t v3 = mesh.indices[i + 2];

            uint16_t m12 = getMidPoint(v1, v2);
            uint16_t m23 = getMidPoint(v2, v3);
            uint16_t m31 = getMidPoint(v3, v1);

            newIndices.insert(newIndices.end(), {
                v1, m12, m31,
                v2, m23, m12,
                v3, m31, m23,
                m12, m23, m31
                });
        }

        mesh.indices = std::move(newIndices);
    }

    // Spherical UVs
    for (auto& vtx : mesh.vertices)
    {
        const vec3& p = vtx.position;
        float u = 0.5f + atan2(p.z, p.x) / (2.0f * 3.1415926535f);
        float v = 0.5f + asinf(p.y / radius) / 3.1415926535f;
        vtx.uv0 = vec2(u, v);
        vtx.uv1 = vtx.uv0;   // duplicate or change if you want lightmap UVs
    }

    mesh.RecomputeTangents();    
    return mesh;
}