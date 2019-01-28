//********************************** Banshee Engine (www.banshee3d.com) **************************************************//
//**************** Copyright (c) 2016 Marko Pintera (marko.pintera@gmail.com). All rights reserved. **********************//
#pragma once

#include "BsScriptEnginePrerequisites.h"
#include "BsScriptObject.h"
#include "Resources/BsResources.h"

namespace bs
{
	class ScriptRRefBase;

	/** @addtogroup ScriptInteropEngine
	 *  @{
	 */

	/**	Interop class between C++ & CLR for Resources & GameResourceManager. */
	class BS_SCR_BE_EXPORT ScriptResources : public ScriptObject<ScriptResources>
	{
	public:
		SCRIPT_OBJ(ENGINE_ASSEMBLY, "BansheeEngine", "Resources")

	private:
		ScriptResources(MonoObject* instance);

		/************************************************************************/
		/* 								CLR HOOKS						   		*/
		/************************************************************************/
		static MonoObject* internal_Load(MonoString* path, ResourceLoadFlag flags);
		static MonoObject* internal_LoadFromUUID(UUID* uuid, ResourceLoadFlag flags);
		static MonoObject* internal_LoadAsync(MonoString* path, ResourceLoadFlag flags);
		static MonoObject* internal_LoadAsyncFromUUID(UUID* uuid, ResourceLoadFlag flags);
		static void internal_Release(ScriptResourceBase* resource);
		static void internal_ReleaseRef(ScriptRRefBase* resource);
		static void internal_UnloadUnused();
		static float internal_GetLoadProgress(ScriptRRefBase* resource, bool loadDependencies);
	};

	/** @} */
}