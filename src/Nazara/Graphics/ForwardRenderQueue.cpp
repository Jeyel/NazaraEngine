// Copyright (C) 2015 Jérôme Leclercq
// This file is part of the "Nazara Engine - Graphics module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Graphics/ForwardRenderQueue.hpp>
#include <Nazara/Graphics/AbstractViewer.hpp>
#include <Nazara/Graphics/Light.hpp>
#include <Nazara/Graphics/Debug.hpp>

///TODO: Replace sinus/cosinus by a lookup table (which will lead to a speed up about 10x)

namespace Nz
{
	/*!
	* \ingroup graphics
	* \class Nz::ForwardRenderQueue
	* \brief Graphics class that represents the rendering queue for forward rendering
	*/

	/*!
	* \brief Adds billboard to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboard
	* \param position Position of the billboard
	* \param size Sizes of the billboard
	* \param sinCos Rotation of the billboard
	* \param color Color of the billboard
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboard(int renderOrder, const Material* material, const Vector3f& position, const Vector2f& size, const Vector2f& sinCos, const Color& color)
	{
		NazaraAssert(material, "Invalid material");

		auto& billboards = GetLayer(renderOrder).billboards;

		auto it = billboards.find(material);
		if (it == billboards.end())
		{
			BatchedBillboardEntry entry;
			entry.materialReleaseSlot.Connect(material->OnMaterialRelease, this, &ForwardRenderQueue::OnMaterialInvalidation);

			it = billboards.insert(std::make_pair(material, std::move(entry))).first;
		}

		BatchedBillboardEntry& entry = it->second;

		auto& billboardVector = entry.billboards;
		billboardVector.push_back(BillboardData{color, position, size, sinCos});
	}

	/*!
	* \brief Adds multiple billboards to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboards
	* \param count Number of billboards
	* \param positionPtr Position of the billboards
	* \param sizePtr Sizes of the billboards
	* \param sinCosPtr Rotation of the billboards if null, Vector2f(0.f, 1.f) is used
	* \param colorPtr Color of the billboards if null, Color::White is used
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboards(int renderOrder, const Material* material, unsigned int count, SparsePtr<const Vector3f> positionPtr, SparsePtr<const Vector2f> sizePtr, SparsePtr<const Vector2f> sinCosPtr, SparsePtr<const Color> colorPtr)
	{
		NazaraAssert(material, "Invalid material");

		Vector2f defaultSinCos(0.f, 1.f); // sin(0) = 0, cos(0) = 1

		if (!sinCosPtr)
			sinCosPtr.Reset(&defaultSinCos, 0); // The trick here is to put the stride to zero, which leads the pointer to be immobile

		if (!colorPtr)
			colorPtr.Reset(&Color::White, 0); // Same

		BillboardData* billboardData = GetBillboardData(renderOrder, material, count);
		for (unsigned int i = 0; i < count; ++i)
		{
			billboardData->center = *positionPtr++;
			billboardData->color = *colorPtr++;
			billboardData->sinCos = *sinCosPtr++;
			billboardData->size = *sizePtr++;
			billboardData++;
		}
	}

	/*!
	* \brief Adds multiple billboards to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboards
	* \param count Number of billboards
	* \param positionPtr Position of the billboards
	* \param sizePtr Sizes of the billboards
	* \param sinCosPtr Rotation of the billboards if null, Vector2f(0.f, 1.f) is used
	* \param alphaPtr Alpha parameters of the billboards if null, 1.f is used
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboards(int renderOrder, const Material* material, unsigned int count, SparsePtr<const Vector3f> positionPtr, SparsePtr<const Vector2f> sizePtr, SparsePtr<const Vector2f> sinCosPtr, SparsePtr<const float> alphaPtr)
	{
		NazaraAssert(material, "Invalid material");

		Vector2f defaultSinCos(0.f, 1.f); // sin(0) = 0, cos(0) = 1

		if (!sinCosPtr)
			sinCosPtr.Reset(&defaultSinCos, 0); // The trick here is to put the stride to zero, which leads the pointer to be immobile

		float defaultAlpha = 1.f;

		if (!alphaPtr)
			alphaPtr.Reset(&defaultAlpha, 0); // Same

		BillboardData* billboardData = GetBillboardData(renderOrder, material, count);
		for (unsigned int i = 0; i < count; ++i)
		{
			billboardData->center = *positionPtr++;
			billboardData->color = Color(255, 255, 255, static_cast<UInt8>(255.f * (*alphaPtr++)));
			billboardData->sinCos = *sinCosPtr++;
			billboardData->size = *sizePtr++;
			billboardData++;
		}
	}

	/*!
	* \brief Adds multiple billboards to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboards
	* \param count Number of billboards
	* \param positionPtr Position of the billboards
	* \param sizePtr Sizes of the billboards
	* \param anglePtr Rotation of the billboards if null, 0.f is used
	* \param colorPtr Color of the billboards if null, Color::White is used
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboards(int renderOrder, const Material* material, unsigned int count, SparsePtr<const Vector3f> positionPtr, SparsePtr<const Vector2f> sizePtr, SparsePtr<const float> anglePtr, SparsePtr<const Color> colorPtr)
	{
		NazaraAssert(material, "Invalid material");

		float defaultRotation = 0.f;

		if (!anglePtr)
			anglePtr.Reset(&defaultRotation, 0); // The trick here is to put the stride to zero, which leads the pointer to be immobile

		if (!colorPtr)
			colorPtr.Reset(&Color::White, 0); // Same

		BillboardData* billboardData = GetBillboardData(renderOrder, material, count);
		for (unsigned int i = 0; i < count; ++i)
		{
			float sin = std::sin(ToRadians(*anglePtr));
			float cos = std::cos(ToRadians(*anglePtr));
			anglePtr++;

			billboardData->center = *positionPtr++;
			billboardData->color = *colorPtr++;
			billboardData->sinCos.Set(sin, cos);
			billboardData->size = *sizePtr++;
			billboardData++;
		}
	}

	/*!
	* \brief Adds multiple billboards to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboards
	* \param count Number of billboards
	* \param positionPtr Position of the billboards
	* \param sizePtr Sizes of the billboards
	* \param anglePtr Rotation of the billboards if null, 0.f is used
	* \param alphaPtr Alpha parameters of the billboards if null, 1.f is used
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboards(int renderOrder, const Material* material, unsigned int count, SparsePtr<const Vector3f> positionPtr, SparsePtr<const Vector2f> sizePtr, SparsePtr<const float> anglePtr, SparsePtr<const float> alphaPtr)
	{
		NazaraAssert(material, "Invalid material");

		float defaultRotation = 0.f;

		if (!anglePtr)
			anglePtr.Reset(&defaultRotation, 0); // The trick here is to put the stride to zero, which leads the pointer to be immobile

		float defaultAlpha = 1.f;

		if (!alphaPtr)
			alphaPtr.Reset(&defaultAlpha, 0); // Same

		BillboardData* billboardData = GetBillboardData(renderOrder, material, count);
		for (unsigned int i = 0; i < count; ++i)
		{
			float sin = std::sin(ToRadians(*anglePtr));
			float cos = std::cos(ToRadians(*anglePtr));
			anglePtr++;

			billboardData->center = *positionPtr++;
			billboardData->color = Color(255, 255, 255, static_cast<UInt8>(255.f * (*alphaPtr++)));
			billboardData->sinCos.Set(sin, cos);
			billboardData->size = *sizePtr++;
			billboardData++;
		}
	}

	/*!
	* \brief Adds multiple billboards to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboards
	* \param count Number of billboards
	* \param positionPtr Position of the billboards
	* \param sizePtr Size of the billboards
	* \param sinCosPtr Rotation of the billboards if null, Vector2f(0.f, 1.f) is used
	* \param colorPtr Color of the billboards if null, Color::White is used
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboards(int renderOrder, const Material* material, unsigned int count, SparsePtr<const Vector3f> positionPtr, SparsePtr<const float> sizePtr, SparsePtr<const Vector2f> sinCosPtr, SparsePtr<const Color> colorPtr)
	{
		NazaraAssert(material, "Invalid material");

		Vector2f defaultSinCos(0.f, 1.f); // sin(0) = 0, cos(0) = 1

		if (!sinCosPtr)
			sinCosPtr.Reset(&defaultSinCos, 0); // The trick here is to put the stride to zero, which leads the pointer to be immobile

		if (!colorPtr)
			colorPtr.Reset(&Color::White, 0); // Same

		BillboardData* billboardData = GetBillboardData(renderOrder, material, count);
		for (unsigned int i = 0; i < count; ++i)
		{
			billboardData->center = *positionPtr++;
			billboardData->color = *colorPtr++;
			billboardData->sinCos = *sinCosPtr++;
			billboardData->size.Set(*sizePtr++);
			billboardData++;
		}
	}

	/*!
	* \brief Adds multiple billboards to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboards
	* \param count Number of billboards
	* \param positionPtr Position of the billboards
	* \param sizePtr Size of the billboards
	* \param sinCosPtr Rotation of the billboards if null, Vector2f(0.f, 1.f) is used
	* \param alphaPtr Alpha parameters of the billboards if null, 1.f is used
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboards(int renderOrder, const Material* material, unsigned int count, SparsePtr<const Vector3f> positionPtr, SparsePtr<const float> sizePtr, SparsePtr<const Vector2f> sinCosPtr, SparsePtr<const float> alphaPtr)
	{
		NazaraAssert(material, "Invalid material");

		Vector2f defaultSinCos(0.f, 1.f); // sin(0) = 0, cos(0) = 1

		if (!sinCosPtr)
			sinCosPtr.Reset(&defaultSinCos, 0); // The trick here is to put the stride to zero, which leads the pointer to be immobile

		float defaultAlpha = 1.f;

		if (!alphaPtr)
			alphaPtr.Reset(&defaultAlpha, 0); // Same

		BillboardData* billboardData = GetBillboardData(renderOrder, material, count);
		for (unsigned int i = 0; i < count; ++i)
		{
			billboardData->center = *positionPtr++;
			billboardData->color = Color(255, 255, 255, static_cast<UInt8>(255.f * (*alphaPtr++)));
			billboardData->sinCos = *sinCosPtr++;
			billboardData->size.Set(*sizePtr++);
			billboardData++;
		}
	}

	/*!
	* \brief Adds multiple billboards to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboards
	* \param count Number of billboards
	* \param positionPtr Position of the billboards
	* \param sizePtr Size of the billboards
	* \param anglePtr Rotation of the billboards if null, 0.f is used
	* \param colorPtr Color of the billboards if null, Color::White is used
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboards(int renderOrder, const Material* material, unsigned int count, SparsePtr<const Vector3f> positionPtr, SparsePtr<const float> sizePtr, SparsePtr<const float> anglePtr, SparsePtr<const Color> colorPtr)
	{
		NazaraAssert(material, "Invalid material");

		float defaultRotation = 0.f;

		if (!anglePtr)
			anglePtr.Reset(&defaultRotation, 0); // The trick here is to put the stride to zero, which leads the pointer to be immobile

		if (!colorPtr)
			colorPtr.Reset(&Color::White, 0); // Same

		BillboardData* billboardData = GetBillboardData(renderOrder, material, count);
		for (unsigned int i = 0; i < count; ++i)
		{
			float sin = std::sin(ToRadians(*anglePtr));
			float cos = std::cos(ToRadians(*anglePtr));
			anglePtr++;

			billboardData->center = *positionPtr++;
			billboardData->color = *colorPtr++;
			billboardData->sinCos.Set(sin, cos);
			billboardData->size.Set(*sizePtr++);
			billboardData++;
		}
	}

	/*!
	* \brief Adds multiple billboards to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboards
	* \param count Number of billboards
	* \param positionPtr Position of the billboards
	* \param sizePtr Size of the billboards
	* \param anglePtr Rotation of the billboards if null, 0.f is used
	* \param alphaPtr Alpha parameters of the billboards if null, 1.f is used
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddBillboards(int renderOrder, const Material* material, unsigned int count, SparsePtr<const Vector3f> positionPtr, SparsePtr<const float> sizePtr, SparsePtr<const float> anglePtr, SparsePtr<const float> alphaPtr)
	{
		NazaraAssert(material, "Invalid material");

		float defaultRotation = 0.f;

		if (!anglePtr)
			anglePtr.Reset(&defaultRotation, 0); // The trick here is to put the stride to zero, which leads the pointer to be immobile

		float defaultAlpha = 1.f;

		if (!alphaPtr)
			alphaPtr.Reset(&defaultAlpha, 0); // Same

		BillboardData* billboardData = GetBillboardData(renderOrder, material, count);
		for (unsigned int i = 0; i < count; ++i)
		{
			float sin = std::sin(ToRadians(*anglePtr));
			float cos = std::cos(ToRadians(*anglePtr));
			anglePtr++;

			billboardData->center = *positionPtr++;
			billboardData->color = Color(255, 255, 255, static_cast<UInt8>(255.f * (*alphaPtr++)));
			billboardData->sinCos.Set(sin, cos);
			billboardData->size.Set(*sizePtr++);
			billboardData++;
		}
	}

	/*!
	* \brief Adds drawable to the queue
	*
	* \param renderOrder Order of rendering
	* \param drawable Drawable user defined
	*
	* \remark Produces a NazaraError if drawable is invalid
	*/

	void ForwardRenderQueue::AddDrawable(int renderOrder, const Drawable* drawable)
	{
		#if NAZARA_GRAPHICS_SAFE
		if (!drawable)
		{
			NazaraError("Invalid drawable");
			return;
		}
		#endif

		auto& otherDrawables = GetLayer(renderOrder).otherDrawables;

		otherDrawables.push_back(drawable);
	}

	/*!
	* \brief Adds mesh to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the mesh
	* \param meshData Data of the mesh
	* \param meshAABB Box of the mesh
	* \param transformMatrix Matrix of the mesh
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddMesh(int renderOrder, const Material* material, const MeshData& meshData, const Boxf& meshAABB, const Matrix4f& transformMatrix)
	{
		NazaraAssert(material, "Invalid material");

		if (material->IsEnabled(RendererParameter_Blend))
		{
			Layer& currentLayer = GetLayer(renderOrder);
			auto& transparentModels = currentLayer.transparentModels;
			auto& transparentModelData = currentLayer.transparentModelData;

			// The material is transparent, we must draw this mesh using another way (after the rendering of opages objects while sorting them)
			unsigned int index = transparentModelData.size();
			transparentModelData.resize(index+1);

			TransparentModelData& data = transparentModelData.back();
			data.material = material;
			data.meshData = meshData;
			data.squaredBoundingSphere = Spheref(transformMatrix.GetTranslation() + meshAABB.GetCenter(), meshAABB.GetSquaredRadius());
			data.transformMatrix = transformMatrix;

			transparentModels.push_back(index);
		}
		else
		{
			Layer& currentLayer = GetLayer(renderOrder);
			auto& opaqueModels = currentLayer.opaqueModels;

			auto it = opaqueModels.find(material);
			if (it == opaqueModels.end())
			{
				BatchedModelEntry entry;
				entry.materialReleaseSlot.Connect(material->OnMaterialRelease, this, &ForwardRenderQueue::OnMaterialInvalidation);

				it = opaqueModels.insert(std::make_pair(material, std::move(entry))).first;
			}

			BatchedModelEntry& entry = it->second;
			entry.enabled = true;

			auto& meshMap = entry.meshMap;

			auto it2 = meshMap.find(meshData);
			if (it2 == meshMap.end())
			{
				MeshInstanceEntry instanceEntry;
				instanceEntry.squaredBoundingSphere = meshAABB.GetSquaredBoundingSphere();

				if (meshData.indexBuffer)
					instanceEntry.indexBufferReleaseSlot.Connect(meshData.indexBuffer->OnIndexBufferRelease, this, &ForwardRenderQueue::OnIndexBufferInvalidation);

				instanceEntry.vertexBufferReleaseSlot.Connect(meshData.vertexBuffer->OnVertexBufferRelease, this, &ForwardRenderQueue::OnVertexBufferInvalidation);

				it2 = meshMap.insert(std::make_pair(meshData, std::move(instanceEntry))).first;
			}

			std::vector<Matrix4f>& instances = it2->second.instances;
			instances.push_back(transformMatrix);

			// Do we have enough instances to perform instancing ?
			if (instances.size() >= NAZARA_GRAPHICS_INSTANCING_MIN_INSTANCES_COUNT)
				entry.instancingEnabled = true; // Thus we can activate it
		}
	}

	/*!
	* \brief Adds sprites to the queue
	*
	* \param renderOrder Order of rendering
	* \param material Material of the sprites
	* \param vertices Buffer of data for the sprites
	* \param spriteCount Number of sprites
	* \param overlay Texture of the sprites
	*
	* \remark Produces a NazaraAssert if material is invalid
	*/

	void ForwardRenderQueue::AddSprites(int renderOrder, const Material* material, const VertexStruct_XYZ_Color_UV* vertices, unsigned int spriteCount, const Texture* overlay)
	{
		NazaraAssert(material, "Invalid material");

		Layer& currentLayer = GetLayer(renderOrder);
		auto& basicSprites = currentLayer.basicSprites;

		auto matIt = basicSprites.find(material);
		if (matIt == basicSprites.end())
		{
			BatchedBasicSpriteEntry entry;
			entry.materialReleaseSlot.Connect(material->OnMaterialRelease, this, &ForwardRenderQueue::OnMaterialInvalidation);

			matIt = basicSprites.insert(std::make_pair(material, std::move(entry))).first;
		}

		BatchedBasicSpriteEntry& entry = matIt->second;
		entry.enabled = true;

		auto& overlayMap = entry.overlayMap;

		auto overlayIt = overlayMap.find(overlay);
		if (overlayIt == overlayMap.end())
		{
			BatchedSpriteEntry overlayEntry;
			if (overlay)
				overlayEntry.textureReleaseSlot.Connect(overlay->OnTextureRelease, this, &ForwardRenderQueue::OnTextureInvalidation);

			overlayIt = overlayMap.insert(std::make_pair(overlay, std::move(overlayEntry))).first;
		}

		auto& spriteVector = overlayIt->second.spriteChains;
		spriteVector.push_back(SpriteChain_XYZ_Color_UV({vertices, spriteCount}));
	}

	/*!
	* \brief Clears the queue
	*
	* \param fully Should everything be cleared or we can keep layers
	*/

	void ForwardRenderQueue::Clear(bool fully)
	{
		AbstractRenderQueue::Clear(fully);

		if (fully)
			layers.clear();
		else
		{
			for (auto it = layers.begin(); it != layers.end();)
			{
				Layer& layer = it->second;
				if (layer.clearCount++ >= 100)
					layers.erase(it++);
				else
				{
					layer.otherDrawables.clear();
					layer.transparentModels.clear();
					layer.transparentModelData.clear();
					++it;
				}
			}
		}
	}

	/*!
	* \brief Sorts the object according to the viewer position, furthest to nearest
	*
	* \param viewer Viewer of the scene
	*/

	void ForwardRenderQueue::Sort(const AbstractViewer* viewer)
	{
		Planef nearPlane = viewer->GetFrustum().GetPlane(FrustumPlane_Near);
		Vector3f viewerPos = viewer->GetEyePosition();
		Vector3f viewerNormal = viewer->GetForward();

		for (auto& pair : layers)
		{
			Layer& layer = pair.second;

			std::sort(layer.transparentModels.begin(), layer.transparentModels.end(), [&layer, &nearPlane, &viewerNormal] (unsigned int index1, unsigned int index2)
			{
				const Spheref& sphere1 = layer.transparentModelData[index1].squaredBoundingSphere;
				const Spheref& sphere2 = layer.transparentModelData[index2].squaredBoundingSphere;

				Vector3f position1 = sphere1.GetNegativeVertex(viewerNormal);
				Vector3f position2 = sphere2.GetNegativeVertex(viewerNormal);

				return nearPlane.Distance(position1) > nearPlane.Distance(position2);
			});

			for (auto& pair : layer.billboards)
			{
				const Material* mat = pair.first;

				if (mat->IsDepthSortingEnabled())
				{
					BatchedBillboardEntry& entry = pair.second;
					auto& billboardVector = entry.billboards;

					std::sort(billboardVector.begin(), billboardVector.end(), [&viewerPos] (const BillboardData& data1, const BillboardData& data2)
					{
						return viewerPos.SquaredDistance(data1.center) > viewerPos.SquaredDistance(data2.center);
					});
				}
			}
		}
	}

	/*!
	* \brief Gets the billboard data
	* \return Pointer to the data of the billboards
	*
	* \param renderOrder Order of rendering
	* \param material Material of the billboard
	*/

	ForwardRenderQueue::BillboardData* ForwardRenderQueue::GetBillboardData(int renderOrder, const Material* material, unsigned int count)
	{
		auto& billboards = GetLayer(renderOrder).billboards;

		auto it = billboards.find(material);
		if (it == billboards.end())
		{
			BatchedBillboardEntry entry;
			entry.materialReleaseSlot.Connect(material->OnMaterialRelease, this, &ForwardRenderQueue::OnMaterialInvalidation);

			it = billboards.insert(std::make_pair(material, std::move(entry))).first;
		}

		BatchedBillboardEntry& entry = it->second;

		auto& billboardVector = entry.billboards;
		unsigned int prevSize = billboardVector.size();
		billboardVector.resize(prevSize + count);

		return &billboardVector[prevSize];
	}

	/*!
	* \brief Gets the ith layer
	* \return Reference to the ith layer for the queue
	*
	* \param i Index of the layer
	*/

	ForwardRenderQueue::Layer& ForwardRenderQueue::GetLayer(int i)
	{
		auto it = layers.find(i);
		if (it == layers.end())
			it = layers.insert(std::make_pair(i, Layer())).first;
		
		Layer& layer = it->second;
		layer.clearCount = 0;

		return layer;
	}

	/*!
	* \brief Handle the invalidation of an index buffer
	*
	* \param indexBuffer Index buffer being invalidated
	*/

	void ForwardRenderQueue::OnIndexBufferInvalidation(const IndexBuffer* indexBuffer)
	{
		for (auto& pair : layers)
		{
			Layer& layer = pair.second;

			for (auto& modelPair : layer.opaqueModels)
			{
				MeshInstanceContainer& meshes = modelPair.second.meshMap;
				for (auto it = meshes.begin(); it != meshes.end();)
				{
					const MeshData& renderData = it->first;
					if (renderData.indexBuffer == indexBuffer)
						it = meshes.erase(it);
					else
						++it;
				}
			}
		}
	}

	/*!
	* \brief Handle the invalidation of a material
	*
	* \param material Material being invalidated
	*/

	void ForwardRenderQueue::OnMaterialInvalidation(const Material* material)
	{
		for (auto& pair : layers)
		{
			Layer& layer = pair.second;

			layer.basicSprites.erase(material);
			layer.billboards.erase(material);
			layer.opaqueModels.erase(material);
		}
	}

	/*!
	* \brief Handle the invalidation of a texture
	*
	* \param texture Texture being invalidated
	*/

	void ForwardRenderQueue::OnTextureInvalidation(const Texture* texture)
	{
		for (auto& pair : layers)
		{
			Layer& layer = pair.second;
			for (auto matIt = layer.basicSprites.begin(); matIt != layer.basicSprites.end(); ++matIt)
			{
				auto& overlayMap = matIt->second.overlayMap;
				overlayMap.erase(texture);
			}
		}
	}

	/*!
	* \brief Handle the invalidation of a vertex buffer
	*
	* \param vertexBuffer Vertex buffer being invalidated
	*/

	void ForwardRenderQueue::OnVertexBufferInvalidation(const VertexBuffer* vertexBuffer)
	{
		for (auto& pair : layers)
		{
			Layer& layer = pair.second;
			for (auto& modelPair : layer.opaqueModels)
			{
				MeshInstanceContainer& meshes = modelPair.second.meshMap;
				for (auto it = meshes.begin(); it != meshes.end();)
				{
					const MeshData& renderData = it->first;
					if (renderData.vertexBuffer == vertexBuffer)
						it = meshes.erase(it);
					else
						++it;
				}
			}
		}
	}

	/*!
	* \brief Functor to compare two batched billboard with material
	* \return true If first material is "smaller" than the second one
	*
	* \param mat1 First material to compare
	* \param mat2 Second material to compare
	*/

	bool ForwardRenderQueue::BatchedBillboardComparator::operator()(const Material* mat1, const Material* mat2) const
	{
		const UberShader* uberShader1 = mat1->GetShader();
		const UberShader* uberShader2 = mat2->GetShader();
		if (uberShader1 != uberShader2)
			return uberShader1 < uberShader2;

		const Shader* shader1 = mat1->GetShaderInstance(ShaderFlags_Billboard | ShaderFlags_VertexColor)->GetShader();
		const Shader* shader2 = mat2->GetShaderInstance(ShaderFlags_Billboard | ShaderFlags_VertexColor)->GetShader();
		if (shader1 != shader2)
			return shader1 < shader2;

		const Texture* diffuseMap1 = mat1->GetDiffuseMap();
		const Texture* diffuseMap2 = mat2->GetDiffuseMap();
		if (diffuseMap1 != diffuseMap2)
			return diffuseMap1 < diffuseMap2;

		return mat1 < mat2;
	}

	/*!
	* \brief Functor to compare two batched model with material
	* \return true If first material is "smaller" than the second one
	*
	* \param mat1 First material to compare
	* \param mat2 Second material to compare
	*/

	bool ForwardRenderQueue::BatchedModelMaterialComparator::operator()(const Material* mat1, const Material* mat2) const
	{
		const UberShader* uberShader1 = mat1->GetShader();
		const UberShader* uberShader2 = mat2->GetShader();
		if (uberShader1 != uberShader2)
			return uberShader1 < uberShader2;

		const Shader* shader1 = mat1->GetShaderInstance()->GetShader();
		const Shader* shader2 = mat2->GetShaderInstance()->GetShader();
		if (shader1 != shader2)
			return shader1 < shader2;

		const Texture* diffuseMap1 = mat1->GetDiffuseMap();
		const Texture* diffuseMap2 = mat2->GetDiffuseMap();
		if (diffuseMap1 != diffuseMap2)
			return diffuseMap1 < diffuseMap2;

		return mat1 < mat2;
	}

	/*!
	* \brief Functor to compare two batched sprites with material
	* \return true If first material is "smaller" than the second one
	*
	* \param mat1 First material to compare
	* \param mat2 Second material to compare
	*/

	bool ForwardRenderQueue::BatchedSpriteMaterialComparator::operator()(const Material* mat1, const Material* mat2)
	{
		const UberShader* uberShader1 = mat1->GetShader();
		const UberShader* uberShader2 = mat2->GetShader();
		if (uberShader1 != uberShader2)
			return uberShader1 < uberShader2;

		const Shader* shader1 = mat1->GetShaderInstance()->GetShader();
		const Shader* shader2 = mat2->GetShaderInstance()->GetShader();
		if (shader1 != shader2)
			return shader1 < shader2;

		const Texture* diffuseMap1 = mat1->GetDiffuseMap();
		const Texture* diffuseMap2 = mat2->GetDiffuseMap();
		if (diffuseMap1 != diffuseMap2)
			return diffuseMap1 < diffuseMap2;

		return mat1 < mat2;
	}

	/*!
	* \brief Functor to compare two mesh data
	* \return true If first mesh is "smaller" than the second one
	*
	* \param data1 First mesh to compare
	* \param data2 Second mesh to compare
	*/

	bool ForwardRenderQueue::MeshDataComparator::operator()(const MeshData& data1, const MeshData& data2) const
	{
		const Buffer* buffer1;
		const Buffer* buffer2;

		buffer1 = (data1.indexBuffer) ? data1.indexBuffer->GetBuffer() : nullptr;
		buffer2 = (data2.indexBuffer) ? data2.indexBuffer->GetBuffer() : nullptr;
		if (buffer1 != buffer2)
			return buffer1 < buffer2;

		buffer1 = data1.vertexBuffer->GetBuffer();
		buffer2 = data2.vertexBuffer->GetBuffer();
		if (buffer1 != buffer2)
			return buffer1 < buffer2;

		return data1.primitiveMode < data2.primitiveMode;
	}
}
