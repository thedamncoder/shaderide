/**
 * OpenGLWidget Class
 *
 * --------------------------------------------------------------------------
 * This file is part of "Shader IDE" -> https://github.com/aspicat/shaderide.
 * --------------------------------------------------------------------------
 *
 * Copyright (c) 2019 Aspicat - Florian Roth
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <QDebug>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <glm/gtc/type_ptr.hpp>
#include "OpenGLWidget.hpp"
#include "src/Core/SyntaxErrorException.hpp"
#include "src/Core/Memory.hpp"
#include "src/GL/GLDefaults.hpp"
#include "src/GL/GLUtility.hpp"
#include "src/GL/Loaders/OBJMeshLoader.hpp"
#include "src/GUI/Style/OpenGLWidgetStyle.hpp"

using namespace ShaderIDE::GUI;

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget             (parent),
      program                   (0),
      vao                       (0),
      planeVAO                  (0),
      vertexBuffer              (0),
      planeVertexBuffer         (0),
      selectedMeshName          (""),
      overlayLayout             (nullptr),
      topLeftLayout             (nullptr),
      cbRealtimeUpdate          (nullptr),
      cbPlane2D                 (nullptr),
      quickLoadModelsLayout     (nullptr),
      btLoadCube                (nullptr),
      btLoadSphere              (nullptr),
      btLoadTorus               (nullptr),
      btLoadTeapot              (nullptr),
      btLoadBunny               (nullptr),
      loadingWidget             (nullptr),
      slot0Texture              (nullptr),
      slot1Texture              (nullptr),
      slot2Texture              (nullptr),
      slot3Texture              (nullptr),
      realtime                  (false),
      plane2D                   (false),
      realtimeCompilation       (false),
      renderTime                (0.0f),
      dragStartX                (0),
      dragStartY                (0),
      zoomStart                 (0),
      zoomLastPosition          (0.0f),
      shiftStartX               (0),
      shiftStartY               (0),
      mouseDragEnabled          (false),
      mouseZoomEnabled          (false),
      mouseShiftEnabled         (false),
      modelStartRotation        (glm::vec3(0.0f, 0.0f, 0.0f)),
      modelRotation             (glm::vec3(0.0f, 0.0f, 0.0f)),
      cameraPosition            (glm::vec3(0.0f, 0.0f, 0.0f)),
      cameraStartPosition       (glm::vec3(0.0f, 0.0f, 0.0f)),
      modelMatrix               (glm::mat4(1.0f)),
      viewMatrix                (glm::mat4(1.0f)),
      projectionMatrix          (glm::mat4(1.0f)),
      identityMatrix            (glm::mat4(1.0f))
{
    setStyleSheet(STYLE_OPENGLWIDGET);

    InitOverlay();
    InitTopLeftLayout();
    InitQuickModelButtons();
    InitLoadingWidget();

    // Load Default Mesh
    LoadOBJMesh(":/models/cube.obj");
    LoadPlaneMesh();

    // TODO Preload all default meshes for the quick actions...

    ResetModelRotation();
    ResetCameraPosition();
}

OpenGLWidget::~OpenGLWidget() {
    DestroyLoadingWidget();
    DestroyTextureSlots();
    DestroyQuickModelButtons();
    DestroyTopLeftLayout();
    DestroyOverlay();
    DestroyVAO();

    modelLoaderThread.quit();
    modelLoaderThread.wait();
}

QString OpenGLWidget::SelectedMeshName() {
    return selectedMeshName;
}

void OpenGLWidget::SelectMesh(const QString &meshName) {
    if (meshName == "Cube") { sl_LoadModelCube(); return; }
    if (meshName == "Sphere") { sl_LoadModelSphere(); return; }
    if (meshName == "Torus") { sl_LoadModelTorus(); return; }
    if (meshName == "Teapot") { sl_LoadModelTeapot(); return; }
    if (meshName == "Bunny") { sl_LoadModelBunny(); }
}

void OpenGLWidget::CheckRealtime(bool realtimeChecked) {
    cbRealtimeUpdate->setChecked(realtimeChecked);
}

bool OpenGLWidget::Realtime() {
    return realtime;
}

void OpenGLWidget::CheckPlane2D(bool plane2DChecked) {
    cbPlane2D->setChecked(plane2DChecked);
}

bool OpenGLWidget::Plane2D() {
    return plane2D;
}

void OpenGLWidget::RotateModel(const glm::vec3 &rotation) {

    // Reset model matrix first.
    modelMatrix = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));

    modelRotation.x = rotation.x;
    modelRotation.y = rotation.y;
    modelRotation.z = rotation.z;

    modelMatrix = glm::rotate(modelMatrix, modelRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, modelRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, modelRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
}

void OpenGLWidget::MoveCamera(const glm::vec3 &position) {
    cameraPosition = position;
    viewMatrix = glm::translate(glm::mat4(1.0f), cameraPosition);
}

glm::vec3 OpenGLWidget::ModelRotation() {
    return modelRotation;
}

glm::vec3 OpenGLWidget::CameraPosition() {
    return cameraPosition;
}

void OpenGLWidget::SetVertexShaderSource(const QString &source) {
    vertexShader->SetSource(source);

    if (realtimeCompilation) {
        sl_CompileShaders();
    }
}

void OpenGLWidget::SetFragmentShaderSource(const QString &source) {
    fragmentShader->SetSource(source);

    if (realtimeCompilation) {
        sl_CompileShaders();
    }
}

void OpenGLWidget::SetRealtimeCompilationEnabled(bool value) {
    realtimeCompilation = value;
}

void OpenGLWidget::ToggleRealtimeCompilation() {
    realtimeCompilation = !realtimeCompilation;
}

bool OpenGLWidget::RealtimeCompilation() {
    return realtimeCompilation;
}

OpenGLWidget::SLOT OpenGLWidget::FindSlotByName(const QString &slotName) {
    auto slot = OpenGLWidget::SLOT::TEX_0;
    if (slotName == GLSL_TEXTURE_SLOT_1_NAME) slot = OpenGLWidget::SLOT::TEX_1;
    if (slotName == GLSL_TEXTURE_SLOT_2_NAME) slot = OpenGLWidget::SLOT::TEX_2;
    if (slotName == GLSL_TEXTURE_SLOT_3_NAME) slot = OpenGLWidget::SLOT::TEX_3;
    return slot;
}

void OpenGLWidget::ApplyTextureToSlot(const QImage &image, SLOT slot) {

    // Image valid?
    if (image.isNull()) {
        return;
    }

    // QImage mirrored for each texture to not be applied upside down.
    switch (slot) {
        case SLOT::TEX_0:
            slot0Texture = new QOpenGLTexture(image.mirrored());
            slot0Texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
            break;

        case SLOT::TEX_1:
            slot1Texture = new QOpenGLTexture(image.mirrored());
            slot1Texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
            break;

        case SLOT::TEX_2:
            slot2Texture = new QOpenGLTexture(image.mirrored());
            slot2Texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
            break;

        case SLOT::TEX_3:
            slot3Texture = new QOpenGLTexture(image.mirrored());
            slot3Texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
            break;
    }

    repaint();
}

void OpenGLWidget::ClearTextureSlot(SLOT slot) {

    // Create black clear image.
    QImage clearImage(256, 256, QImage::Format_ARGB32);
    clearImage.fill(Qt::black);

    switch (slot) {
        case SLOT::TEX_0:
            RecreateTexture(slot0Texture, clearImage);
            break;

        case SLOT::TEX_1:
            RecreateTexture(slot1Texture, clearImage);
            break;

        case SLOT::TEX_2:
            RecreateTexture(slot2Texture, clearImage);
            break;

        case SLOT::TEX_3:
            RecreateTexture(slot3Texture, clearImage);
            break;
    }

    repaint();
}

void OpenGLWidget::ResetUI() {

    // Uncheck top left layout checkboxes.
    cbRealtimeUpdate->setChecked(false);
    cbPlane2D->setChecked(false);

    // Load default model and reset matrices.
    sl_LoadModelCube();
    ResetModelRotation();
    ResetCameraPosition();

    repaint();
}

void OpenGLWidget::sl_CompileShaders() {
    initializeOpenGLFunctions();

    // Vertex Shader
    try {
        vertexShader->Compile(program);
        emit si_CompileSuccess("Vertex shader compilation finished without errors.");

    } catch (SyntaxErrorException &e) {
        LinkProgramAndRepaint();
        auto error = GLSLCompileError(QString::fromStdString(e.File()), e.what());
        emit si_CompileError(error);
        return;
    }

    // Fragment Shader
    try {
        fragmentShader->Compile(program);
        emit si_CompileSuccess("Fragment shader compilation finished without errors.");

    } catch (SyntaxErrorException &e) {
        LinkProgramAndRepaint();
        auto error = GLSLCompileError(QString::fromStdString(e.File()), e.what());
        emit si_CompileError(error);
        return;
    }

    LinkProgramAndRepaint();
}

void OpenGLWidget::sl_LoadModelCube() {
    LoadModelAsync("Cube", ":/models/cube.obj", cubeVertices);
}

void OpenGLWidget::sl_LoadModelSphere() {
    LoadModelAsync("Sphere", ":/models/sphere.obj", sphereVertices);
}

void OpenGLWidget::sl_LoadModelTorus() {
    LoadModelAsync("Torus", ":/models/torus.obj", torusVertices);
}

void OpenGLWidget::sl_LoadModelTeapot() {
    LoadModelAsync("Teapot", ":/models/teapot.obj", teapotVertices);
}

void OpenGLWidget::sl_LoadModelBunny() {
    LoadModelAsync("Bunny", ":/models/bunny.obj", bunnyVertices);
}

void OpenGLWidget::sl_ModelLoaded(const QString &name) {
    loadingWidget->Hide();
    selectedMeshName = name;
    emit si_StateUpdated(QString("Model \"") + name + "\" loaded.");

    InitVAO();
    repaint();
}

void OpenGLWidget::sl_RealtimeUpdateStateChanged(const int &state) {
    switch (state) {
        case Qt::Checked:
            EnableRealtime();
            break;

        case Qt::Unchecked:
            DisableRealtime();
            break;

        default: break;
    }
}

void OpenGLWidget::sl_Plane2DStateChanged(const int &state) {
    switch (state) {
        case Qt::Checked:
            EnablePlane2D();
            break;

        case Qt::Unchecked:
            DisablePlane2D();
            break;

        default: break;
    }
}

void OpenGLWidget::sl_Tick() {
    if (realtime) {
        repaint();
        UpdateRenderTime();
        UpdateRealtime();
    }
}

void OpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    InitShaders();
    InitVAO();
    InitPlaneVAO();

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    emit si_GLInitialized();
}

void OpenGLWidget::resizeGL(int w, int h) {
    float ratio = static_cast<float>(w) / static_cast<float>(h);
    projectionMatrix = glm::perspective(glm::radians(60.0f), ratio, 0.1f, 100.0f);

    loadingWidget->move(w - loadingWidget->width() - 10,
                        h - loadingWidget->height() - 10);
}

void OpenGLWidget::paintGL() {
    initializeOpenGLFunctions();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Shader Uniforms
    glUseProgram(program);

    auto timeLocation           = glGetUniformLocation(program, "time");
    auto resolutionLocation     = glGetUniformLocation(program, "resolution");
    auto modelMatLocation       = glGetUniformLocation(program, "modelMat");
    auto viewMatLocation        = glGetUniformLocation(program, "viewMat");
    auto projectionMatLocation  = glGetUniformLocation(program, "projectionMat");

    // TODO Lights, math constants, camera position etc.

    // Apply Uniform Data
    glUniform1f(timeLocation, renderTime);
    glUniform2fv(resolutionLocation, 1, glm::value_ptr(glm::vec2(width(), height())));
    glUniformMatrix4fv(modelMatLocation, 1, GL_FALSE, GetModelMatrix());
    glUniformMatrix4fv(viewMatLocation, 1, GL_FALSE, GetViewMatrix());
    glUniformMatrix4fv(projectionMatLocation, 1, GL_FALSE, GetProjectionMatrix());

    // Samplers (Textures)
    BindTextures();

    // Draw
    DrawVAO();
    DrawPlaneVAO();

    // Reset Shader Program
    glUseProgram(0);

    ReleaseTextures();
}

void OpenGLWidget::mousePressEvent(QMouseEvent *event) {

    // Disable navigation for plane 2D view.
    if (plane2D) {
        return;
    }

    // Left Button
    if (event->button() == Qt::MouseButton::LeftButton) {
        EnableMouseDrag();
    }

    // Mid Button
    else if (event->button() == Qt::MouseButton::MidButton) {
        EnableMouseZoom();
    }

    // Right Button
    else if (event->button() == Qt::MouseButton::RightButton) {
        EnableMouseShift();
    }
}

void OpenGLWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (plane2D) {
        return;
    }

    ResetModelRotation();
    ResetCameraPosition();
    repaint();
}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event) {
    DisableMouseDrag();
    DisableMouseZoom();
    DisableMouseShift();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent *event) {

    // Mouse Drag / Model Rotation
    if (mouseDragEnabled) {
        ApplyMouseDrag(event);
    }

    // Mouse Drag / Model Zoom (Camera Forward/Backwards)
    else if (mouseZoomEnabled) {
        ApplyMouseZoom(event);
    }

    // Mouse Shift / Model Shifting
    else if (mouseShiftEnabled) {
        ApplyMouseShift(event);
    }

    repaint();
}

void OpenGLWidget::InitShaders() {
    program = glCreateProgram();

    vertexShader = Shader::MakeShared("", GL_VERTEX_SHADER);
    vertexShader->SetFile("VS");

    fragmentShader = Shader::MakeShared("", GL_FRAGMENT_SHADER);
    fragmentShader->SetFile("FS");
}

void OpenGLWidget::InitVAO() {
    if (!vao) {
        glGenVertexArrays(1, &vao);
    }

    glBindVertexArray(vao);

    if (!vertexBuffer) {
        glGenBuffers(1, &vertexBuffer);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * STRIDE_SIZE * sizeof(GLfloat),
                 vertices.data(), GL_STATIC_DRAW);

    InitAttribsForVAO();
    glBindVertexArray(0);

}

void OpenGLWidget::InitPlaneVAO() {
    if (!planeVAO) {
        glGenVertexArrays(1, &planeVAO);
    }

    glBindVertexArray(planeVAO);

    if (!planeVertexBuffer) {
        glGenBuffers(1, &planeVertexBuffer);
    }

    glBindBuffer(GL_ARRAY_BUFFER, planeVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * STRIDE_SIZE * sizeof(GLfloat),
                 planeVertices.data(), GL_STATIC_DRAW);

    InitAttribsForVAO();
    glBindVertexArray(0);
}

void OpenGLWidget::InitOverlay() {

    // Overlay Layout
    overlayLayout = new QVBoxLayout();
    overlayLayout->setContentsMargins(10, 10, 10, 10);
    overlayLayout->setAlignment(Qt::AlignLeft);
    setLayout(overlayLayout);
}

void OpenGLWidget::InitTopLeftLayout() {

    // Top Left Layout
    topLeftLayout = new QHBoxLayout();
    topLeftLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    overlayLayout->addLayout(topLeftLayout);

    // Realtime Update Checkbox
    cbRealtimeUpdate = new QCheckBox("Realtime");
    topLeftLayout->addWidget(cbRealtimeUpdate);

    connect(cbRealtimeUpdate, SIGNAL(stateChanged(int)),
            this, SLOT(sl_RealtimeUpdateStateChanged(int)));

    // Plane 2D Checkbox
    cbPlane2D = new QCheckBox("Plane 2D");
    topLeftLayout->addWidget(cbPlane2D);

    connect(cbPlane2D, SIGNAL(stateChanged(int)),
            this, SLOT(sl_Plane2DStateChanged(int)));
}

void OpenGLWidget::InitQuickModelButtons() {

    // Quick Load Models Layout
    quickLoadModelsLayout = new QHBoxLayout();
    quickLoadModelsLayout->setSpacing(10);
    quickLoadModelsLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->addLayout(quickLoadModelsLayout, 0);

    // Load Cube
    btLoadCube = new ImageButton(":/images/64/cube.png");
    quickLoadModelsLayout->addWidget(btLoadCube, 0, Qt::AlignLeft);
    connect(btLoadCube, SIGNAL(clicked()), this, SLOT(sl_LoadModelCube()));

    // Load Sphere
    btLoadSphere = new ImageButton(":/images/64/sphere.png");
    quickLoadModelsLayout->addWidget(btLoadSphere, 0, Qt::AlignLeft);
    connect(btLoadSphere, SIGNAL(clicked()), this, SLOT(sl_LoadModelSphere()));

    // Load Torus
    btLoadTorus = new ImageButton(":/images/64/torus.png");
    quickLoadModelsLayout->addWidget(btLoadTorus, 0, Qt::AlignLeft);
    connect(btLoadTorus, SIGNAL(clicked()), this, SLOT(sl_LoadModelTorus()));

    // Load Teapot
    btLoadTeapot = new ImageButton(":/images/64/teapot.png");
    quickLoadModelsLayout->addWidget(btLoadTeapot, 0, Qt::AlignLeft);
    connect(btLoadTeapot, SIGNAL(clicked()), this, SLOT(sl_LoadModelTeapot()));

    // Load Bunny
    btLoadBunny = new ImageButton(":/images/64/bunny.png");
    quickLoadModelsLayout->addWidget(btLoadBunny, 0, Qt::AlignLeft);
    connect(btLoadBunny, SIGNAL(clicked()), this, SLOT(sl_LoadModelBunny()));
}

void OpenGLWidget::InitLoadingWidget() {
    loadingWidget = new LoadingWidget(this);
}

void OpenGLWidget::DestroyLoadingWidget() {
    Memory::Release(loadingWidget);
}

void OpenGLWidget::DestroyTextureSlots() {
    // TODO SIGSEGV (Segmentation fault)

/* QOpenGLTexture::destroy():
       sub    $0x28,%rsp
       mov    (%rcx),%rcx
       callq  0xe07a098                  # <QOpenGLTexturePrivate::destroy()>
       nop
       add    $0x28,%rsp
       retq*/
}

void OpenGLWidget::DestroyQuickModelButtons() {
    Memory::Release(btLoadBunny);
    Memory::Release(btLoadTeapot);
    Memory::Release(btLoadTorus);
    Memory::Release(btLoadSphere);
    Memory::Release(btLoadCube);
    Memory::Release(quickLoadModelsLayout);
}

void OpenGLWidget::DestroyTopLeftLayout() {
    Memory::Release(cbPlane2D);
    Memory::Release(cbRealtimeUpdate);
    Memory::Release(topLeftLayout);
}

void OpenGLWidget::DestroyOverlay() {
    Memory::Release(overlayLayout);
}

void OpenGLWidget::DestroyVAO() {
    glDeleteBuffers(1, &planeVertexBuffer);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}

void OpenGLWidget::ShowQuickLoadModelsLayout() {
    btLoadCube->setVisible(true);
    btLoadSphere->setVisible(true);
    btLoadTorus->setVisible(true);
    btLoadTeapot->setVisible(true);
    btLoadBunny->setVisible(true);
}

void OpenGLWidget::HideQuickLoadModelsLayout() {
    btLoadCube->setVisible(false);
    btLoadSphere->setVisible(false);
    btLoadTorus->setVisible(false);
    btLoadTeapot->setVisible(false);
    btLoadBunny->setVisible(false);
}

void OpenGLWidget::EnableRealtime() {
    realtime = true;
    DetermineStartTime();
    UpdateRealtime();
}

void OpenGLWidget::DisableRealtime() {
    realtime = false;
}

void OpenGLWidget::UpdateRealtime() {
    // TODO Calculate FPS delay.
    QTimer::singleShot(33, this, SLOT(sl_Tick()));
}

void OpenGLWidget::EnablePlane2D() {
    plane2D = true;
    HideQuickLoadModelsLayout();
    repaint();
}

void OpenGLWidget::DisablePlane2D() {
    plane2D = false;
    ShowQuickLoadModelsLayout();
    repaint();
}

void OpenGLWidget::EnableMouseDrag() {
    mouseDragEnabled   = true;
    dragStartX         = 0;
    dragStartY         = 0;
    modelStartRotation = modelRotation;
}

void OpenGLWidget::DisableMouseDrag() {
    mouseDragEnabled = false;
}

void OpenGLWidget::EnableMouseZoom() {
    mouseZoomEnabled = true;
    zoomStart        = 0;
    zoomLastPosition = cameraPosition.z;
}

void OpenGLWidget::DisableMouseZoom() {
    mouseZoomEnabled = false;
}

void OpenGLWidget::EnableMouseShift() {
    mouseShiftEnabled   = true;
    shiftStartX         = 0;
    shiftStartY         = 0;
    cameraStartPosition = cameraPosition;
}

void OpenGLWidget::DisableMouseShift() {
    mouseShiftEnabled = false;
}

void OpenGLWidget::ApplyMouseDrag(QMouseEvent *event) {
    if (!dragStartX) dragStartX = mapToParent(event->pos()).x();
    if (!dragStartY) dragStartY = mapToParent(event->pos()).y();

    auto rotX = static_cast<GLfloat>(-(dragStartY - mapToParent(event->pos()).y()));
    auto rotY = static_cast<GLfloat>(-(dragStartX - mapToParent(event->pos()).x()));

    modelRotation = modelStartRotation + glm::vec3(
            glm::radians(rotX) * MODEL_ROTATION_INTENSITY,
            glm::radians(rotY) * MODEL_ROTATION_INTENSITY,
            0 /* Z */
    );

    modelMatrix = glm::rotate(glm::mat4(1.0f), modelRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, modelRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
}

void OpenGLWidget::ApplyMouseZoom(QMouseEvent *event) {
    if (!zoomStart) {
        zoomStart = mapToParent(event->pos()).y();
    }

    auto zoom = -(zoomStart - mapToParent(event->pos()).y());

    MoveCamera(glm::vec3(
            cameraPosition.x,
            cameraPosition.y,
            (zoom * MODEL_ZOOM_INTENSITY + zoomLastPosition))
    );
}

void OpenGLWidget::ApplyMouseShift(QMouseEvent *event) {
    if (!shiftStartX) shiftStartX = mapToParent(event->pos()).x();
    if (!shiftStartY) shiftStartY = mapToParent(event->pos()).y();

    auto shiftX = -(shiftStartX - mapToParent(event->pos()).x());
    auto shiftY = shiftStartY - mapToParent(event->pos()).y();

    cameraPosition = cameraStartPosition + glm::vec3(
            static_cast<GLfloat>(shiftX) * MODEL_SHIFT_INTENSITY,
            static_cast<GLfloat>(shiftY) * MODEL_SHIFT_INTENSITY,
            0 /* Z */
    );

    viewMatrix = glm::translate(glm::mat4(1.0f), cameraPosition);
}

void OpenGLWidget::ResetModelRotation() {
    RotateModel(OPENGLWIDGET_DEFAULT_MODEL_ROTATION);
}

void OpenGLWidget::ResetCameraPosition() {
    zoomStart = 0;
    zoomLastPosition = 0.0f;
    MoveCamera(OPENGLWIDGET_DEFAULT_CAMERA_POSITION);
}

void OpenGLWidget::LoadOBJMesh(const QString &path) {
    try {
        auto meshLoader = OBJMeshLoader(path);
        auto mesh = meshLoader.GetMesh();
        vertices = mesh.ComposedVertices();

    } catch (GeneralException &e) {
        emit si_GeneralError(e);
    }
}

void OpenGLWidget::LoadPlaneMesh() {
    try {
        auto meshLoader = OBJMeshLoader(":/models/plane.obj");
        auto mesh = meshLoader.GetMesh();
        planeVertices = mesh.ComposedVertices();

    } catch (GeneralException &e) {
        emit si_GeneralError(e);
    }
}

void OpenGLWidget::LoadModelAsync(const QString &name,
                                  const QString &file,
                                  VertexVec &vertexStore)
{
    // Apply only vertices, if the model was already loaded before.
    if (!vertexStore.empty()) {
        ApplyVertices(vertexStore);
        sl_ModelLoaded(name);
        return;
    }

    loadingWidget->Show("Loading \"" + name + "\"");

    // Async Load Model
    auto *loader = new OpenGLWidget_ModelLoader(this, name, file, vertexStore);
    loader->moveToThread(&modelLoaderThread);

    connect(&modelLoaderThread, &QThread::finished, loader, &QObject::deleteLater);
    connect(this, SIGNAL(si_TriggerModelLoading()), loader, SLOT(sl_Load()));

    connect(loader, SIGNAL(si_ModelLoaded(const QString&)),
            this, SLOT(sl_ModelLoaded(const QString&)));

    modelLoaderThread.start();
    emit si_TriggerModelLoading();
}

void OpenGLWidget::LoadOBJMeshIntoBuffer(const QString &file, VertexVec &buffer) {
    try {
        auto meshLoader = OBJMeshLoader(file);
        auto mesh = meshLoader.GetMesh();
        buffer = mesh.ComposedVertices();

    } catch (GeneralException &e) {
        emit si_GeneralError(e);
    }
}

void OpenGLWidget::ApplyVertices(const VertexVec &newVertices) {
    modelLoaderMutex.lock();
    vertices = newVertices;
    modelLoaderMutex.unlock();
}

GLfloat* OpenGLWidget::GetModelMatrix() {
    if (plane2D) {
        return glm::value_ptr(identityMatrix);
    }

    return glm::value_ptr(modelMatrix);
}

GLfloat* OpenGLWidget::GetViewMatrix() {
    if (plane2D) {
        return glm::value_ptr(identityMatrix);
    }

    return glm::value_ptr(viewMatrix);
}

GLfloat* OpenGLWidget::GetProjectionMatrix() {
    if (plane2D) {
        return glm::value_ptr(identityMatrix);
    }

    return glm::value_ptr(projectionMatrix);
}

void OpenGLWidget::DetermineStartTime() {
    startTime = QDateTime::currentDateTime();
}

void OpenGLWidget::UpdateRenderTime() {
    auto millisDiff = static_cast<float>(startTime.msecsTo(QDateTime::currentDateTime()));
    renderTime = millisDiff / 1000.0f;
}

void OpenGLWidget::RecreateTexture(QOpenGLTexture *texture, const QImage &image) {
    if (texture == nullptr) {
        return;
    }

    texture->destroy();
    texture->setData(image, QOpenGLTexture::MipMapGeneration::GenerateMipMaps);
    texture->create();
}

void OpenGLWidget::BindTexture(QOpenGLTexture *texture,
                               const QString &location,
                               const uint8_t &unit)
{
    if (texture != nullptr) {
        auto uniformLocation = glGetUniformLocation(program, location.toStdString().c_str());

        if (uniformLocation) {
            glUniform1i(uniformLocation, unit);
            texture->bind(unit);
        }
    }
}

void OpenGLWidget::BindTextures() {
    BindTexture(slot0Texture, GLSL_TEXTURE_SLOT_0_NAME, 0);
    BindTexture(slot1Texture, GLSL_TEXTURE_SLOT_1_NAME, 1);
    BindTexture(slot2Texture, GLSL_TEXTURE_SLOT_2_NAME, 2);
    BindTexture(slot3Texture, GLSL_TEXTURE_SLOT_3_NAME, 3);
}

void OpenGLWidget::ReleaseTextures() {
    if (slot0Texture) slot0Texture->release();
    if (slot1Texture) slot1Texture->release();
    if (slot2Texture) slot2Texture->release();
    if (slot3Texture) slot3Texture->release();
}

void OpenGLWidget::InitAttribsForVAO() {

    // Vertex Position Attrib
    auto vPosLocation = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(vPosLocation);
    glVertexAttribPointer(vPosLocation, 3, GL_FLOAT, GL_FALSE,
                          GLUtility::StrideSize(STRIDE_SIZE), GLUtility::StridePtr(0));

    // Normal Attrib
    auto vNormalLocation = glGetAttribLocation(program, "normal");
    glEnableVertexAttribArray(vNormalLocation);
    glVertexAttribPointer(vNormalLocation, 3, GL_FLOAT, GL_FALSE,
                          GLUtility::StrideSize(STRIDE_SIZE), GLUtility::StridePtr(3));

    // UV Attrib
    auto vTexUVLocation = glGetAttribLocation(program, "uv");
    glEnableVertexAttribArray(vTexUVLocation);
    glVertexAttribPointer(vTexUVLocation, 2, GL_FLOAT, GL_FALSE,
                          GLUtility::StrideSize(STRIDE_SIZE), GLUtility::StridePtr(6));
}

void OpenGLWidget::LinkProgramAndRepaint() {
    glLinkProgram(program);
    InitVAO();
    InitPlaneVAO();
    repaint();
}

void OpenGLWidget::DrawVAO() {
    if (plane2D) {
        return;
    }

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}

void OpenGLWidget::DrawPlaneVAO() {
    if (!plane2D) {
        return;
    }

    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(planeVertices.size()));
    glBindVertexArray(0);
}
