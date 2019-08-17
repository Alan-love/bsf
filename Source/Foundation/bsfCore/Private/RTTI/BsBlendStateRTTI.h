//************************************ bs::framework - Copyright 2018 Marko Pintera **************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#pragma once

#include "BsCorePrerequisites.h"
#include "Reflection/BsRTTIType.h"
#include "RenderAPI/BsBlendState.h"
#include "Managers/BsRenderStateManager.h"

namespace bs
{
	/** @cond RTTI */
	/** @addtogroup RTTI-Impl-Core
	 *  @{
	 */

	class BS_CORE_EXPORT BlendStateRTTI : public RTTIType<BlendState, IReflectable, BlendStateRTTI>
	{
	private:
		BLEND_STATE_DESC& getData(BlendState* obj) { return obj->mProperties.mData; }
		void setData(BlendState* obj, BLEND_STATE_DESC& val) { obj->mProperties.mData = val; }

	public:
		BlendStateRTTI()
		{
			addPlainField("mData", 0, &BlendStateRTTI::getData, &BlendStateRTTI::setData);
		}

		void onDeserializationEnded(IReflectable* obj, SerializationContext* context) override
		{
			BlendState* blendState = static_cast<BlendState*>(obj);
			blendState->initialize();
		}

		const String& getRTTIName() override
		{
			static String name = "BlendState";
			return name;
		}

		UINT32 getRTTIId() override
		{
			return TID_BlendState;
		}

		SPtr<IReflectable> newRTTIObject() override
		{
			return RenderStateManager::instance()._createBlendStatePtr(BLEND_STATE_DESC());
		}
	};

	template<> struct RTTIPlainType<BLEND_STATE_DESC>
	{
		enum { id = TID_BLEND_STATE_DESC }; enum { hasDynamicSize = 1 };

		static void toMemory(const BLEND_STATE_DESC& data, char* memory)
		{
			UINT32 size = getDynamicSize(data);

			memcpy(memory, &size, sizeof(UINT32));
			memory += sizeof(UINT32);
			size -= sizeof(UINT32);
			memcpy(memory, &data, size);
		}

		static UINT32 fromMemory(BLEND_STATE_DESC& data, char* memory)
		{
			UINT32 size;
			memcpy(&size, memory, sizeof(UINT32));
			memory += sizeof(UINT32);

			UINT32 dataSize = size - sizeof(UINT32);
			memcpy((void*)& data, memory, dataSize);

			return size;
		}

		static UINT32 getDynamicSize(const BLEND_STATE_DESC& data)
		{
			UINT64 dataSize = sizeof(data) + sizeof(UINT32);

#if BS_DEBUG_MODE
			if (dataSize > std::numeric_limits<UINT32>::max())
			{
				BS_EXCEPT(InternalErrorException, "Data overflow! Size doesn't fit into 32 bits.");
			}
#endif

			return (UINT32)dataSize;
		}
	};

	/** @} */
	/** @endcond */
}
