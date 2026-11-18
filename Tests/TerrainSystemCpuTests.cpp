// Standalone acceptance tests: no graphics context or engine startup required.
#include "OD/TerrainSystem/TerrainAlgorithms.h"
#include <iostream>
#include <stdexcept>
#include <set>

using namespace OD::TerrainCpu;
static int checks = 0;

static void Check(bool ok, const char* message){
    ++checks;
    if(!ok)
        throw std::runtime_error(message);
}

static bool Near(float a, float b){
    return std::abs(a - b) < 0.0001f;
}

int main(){
    try{
        float heights[] = {0, 0.5f, 1, 0.25f, 0.5f, 0.75f};
        HeightfieldView view{heights, 3, 2, 32, 12, 100};
        Check(view.Valid(), "rectangular view valid");
        Check(Near(view.Sample(0, 0), 0), "origin");
        Check(Near(view.Sample(32, 12), 75), "last sample, no off by one");
        Check(Near(view.Sample(8, 6), 31.25f), "bilinear rectangular sampling");
        Check(Near(view.Sample(-100, -10), 0), "clamped edges");
        Check(!HeightfieldView{}.Valid(), "missing data rejected");
        Check(Near(HeightfieldView{}.Sample(0, 0), 0), "missing sample safe");
        Check(NormalizeCells(31) == 32 && NormalizeCells(0) == 2, "cell settings normalize");

        for(int mask = 0; mask < 16; ++mask){
            auto idx = GridIndices(8, mask);
            Check(!idx.empty() && idx.size() % 3 == 0, "triangles exist");
            double area = 0;
            for(size_t i = 0; i < idx.size(); i += 3){
                int a = idx[i], b = idx[i + 1], c = idx[i + 2];
                Check(a < 81 && b < 81 && c < 81, "index bounds");
                int ax = a % 9, az = a / 9, bx = b % 9, bz = b / 9, cx = c % 9, cz = c / 9;
                int twice = (bz - az) * (cx - ax) - (bx - ax) * (cz - az);
                Check(twice > 0, "upward winding and no degenerate triangles");
                area += twice * 0.5;
                for(int v : {a, b, c}){
                    int x = v % 9, z = v / 9;
                    Check(!(mask & 1) || x != 0 || z % 2 == 0, "left stitch even");
                    Check(!(mask & 2) || x != 8 || z % 2 == 0, "right stitch even");
                    Check(!(mask & 4) || z != 0 || x % 2 == 0, "near stitch even");
                    Check(!(mask & 8) || z != 8 || x % 2 == 0, "far stitch even");
                }
            }
            Check(Near(float(area), 64), "stitched grid covers exact area");
        }
        for(int budget : {1, 4, 16, 64, 256, 1024}){
            QuadSettings s;
            s.width = 32000;
            s.length = 18000;
            s.height = 500;
            s.eyeX = 1700;
            s.eyeZ = 11000;
            s.eyeY = 100;
            s.maxPatches = budget;
            s.minPatchSize = 64;
            s.pixelError = 2;
            s.projectionScale = 1000;
            auto leaves = SelectPatches(s);
            Check(!leaves.empty() && int(leaves.size()) <= budget, "quadtree budget");
            double coverage = 0;
            for(auto a : leaves){
                double span = std::ldexp(1., -a.depth);
                coverage += span * span;
                Check(a.depth <= s.maxDepth, "depth bounded");
                for(auto b : leaves){
                    if(Neighbors(a, b))
                        Check(std::abs(a.depth - b.depth) <= 1, "2:1 balanced");
                }
            }
            Check(std::abs(coverage - 1) < 1e-9, "no dropped world coverage at budget");
        }
        QuadSettings bad;
        bad.width = 0;
        Check(SelectPatches(bad).empty(), "zero dimensions rejected");
        bad.width = std::numeric_limits<float>::quiet_NaN();
        Check(SelectPatches(bad).empty(), "NaN dimensions rejected");

        float flat[81]{};
        HeightfieldView edit{flat, 9, 9, 8, 8, 100};
        Brush brush;
        brush.x = 4;
        brush.z = 4;
        brush.radius = 2;
        brush.amount = 10;
        auto edits = HeightBrush(edit, brush);
        Check(!edits.empty(), "brush produces edits");
        ApplyEdits(flat, 81, edits, true);
        Check(Near(flat[40], 0.1f), "brush strength in metres");
        Check(Near(flat[0], 0), "brush region bounded");
        ApplyEdits(flat, 81, edits, false);
        Check(Near(flat[40], 0), "undo exact");
        ApplyEdits(flat, 81, edits, true);
        brush.mode = BrushMode::Flatten;
        brush.target = 25;
        brush.amount = 1;
        ApplyEdits(flat, 81, HeightBrush(edit, brush), true);
        Check(Near(flat[40], 0.25f), "flatten target metres");
        brush.mode = BrushMode::Smooth;
        brush.amount = 0.5f;
        auto smooth = HeightBrush(edit, brush);
        ApplyEdits(flat, 81, smooth, true);
        Check(flat[40] < 0.25f, "smooth reads original neighborhood");
        brush.radius = -1;
        Check(HeightBrush(edit, brush).empty(), "invalid brush safe");
        brush.radius = 2;
        brush.hardness = std::numeric_limits<float>::quiet_NaN();
        Check(HeightBrush(edit, brush).empty(), "nonfinite brush falloff rejected");

        auto bytes = EncodeRaw16(heights, 6, false);
        auto decoded = DecodeRaw16(bytes, 3, 2, false);
        Check(decoded.size() == 6 && std::abs(decoded[1] - 0.5f) < 0.00002f, "raw16 roundtrip");
        Check(DecodeRaw16(bytes, 4, 4, false).empty(), "raw byte count validated");
        auto big = EncodeRaw16(heights, 6, true);
        Check(DecodeRaw16(big, 3, 2, true) == decoded, "endian support");
        float t = 0;
        Check(Raycast(edit, {4, 150, 4}, {0, -1, 0}, 1000, 1, t), "vertical pick");
        Check(t > 120 && t < 150, "pick height");
        Check(!Raycast(edit, {-5, 100, -5}, {0, -1, 0}, 1000, 1, t), "outside ray rejected");
        std::cout << "Terrain CPU: " << checks << " checks passed\n";
    } catch(const std::exception& e){
        std::cerr << e.what() << '\n';
        return 1;
    }
}
