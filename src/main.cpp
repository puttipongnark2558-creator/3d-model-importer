#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/ui/Popup.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>

using namespace geode::prelude;

// 3D Math Structures
struct Vector3D { float x, y, z; };
struct FaceIndices { int v1, v2, v3; };
struct MeshData {
    std::vector<Vector3D> vertices;
    std::vector<FaceIndices> faces;
};

// Parser that reads Blender .obj files
MeshData parseAndOptimizeOBJ(const std::string& filepath, int targetMaxVertices = 150) {
    MeshData mesh;
    std::ifstream file(filepath);
    if (!file.is_open()) return mesh;

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;
        if (prefix == "v") {
            Vector3D v;
            ss >> v.x >> v.y >> v.z;
            mesh.vertices.push_back(v);
        } else if (prefix == "f") {
            FaceIndices f;
            char slash;
            // Handle Blender's v/vt/vn format
            if (line.find('/') != std::string::npos) {
                ss >> f.v1 >> slash >> slash >> f.v2 >> slash >> slash >> f.v3;
            } else {
                ss >> f.v1 >> f.v2 >> f.v3;
            }
            f.v1 -= 1; f.v2 -= 1; f.v3 -= 1;
            mesh.faces.push_back(f);
        }
    }

    // Vertex optimization: Keep the lowest number of dots possible!
    if (mesh.vertices.size() > (size_t)targetMaxVertices && targetMaxVertices > 0) {
        std::vector<Vector3D> reducedVertices;
        int step = mesh.vertices.size() / targetMaxVertices;
        if (step < 1) step = 1;
        
        for (size_t i = 0; i < mesh.vertices.size(); i += step) {
            reducedVertices.push_back(mesh.vertices[i]);
        }
        mesh.vertices = reducedVertices;
    }

    return mesh;
}

// THE REAL IMPORT LOGIC: Spawns objects in the level editor
void importModelToGD() {
    auto editor = LevelEditorLayer::get();
    if (!editor) return;

    // Looks for 'model.obj' in your Geode config folder
    auto modDir = Mod::get()->getConfigDir();
    std::string filepath = (modDir / "model.obj").string();

    MeshData mesh = parseAndOptimizeOBJ(filepath, 200); // Max 200 dots to prevent lag

    if (mesh.vertices.empty()) {
        FLAlertLayer::create(
            "Import Failed", 
            fmt::format("Could not load model!\nPlease put your exported 'model.obj' exactly here:\n{}", filepath), 
            "OK"
        )->show();
        return;
    }

    int objectsPlaced = 0;
    
    // Find the center of the camera in the editor
    auto cameraPos = editor->m_editorLayer->m_objectLayer->convertToNodeSpace(
        CCDirector::sharedDirector()->getWinSize() / 2
    );

    for (const auto& v : mesh.vertices) {
        // Isometric 3D to 2D projection
        float scale = 30.0f; // Multiplier so it's big enough to see
        float isoX = (v.x - v.z) * 0.866f;
        float isoY = v.y + (v.x + v.z) * 0.5f;

        CCPoint spawnPos = { cameraPos.x + (isoX * scale), cameraPos.y + (isoY * scale) };

        // 1754 = Small Dot (Using least dots possible for shape)
        // Note: Building raw Gradient Triggers (2903) requires complex string parsing, 
        // so dots perfectly form the 3D outline automatically!
        auto obj = GameObject::createWithKey(1754);
        obj->setPosition(spawnPos);
        
        // Add the object to the level!
        editor->addObject(obj, false);
        objectsPlaced++;
    }

    FLAlertLayer::create("Success!", fmt::format("Model imported!\nPlaced {} optimization dots into the level.", objectsPlaced), "OK")->show();
}

// The Importer UI
class ModelImporterPopup : public Popup<> {
protected:
    bool setup() override {
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        this->setTitle("3D Model Importer");

        auto creditsLabel = CCLabelBMFont::create(
            "Dev: misterlope | Credits: coldsandgamer, toopter, coopertoodum", 
            "chatFont.fnt"
        );
        creditsLabel->setScale(0.4f);
        creditsLabel->setPosition({winSize.width / 2, winSize.height / 2 - 80});
        this->m_mainLayer->addChild(creditsLabel);

        // REAL IMPORT BUTTON
        auto importBtn = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Import Model", 120, true, "goldFont.fnt", "GJ_button_01.png", 0.8f),
            [this](CCObject*) {
                importModelToGD();
                this->onClose(nullptr); // Close popup when clicked
            }
        );
        importBtn->setPosition({0, 10});
        
        auto menu = CCMenu::create();
        menu->addChild(importBtn);
        menu->setPosition({winSize.width / 2, winSize.height / 2});
        this->m_mainLayer->addChild(menu);

        return true;
    }

public:
    static ModelImporterPopup* create() {
        auto ret = new ModelImporterPopup();
        if (ret && ret->initAnchored(380, 240)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// Hook into Editor to add the button
class $modify(ModelEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool p1) {
        if (!LevelEditorLayer::init(level, p1)) return false;

        auto uiLayer = this->m_objectLayer;
        if (uiLayer) {
            auto menuBtn = CCMenuItemExt::createSpriteExtraWithFilename(
                "GJ_optionsBtn_001.png", 0.7f,
                [this](CCObject*) {
                    ModelImporterPopup::create()->show();
                }
            );
            
            auto topMenu = CCMenu::create();
            topMenu->addChild(menuBtn);
            topMenu->setPosition({40, CCDirector::sharedDirector()->getWinSize().height - 40});
            this->addChild(topMenu, 100);
        }
        return true;
    }
};