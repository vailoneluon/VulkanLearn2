#include "pch.h"
#include "ModelLoader.h"

ModelLoader::ModelLoader() {}
ModelLoader::~ModelLoader() {}

ModelData ModelLoader::LoadModelFromFile(const std::string& filePath)
{
	ModelData modelData{};
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filePath,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_GenNormals
	);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return modelData;
	}

	if (scene->mNumMeshes == 0)
	{
		std::cerr << "ERROR::ASSIMP:: Model has no meshes!" << std::endl;
		return modelData;
	}

	aiMesh* mesh = scene->mMeshes[0];

	// 5. Lấy Dữ liệu Vertex (Giữ nguyên)
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex{};
		vertex.pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		if (mesh->HasNormals())
		{
			vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		}
		if (mesh->mTextureCoords[0])
		{
			vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
		else
		{
			vertex.uv = { 0.0f, 0.0f };
		}
		modelData.vertices.push_back(vertex);
	}

	// 6. Lấy Dữ liệu Index (Giữ nguyên)
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			modelData.indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
		}
	}

	// 7. Lấy Texture (ĐÃ SỬA LẠI)
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiString texPath_ai; // Tên gốc (có thể "xấu") từ Assimp

		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath_ai) == AI_SUCCESS)
		{
			// Lấy đường dẫn thư mục CỦA MODEL (ví dụ: "Resources/AnimeGirlModel/source/")
			std::string modelDirectory = "";
			size_t lastSlash_model = filePath.find_last_of("/\\");
			if (lastSlash_model != std::string::npos)
			{
				modelDirectory = filePath.substr(0, lastSlash_model + 1);
			}

			// Lấy đường dẫn "xấu" từ Assimp (ví dụ: "..\\Downloads\\AnimeGirl_01_2023_dif.png")
			std::string badTexPath = std::string(texPath_ai.C_Str());

			// Tách CHỈ tên file ra (ví dụ: "AnimeGirl_01_2023_dif.png")
			std::string filename = badTexPath;
			size_t lastSlash_tex = badTexPath.find_last_of("/\\");
			if (lastSlash_tex != std::string::npos)
			{
				filename = badTexPath.substr(lastSlash_tex + 1);
			}

			// Giả định thư mục texture nằm ở: [thư mục model]/../textures/
			// Dựa trên thông tin bạn cung cấp.
			std::string textureDirectory = modelDirectory + "../textures/";

			// Ghép lại để ra đường dẫn đúng
			// (Kết quả: "Resources/AnimeGirlModel/source/../textures/AnimeGirl_01_2023_dif.png"
			//  Hệ thống file sẽ tự hiểu thành: "Resources/AnimeGirlModel/textures/AnimeGirl_01_2023_dif.png")
			modelData.textureFilePath = textureDirectory + filename;
		}
	}

	modelData.vertexBufferSize = modelData.vertices.size() * sizeof(modelData.vertices[0]);
	modelData.indexBufferSize = modelData.indices.size() * sizeof(modelData.indices[0]);

	return modelData;
}