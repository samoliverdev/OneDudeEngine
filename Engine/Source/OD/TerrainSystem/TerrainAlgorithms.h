#pragma once
// CPU-only terrain contracts. Deliberately independent of ECS, graphics and physics.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OD{
namespace TerrainCpu{

inline float Finite(float v, float fallback){
    return std::isfinite(v) ? v : fallback;
}

inline int NormalizeCells(int value){
    value = std::clamp(value, 2, 128);
    int n = 2;
    while(n < value)
        n *= 2;
    return n;
}

struct HeightfieldView{
    const float* samples = nullptr;
    int columns = 0, rows = 0;
    float width = 0, length = 0, height = 0;

    bool Valid() const{
        return samples && columns >= 2 && rows >= 2 && std::isfinite(width) && std::isfinite(length) &&
               std::isfinite(height) && width > 0 && length > 0 && height > 0;
    }

    float Unit(int x, int z) const{
        return std::clamp(
            Finite(samples[size_t(std::clamp(z, 0, rows - 1)) * columns + std::clamp(x, 0, columns - 1)], 0), 0.f, 1.f);
    }

    float Sample(float x, float z) const{
        if(!Valid() || !std::isfinite(x) || !std::isfinite(z))
            return 0;
        float sx = std::clamp(x / width, 0.f, 1.f) * (columns - 1);
        float sz = std::clamp(z / length, 0.f, 1.f) * (rows - 1);
        int ix = int(sx), iz = int(sz);
        float a = Unit(ix, iz) * (1 - (sx - ix)) + Unit(ix + 1, iz) * (sx - ix);
        float b = Unit(ix, iz + 1) * (1 - (sx - ix)) + Unit(ix + 1, iz + 1) * (sx - ix);
        return (a * (1 - (sz - iz)) + b * (sz - iz)) * height;
    }
};

struct Patch{
    int x = 0, z = 0, depth = 0;
    uint8_t stitch = 0; // left, right, near, far
};

inline uint64_t Key(const Patch& n){
    return (uint64_t(n.depth) << 56) | (uint64_t(n.x) << 28) | uint64_t(n.z);
}

inline bool Neighbors(Patch a, Patch b){
    int d = (std::max)(a.depth, b.depth);
    int ax = a.x << (d - a.depth), az = a.z << (d - a.depth), as = 1 << (d - a.depth);
    int bx = b.x << (d - b.depth), bz = b.z << (d - b.depth), bs = 1 << (d - b.depth);
    return ((ax + as == bx || bx + bs == ax) && az < bz + bs && bz < az + as) ||
           ((az + as == bz || bz + bs == az) && ax < bx + bs && bx < ax + as);
}

struct QuadSettings{
    float width = 32000, length = 32000, height = 1000;
    float eyeX = 0, eyeY = 0, eyeZ = 0;
    float minPatchSize = 128, pixelError = 6, projectionScale = 600;
    int cells = 32, maxDepth = 12, maxPatches = 2048;
    bool orthographic = false;
};

inline std::vector<unsigned int> GridIndices(int inCells, int mask){
    int c = NormalizeCells(inCells);
    auto vertex = [&](int x, int z){
        if(((mask & 1) && x == 0) || ((mask & 2) && x == c))
            z -= z % 2;
        if(((mask & 4) && z == 0) || ((mask & 8) && z == c))
            x -= x % 2;
        return unsigned(z * (c + 1) + x);
    };
    std::vector<unsigned int> result;
    result.reserve(size_t(c) * c * 6);
    auto tri = [&](unsigned a, unsigned b, unsigned d){
        int ax = int(a) % (c + 1), az = int(a) / (c + 1), bx = int(b) % (c + 1), bz = int(b) / (c + 1);
        int dx = int(d) % (c + 1), dz = int(d) / (c + 1);
        if((bz - az) * (dx - ax) - (bx - ax) * (dz - az) != 0){
            result.push_back(a);
            result.push_back(b);
            result.push_back(d);
        }
    };
    for(int z = 0; z < c; ++z)
        for(int x = 0; x < c; ++x){
            auto a = vertex(x, z), b = vertex(x, z + 1), d = vertex(x + 1, z), e = vertex(x + 1, z + 1);
            // Match Jolt heightfields: diagonal from (x,z) to (x+1,z+1).
            tri(a, b, e);
            tri(a, e, d);
        }
    return result;
}

inline std::vector<Patch> SelectPatches(QuadSettings s){
    if(!std::isfinite(s.width) || !std::isfinite(s.length) || s.width <= 0 || s.length <= 0)
        return {};
    s.cells = NormalizeCells(s.cells);
    s.maxDepth = std::clamp(s.maxDepth, 0, 16);
    s.maxPatches = std::clamp(s.maxPatches, 1, 65536);
    s.minPatchSize = (std::max)(1.f, Finite(s.minPatchSize, 128));
    s.pixelError = (std::max)(0.25f, Finite(s.pixelError, 6));
    s.projectionScale = (std::max)(1.f, Finite(s.projectionScale, 600));
    s.eyeX = Finite(s.eyeX, 0);
    s.eyeY = Finite(s.eyeY, 0);
    s.eyeZ = Finite(s.eyeZ, 0);
    int depth = 0;
    while(depth < s.maxDepth && (std::min)(s.width, s.length) / float(1 << (depth + 1)) >= s.minPatchSize)
        ++depth;
    s.maxDepth = depth;
    std::unordered_map<uint64_t, Patch> leaves;
    leaves.emplace(0, Patch{});
    auto neighbor = [&](Patch p, int edge, Patch& found){
        if(edge == 0)
            --p.x;
        if(edge == 1)
            ++p.x;
        if(edge == 2)
            --p.z;
        if(edge == 3)
            ++p.z;
        if(p.x < 0 || p.z < 0 || p.x >= (1 << p.depth) || p.z >= (1 << p.depth))
            return false;
        for(;;){
            auto it = leaves.find(Key(p));
            if(it != leaves.end()){
                found = it->second;
                return true;
            }
            if(!p.depth)
                return false;
            --p.depth;
            p.x /= 2;
            p.z /= 2;
        }
    };
    auto metric = [&](Patch p){
        float span = std::ldexp(1.f, -p.depth), w = s.width * span, l = s.length * span;
        float dx = (std::max)({p.x * w - s.eyeX, 0.f, s.eyeX - (p.x + 1) * w});
        float dz = (std::max)({p.z * l - s.eyeZ, 0.f, s.eyeZ - (p.z + 1) * l});
        float dy = (std::max)({-s.eyeY, 0.f, s.eyeY - (std::max)(0.f, s.height)});
        float dist = s.orthographic ? 1.f : (std::max)(1.f, std::sqrt(dx * dx + dy * dy + dz * dz));
        return (std::max)(w, l) / s.cells * s.projectionScale / dist;
    };

    struct Candidate{
        float score;
        Patch patch;

        bool operator<(const Candidate& b) const{
            return score < b.score;
        }
    };

    std::priority_queue<Candidate> queue;
    queue.push({metric({}), {}});
    while(!queue.empty()){
        auto candidate = queue.top();
        queue.pop();
        auto n = candidate.patch;
        if(candidate.score <= s.pixelError)
            break;
        if(n.depth >= s.maxDepth || !leaves.count(Key(n)))
            continue;
        // Plan the entire split closure first. A budget never drops coverage or
        // permits an unbalanced edge: if the closure does not fit, retain the parent.
        std::vector<Patch> closure{n};
        std::unordered_set<uint64_t> seen{Key(n)};
        for(size_t i = 0; i < closure.size(); ++i){
            for(int edge = 0; edge < 4; ++edge){
                Patch other;
                if(neighbor(closure[i], edge, other) && other.depth < closure[i].depth &&
                   seen.insert(Key(other)).second)
                    closure.push_back(other);
            }
        }
        if(leaves.size() + closure.size() * 3 > size_t(s.maxPatches))
            continue;
        std::sort(closure.begin(), closure.end(), [](Patch a, Patch b){
            return a.depth < b.depth;
        });
        for(auto parent : closure){
            leaves.erase(Key(parent));
            for(int z = 0; z < 2; ++z)
                for(int x = 0; x < 2; ++x){
                    Patch child{parent.x * 2 + x, parent.z * 2 + z, parent.depth + 1, 0};
                    leaves.emplace(Key(child), child);
                    if(child.depth < s.maxDepth)
                        queue.push({metric(child), child});
                }
        }
    }
    std::vector<Patch> result;
    result.reserve(leaves.size());
    for(auto& pair : leaves){
        auto p = pair.second;
        for(int edge = 0; edge < 4; ++edge){
            Patch other;
            if(neighbor(p, edge, other) && other.depth < p.depth)
                p.stitch |= uint8_t(1 << edge);
        }
        result.push_back(p);
    }
    std::sort(result.begin(), result.end(), [](Patch a, Patch b){
        return Key(a) < Key(b);
    });
    return result;
}

enum class BrushMode{
    Raise,
    Lower,
    Flatten,
    Smooth,
    Paint
};

struct Brush{
    BrushMode mode = BrushMode::Raise;
    float x = 0, z = 0, radius = 32, amount = 1, target = 0, hardness = 0.25f;
};

struct Edit{
    size_t index;
    float before, after;
};

inline float BrushWeight(float distance, float radius, float hardness){
    if(radius <= 0 || distance >= radius)
        return 0;
    float inner = std::clamp(hardness, 0.f, 0.99f);
    float t = std::clamp((distance / radius - inner) / (1 - inner), 0.f, 1.f);
    return 1 - t * t * (3 - 2 * t);
}

inline std::vector<Edit> HeightBrush(HeightfieldView v, const Brush& b){
    std::vector<Edit> result;
    if(!v.Valid() || !std::isfinite(b.radius) || b.radius <= 0 || !std::isfinite(b.amount) || !std::isfinite(b.x) ||
       !std::isfinite(b.z) || !std::isfinite(b.target) || !std::isfinite(b.hardness))
        return result;
    int x0 = int(std::clamp((b.x - b.radius) / v.width, 0.f, 1.f) * (v.columns - 1));
    int z0 = int(std::clamp((b.z - b.radius) / v.length, 0.f, 1.f) * (v.rows - 1));
    int x1 = int(std::ceil(std::clamp((b.x + b.radius) / v.width, 0.f, 1.f) * (v.columns - 1)));
    int z1 = int(std::ceil(std::clamp((b.z + b.radius) / v.length, 0.f, 1.f) * (v.rows - 1)));
    for(int z = z0; z <= z1; ++z)
        for(int x = x0; x <= x1; ++x){
            float dx = x * v.width / (v.columns - 1) - b.x, dz = z * v.length / (v.rows - 1) - b.z;
            float weight = BrushWeight(std::sqrt(dx * dx + dz * dz), b.radius, b.hardness);
            if(weight <= 0)
                continue;
            size_t index = size_t(z) * v.columns + x;
            float before = v.Unit(x, z), after = before;
            if(b.mode == BrushMode::Raise || b.mode == BrushMode::Lower)
                after += weight * b.amount / v.height * (b.mode == BrushMode::Lower ? -1 : 1);
            if(b.mode == BrushMode::Flatten)
                after += (b.target / v.height - before) * std::clamp(weight * b.amount, 0.f, 1.f);
            if(b.mode == BrushMode::Smooth){
                float sum = 0;
                for(int j = -1; j <= 1; ++j)
                    for(int i = -1; i <= 1; ++i)
                        sum += v.Unit(x + i, z + j);
                after += (sum / 9 - before) * std::clamp(weight * b.amount, 0.f, 1.f);
            }
            after = std::clamp(after, 0.f, 1.f);
            if(after != before)
                result.push_back({index, before, after});
        }
    return result;
}

inline void ApplyEdits(float* data, size_t count, const std::vector<Edit>& edits, bool forward){
    for(auto& edit : edits)
        if(edit.index < count)
            data[edit.index] = forward ? edit.after : edit.before;
}

inline std::vector<uint8_t> EncodeRaw16(const float* data, size_t count, bool bigEndian){
    std::vector<uint8_t> result(count * 2);
    for(size_t i = 0; i < count; ++i){
        auto v = uint16_t(std::lround(std::clamp(Finite(data[i], 0), 0.f, 1.f) * 65535));
        result[i * 2 + (bigEndian ? 1 : 0)] = uint8_t(v & 255);
        result[i * 2 + (bigEndian ? 0 : 1)] = uint8_t(v >> 8);
    }
    return result;
}

inline std::vector<float> DecodeRaw16(const std::vector<uint8_t>& bytes, int width, int height, bool bigEndian){
    if(width < 2 || height < 2 || width > 16385 || height > 16385 || bytes.size() != size_t(width) * height * 2)
        return {};
    std::vector<float> result(size_t(width) * height);
    for(size_t i = 0; i < result.size(); ++i)
        result[i] =
            float(uint16_t(bytes[i * 2 + (bigEndian ? 1 : 0)]) | (uint16_t(bytes[i * 2 + (bigEndian ? 0 : 1)]) << 8)) /
            65535;
    return result;
}

inline bool Raycast(HeightfieldView v,
                    std::array<float, 3> origin,
                    std::array<float, 3> dir,
                    float maxDistance,
                    float step,
                    float& hit){
    if(!v.Valid() || !std::isfinite(maxDistance) || maxDistance <= 0)
        return false;
    float lo = 0, hi = maxDistance;
    const float bounds[3] = {v.width, v.height, v.length};
    for(int axis = 0; axis < 3; ++axis){
        if(!std::isfinite(dir[axis]) || !std::isfinite(origin[axis]))
            return false;
        if(std::abs(dir[axis]) < 1e-8f){
            if(origin[axis] < 0 || origin[axis] > bounds[axis])
                return false;
        } else{
            float a = -origin[axis] / dir[axis], b = (bounds[axis] - origin[axis]) / dir[axis];
            if(a > b)
                std::swap(a, b);
            lo = (std::max)(lo, a);
            hi = (std::min)(hi, b);
        }
    }
    if(lo > hi)
        return false;
    auto delta = [&](float t){
        return origin[1] + dir[1] * t - v.Sample(origin[0] + dir[0] * t, origin[2] + dir[2] * t);
    };
    step = (std::max)(0.01f, Finite(step, 1));
    float a = lo, fa = delta(a);
    if(std::abs(fa) < 0.0001f){
        hit = a;
        return true;
    }
    while(a < hi){
        float b = (std::min)(a + step, hi), fb = delta(b);
        if((fa >= 0 && fb <= 0) || (fa <= 0 && fb >= 0)){
            for(int i = 0; i < 22; ++i){
                float mid = (a + b) * 0.5f, fm = delta(mid);
                if((fa >= 0 && fm >= 0) || (fa <= 0 && fm <= 0)){
                    a = mid;
                    fa = fm;
                } else
                    b = mid;
            }
            hit = (a + b) * 0.5f;
            return true;
        }
        a = b;
        fa = fb;
    }
    return false;
}

}
}
