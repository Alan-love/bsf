//************************************ bs::framework - Copyright 2018 Marko Pintera **************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#include "GUI/BsGUIWidget.h"
#include "GUI/BsGUIManager.h"
#include "GUI/BsGUISkin.h"
#include "GUI/BsGUILabel.h"
#include "GUI/BsGUIPanel.h"
#include "GUI/BsGUINavGroup.h"
#include "Math/BsVector2I.h"
#include "Mesh/BsMesh.h"
#include "Components/BsCCamera.h"
#include "RenderAPI/BsViewport.h"
#include "Scene/BsSceneObject.h"
#include "Resources/BsBuiltinResources.h"
#include "RenderAPI/BsVertexDataDesc.h"

namespace bs
{
	namespace impl
	{
		SPtr<VertexDataDesc> gGUITriangleMeshDesc()
		{
			static SPtr<VertexDataDesc> sDesc;

			if(!sDesc)
			{
				sDesc = bs_shared_ptr_new<VertexDataDesc>();
				sDesc->addVertElem(VET_FLOAT2, VES_POSITION);
				sDesc->addVertElem(VET_FLOAT2, VES_TEXCOORD);
			}

			return sDesc;
		}

		SPtr<VertexDataDesc> gGUILineMeshDesc()
		{
			static SPtr<VertexDataDesc> sDesc;

			if(!sDesc)
			{
				sDesc = bs_shared_ptr_new<VertexDataDesc>();
				sDesc->addVertElem(VET_FLOAT2, VES_POSITION);
			}

			return sDesc;
			
		}
	}
	
	GUIDrawGroups::GUIDrawGroups()
	{
		GUIDrawGroup mainDrawGroup;
		mainDrawGroup.minDepth = 0;
		mainDrawGroup.depthRange = std::numeric_limits<UINT32>::max();
		mainDrawGroup.id = mNextDrawGroupId++;
		
		mEntries.push_back(mainDrawGroup);
	}

	void GUIDrawGroups::add(GUIElement* element)
	{
		UINT32 numRenderElems = element->_getNumRenderElements();

		for (UINT32 i = 0; i < numRenderElems; i++)
		{
			UINT32 elemDepth = element->_getRenderElementDepth(i);
			Rect2I bounds = element->_getClippedBounds();

			SpriteMaterial* spriteMaterial = nullptr;
			element->_getMaterial(i, &spriteMaterial);
			assert(spriteMaterial != nullptr);

			// Groups are expected to be sorted by minDepth
			for (UINT32 j = 0; j < (UINT32)mEntries.size(); j++)
			{
				GUIDrawGroup& group = mEntries[j];
				if (elemDepth < group.minDepth)
					continue;

				if (spriteMaterial->allowBatching())
				{
					group.cachedElements.push_back(GUIGroupElement(element, i));
					group.bounds.encapsulate(bounds);
					element->_setDrawGroupId(group.id);
				}
				else
				{
					bool needsSplit = elemDepth != group.minDepth;
					if (needsSplit)
					{
						GUIDrawGroup& newGroup = split(j, elemDepth);

						newGroup.nonCachedElements.push_back(GUIGroupElement(element, i));
						element->_setDrawGroupId(newGroup.id);
					}
					else
					{
						group.nonCachedElements.push_back(GUIGroupElement(element, i));
						element->_setDrawGroupId(group.id);
					}
				}

				break;
			}
		}
	}

	void GUIDrawGroups::remove(GUIElement* element)
	{
		UINT32 drawGroupId = element->_getDrawGroupId();
		auto iterFind2 = std::find_if(mEntries.begin(), mEntries.end(),
			[drawGroupId](const GUIDrawGroup& group) { return group.id == drawGroupId; });

		assert(iterFind2 != mEntries.end());
		if(iterFind2 != mEntries.end())
		{
			GUIDrawGroup& group = *iterFind2;

			group.cachedElements.erase(std::remove_if(group.cachedElements.begin(), group.cachedElements.end(),
				[element](const GUIGroupElement& x) { return x.element == element; }), group.cachedElements.end());
			
			group.nonCachedElements.erase(std::remove_if(group.nonCachedElements.begin(), group.nonCachedElements.end(),
				[element](const GUIGroupElement& x) { return x.element == element; }), group.nonCachedElements.end());

			if (element->_isVisible())
				group.dirtyBounds = true;

			// Attempt to merge with previous group
			if(group.nonCachedElements.empty() && group.minDepth > 0)
			{
				assert(iterFind2 != mEntries.begin());

				auto prevGroupIter = iterFind2 - 1;
				GUIDrawGroup& prevGroup = *prevGroupIter;

				prevGroup.depthRange += group.depthRange;

				for (auto& entry : group.cachedElements)
				{
					entry.element->_setDrawGroupId(prevGroup.id);

					if(entry.element->_isVisible())
						prevGroup.dirtyBounds = true;
				}
				
				std::move(group.cachedElements.begin(), group.cachedElements.end(), std::back_inserter(prevGroup.cachedElements));
				mEntries.erase(iterFind2);
			}
		}
	}

	void GUIDrawGroups::rebuildDirty()
	{
		for(auto& entry : mEntries)
		{
			if(entry.dirtyBounds)
			{
				updateBounds(entry);
				entry.dirtyBounds = false;

				// TODO - Update texture if needed
			}
		}
		
		// TODO
	}

	void GUIDrawGroups::rebuildMeshes()
	{
		struct GUIMaterialGroup
		{
			SpriteMaterial* material;
			SpriteMaterialInfo matInfo;
			GUIMeshType meshType;
			UINT32 numVertices;
			UINT32 numIndices;
			UINT32 depth;
			UINT32 minDepth;
			Rect2I bounds;
			Vector<GUIGroupElement> elements;
		};

		struct GUIMaterialGroupSet
		{
			using SortedGroupSet = FrameSet<GUIMaterialGroup*, std::function<bool(GUIMaterialGroup*, GUIMaterialGroup*)>>;
			
			UINT32 numMeshes = 0;
			UINT32 numIndices[2] = { 0, 0 };
			UINT32 numVertices[2] = { 0, 0 };

			SortedGroupSet sortedGroups;
		};
		
		bs_frame_mark();
		{
			// Make a list of all GUI elements, sorted from farthest to nearest (highest depth to lowest)
			auto elemComp = [](const GUIGroupElement& a, const GUIGroupElement& b)
			{
				UINT32 aDepth = a.element->_getRenderElementDepth(a.renderElement);
				UINT32 bDepth = b.element->_getRenderElementDepth(b.renderElement);

				// Compare pointers just to differentiate between two elements with the same depth, their order doesn't really matter, but std::set
				// requires all elements to be unique
				return (aDepth > bDepth) ||
					(aDepth == bDepth && a.element > b.element) ||
					(aDepth == bDepth && a.element == b.element && a.renderElement > b.renderElement);
			};

			FrameVector<GUIMaterialGroupSet> groupSets;
			groupSets.reserve(mEntries.size());

			for (auto& entry : mEntries)
			{
				// Note: If we keep visible elements separate from invisible, plus provide sorting on insert, we could avoid this
				// re-sorting and re-inserting step.
				FrameSet<GUIGroupElement, std::function<bool(const GUIGroupElement&, const GUIGroupElement&)>> allElements(elemComp);
				for (auto& element : entry.cachedElements)
				{
					if (!element.element->_isVisible())
						continue;

					allElements.insert(element);
				}

				for (auto& element : entry.nonCachedElements)
				{
					if (!element.element->_isVisible())
						continue;

					allElements.insert(element);
				}

				// Group the elements in such a way so that we end up with a smallest amount of
				// meshes, without breaking back to front rendering order
				FrameUnorderedMap<UINT64, FrameVector<GUIMaterialGroup>> materialGroups;
				for (auto& elem : allElements)
				{
					GUIElement* guiElem = elem.element;
					UINT32 renderElemIdx = elem.renderElement;
					UINT32 elemDepth = guiElem->_getRenderElementDepth(renderElemIdx);
					Rect2I bounds = guiElem->_getClippedBounds();

					SpriteMaterial* spriteMaterial = nullptr;
					const SpriteMaterialInfo& matInfo = guiElem->_getMaterial(renderElemIdx, &spriteMaterial);
					assert(spriteMaterial != nullptr);

					UINT64 hash = spriteMaterial->getMergeHash(matInfo);
					FrameVector<GUIMaterialGroup>& groupsPerMaterial = materialGroups[hash];

					// Try to find a group this material will fit in:
					//  - Group that has a depth value same or one below elements depth will always be a match
					//  - Otherwise, we search higher depth values as well, but we only use them if no elements in between those depth values
					//    overlap the current elements bounds.
					GUIMaterialGroup* foundGroup = nullptr;

					if (spriteMaterial->allowBatching())
					{
						for (auto groupIter = groupsPerMaterial.rbegin(); groupIter != groupsPerMaterial.rend(); ++groupIter)
						{
							GUIMaterialGroup& group = *groupIter;
							if (group.depth == elemDepth)
							{
								foundGroup = &group;
								break;
							}
							else
							{
								UINT32 startDepth = elemDepth;
								UINT32 endDepth = group.depth;

								Rect2I potentialGroupBounds = group.bounds;
								potentialGroupBounds.encapsulate(bounds);

								bool foundOverlap = false;
								for (auto& material : materialGroups)
								{
									for (auto& matGroup : material.second)
									{
										if (&matGroup == &group)
											continue;

										if ((matGroup.minDepth >= startDepth && matGroup.minDepth <= endDepth)
											|| (matGroup.depth >= startDepth && matGroup.depth <= endDepth))
										{
											if (matGroup.bounds.overlaps(potentialGroupBounds))
											{
												foundOverlap = true;
												break;
											}
										}
									}
								}

								if (!foundOverlap)
								{
									foundGroup = &group;
									break;
								}
							}
						}
					}

					if (foundGroup == nullptr)
					{
						groupsPerMaterial.push_back(GUIMaterialGroup());
						foundGroup = &groupsPerMaterial[groupsPerMaterial.size() - 1];

						foundGroup->depth = elemDepth;
						foundGroup->minDepth = elemDepth;
						foundGroup->bounds = bounds;
						foundGroup->elements.push_back(GUIGroupElement(guiElem, renderElemIdx));
						foundGroup->matInfo = matInfo.clone();
						foundGroup->material = spriteMaterial;

						guiElem->_getMeshInfo(renderElemIdx, foundGroup->numVertices, foundGroup->numIndices, foundGroup->meshType);
					}
					else
					{
						foundGroup->bounds.encapsulate(bounds);
						foundGroup->elements.push_back(GUIGroupElement(guiElem, renderElemIdx));
						foundGroup->minDepth = std::min(foundGroup->minDepth, elemDepth);

						UINT32 numVertices;
						UINT32 numIndices;
						GUIMeshType meshType;
						guiElem->_getMeshInfo(renderElemIdx, numVertices, numIndices, meshType);
						assert(meshType == foundGroup->meshType); // It's expected that GUI element doesn't use same material for different mesh types so this should always be true

						foundGroup->numVertices += numVertices;
						foundGroup->numIndices += numIndices;

						spriteMaterial->merge(foundGroup->matInfo, matInfo);
					}
				}

				// Make a list of all GUI elements, sorted from farthest to nearest (highest depth to lowest)
				auto groupComp = [](GUIMaterialGroup* a, GUIMaterialGroup* b)
				{
					return (a->depth > b->depth) || (a->depth == b->depth && a > b);
					// Compare pointers just to differentiate between two elements with the same depth, their order doesn't really matter, but std::set
					// requires all elements to be unique
				};

				groupSets.push_back(GUIMaterialGroupSet());
				GUIMaterialGroupSet& groupSet = groupSets.back();

				groupSet.sortedGroups = GUIMaterialGroupSet::SortedGroupSet(groupComp);
				for (auto& material : materialGroups)
				{
					for (auto& group : material.second)
					{
						groupSet.sortedGroups.insert(&group);

						UINT32 typeIdx = (UINT32)group.meshType;
						groupSet.numIndices[typeIdx] += group.numIndices;
						groupSet.numVertices[typeIdx] += group.numVertices;

						groupSet.numMeshes++;
					}
				}
			}

			mTriangleMesh = nullptr;
			mLineMesh = nullptr;

			UINT32 totalNumIndices[2] = { 0, 0 };
			UINT32 totalNumVertices[2] = { 0, 0 };

			for (UINT32 i = 0; i < (UINT32)mEntries.size(); i++)
			{
				GUIMaterialGroupSet& set = groupSets[i];
				mEntries[i].meshes.resize(set.numMeshes);

				for (UINT32 j = 0; j < 2; j++)
				{
					totalNumIndices[j] += set.numIndices[j];
					totalNumVertices[j] += set.numVertices[j];
				}
			}

			SPtr<MeshData> meshData[2];
			SPtr<VertexDataDesc> vertexDesc[2] = { impl::gGUITriangleMeshDesc(), impl::gGUILineMeshDesc() };

			UINT8* vertices[2] = { nullptr, nullptr };
			UINT32* indices[2] = { nullptr, nullptr };

			for (UINT32 i = 0; i < 2; i++)
			{
				if (totalNumVertices[i] > 0 && totalNumIndices[i] > 0)
				{
					meshData[i] = MeshData::create(totalNumVertices[i], totalNumIndices[i], vertexDesc[i]);

					vertices[i] = meshData[i]->getElementData(VES_POSITION);
					indices[i] = meshData[i]->getIndices32();
				}
			}

			UINT32 vertexOffset[2] = { 0, 0 };
			UINT32 indexOffset[2] = { 0, 0 };

			for (UINT32 i = 0; i < (UINT32)mEntries.size(); i++)
			{
				GUIMaterialGroupSet& set = groupSets[i];

				// Fill buffers for each group and update their meshes
				UINT32 meshIdx = 0;

				for (auto& group : set.sortedGroups)
				{
					GUIMesh& guiMesh = mEntries[i].meshes[meshIdx];
					guiMesh.matInfo = group->matInfo;
					guiMesh.material = group->material;
					guiMesh.isLine = group->meshType == GUIMeshType::Line;

					auto typeIdx = (UINT32)group->meshType;
					guiMesh.indexOffset = indexOffset[typeIdx];

					UINT32 groupNumIndices = 0;
					for (auto& matElement : group->elements)
					{
						matElement.element->_fillBuffer(
							vertices[typeIdx], indices[typeIdx],
							vertexOffset[typeIdx], indexOffset[typeIdx],
							totalNumVertices[typeIdx], totalNumVertices[typeIdx], matElement.renderElement);

						UINT32 elemNumVertices;
						UINT32 elemNumIndices;
						GUIMeshType meshType;
						matElement.element->_getMeshInfo(matElement.renderElement, elemNumVertices, elemNumIndices, meshType);

						UINT32 indexStart = indexOffset[typeIdx];
						UINT32 indexEnd = indexStart + elemNumIndices;

						for (UINT32 j = indexStart; j < indexEnd; j++)
							indices[typeIdx][j] += vertexOffset[typeIdx];

						indexOffset[typeIdx] += elemNumIndices;
						vertexOffset[typeIdx] += elemNumVertices;

						groupNumIndices += elemNumIndices;
					}

					guiMesh.indexCount = groupNumIndices;

					meshIdx++;
				}
			}

			if (meshData[0])
				mTriangleMesh = Mesh::_createPtr(meshData[0], MU_STATIC, DOT_TRIANGLE_LIST);

			if (meshData[1])
				mLineMesh = Mesh::_createPtr(meshData[1], MU_STATIC, DOT_LINE_LIST);
		}

		bs_frame_clear();
	}



	// TODO - Need to handle the case when element depth, reneder element count or material change - Update the draw groups
	// TODO - Need to handle the case where element size changes - Update the draw group bounds

	GUIDrawGroups::GUIDrawGroup& GUIDrawGroups::split(UINT32 groupIdx, UINT32 depth)
	{
		GUIDrawGroup& group = mEntries[groupIdx];
		assert(depth > group.minDepth);
		
		UINT32 maxDepth = group.minDepth + group.depthRange;
		group.depthRange = depth - group.minDepth;

		GUIDrawGroup newSplitGroup;
		newSplitGroup.minDepth = depth;
		newSplitGroup.depthRange = maxDepth - newSplitGroup.minDepth + 1;
		newSplitGroup.id = mNextDrawGroupId++;

		auto it = std::partition(group.cachedElements.begin(), group.cachedElements.end(),
			[depth](const GUIGroupElement& x)
		{
				UINT32 elemDepth = x.element->_getRenderElementDepth(x.renderElement);
				return elemDepth < depth;
		});

		std::move(it, group.cachedElements.end(), std::back_inserter(newSplitGroup.cachedElements));
		group.cachedElements.erase(it, group.cachedElements.end());

		for (auto& entry : newSplitGroup.cachedElements)
			entry.element->_setDrawGroupId(newSplitGroup.id);

		group.dirtyBounds = true;
		newSplitGroup.dirtyBounds = true;

		mEntries.insert(mEntries.begin() + groupIdx, std::move(newSplitGroup));
		return mEntries[groupIdx + 1];
	}

	void GUIDrawGroups::updateBounds(GUIDrawGroup& group)
	{
		group.bounds = Rect2I();

		for(auto& entry : group.cachedElements)
		{
			if (!entry.element->_isVisible())
				continue;
			
			Rect2I bounds = entry.element->_getClippedBounds();
			group.bounds.encapsulate(bounds);
		}
	}

	GUIWidget::GUIWidget(const SPtr<Camera>& camera)
		: mCamera(camera)
	{
		construct(camera);
	}

	GUIWidget::GUIWidget(const HCamera& camera)
		: mCamera(camera->_getCamera())
	{
		construct(mCamera);
	}

	void GUIWidget::construct(const SPtr<Camera>& camera)
	{
		if (mCamera != nullptr)
		{
			SPtr<RenderTarget> target = mCamera->getViewport()->getTarget();

			if (target != nullptr)
				mCachedRTId = target->getInternalID();
		}

		mDefaultNavGroup = GUINavGroup::create();

		GUIManager::instance().registerWidget(this);

		mPanel = GUIPanel::create();
		mPanel->_changeParentWidget(this);
		updateRootPanel();
	}

	GUIWidget::~GUIWidget()
	{
		_destroy();
	}

	SPtr<GUIWidget> GUIWidget::create(const SPtr<Camera>& camera)
	{
		return bs_shared_ptr(new (bs_alloc<GUIWidget>()) GUIWidget(camera));
	}

	SPtr<GUIWidget> GUIWidget::create(const HCamera& camera)
	{
		return bs_shared_ptr(new (bs_alloc<GUIWidget>()) GUIWidget(camera));
	}

	void GUIWidget::_destroy()
	{
		if (mPanel != nullptr)
		{
			GUILayout::destroy(mPanel);
			mPanel = nullptr;
		}

		if (mCamera != nullptr)
		{
			GUIManager::instance().unregisterWidget(this);
			mCamera = nullptr;
		}

		mElements.clear();
		mDirtyContents.clear();
	}

	void GUIWidget::setDepth(UINT8 depth)
	{
		mDepth = depth;
		mWidgetIsDirty = true;

		updateRootPanel();
	}

	Viewport* GUIWidget::getTarget() const
	{
		if(mCamera != nullptr)
			return mCamera->getViewport().get();

		return nullptr;
	}

	void GUIWidget::_updateTransform(const HSceneObject& parent)
	{
		// If the widgets parent scene object moved, we need to mark it as dirty
		// as the GUIManager batching relies on object positions, so it needs to be updated.
		const float diffEpsilon = 0.0001f;

		const Transform& tfrm = parent->getTransform();
		Vector3 position = tfrm.getPosition();
		Quaternion rotation = tfrm.getRotation();
		Vector3 scale = tfrm.getScale();

		if(!mWidgetIsDirty)
		{
			if(!Math::approxEquals(mPosition, position, diffEpsilon))
				mWidgetIsDirty = true;
			else
			{
				if(!Math::approxEquals(mRotation, rotation, diffEpsilon))
					mWidgetIsDirty = true;
				else
				{
					if(Math::approxEquals(mScale, scale))
						mWidgetIsDirty = true;
				}
			}
		}

		mPosition = position;
		mRotation = rotation;
		mScale = scale;
		mTransform = parent->getWorldMatrix();
	}

	void GUIWidget::_updateRT()
	{
		SPtr<RenderTarget> rt;
		UINT64 newRTId = 0;
		if(mCamera != nullptr)
		{
			rt = mCamera->getViewport()->getTarget();
			if (rt != nullptr)
				newRTId = rt->getInternalID();
		}

		if(mCachedRTId != newRTId)
		{
			mCachedRTId = newRTId;
			updateRootPanel();
		}
	}

	void GUIWidget::_updateLayout()
	{
		// Check if render target size changed and update if needed
		// Note: Purposely not relying to the RenderTarget::onResized callback, as it will trigger /before/ Input events.
		// These events might trigger a resize, meaning the size would be delayed one frame, resulting in a visual artifact
		// where the GUI doesn't match the target size.
		Viewport* target = getTarget();
		if (target != nullptr)
		{
			Rect2I area = target->getPixelArea();
			UINT32 width = area.width;
			UINT32 height = area.height;

			const Rect2I& panelArea = mPanel->_getLayoutData().area;
			if(panelArea.width != width || panelArea.height != height)
			{
				updateRootPanel();
				onOwnerTargetResized();
			}
		}

		bs_frame_mark();

		// Determine dirty contents and layouts
		FrameStack<GUIElementBase*> todo;
		todo.push(mPanel);

		while (!todo.empty())
		{
			GUIElementBase* currentElem = todo.top();
			todo.pop();

			if (currentElem->_isDirty())
			{
				GUIElementBase* updateParent = currentElem->_getUpdateParent();
				assert(updateParent != nullptr || currentElem == mPanel);

				if (updateParent != nullptr)
					_updateLayout(updateParent);
				else // Must be root panel
					_updateLayout(mPanel);
			}
			else
			{
				UINT32 numChildren = currentElem->_getNumChildren();
				for (UINT32 i = 0; i < numChildren; i++)
					todo.push(currentElem->_getChild(i));
			}
		}

		bs_frame_clear();
	}

	void GUIWidget::_updateLayout(GUIElementBase* elem)
	{
		GUIElementBase* parent = elem->_getParent();
		bool isPanelOptimized = parent != nullptr && parent->_getType() == GUIElementBase::Type::Panel;

		GUIElementBase* updateParent = nullptr;

		if (isPanelOptimized)
			updateParent = parent;
		else
			updateParent = elem;

		// For GUIPanel we can do a an optimization and update only the element in question instead
		// of all the children
		if (isPanelOptimized)
		{
			GUIPanel* panel = static_cast<GUIPanel*>(updateParent);

			GUIElementBase* dirtyElement = elem;
			dirtyElement->_updateOptimalLayoutSizes();

			LayoutSizeRange elementSizeRange = panel->_getElementSizeRange(dirtyElement);
			Rect2I elementArea = panel->_getElementArea(panel->_getLayoutData().area, dirtyElement, elementSizeRange);

			GUILayoutData childLayoutData = panel->_getLayoutData();
			panel->_updateDepthRange(childLayoutData);
			childLayoutData.area = elementArea;

			panel->_updateChildLayout(dirtyElement, childLayoutData);
		}
		else
		{
			GUILayoutData childLayoutData = updateParent->_getLayoutData();
			updateParent->_updateLayout(childLayoutData);
		}
		
		// Mark dirty contents
		bs_frame_mark();
		{
			FrameStack<GUIElementBase*> todo;
			todo.push(elem);

			while (!todo.empty())
			{
				GUIElementBase* currentElem = todo.top();
				todo.pop();

				if (currentElem->_getType() == GUIElementBase::Type::Element)
					mDirtyContents.insert(static_cast<GUIElement*>(currentElem));

				currentElem->_markAsClean();

				UINT32 numChildren = currentElem->_getNumChildren();
				for (UINT32 i = 0; i < numChildren; i++)
					todo.push(currentElem->_getChild(i));
			}
		}
		bs_frame_clear();
	}

	void GUIWidget::_registerElement(GUIElementBase* elem)
	{
		assert(elem != nullptr && !elem->_isDestroyed());

		if (elem->_getType() == GUIElementBase::Type::Element)
		{
			mElements.push_back(static_cast<GUIElement*>(elem));
			mWidgetIsDirty = true;

			// Find a draw group
			auto guiElem = static_cast<GUIElement*>(elem);
			mDrawGroups.add(guiElem);
		}
	}

	void GUIWidget::_unregisterElement(GUIElementBase* elem)
	{
		assert(elem != nullptr);

		auto iterFind = std::find(begin(mElements), end(mElements), elem);

		if (iterFind != mElements.end())
		{
			mElements.erase(iterFind);
			mWidgetIsDirty = true;
		}

		if (elem->_getType() == GUIElementBase::Type::Element)
		{
			mDirtyContents.erase(static_cast<GUIElement*>(elem));

			auto guiElem = static_cast<GUIElement*>(elem);
			mDrawGroups.remove(guiElem);
		}
	}

	void GUIWidget::_markMeshDirty(GUIElementBase* elem)
	{
		mWidgetIsDirty = true;
	}

	void GUIWidget::_markContentDirty(GUIElementBase* elem)
	{
		if (elem->_getType() == GUIElementBase::Type::Element)
			mDirtyContents.insert(static_cast<GUIElement*>(elem));
	}

	void GUIWidget::setSkin(const HGUISkin& skin)
	{
		mSkin = skin;

		for(auto& element : mElements)
			element->_refreshStyle();
	}

	const GUISkin& GUIWidget::getSkin() const
	{
		if(mSkin.isLoaded())
			return *mSkin;
		else
			return *BuiltinResources::instance().getGUISkin();
	}

	void GUIWidget::setCamera(const SPtr<Camera>& camera)
	{
		SPtr<Camera> newCamera = camera;
		if(newCamera != nullptr)
		{
			if (newCamera->getViewport()->getTarget() == nullptr)
				newCamera = nullptr;
		}

		if (mCamera == newCamera)
			return;

		GUIManager::instance().unregisterWidget(this);
		mCamera = newCamera;
		GUIManager::instance().registerWidget(this);

		updateRootPanel();
	}

	void GUIWidget::setIsActive(bool active)
	{
		mIsActive = active;
	}

	bool GUIWidget::isDirty(bool cleanIfDirty)
	{
		if (!mIsActive)
			return false;

		const bool dirty = mWidgetIsDirty || !mDirtyContents.empty();

		if(cleanIfDirty && dirty)
		{
			mWidgetIsDirty = false;

			// Update render contents recursively because updates can cause child GUI elements to become dirty
			while(!mDirtyContents.empty())
			{
				mDirtyContentsTemp.swap(mDirtyContents);

				for (auto& dirtyElement : mDirtyContentsTemp)
					dirtyElement->_updateRenderElements();

				mDirtyContentsTemp.clear();
			}

			updateBounds();
		}
		
		return dirty;
	}

	bool GUIWidget::inBounds(const Vector2I& position) const
	{
		Viewport* target = getTarget();
		if (target == nullptr)
			return false;

		// Technically GUI widget bounds can be larger than the viewport, so make sure we clip to viewport first
		if(!target->getPixelArea().contains(position))
			return false;

		Vector3 vecPos((float)position.x, (float)position.y, 0.0f);
		vecPos = mTransform.inverse().multiplyAffine(vecPos);

		Vector2I localPos(Math::roundToInt(vecPos.x), Math::roundToInt(vecPos.y));
		return mBounds.contains(localPos);
	}

	void GUIWidget::updateBounds() const
	{
		if(!mElements.empty())
			mBounds = mElements[0]->_getClippedBounds();

		for(auto& elem : mElements)
		{
			Rect2I elemBounds = elem->_getClippedBounds();
			mBounds.encapsulate(elemBounds);
		}
	}

	void GUIWidget::ownerWindowFocusChanged()
	{
		onOwnerWindowFocusChanged();
	}

	void GUIWidget::updateRootPanel()
	{
		Viewport* target = getTarget();
		if (target == nullptr)
			return;

		Rect2I area = target->getPixelArea();
		UINT32 width = area.width;
		UINT32 height = area.height;

		GUILayoutData layoutData;
		layoutData.area.width = width;
		layoutData.area.height = height;
		layoutData.clipRect = Rect2I(0, 0, width, height);
		layoutData.setWidgetDepth(mDepth);

		mPanel->setWidth(width);
		mPanel->setHeight(height);

		mPanel->_setLayoutData(layoutData);
		mPanel->_markLayoutAsDirty();
	}
}
