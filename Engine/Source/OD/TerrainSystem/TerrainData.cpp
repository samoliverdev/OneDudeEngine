#include "OD/pch.h"
#include "TerrainData.h"
#include <fstream>
#include <cstring>
#include <deque>

namespace OD{

bool TerrainPaintMap::Resize(int columns, int rows){
    if(columns < 2 || rows < 2 || columns > 8192 || rows > 8192)
        return false;
    width = columns;
    height = rows;
    weights.assign(size_t(width) * height * 4, 0);
    for(size_t i = 0; i < weights.size(); i += 4)
        weights[i] = 255;
    return true;
}

bool TerrainPaintMap::LoadFromFile(const std::string& file){
    std::ifstream in(file, std::ios::binary);
    uint32_t header[3]{};
    if(!in.read(reinterpret_cast<char*>(header), sizeof(header)) || header[0] != 0x31504D54u || header[1] < 2 ||
       header[2] < 2 || header[1] > 8192 || header[2] > 8192)
        return false;
    size_t count = size_t(header[1]) * header[2] * 4;
    in.seekg(0, std::ios::end);
    if(in.tellg() != std::streamoff(sizeof(header) + count))
        return false;
    in.seekg(sizeof(header));
    std::vector<uint8_t> bytes(count);
    if(!in.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(count)))
        return false;
    width = int(header[1]);
    height = int(header[2]);
    weights = std::move(bytes);
    path = file;
    return true;
}

bool TerrainPaintMap::Save(const std::string& file, SaveType type){
    if(type == SaveType::SettingOnly || !Valid())
        return false;
    std::ofstream out(file, std::ios::binary);
    uint32_t header[] = {0x31504D54u, uint32_t(width), uint32_t(height)};
    out.write(reinterpret_cast<const char*>(header), sizeof(header));
    out.write(reinterpret_cast<const char*>(weights.data()), std::streamsize(weights.size()));
    if(!out.good())
        return false;
    path = file;
    return true;
}

struct PaintEdit{
    size_t index;
    std::array<uint8_t, 4> before, after;
};

struct TerrainStroke{
    std::vector<TerrainCpu::Edit> heights;
    std::vector<PaintEdit> paint;

    size_t Bytes() const{
        return heights.size() * sizeof(TerrainCpu::Edit) + paint.size() * sizeof(PaintEdit);
    }
};

struct TerrainEditingState{
    std::deque<TerrainStroke> undo, redo;
    TerrainStroke active;
    std::unordered_map<size_t, size_t> heightLookup, paintLookup;
    bool stroking = false, ownHeights = false, ownPaint = false;
    size_t bytes = 0;
};

TerrainDataComponent::TerrainDataComponent(const TerrainDataComponent& other){
    *this = other;
}

TerrainDataComponent& TerrainDataComponent::operator=(const TerrainDataComponent& other){
    if(this == &other)
        return *this;
    heightmap = other.heightmap;
    paintmap = other.paintmap;
    size = other.size;
    historyMiB = other.historyMiB;
    maxBrushSamples = other.maxBrushSamples;
    revision = other.revision;
    paintRevision = other.paintRevision;
    editing.reset();
    heightChanges.clear();
    paintChanges.clear();
    return *this;
}

TerrainCpu::HeightfieldView TerrainDataComponent::View() const{
    if(!heightmap || heightmap->width < 2 || heightmap->height < 2 ||
       heightmap->data.size() != size_t(heightmap->width) * heightmap->height)
        return {};
    return {heightmap->data.data(), heightmap->width, heightmap->height, size.x, size.z, size.y};
}

void TerrainDataComponent::Validate(){
    for(int i = 0; i < 3; ++i)
        size[i] = std::clamp(TerrainCpu::Finite(size[i], i == 1 ? 1000.f : 32000.f), 0.01f, 1000000.f);
    historyMiB = std::clamp(historyMiB, 1, 1024);
    maxBrushSamples = std::clamp(maxBrushSamples, 64, 4194304);
}

bool TerrainDataComponent::Create(int columns, int rows, int paintColumns, int paintRows, float initial){
    if(columns < 2 || rows < 2 || columns > 16385 || rows > 16385 || paintColumns < 2 || paintRows < 2 ||
       paintColumns > 8192 || paintRows > 8192)
        return false;
    auto heights = CreateRef<Heightmap>(columns, rows);
    std::fill(heights->data.begin(), heights->data.end(), std::clamp(TerrainCpu::Finite(initial, 0), 0.f, 1.f));
    auto paint = CreateRef<TerrainPaintMap>();
    if(!paint->Resize(paintColumns, paintRows))
        return false;
    heightmap = std::move(heights);
    paintmap = std::move(paint);
    Validate();
    ClearHistory();
    Touch();
    Touch(true);
    return true;
}

bool TerrainDataComponent::ImportRaw16(const std::string& file, int columns, int rows, bool bigEndian){
    if(columns < 2 || rows < 2 || columns > 16385 || rows > 16385)
        return false;
    std::ifstream in(file, std::ios::binary | std::ios::ate);
    size_t bytes = size_t(columns) * rows * 2;
    if(!in || in.tellg() != std::streamoff(bytes))
        return false;
    in.seekg(0);
    std::vector<uint8_t> raw(bytes);
    if(!in.read(reinterpret_cast<char*>(raw.data()), std::streamsize(bytes)))
        return false;
    auto values = TerrainCpu::DecodeRaw16(raw, columns, rows, bigEndian);
    if(values.empty())
        return false;
    auto heights = CreateRef<Heightmap>();
    heights->width = columns;
    heights->height = rows;
    heights->data = std::move(values);
    heightmap = std::move(heights);
    ClearHistory();
    Touch();
    return true;
}

bool TerrainDataComponent::ExportRaw16(const std::string& file, bool bigEndian) const{
    if(!View().Valid())
        return false;
    auto bytes = TerrainCpu::EncodeRaw16(heightmap->data.data(), heightmap->data.size(), bigEndian);
    std::ofstream out(file, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    return out.good();
}

bool TerrainDataComponent::SaveAssets(const std::string& heightFile, const std::string& paintFile){
    if(!View().Valid() || heightFile.empty() || heightFile[0] == '#')
        return false;
    if(paintmap && paintmap->Valid() && (paintFile.empty() || paintFile[0] == '#'))
        return false;
    for(const auto& file : {heightFile, paintFile}){
        if(file.empty())
            continue;
        auto parent = std::filesystem::path(file).parent_path();
        std::error_code error;
        if(!parent.empty())
            std::filesystem::create_directories(parent, error);
        if(error)
            return false;
    }
    bool ok = heightmap->Save(heightFile, Resource::SaveType::AssetBinary);
    if(paintmap && paintmap->Valid())
        ok = paintmap->Save(paintFile, Resource::SaveType::AssetBinary) && ok;
    return ok;
}

void TerrainDataComponent::Touch(bool paint){
    int columns = paint ? (paintmap ? paintmap->width : 0) : (heightmap ? heightmap->width : 0);
    int rows = paint ? (paintmap ? paintmap->height : 0) : (heightmap ? heightmap->height : 0);
    TouchRegion(paint, 0, 0, columns, rows);
}

void TerrainDataComponent::TouchRegion(bool paint, int x0, int z0, int x1, int z1){
    auto& version = paint ? paintRevision : revision;
    auto& changes = paint ? paintChanges : heightChanges;
    changes.push_back({x0, z0, x1, z1, ++version});
    if(changes.size() > 64)
        changes.erase(changes.begin());
}

TerrainDirtyRegion TerrainDataComponent::ChangesSince(uint64_t from, bool paint) const{
    auto& changes = paint ? paintChanges : heightChanges;
    int columns = paint ? (paintmap ? paintmap->width : 0) : (heightmap ? heightmap->width : 0);
    int rows = paint ? (paintmap ? paintmap->height : 0) : (heightmap ? heightmap->height : 0);
    if(changes.empty() || from + 1 < changes.front().revision)
        return {0, 0, columns, rows, paint ? paintRevision : revision};
    TerrainDirtyRegion r{columns, rows, 0, 0, paint ? paintRevision : revision};
    for(auto& c : changes)
        if(c.revision > from){
            r.x0 = (std::min)(r.x0, c.x0);
            r.z0 = (std::min)(r.z0, c.z0);
            r.x1 = (std::max)(r.x1, c.x1);
            r.z1 = (std::max)(r.z1, c.z1);
        }
    return r;
}

void TerrainDataComponent::MakeEditable(bool paint){
    if(!editing)
        editing = CreateRef<TerrainEditingState>();
    if(!paint && heightmap && heightmap.use_count() > 1){
        auto copy = CreateRef<Heightmap>();
        copy->width = heightmap->width;
        copy->height = heightmap->height;
        copy->data = heightmap->data;
        copy->Path() = heightmap->Path();
        heightmap = std::move(copy);
        editing->ownHeights = true;
    }
    if(paint && paintmap && paintmap.use_count() > 1){
        auto copy = CreateRef<TerrainPaintMap>();
        copy->width = paintmap->width;
        copy->height = paintmap->height;
        copy->weights = paintmap->weights;
        copy->Path() = paintmap->Path();
        paintmap = std::move(copy);
        editing->ownPaint = true;
    }
}

void TerrainDataComponent::BeginStroke(){
    if(!editing)
        editing = CreateRef<TerrainEditingState>();
    if(editing->stroking)
        return;
    editing->stroking = true;
    editing->active = {};
    editing->heightLookup.clear();
    editing->paintLookup.clear();
}

void TerrainDataComponent::EndStroke(){
    if(!editing || !editing->stroking)
        return;
    auto& s = *editing;
    s.stroking = false;
    if(s.active.Bytes()){
        for(auto& stroke : s.redo)
            s.bytes -= stroke.Bytes();
        s.redo.clear();
        s.bytes += s.active.Bytes();
        s.undo.push_back(std::move(s.active));
        while(s.bytes > size_t(historyMiB) * 1024 * 1024 && !s.undo.empty()){
            s.bytes -= s.undo.front().Bytes();
            s.undo.pop_front();
        }
    }
    s.active = {};
    s.heightLookup.clear();
    s.paintLookup.clear();
}

bool TerrainDataComponent::ApplyBrush(const TerrainCpu::Brush& b, int paintLayer){
    Validate();
    auto v = View();
    if(!v.Valid() || !std::isfinite(b.radius) || b.radius <= 0 || !std::isfinite(b.x) || !std::isfinite(b.z) ||
       !std::isfinite(b.amount) || !std::isfinite(b.target) || !std::isfinite(b.hardness))
        return false;
    bool painting = b.mode == TerrainCpu::BrushMode::Paint;
    if(painting && (!paintmap || !paintmap->Valid()))
        return false;
    int columns = painting ? paintmap->width : v.columns, rows = painting ? paintmap->height : v.rows;
    double touchedX = (std::min)(double(columns), std::ceil(2.0 * b.radius / v.width * (columns - 1)) + 3);
    double touchedZ = (std::min)(double(rows), std::ceil(2.0 * b.radius / v.length * (rows - 1)) + 3);
    if(touchedX * touchedZ > maxBrushSamples)
        return false;
    bool single = !editing || !editing->stroking;
    if(single)
        BeginStroke();
    MakeEditable(painting);
    auto& s = *editing;
    bool changed = false;
    size_t limit = size_t(historyMiB) * 1024 * 1024;
    if(!painting){
        auto edits = TerrainCpu::HeightBrush(View(), b);
        for(auto& e : edits){
            auto it = s.heightLookup.find(e.index);
            if(it == s.heightLookup.end()){
                if(s.active.Bytes() + sizeof(e) > limit)
                    break;
                s.heightLookup[e.index] = s.active.heights.size();
                s.active.heights.push_back(e);
            } else
                s.active.heights[it->second].after = e.after;
            heightmap->data[e.index] = e.after;
            changed = true;
        }
    } else{
        int x0 = int(std::clamp((b.x - b.radius) / v.width, 0.f, 1.f) * (columns - 1));
        int z0 = int(std::clamp((b.z - b.radius) / v.length, 0.f, 1.f) * (rows - 1));
        int x1 = int(std::ceil(std::clamp((b.x + b.radius) / v.width, 0.f, 1.f) * (columns - 1)));
        int z1 = int(std::ceil(std::clamp((b.z + b.radius) / v.length, 0.f, 1.f) * (rows - 1)));
        paintLayer = std::clamp(paintLayer, 0, 3);
        for(int z = z0; z <= z1; ++z)
            for(int x = x0; x <= x1; ++x){
                float dx = x * v.width / (columns - 1) - b.x, dz = z * v.length / (rows - 1) - b.z;
                float w = TerrainCpu::BrushWeight(std::sqrt(dx * dx + dz * dz), b.radius, b.hardness) *
                          std::clamp(TerrainCpu::Finite(b.amount, 0), 0.f, 1.f);
                if(w <= 0)
                    continue;
                PaintEdit e{};
                e.index = (size_t(z) * columns + x) * 4;
                std::copy_n(paintmap->weights.data() + e.index, 4, e.before.begin());
                int sum = 0;
                for(int i = 0; i < 4; ++i){
                    e.after[i] = uint8_t(
                        std::clamp(int(std::lround(e.before[i] * (1 - w) + (i == paintLayer ? 255 * w : 0))), 0, 255));
                    sum += e.after[i];
                }
                e.after[paintLayer] = uint8_t(std::clamp(int(e.after[paintLayer]) + 255 - sum, 0, 255));
                if(e.before == e.after)
                    continue;
                auto it = s.paintLookup.find(e.index);
                if(it == s.paintLookup.end()){
                    if(s.active.Bytes() + sizeof(e) > limit)
                        continue;
                    s.paintLookup[e.index] = s.active.paint.size();
                    s.active.paint.push_back(e);
                } else
                    s.active.paint[it->second].after = e.after;
                std::copy(e.after.begin(), e.after.end(), paintmap->weights.data() + e.index);
                changed = true;
            }
    }
    if(changed){
        int x0 = int(std::clamp((b.x - b.radius) / v.width, 0.f, 1.f) * (columns - 1));
        int z0 = int(std::clamp((b.z - b.radius) / v.length, 0.f, 1.f) * (rows - 1));
        int x1 =
            (std::min)(columns, int(std::ceil(std::clamp((b.x + b.radius) / v.width, 0.f, 1.f) * (columns - 1))) + 1);
        int z1 = (std::min)(rows, int(std::ceil(std::clamp((b.z + b.radius) / v.length, 0.f, 1.f) * (rows - 1))) + 1);
        TouchRegion(painting, x0, z0, x1, z1);
    }
    if(single)
        EndStroke();
    return changed;
}

static void ApplyStroke(TerrainDataComponent& data, const TerrainStroke& stroke, bool forward){
    if(data.heightmap && !stroke.heights.empty()){
        TerrainCpu::ApplyEdits(data.heightmap->data.data(), data.heightmap->data.size(), stroke.heights, forward);
        data.Touch();
    }
    if(data.paintmap && !stroke.paint.empty()){
        for(auto& e : stroke.paint)
            if(e.index + 4 <= data.paintmap->weights.size()){
                auto& value = forward ? e.after : e.before;
                std::copy(value.begin(), value.end(), data.paintmap->weights.data() + e.index);
            }
        data.Touch(true);
    }
}

bool TerrainDataComponent::Undo(){
    EndStroke();
    if(!CanUndo())
        return false;
    if(!editing->undo.back().heights.empty())
        MakeEditable(false);
    if(!editing->undo.back().paint.empty())
        MakeEditable(true);
    auto stroke = std::move(editing->undo.back());
    editing->undo.pop_back();
    ApplyStroke(*this, stroke, false);
    editing->redo.push_back(std::move(stroke));
    return true;
}

bool TerrainDataComponent::Redo(){
    EndStroke();
    if(!CanRedo())
        return false;
    if(!editing->redo.back().heights.empty())
        MakeEditable(false);
    if(!editing->redo.back().paint.empty())
        MakeEditable(true);
    auto stroke = std::move(editing->redo.back());
    editing->redo.pop_back();
    ApplyStroke(*this, stroke, true);
    editing->undo.push_back(std::move(stroke));
    return true;
}

bool TerrainDataComponent::CanUndo() const{
    return editing && !editing->undo.empty();
}

bool TerrainDataComponent::CanRedo() const{
    return editing && !editing->redo.empty();
}

void TerrainDataComponent::ClearHistory(){
    editing.reset();
}

}
