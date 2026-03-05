#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "core.h"
#include "engine/camera/perspectiveCamera.h"
#include "engine/application.h"
#include "engine/camera/gameCameraControl.h"
#include "engine/texture.h"
#include "engine/asset.h"
#include "engine/pipeline.h"
#include "engine/renderer.h"
#include "engine/shader.h"
#include "engine/textureInfo.h"
#include "tools/modelLoader.h"
#include "tools/matrix.h"
#include "tools/LTC.h"

#define GENERATE_IBL 0

extern "C" {
    __declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#if GENERATE_IBL
int windowHeight = 1024;
int windowWidth = 1024;
#else
int windowHeight = 1080;
int windowWidth  = 1920;
#endif

float lightHeight = 0.5f;
float lightWidth = 0.5f;
float cameraNear = 0.1f;
float cameraFar = 100.0f;
std::unique_ptr<Application> application = std::make_unique<Application>();
std::shared_ptr<PerspectiveCamera> camera = std::make_unique<PerspectiveCamera>(60.0f, float(windowWidth) / float(windowHeight), cameraNear, cameraFar);
std::shared_ptr<GameCameraControl> cameraControl = std::make_shared<GameCameraControl>(camera);
std::shared_ptr<FLight> mainLight = std::make_shared<FLight>(lightWidth, lightHeight);
const float lightRadius = 10.0f;

std::shared_ptr<FScene> scene = std::make_shared<FScene>(windowWidth, windowHeight);
std::shared_ptr<FPipeline> renderPipeline = nullptr;
std::shared_ptr<FModelLoader> modelLoader = std::make_unique<FModelLoader>();

void onResize(int width, int height) {
    windowHeight = height;
    windowWidth = width;
    scene->mWindowHeight = height;
	scene->mWindowWidth = width;
    glViewport(0, 0, width, height);
}

void onKeyboard(int key, int action, int mods) {
    cameraControl->onKeyboard(key, action, mods);
}

void onMouse(int button, int action, int mods) {
    double x = 0.0, y = 0.0;
    application->getMouseCursor(&x, &y);
    cameraControl->onMouse(button, action, x, y);
}

void onCursor(double xpos, double ypos) {
    cameraControl->onCursor(xpos, ypos);
}

void onScroll(double yoffset) {
    cameraControl->onScroll(yoffset);
}

#if GENERATE_IBL
void prepare() {
    std::vector<std::string> path = {};
    //path.push_back("./assets/textures/IBL/seaside/origin/sea_0_RT.jpg");
    //path.push_back("./assets/textures/IBL/seaside/origin/sea_1_LF.jpg");
    //path.push_back("./assets/textures/IBL/seaside/origin/sea_2_UP.jpg");
    //path.push_back("./assets/textures/IBL/seaside/origin/sea_3_DN.jpg");
    //path.push_back("./assets/textures/IBL/seaside/origin/sea_4_BK.jpg");
    //path.push_back("./assets/textures/IBL/seaside/origin/sea_5_FR.jpg");
    path.push_back("./assets/textures/IBL/cubemap_test/origin/px.hdr");
    path.push_back("./assets/textures/IBL/cubemap_test/origin/nx.hdr");
    path.push_back("./assets/textures/IBL/cubemap_test/origin/py.hdr");
    path.push_back("./assets/textures/IBL/cubemap_test/origin/ny.hdr");
    path.push_back("./assets/textures/IBL/cubemap_test/origin/pz.hdr");
    path.push_back("./assets/textures/IBL/cubemap_test/origin/nz.hdr");
    //path.push_back("./assets/textures/IBL/cubemap_test/origin/px_tonemap.png");
    //path.push_back("./assets/textures/IBL/cubemap_test/origin/nx_tonemap.png");
    //path.push_back("./assets/textures/IBL/cubemap_test/origin/py_tonemap.png");
    //path.push_back("./assets/textures/IBL/cubemap_test/origin/ny_tonemap.png");
    //path.push_back("./assets/textures/IBL/cubemap_test/origin/pz_tonemap.png");
    //path.push_back("./assets/textures/IBL/cubemap_test/origin/nz_tonemap.png");
    std::string outPath = "./assets/textures/IBL/cubemap_test/lut/";
    buildIBLGeneratePipeline(renderPipeline, scene, path, outPath, true);
}

#else
void prepare() {
	scene->mMainLight = mainLight;
    mainLight->mPosition = glm::vec4(-0.80f, 0.65f, 1.8f, 1.0f);
	mainLight->mDirection = glm::vec4(glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f)), 1.0f);
    mainLight->mRadius = lightRadius;
    mainLight->mIntensity = 15.0f;
    std::shared_ptr<FUniformBuffer> cameraUBO = std::make_shared<FUniformBuffer>("camera");
    cameraUBO->initBuffer(sizeof(FCameraInfo), 0);
    std::shared_ptr<FUniformBuffer> lightUBO = std::make_shared<FUniformBuffer>("RectLight");
    lightUBO->initBuffer(sizeof(FLight), 1);
	scene->mCameraUBO = cameraUBO;
	scene->mLightUBO = lightUBO;
    scene->historyCameraView = camera->getViewMatrix();
    scene->mCameraFar = cameraFar;
    scene->mCameraNear = cameraNear;
    scene->mProbeCenter = glm::vec3(0.0f, 0.0f, 3.0f);
    scene->mProbeSize = glm::vec3(4.0f, 3.0f, 5.0f);

    std::vector<float> lvVBO;
    std::vector<uint32_t> lvEBO;
    std::shared_ptr<FVertexBufferDesc> lvDesc = std::make_shared<FVertexBufferDesc>();
	std::shared_ptr<FMesh> lightVolumeMesh = std::make_shared<FMesh>();
    GenerateUVSphereBufferData(
        mainLight->mPosition, lightRadius, 32, 16,
        true,
		lvVBO, lvEBO, lvDesc, true
    );
    lightVolumeMesh->uploadCompleteData(std::move(lvVBO), std::move(lvEBO), lvDesc);
	scene->mMainLightVolume = lightVolumeMesh;

	std::shared_ptr<FAsset> house = nullptr;
    modelLoader->importModel(house, "./assets/models/house/house.glb", false);
    house->setModelMatrix(
        createSimpleModelMatrix(
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.01f, 0.01f, 0.01f)
        )
    );
    house->mShadow = true;
	scene->mAsset.push_back(house);

    std::shared_ptr<FAsset> metal = nullptr;
    modelLoader->importModel(metal, "./assets/models/combine/girlFix.glb", false);
    metal->setModelMatrix(
        createSimpleModelMatrix(
            glm::vec3(-1.25f, -1.725f, 3.6f),
            glm::vec3(0.0f, 180.0f, 0.0f),
            glm::vec3(1.1f, 1.1f, 1.1f)
        )
    );
    metal->mShadow = true;
    scene->mAsset.push_back(metal);

	scene->mCamera = camera;

    std::shared_ptr<FTexture> LTCMat = std::make_shared<FTexture>();
    std::shared_ptr<FTexture> LTCAmp = std::make_shared<FTexture>();
    std::shared_ptr<FTextureDesc> LTCDesc = std::make_shared<FTextureDesc>(
        GetGLFormat(TextureFormat::RGBA32F), LTC_Size, LTC_Size, false
    );
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	LTCMat->applyResource(LTCDesc);
	LTCMat->allocateResourceTemplate(const_cast<float*>(LTC_Mat), true);
	LTCAmp->applyResource(LTCDesc);
	LTCAmp->allocateResourceTemplate(const_cast<float*>(LTC_Amp), true);
#else
    LTCMat->allocateConstTextureResource<float>(0, LTCDesc, const_cast<float*>(LTC_Mat));
    LTCAmp->allocateConstTextureResource<float>(0, LTCDesc, const_cast<float*>(LTC_Amp));
#endif
    scene->mLUT["LTCMat"] = LTCMat;
    scene->mLUT["LTCAmp"] = LTCAmp;

    scene->makeRTAsset();
	scene->mBVHBuilder = std::make_unique<BinnedBvhBuilder>();
	scene->mSceneBVH = scene->mBVHBuilder->build_bvh(scene->mRTAsset->mSceneTriangleInfo);
	scene->makeSceneBVH();
    buildDebugPipeline(renderPipeline, scene);
}
#endif

void render() {
    renderPipeline->update(scene);
	renderPipeline->render();
    scene->historyCameraView = camera->getViewMatrix();
}

int main() {
    stbi_set_flip_vertically_on_load(true);
    if (!application->init(windowWidth, windowHeight)) {
        return -1;
    }
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.2f, 0.3f, 0.1f, 1.0f);

    application->setResizeCallback(onResize);
    application->setKeyboardCallback(onKeyboard);
    application->setMouseCallback(onMouse);
    application->setCursorCallback(onCursor);
    application->setScrollCallback(onScroll);
    if (!GLAD_GL_ARB_bindless_texture)
    {
        std::cerr << "GL_ARB_bindless_texture not supported" << std::endl;
    }
    prepare();
#if GENERATE_IBL
    render();
#else
    while (application->update()) {
        cameraControl->update();
        render();
        static float lastTime = 0.0f;
        static int frameCount = 0;
        float currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastTime >= 1.0f) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = currentTime;
        }
    }
#endif
    application->destroy();
    return 0;
}
