#include "OD/pch.h"
#include "TerrainEditor.h"
#include "OD/Core/ImGui.h"
#include <cstdio>

namespace OD{
namespace{
struct TerrainUi{
    Scene* scene = nullptr;
    Entity entity = EntityNull;

    std::string sceneFile;
    std::string defaultHeight;
    std::string defaultPaint;

    int dimensions[2] = {1025, 1025};
    int paintDimensions[2] = {1024, 1024};
    float initialHeight = 0;
    bool bigEndian = false;
    char rawFile[1024]{};
    char heightFile[1024]{};
    char paintFile[1024]{};
    std::string status;
};

TerrainUi& Ui(Scene& scene, Entity e){
    static TerrainUi state;

    if(state.scene != &scene || state.entity != e){
        state = {};
        state.scene = &scene;
        state.entity = e;

        std::string base = scene.PathIsValid() ? scene.Path() : "Terrain";
        base += "_TerrainSystem_" + std::to_string(uint32_t(e));

        state.sceneFile = scene.Path();
        state.defaultHeight = base + ".heightmap";
        state.defaultPaint = base + ".terrainpaint";

        std::snprintf(state.heightFile, sizeof(state.heightFile), "%s.heightmap", base.c_str());
        std::snprintf(state.paintFile, sizeof(state.paintFile), "%s.terrainpaint", base.c_str());

        auto* data = scene.GetRegistry().try_get<TerrainDataComponent>(e);
        if(data && data->heightmap && data->heightmap->PathIsValid())
            std::snprintf(state.heightFile, sizeof(state.heightFile), "%s", data->heightmap->Path().c_str());
        if(data && data->paintmap && data->paintmap->PathIsValid())
            std::snprintf(state.paintFile, sizeof(state.paintFile), "%s", data->paintmap->Path().c_str());
    }

    if(state.sceneFile != scene.Path()){
        std::string base =
            (scene.PathIsValid() ? scene.Path() : "Terrain") + "_TerrainSystem_" + std::to_string(uint32_t(e));
        if(state.defaultHeight == state.heightFile)
            std::snprintf(state.heightFile, sizeof(state.heightFile), "%s.heightmap", base.c_str());
        if(state.defaultPaint == state.paintFile)
            std::snprintf(state.paintFile, sizeof(state.paintFile), "%s.terrainpaint", base.c_str());
        state.sceneFile = scene.Path();
        state.defaultHeight = base + ".heightmap";
        state.defaultPaint = base + ".terrainpaint";
    }

    return state;
}

void RequireData(Scene& scene, Entity e){
    if(!scene.HasComponent<TerrainDataComponent>(e))
        ImGui::TextWrapped("Add TerrainDataComponent to this entity to provide terrain data.");
}
}

void TerrainDataComponent::OnGui(Entity e, Scene& scene){
    auto& data = scene.GetComponent<TerrainDataComponent>(e);
    auto& ui = Ui(scene, e);
    bool assigned = ImGui::DrawAsset<Heightmap>("Heightmap", data.heightmap);
    assigned |= ImGui::DrawAsset<TerrainPaintMap>("Paint weights", data.paintmap);
    if(assigned){
        data.ClearHistory();
        data.Touch();
        data.Touch(true);
    }
    if(ImGui::DragFloat3("World size X / height / Z", &data.size.x, 10, 0.01f, 1000000.f))
        data.Validate();
    if(data.View().Valid()){
        ImGui::Text("Height samples: %d x %d", data.heightmap->width, data.heightmap->height);
        ImGui::Text("Sample spacing: %.3f x %.3f m",
                    data.size.x / (data.heightmap->width - 1),
                    data.size.z / (data.heightmap->height - 1));
        size_t bytes =
            data.heightmap->data.size() * sizeof(float) + (data.paintmap ? data.paintmap->weights.size() : 0);
        ImGui::Text("Resident source data: %.1f MiB", double(bytes) / (1024 * 1024));
    } else
        ImGui::TextWrapped("Assign a heightmap or create/import terrain data below.");
    if(ImGui::Button("Add terrain components")){
        scene.AddOrGetComponent<TerrainRendererComponent>(e);
        scene.AddOrGetComponent<TerrainCollisionComponent>(e);
        scene.AddOrGetComponent<TerrainNavmeshDataComponent>(e);
        scene.AddOrGetComponent<TerrainEditorComponent>(e);
    }
    if(ImGui::CollapsingHeader("Create / replace resident data")){
        ImGui::InputInt2("Heightmap columns / rows", ui.dimensions);
        ImGui::InputInt2("Paintmap columns / rows", ui.paintDimensions);
        ImGui::SliderFloat("Initial normalized height", &ui.initialHeight, 0, 1);
        if(ImGui::Button("32 km / 8193 samples preset")){
            ui.dimensions[0] = ui.dimensions[1] = 8193;
            data.size = Vector3(32000, 1000, 32000);
        }
        double count = double((std::max)(2, ui.dimensions[0])) * (std::max)(2, ui.dimensions[1]);
        ImGui::Text("Height data allocation: %.1f MiB", count * 4 / (1024 * 1024));
        ImGui::TextWrapped("Create replaces this entity's data. Save assets explicitly before saving the scene.");
        if(ImGui::Button("Create height and paint data")){
            bool ok = data.Create(
                ui.dimensions[0], ui.dimensions[1], ui.paintDimensions[0], ui.paintDimensions[1], ui.initialHeight);
            ui.status = ok ? "Created resident terrain data." : "Invalid resolution (height 2..16385; paint 2..8192).";
        }
        if(ImGui::Button("Create / replace paint data only")){
            auto paint = CreateRef<TerrainPaintMap>();
            if(paint->Resize(ui.paintDimensions[0], ui.paintDimensions[1])){
                data.paintmap = paint;
                data.ClearHistory();
                data.Touch(true);
                ui.status = "Created paint weights.";
            } else
                ui.status = "Invalid paint resolution.";
        }
    }
    if(ImGui::CollapsingHeader("Import / export RAW16")){
        ImGui::InputText("RAW16 file", ui.rawFile, sizeof(ui.rawFile));
        ImGui::InputInt2("Import columns / rows", ui.dimensions);
        ImGui::Checkbox("Big endian", &ui.bigEndian);
        ImGui::TextWrapped(
            "Row-major unsigned 16-bit heights, first row at local Z=0. File length must match exactly.");
        if(ImGui::Button("Import height RAW16"))
            ui.status = data.ImportRaw16(ui.rawFile, ui.dimensions[0], ui.dimensions[1], ui.bigEndian)
                            ? "Imported height data."
                            : "Import failed: check path, dimensions and byte count.";
        ImGui::SameLine();
        if(ImGui::Button("Export height RAW16"))
            ui.status = data.ExportRaw16(ui.rawFile, ui.bigEndian) ? "Exported height data." : "Export failed.";
    }
    if(ImGui::CollapsingHeader("Save terrain assets", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::InputText("Height asset file", ui.heightFile, sizeof(ui.heightFile));
        ImGui::InputText("Paint asset file", ui.paintFile, sizeof(ui.paintFile));
        if(ImGui::Button("Save terrain assets + scene")){
            if(data.SaveAssets(ui.heightFile, ui.paintFile)){
                if(scene.PathIsValid() && !scene.Running()){
                    scene.Save(scene.Path().c_str(), EntityNull);
                    ui.status = "Saved terrain assets and scene references: " + data.heightmap->Path();
                } else
                    ui.status = "Saved assets. Save the edit scene to persist their references.";
            } else
                ui.status = "Save failed: use real file paths for the height and paint assets.";
        }
    }
    ImGui::DragInt("Undo memory budget (MiB)", &data.historyMiB, 1, 1, 1024);
    ImGui::DragInt("Maximum samples per brush stamp", &data.maxBrushSamples, 1024, 64, 4194304);
    data.Validate();
    if(ImGui::Button("Undo terrain edit"))
        data.Undo();
    ImGui::SameLine();
    if(ImGui::Button("Redo terrain edit"))
        data.Redo();
    if(ImGui::Button("Notify external height edit")){
        data.ClearHistory();
        data.Touch();
    }
    ImGui::SameLine();
    if(ImGui::Button("Notify external paint edit")){
        data.ClearHistory();
        data.Touch(true);
    }
    if(!ui.status.empty())
        ImGui::TextWrapped("%s", ui.status.c_str());
}

void TerrainRendererComponent::OnGui(Entity e, Scene& scene){
    auto& r = scene.GetComponent<TerrainRendererComponent>(e);
    RequireData(scene, e);
    ImGui::Checkbox("Render terrain", &r.enabled);
    ImGui::Checkbox("GPU instancing", &r.useInstancing);
    ImGui::Checkbox("Cast shadows", &r.castShadows);
    const int cells[] = {2, 4, 8, 16, 32, 64, 128};
    int selected = 0;
    for(int i = 0; i < 7; ++i)
        if(cells[i] == r.patchCells)
            selected = i;
    if(ImGui::Combo("Patch cells per edge",
                    &selected,
                    "2\0"
                    "4\0"
                    "8\0"
                    "16\0"
                    "32\0"
                    "64\0"
                    "128\0"))
        r.patchCells = cells[selected];
    ImGui::DragInt("Maximum quadtree depth", &r.maxDepth, 1, 0, 16);
    ImGui::DragFloat("Minimum patch edge (m)", &r.minPatchSize, 1, 1, 1000000);
    ImGui::DragFloat("Projected vertex spacing (pixels)", &r.pixelError, 0.25f, 0.25f, 256);
    ImGui::TextWrapped(
        "Lower pixel spacing increases detail. Patch budget always preserves full terrain coverage and balanced edges.");
    ImGui::DragInt("Maximum selected patches", &r.maxPatches, 16, 1, 65536);
    ImGui::DragFloat("Maximum draw distance (m)", &r.maxDrawDistance, 100, 1, 10000000);
    ImGui::Checkbox("32-bit GPU height precision", &r.heightTexture32Bit);
    ImGui::DragFloat("Edit texture update interval (s)", &r.textureUpdateInterval, 0.01f, 0, 10);
    ImGui::Text("Last camera: %d selected / %d submitted", r.selectedPatches, r.submittedPatches);
    for(int i = 0; i < 4; ++i){
        ImGui::PushID(i);
        if(ImGui::TreeNode(("Surface layer " + std::to_string(i)).c_str())){
            auto& l = r.layers[i];
            ImGui::DrawAsset<Texture2D>("Albedo", l.albedo);
            ImGui::DrawAsset<Texture2D>("Normal map", l.normal);
            ImGui::DragFloat2("Texture repeat size (m)", &l.tileSize.x, 0.1f, 0.01f, 100000);
            ImGui::ColorEdit4("Tint", &l.tint.x);
            ImGui::SliderFloat("Metallic", &l.metallic, 0, 1);
            ImGui::SliderFloat("Roughness", &l.roughness, 0.025f, 1);
            ImGui::SliderFloat("Normal strength", &l.normalStrength, 0, 10);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    r.Validate();
}

void TerrainCollisionComponent::OnGui(Entity e, Scene& scene){
    auto& c = scene.GetComponent<TerrainCollisionComponent>(e);
    RequireData(scene, e);
    ImGui::Checkbox("Terrain collision", &c.enabled);
    ImGui::Checkbox("Rebuild edited regions automatically", &c.autoRebuild);
    ImGui::Checkbox("Prepare all collision before simulation", &c.buildAllOnStart);
    ImGui::DragInt("Collision tiles X", &c.tilesX, 1, 1, 128);
    ImGui::DragInt("Collision tiles Z", &c.tilesZ, 1, 1, 128);
    ImGui::DragInt("Samples per tile edge", &c.samplesPerTile, 1, 9, 1025);
    ImGui::TextWrapped(
        "Power-of-two cells plus one endpoint: 257 samples = 256 cells. Coarser collision sampling or render LOD can still differ on curved terrain.");
    ImGui::DragInt("Tile rebuilds per frame", &c.rebuildsPerFrame, 1, 1, 64);
    c.Validate();
    ImGui::Text("Resident collision samples: %.1f MiB",
                double(c.tilesX) * c.tilesZ * c.samplesPerTile * c.samplesPerTile * 4 / (1024 * 1024));
    if(auto* d = scene.GetRegistry().try_get<TerrainDataComponent>(e))
        ImGui::Text("Collision spacing: %.2f x %.2f m",
                    d->size.x / c.tilesX / (c.samplesPerTile - 1),
                    d->size.z / c.tilesZ / (c.samplesPerTile - 1));
    ImGui::TextWrapped(
        "All collision tiles are resident during play. Initial collision is prepared before simulation by default. Disabling that option spreads creation over frames; actors can fall through tiles that are not prepared yet.");
    if(ImGui::Button("Rebuild terrain collision"))
        ++c.rebuild;
}

void TerrainNavmeshDataComponent::OnGui(Entity e, Scene& scene){
    auto& n = scene.GetComponent<TerrainNavmeshDataComponent>(e);
    RequireData(scene, e);
    ImGui::Checkbox("Include terrain in navigation bakes", &n.enabled);
    ImGui::DragFloat("Navigation source spacing (m)", &n.sampleSpacing, 0.5f, 0.1f, 10000);
    ImGui::DragInt("Maximum cells per bake region axis", &n.maxCellsPerAxis, 8, 4, 1024);
    n.Validate();
    ImGui::TextWrapped(
        "Use the existing NavmeshComponent to configure agents, bake bounds, tile size and save the navigation asset. Terrain geometry is sampled directly inside each bake region, independent of rendering and collision.");
}

void TerrainEditorComponent::OnGui(Entity e, Scene& scene){
    auto& b = scene.GetComponent<TerrainEditorComponent>(e);
    RequireData(scene, e);
    ImGui::Checkbox("Edit selected terrain in viewport", &b.edit);
    int tool = int(b.tool);
    if(ImGui::Combo("Tool", &tool, "Raise\0Lower\0Flatten\0Smooth\0Paint layers\0"))
        b.tool = TerrainCpu::BrushMode(tool);
    ImGui::DragFloat("Brush radius (terrain metres)", &b.radius, 1, 0.1f, 100000);
    ImGui::DragFloat("Sculpt rate (m/s)", &b.strength, 1, 0, 10000);
    ImGui::SliderFloat("Brush hardness", &b.hardness, 0, 0.99f);
    ImGui::DragFloat("Flatten height (terrain metres)", &b.flattenHeight, 1, 0, 1000000);
    ImGui::DragFloat("Smooth / paint blend rate (/s)", &b.paintStrength, 0.1f, 0, 100);
    ImGui::SliderInt("Paint layer", &b.layer, 0, 3);
    ImGui::ColorEdit4("Brush outline", &b.brushColor.x);
    ImGui::DragFloat("Pick maximum distance (m)", &b.rayDistance, 100, 1, 10000000);
    ImGui::DragFloat("Pick march step (m)", &b.rayStep, 0.1f, 0.01f, 1000);
    b.Validate();
    ImGui::TextWrapped(
        "Select this entity, enable editing, then drag left mouse on the terrain. Shift reverses raise/lower. Ctrl+Z / Ctrl+Y undo/redo one stroke. Brush changes are isolated from shared source assets until explicitly saved.");
    if(auto* d = scene.GetRegistry().try_get<TerrainDataComponent>(e)){
        if(ImGui::Button("Undo stroke"))
            d->Undo();
        ImGui::SameLine();
        if(ImGui::Button("Redo stroke"))
            d->Redo();
    }
}

bool TerrainEditor::IsEditing(Scene& scene, Entity selected){
    if(scene.Running() || !scene.IsValid(selected))
        return false;
    auto* tool = scene.GetRegistry().try_get<TerrainEditorComponent>(selected);
    auto* data = scene.GetRegistry().try_get<TerrainDataComponent>(selected);
    return tool && tool->edit && data && data->View().Valid();
}

void TerrainEditor::DrawViewport(
    Scene& scene, Entity selected, const Camera& camera, Vector2 min, Vector2 max, bool flipY){
    struct StrokeState{
        Scene* scene = nullptr;
        Entity entity = EntityNull;
        bool active = false;
    };

    static StrokeState stroke;
    bool editing = IsEditing(scene, selected);
    bool held = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if(stroke.active && (!held || stroke.scene != &scene || stroke.entity != selected || !editing)){
        if(stroke.scene == &scene)
            if(auto* data = scene.GetRegistry().try_get<TerrainDataComponent>(stroke.entity))
                data->EndStroke();
        stroke = {};
    }
    if(!editing || max.x <= min.x || max.y <= min.y)
        return;
    auto& io = ImGui::GetIO();
    auto& tool = scene.GetComponent<TerrainEditorComponent>(selected);
    auto& data = scene.GetComponent<TerrainDataComponent>(selected);
    tool.Validate();
    Vector2 mouse(io.MousePos.x, io.MousePos.y);
    bool hovered = mouse.x >= min.x && mouse.y >= min.y && mouse.x <= max.x && mouse.y <= max.y &&
                   ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    if(!hovered)
        return;
    if(io.KeyAlt || ImGui::IsMouseDown(ImGuiMouseButton_Right))
        return;
    Vector2 uv = (mouse - min) / (max - min);
    // Image UV flip maps top of the displayed image to camera NDC +Y.
    float ndcY = flipY ? 1 - 2 * uv.y : 2 * uv.y - 1;
    Matrix4 invCamera = math::inverse(camera.projection * camera.view);
    Vector4 near = invCamera * Vector4(uv.x * 2 - 1, ndcY, -1, 1), far = invCamera * Vector4(uv.x * 2 - 1, ndcY, 1, 1);
    if(std::abs(near.w) < 1e-8f || std::abs(far.w) < 1e-8f)
        return;
    Vector3 origin = Vector3(near) / near.w, direction = math::normalize(Vector3(far) / far.w - origin);
    auto& transform = scene.GetComponent<TransformComponent>(selected);
    Matrix4 root = transform.GlobalModelMatrix();
    if(std::abs(math::determinant(root)) < 1e-10f)
        return;
    Matrix4 invRoot = math::inverse(root);
    Vector3 localOrigin = Vector3(invRoot * Vector4(origin, 1)),
            localDirection = Vector3(invRoot * Vector4(direction, 0));
    auto field = data.View();
    float hit = 0;
    float step = tool.rayStep;
    if(std::abs(localDirection.x) > 1e-6f)
        step = (std::min)(step, field.width / (field.columns - 1) / std::abs(localDirection.x) * 0.5f);
    if(std::abs(localDirection.z) > 1e-6f)
        step = (std::min)(step, field.length / (field.rows - 1) / std::abs(localDirection.z) * 0.5f);
    if(!TerrainCpu::Raycast(field,
                            {localOrigin.x, localOrigin.y, localOrigin.z},
                            {localDirection.x, localDirection.y, localDirection.z},
                            tool.rayDistance,
                            step,
                            hit))
        return;
    Vector3 point = localOrigin + localDirection * hit;
    auto project = [&](Vector3 local, ImVec2& result){
        Vector4 clip = camera.projection * camera.view * root * Vector4(local, 1);
        if(clip.w <= 0.0001f)
            return false;
        Vector3 p = Vector3(clip) / clip.w;
        result = {min.x + (p.x * 0.5f + 0.5f) * (max.x - min.x),
                  min.y + (flipY ? (0.5f - p.y * 0.5f) : (p.y * 0.5f + 0.5f)) * (max.y - min.y)};
        return true;
    };
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect({min.x, min.y}, {max.x, max.y}, true);
    ImU32 color =
        ImGui::ColorConvertFloat4ToU32({tool.brushColor.x, tool.brushColor.y, tool.brushColor.z, tool.brushColor.w});
    for(int i = 0; i < 64; ++i){
        float a = float(i) * 6.2831853f / 64, b = float(i + 1) * 6.2831853f / 64;
        Vector3 p(point.x + std::cos(a) * tool.radius, 0, point.z + std::sin(a) * tool.radius);
        Vector3 q(point.x + std::cos(b) * tool.radius, 0, point.z + std::sin(b) * tool.radius);
        p.y = field.Sample(p.x, p.z) + 0.2f;
        q.y = field.Sample(q.x, q.z) + 0.2f;
        ImVec2 sp, sq;
        if(project(p, sp) && project(q, sq))
            draw->AddLine(sp, sq, color, 2);
    }
    draw->PopClipRect();
    if(held && !io.KeyCtrl){
        if(!stroke.active){
            if(!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                return;
            data.BeginStroke();
            stroke = {&scene, selected, true};
        }
        TerrainCpu::Brush brush;
        brush.mode = tool.tool;
        brush.radius = tool.radius;
        brush.hardness = tool.hardness;
        brush.x = point.x;
        brush.z = point.z;
        brush.target = tool.flattenHeight;
        brush.amount = (brush.mode == TerrainCpu::BrushMode::Raise || brush.mode == TerrainCpu::BrushMode::Lower
                            ? tool.strength
                            : tool.paintStrength) *
                       (std::min)(io.DeltaTime, 0.1f);
        if(io.KeyShift){
            if(brush.mode == TerrainCpu::BrushMode::Raise)
                brush.mode = TerrainCpu::BrushMode::Lower;
            else if(brush.mode == TerrainCpu::BrushMode::Lower)
                brush.mode = TerrainCpu::BrushMode::Raise;
        }
        data.ApplyBrush(brush, tool.layer);
    }
}
}
