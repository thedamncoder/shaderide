/**
 * ShaderProject Class
 *
 * -------------------------------------------------------------------------------
 * This file is part of "Shader IDE" -> https://github.com/thedamncoder/shaderide.
 * -------------------------------------------------------------------------------
 *
 * Copyright (c) 2019 - 2021 Florian Roth (The Damn Coder)
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

#include <utility>
#include <fstream>
#include <QJsonObject>
#include <QJsonDocument>
#include "ShaderProject.hpp"
#include "ProjectException.hpp"

using namespace ShaderIDE::Project;

ShaderProject* ShaderProject::Load(const QString& path)
{
    std::ifstream file = OpenInputFile(path);
    auto* shaderProject = new ShaderProject(path);

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto jsonDocument = QJsonDocument::fromJson(content.c_str());
    auto project = jsonDocument.object();

    if (project.isEmpty()) {
        throw GeneralException("Could not read project file \"" + path + "\".");
    }

    // File Version
    if (project.find("file_version") != project.end()) {
        shaderProject->SetFileVersion(project.find("file_version")->toString());
    }

    // Path
    shaderProject->SetPath(path);

    // VS Source
    if (project.find("vsSource") != project.end()) {
        shaderProject->SetVertexShaderSource(project.find("vsSource")->toString());
    }

    // FS Source
    if (project.find("fsSource") != project.end()) {
        shaderProject->SetFragmentShaderSource(project.find("fsSource")->toString());
    }

    // Mesh Name
    if (project.find("meshName") != project.end()) {
        shaderProject->SetMeshName(project.find("meshName")->toString());
    }

    // Texture Paths
    if (project.find("textureData") != project.end())
    {
        auto tp = project.find("textureData")->toObject();

        for (auto it = tp.begin(); it != tp.end(); ++it) {
            shaderProject->SetTextureData(it.key(), it.value().toString());
        }
    }

    // Realtime
    if (project.find("realtime") != project.end()) {
        shaderProject->SetRealtime(project.find("realtime")->toBool());
    }

    // Plane 2D
    if (project.find("plane2D") != project.end()) {
        shaderProject->SetPlane2D(project.find("plane2D")->toBool());
    }

    // Model Rotation
    if (project.find("modelRotation") != project.end()) {
        shaderProject->SetModelRotation(SerializableVector3::FromJsonArray(project.find("modelRotation")->toArray()));
    }

    // Camera Position
    if (project.find("cameraPosition") != project.end()) {
        shaderProject->SetCameraPosition(SerializableVector3::FromJsonArray(project.find("cameraPosition")->toArray()));
    }

    file.close();

    // Mark saved initially.
    shaderProject->MarkSaved();

    return shaderProject;
}

ShaderProject::ShaderProject(QString path)
        : path(std::move(path))
{}

QString ShaderProject::Version()
{
    return file_version;
}

QString ShaderProject::Path()
{
    return path;
}

QString ShaderProject::VertexShaderSource()
{
    return vsSource;
}

QString ShaderProject::FragmentShaderSource()
{
    return fsSource;
}

QString ShaderProject::MeshName()
{
    return meshName;
}

std::unordered_map<QString, QString> ShaderProject::TextureData()
{
    return textureData;
}

bool ShaderProject::Realtime() const
{
    return realtime;
}

bool ShaderProject::Plane2D() const
{
    return plane2D;
}

glm::vec3 ShaderProject::ModelRotation()
{
    return modelRotation;
}

glm::vec3 ShaderProject::CameraPosition()
{
    return cameraPosition;
}

QString ShaderProject::PathOnly()
{
    const auto div = LastSlashPos();

    if (div != std::string::npos) {
        return QString::fromStdString(path.toStdString().substr(0, div + 1));
    }

    return "./";
}

bool ShaderProject::Saved() const
{
    return saved;
}

void ShaderProject::SetFileVersion(const QString& newFileVersion)
{
    file_version = newFileVersion;
    MarkUnsaved();
}

void ShaderProject::SetPath(const QString& newPath)
{
    path = newPath;
    MarkUnsaved();
}

void ShaderProject::SetVertexShaderSource(const QString& newVSSource)
{
    vsSource = newVSSource;
    MarkUnsaved();
}

void ShaderProject::SetFragmentShaderSource(const QString& newFSSource)
{
    fsSource = newFSSource;
    MarkUnsaved();
}

void ShaderProject::SetMeshName(const QString& newMeshName)
{
    meshName = newMeshName;
    MarkUnsaved();
}

void ShaderProject::SetRealtime(bool newRealtime)
{
    realtime = newRealtime;
    MarkUnsaved();
}

void ShaderProject::SetPlane2D(bool newPlane2D)
{
    plane2D = newPlane2D;
    MarkUnsaved();
}

void ShaderProject::SetModelRotation(const glm::vec3& newModelRotation)
{
    modelRotation = newModelRotation;
    MarkUnsaved();
}

void ShaderProject::SetCameraPosition(const glm::vec3& newCameraPosition)
{
    cameraPosition = newCameraPosition;
    MarkUnsaved();
}

void ShaderProject::SetTextureData(const QString& name, const QString& data)
{
    textureData[name] = data;
    MarkUnsaved();
}

void ShaderProject::ClearTextureData(const QString& name)
{
    if (textureData.find(name) != textureData.end()) {
        textureData[name] = "";
    }

    MarkUnsaved();
}

void ShaderProject::Save()
{
    std::ofstream file = OpenOutputFile(path);

    if (!file.is_open())
    {
        throw ProjectException(
                "Could not create and save shader project.",
                ProjectException::ExceptionCode::DEFAULT
        );
    }

    QJsonObject project;
    project["file_version"] = SHADERIDE_VERSION;
    project["vsSource"] = vsSource;
    project["fsSource"] = fsSource;
    project["meshName"] = meshName;
    project["textureData"] = MakeJsonObjectFromTextureData();
    project["realtime"] = realtime;
    project["plane2D"] = plane2D;
    project["modelRotation"] = modelRotation.ToJsonArray();
    project["cameraPosition"] = cameraPosition.ToJsonArray();

    QJsonDocument jsonDocument(project);
    file << jsonDocument.toJson().toStdString();
    file.close();

    MarkSaved();
}

void ShaderProject::CheckPath(const QString& filePath)
{
    if (filePath.isEmpty())
    {
        throw ProjectException(
                "Could not save shader project. "
                "Project path not provided.",
                ProjectException::ExceptionCode::PATH_EMPTY
        );
    }
}

std::ifstream ShaderProject::OpenInputFile(const QString& filePath)
{
    std::ifstream file(filePath.toStdString());

    if (!file.is_open())
    {
        throw GeneralException(
                QString("Could not load shader project file \"") + filePath + "\""
        );
    }

    return file;
}

std::ofstream ShaderProject::OpenOutputFile(const QString& filePath)
{
    CheckPath(filePath);

    std::ofstream file(filePath.toStdString());

    if (!file.is_open())
    {
        throw GeneralException(
                QString("Could not save shader project to \"") + filePath + "\""
        );
    }

    return file;
}

QJsonObject ShaderProject::MakeJsonObjectFromTextureData()
{
    QJsonObject jsonArray;

    for (auto& it : textureData) {
        jsonArray[it.first] = it.second;
    }

    return jsonArray;
}

size_t ShaderProject::LastSlashPos()
{
    const auto npos = std::string::npos;
    const auto divFS = path.toStdString().find_last_of('/');
    const auto divBS = path.toStdString().find_last_of('\\');
    return (divFS != npos) ? divFS : divBS;
}

void ShaderProject::MarkSaved()
{
    if (path.isEmpty()) {
        return;
    }

    saved = true;
    emit NotifyMarkSaved();
}

void ShaderProject::MarkUnsaved()
{
    if (path.isEmpty()) {
        return;
    }

    saved = false;
    emit NotifyMarkUnsaved();
}
