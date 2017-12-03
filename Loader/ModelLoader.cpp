#include "modelloader.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <QDebug>
#include <limits>

using SpanningScanline::MaterialInfo;
using SpanningScanline::LightInfo;
using SpanningScanline::Mesh;
using SpanningScanline::Node;

using SpanningScanline::ModelLoader;

#define DEBUGOUTPUT_NORMALS(nodeIndex) (false)//( QList<int>{1}.contains(nodeIndex) )//(false)

ModelLoader::ModelLoader(bool transformToUnitCoordinates) :
    m_transformToUnitCoordinates(transformToUnitCoordinates)
{

}

std::string SpanningScanline::ModelLoader::getSupportedTypes()
{
	std::string types;
	Assimp::Importer importer;
	importer.GetExtensionList(types);

	return types;
}

// look for file using relative path
QString findFile(QString relativeFilePath, int scanDepth)
{
    QString str = relativeFilePath;
    for(int ii=-1; ii<scanDepth; ++ii)
    {
        if(QFile::exists(str))
        {
            return str;
        }
        str.prepend("../");
    }
    return "";
}

bool ModelLoader::load(QString filePath, PathType pathType)
{
    QString l_filePath;
    if (pathType == RelativePath)
        l_filePath = findFile(filePath, 5);
    else
        l_filePath = filePath;

    Assimp::Importer importer;
	
    const aiScene* scene = importer.ReadFile( l_filePath.toStdString(),
            aiProcess_GenSmoothNormals      |
            aiProcess_CalcTangentSpace       |
            aiProcess_Triangulate       |
            aiProcess_JoinIdenticalVertices  |
            aiProcess_SortByPType
                                              );

    if( !scene)
    {
        qDebug() << "Error loading file: (assimp:) " << importer.GetErrorString();
        return false;
    }

    if(scene->HasMaterials())
    {
        for(unsigned int ii=0; ii<scene->mNumMaterials; ++ii)
        {
            QSharedPointer<MaterialInfo> mater = processMaterial(scene->mMaterials[ii]);
            m_materials.push_back(mater);
        }
    }

    if(scene->HasMeshes())
    {
        for(unsigned int ii=0; ii<scene->mNumMeshes; ++ii)
        {
            m_meshes.push_back(processMesh(scene->mMeshes[ii]));
        }
    }
    else
    {
        qDebug() << "Error: No meshes found";
        return false;
    }

    if(scene->HasLights())
    {
        qDebug() << "Has Lights";
    }

    if(scene->mRootNode != NULL)
    {
        Node *rootNode = new Node;
        processNode(scene, scene->mRootNode, 0, *rootNode);
        m_rootNode.reset(rootNode);
    }
    else
    {
        qDebug() << "Error loading model";
        return false;
    }

    // This will transform the model to unit coordinates, so a model of any size or shape will fit on screen
    if (m_transformToUnitCoordinates)
        transformToUnitCoordinates();

    return true;
}

void ModelLoader::getBufferData( QVector<float> **vertices, QVector<float> **normals, QVector<unsigned int> **indices)
{
    if(vertices != 0)
        *vertices = &m_vertices;

    if(normals != 0)
        *normals = &m_normals;

    if(indices != 0)
        *indices = &m_indices;
}

void ModelLoader::getTextureData(QVector<QVector<float> > **textureUV, QVector<float> **tangents, QVector<float> **bitangents)
{
    if(textureUV != 0)
        *textureUV = &m_textureUV;

    if(tangents != 0)
        *tangents = &m_tangents;

    if(bitangents != 0)
        *bitangents = &m_bitangents;
}

QSharedPointer<SpanningScanline::MaterialInfo> ModelLoader::processMaterial(aiMaterial *material)
{
    QSharedPointer<MaterialInfo> mater(new MaterialInfo);

    aiString mname;
    material->Get( AI_MATKEY_NAME, mname);
    if(mname.length > 0)
        mater->Name = mname.C_Str();
    mater->isTexture = false;

    int shadingModel;
    material->Get( AI_MATKEY_SHADING_MODEL, shadingModel );

    if(shadingModel != aiShadingMode_Phong && shadingModel != aiShadingMode_Gouraud)
    {
        qDebug() << "This mesh's shading model is not implemented in this loader, setting to default material";
        mater->Name = "DefaultMaterial";
    }
    else
    {
        aiColor3D dif (0.f,0.f,0.f);
        aiColor3D amb (0.f,0.f,0.f);
        aiColor3D spec (0.f,0.f,0.f);
        float shine = 0.0;

        material->Get( AI_MATKEY_COLOR_AMBIENT, amb);
        material->Get( AI_MATKEY_COLOR_DIFFUSE, dif); //->Get(<material-key>,<where-to-store>))
        material->Get( AI_MATKEY_COLOR_SPECULAR, spec);
        material->Get( AI_MATKEY_SHININESS, shine);

        mater->Ambient = QVector3D(amb.r, amb.g, amb.b);
        mater->Diffuse = QVector3D(dif.r, dif.g, dif.b);
        mater->Specular = QVector3D(spec.r, spec.g, spec.b);
        mater->Shininess = shine;

        mater->Ambient *= .2f;
        if( mater->Shininess == 0.0) mater->Shininess = 30;

        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            qDebug() << "Diffuse Texture(s) Found:" << material->GetTextureCount(aiTextureType_DIFFUSE)
                     << "for Material:" << mater->Name;
            aiString texPath;

            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                QString texturePath = texPath.data;
                mater->isTexture = true;
                mater->textureName = texturePath;
                qDebug() << "  Texture path" << texturePath;
            }
            else
                qDebug() << "  Failed to get texture for material";
        }
    }

    return mater;
}

QSharedPointer<SpanningScanline::Mesh> ModelLoader::processMesh(aiMesh *mesh)
{
    QSharedPointer<Mesh> newMesh(new Mesh);
    newMesh->name = mesh->mName.length != 0 ? mesh->mName.C_Str() : "";
    newMesh->indexOffset = m_indices.size();
    unsigned int indexCountBefore = m_indices.size();
    int vertindexoffset = m_vertices.size()/3;

    newMesh->numUVChannels = mesh->GetNumUVChannels();
    newMesh->hasTangentsAndBitangents = mesh->HasTangentsAndBitangents();
    newMesh->hasNormals = mesh->HasNormals();
    newMesh->hasBones = mesh->HasBones();

    // Get Vertices
    if(mesh->mNumVertices > 0)
    {
        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mVertices[ii];

            m_vertices.push_back(vec.x);
            m_vertices.push_back(vec.y);
            m_vertices.push_back(vec.z);
        }
    }

    // Get Normals
    if(mesh->HasNormals())
    {
        m_normals.resize(m_vertices.size());

        int nind = vertindexoffset * 3;

        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mNormals[ii];
            m_normals[nind] = vec.x;
            m_normals[nind+1] = vec.y;
            m_normals[nind+2] = vec.z;
            nind += 3;
        };
    }

    // Get Texture coordinates
    if(mesh->GetNumUVChannels() > 0)
    {
        if((unsigned int)m_textureUV.size() < mesh->GetNumUVChannels()) // Caution, assumes all meshes in this model have same number of uv channels
        {
            m_textureUV.resize(mesh->GetNumUVChannels());
            m_numUVComponents.resize(mesh->GetNumUVChannels());
        }

        for( unsigned int mchanInd = 0; mchanInd < mesh->GetNumUVChannels(); ++mchanInd)
        {
            Q_ASSERT(mesh->mNumUVComponents[mchanInd] == 2 && "Error: Texture Mapping Component Count must be 2. Others not supported");

            m_numUVComponents[mchanInd] = mesh->mNumUVComponents[mchanInd];
            m_textureUV[mchanInd].resize((m_vertices.size()/3)*2);

            int uvind = vertindexoffset * m_numUVComponents[mchanInd];

            for(uint iind = 0; iind<mesh->mNumVertices; ++iind)
            {
                // U
                m_textureUV[mchanInd][uvind] = mesh->mTextureCoords[mchanInd][iind].x;
                if(mesh->mNumUVComponents[mchanInd] > 1)
                {
                    // V
                    m_textureUV[mchanInd][uvind+1] = mesh->mTextureCoords[mchanInd][iind].y;
                    if(mesh->mNumUVComponents[mchanInd] > 2)
                    {
                        // W
                        m_textureUV[mchanInd][uvind+2] = mesh->mTextureCoords[mchanInd][iind].z;
                    }
                }
                uvind += m_numUVComponents[mchanInd];
            }
        }
    }

    // Get Tangents and bitangents
    if(mesh->HasTangentsAndBitangents())
    {
        m_tangents.resize(m_vertices.size());
        m_bitangents.resize(m_vertices.size());

        int tind = vertindexoffset * 3;

        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mTangents[ii];
            m_tangents[tind] = vec.x;
            m_tangents[tind+1] = vec.y;
            m_tangents[tind+2] = vec.z;

            aiVector3D &vec2 = mesh->mBitangents[ii];
            m_bitangents[tind] = vec2.x;
            m_bitangents[tind+1] = vec2.y;
            m_bitangents[tind+2] = vec2.z;

            tind += 3;
        };
    }

    // Get mesh indexes
    for(uint t = 0; t<mesh->mNumFaces; ++t)
    {
        aiFace* face = &mesh->mFaces[t];
        if(face->mNumIndices != 3)
        {
            qDebug() << "Warning: Mesh face with not exactly 3 indices, ignoring this primitive." << face->mNumIndices;
            continue;
        }

        m_indices.push_back(face->mIndices[0]+vertindexoffset);
        m_indices.push_back(face->mIndices[1]+vertindexoffset);
        m_indices.push_back(face->mIndices[2]+vertindexoffset);
    }

    newMesh->indexCount = m_indices.size() - indexCountBefore;
    newMesh->material = m_materials.at(mesh->mMaterialIndex);

    return newMesh;
}

void ModelLoader::processNode(const aiScene *scene, aiNode *node, Node *parentNode, Node &newNode)
{
    static int nodeIndex = 0;

    newNode.name = node->mName.length != 0 ? node->mName.C_Str() : "";

    newNode.transformation = QMatrix4x4(node->mTransformation[0]);

    newNode.meshes.resize(node->mNumMeshes);
    for(uint imesh = 0; imesh < node->mNumMeshes; ++imesh)
    {
        QSharedPointer<Mesh> mesh = m_meshes[node->mMeshes[imesh]];
        newNode.meshes[imesh] = mesh;

        if (DEBUGOUTPUT_NORMALS(nodeIndex)) {
            qDebug() << "Start Print Normals for nodeIndex:" << nodeIndex << ". MeshIndex:" << imesh << ". NodeName" <<newNode.name;
            for (quint32 ii=newNode.meshes[imesh]->indexOffset;
                 ii<newNode.meshes[imesh]->indexCount+newNode.meshes[imesh]->indexOffset;
                 ++ii) {
                int ind = m_indices[ii] * 3;
                qDebug() << (m_normals[ind]) << (m_normals[ind]+1) << (m_normals[ind]+2);
            }
            qDebug() << "End Print Normals for nodeIndex:" << nodeIndex << ". MeshIndex:" << imesh << ". NodeName" <<newNode.name;
        }
    }

    qDebug() << "NodeName" << newNode.name;
    qDebug() << "  NodeIndex" << nodeIndex;
    qDebug() << "  NumChildren" << node->mNumChildren;
    qDebug() << "  NumMeshes" << newNode.meshes.size();
    for (int ii=0; ii<newNode.meshes.size(); ++ii) {
        qDebug() << "    MeshName" << newNode.meshes[ii]->name;
        qDebug() << "    MaterialName" << newNode.meshes[ii]->material->Name;
        qDebug() << "    MeshVertices" << newNode.meshes[ii]->indexCount;
        qDebug() << "    numUVChannels" << newNode.meshes[ii]->numUVChannels;
        qDebug() << "    hasTangAndBit" << newNode.meshes[ii]->hasTangentsAndBitangents;
        qDebug() << "    hasNormals" << newNode.meshes[ii]->hasNormals;
        qDebug() << "    hasBones" << newNode.meshes[ii]->hasBones;
    }

    ++nodeIndex;

    for(uint ich = 0; ich < node->mNumChildren; ++ich)
    {
        newNode.nodes.push_back(Node());
        processNode(scene, node->mChildren[ich], parentNode, newNode.nodes[ich]);
    }
}

void ModelLoader::transformToUnitCoordinates()
{
    // This will transform the model to unit coordinates, so a model of any size or shape will fit on screen

    double amin = std::numeric_limits<double>::max();
    double amax = std::numeric_limits<double>::min();
    QVector3D minDimension(amin,amin,amin);
    QVector3D maxDimension(amax,amax,amax);

    // Get the minimum and maximum x,y,z values for the model
    findObjectDimensions(m_rootNode.data(), QMatrix4x4(), minDimension, maxDimension);

    // Calculate scale and translation needed to center and fit on screen
    float dist = qMax(maxDimension.x() - minDimension.x(), qMax(maxDimension.y()-minDimension.y(), maxDimension.z() - minDimension.z()));
    float sc = 1.0/dist;
    QVector3D center = (maxDimension - minDimension)/2;
    QVector3D trans = -(maxDimension - center);

    // Apply the scale and translation to a matrix
    QMatrix4x4 transformation;
    transformation.setToIdentity();
    transformation.scale(sc);
    transformation.translate(trans);

    // Multiply the transformation to the root node transformation matrix
    m_rootNode.data()->transformation = transformation * m_rootNode.data()->transformation;
}

void ModelLoader::findObjectDimensions(Node *node, QMatrix4x4 transformation, QVector3D &minDimension, QVector3D &maxDimension)
{
    transformation *= node->transformation;

    for (int ii=0; ii<node->meshes.size(); ++ii) {
        int start = node->meshes[ii]->indexOffset;
        int end = start + node->meshes[ii]->indexCount;
        for(int ii=start; ii<end; ++ii)
        {
            int ind = m_indices[ii] * 3;
            QVector4D vec(m_vertices[ind], m_vertices[ind+1], m_vertices[ind+2], 1.0);
            vec = transformation * vec;

            if(vec.x() < minDimension.x())
                minDimension.setX(vec.x());
            if(vec.y() < minDimension.y())
                minDimension.setY(vec.y());
            if(vec.z() < minDimension.z())
                minDimension.setZ(vec.z());
            if(vec.x() > maxDimension.x())
                maxDimension.setX(vec.x());
            if(vec.y() > maxDimension.y())
                maxDimension.setY(vec.y());
            if(vec.z() > maxDimension.z())
                maxDimension.setZ(vec.z());
        }
    }

    for (int ii=0; ii<node->nodes.size(); ++ii) {
        findObjectDimensions(&(node->nodes[ii]), transformation, minDimension, maxDimension);
    }
}
