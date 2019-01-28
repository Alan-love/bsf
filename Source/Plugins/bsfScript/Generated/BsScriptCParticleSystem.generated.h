#pragma once

#include "BsScriptEnginePrerequisites.h"
#include "Wrappers/BsScriptComponent.h"
#include "../../../../../Foundation/bsfCore/Particles/BsParticleSystem.h"
#include "../../../../../Foundation/bsfCore/Particles/BsParticleSystem.h"

namespace bs
{
	class CParticleSystem;

	class BS_SCR_BE_EXPORT ScriptCParticleSystem : public TScriptComponent<ScriptCParticleSystem, CParticleSystem>
	{
	public:
		SCRIPT_OBJ(ENGINE_ASSEMBLY, "bs", "ParticleSystem")

		ScriptCParticleSystem(MonoObject* managedInstance, const GameObjectHandle<CParticleSystem>& value);

	private:
		static void Internal_setSettings(ScriptCParticleSystem* thisPtr, MonoObject* settings);
		static MonoObject* Internal_getSettings(ScriptCParticleSystem* thisPtr);
		static void Internal_setGpuSimulationSettings(ScriptCParticleSystem* thisPtr, MonoObject* settings);
		static MonoObject* Internal_getGpuSimulationSettings(ScriptCParticleSystem* thisPtr);
		static void Internal_setEmitters(ScriptCParticleSystem* thisPtr, MonoArray* emitters);
		static MonoArray* Internal_getEmitters(ScriptCParticleSystem* thisPtr);
		static void Internal_setEvolvers(ScriptCParticleSystem* thisPtr, MonoArray* evolvers);
		static MonoArray* Internal_getEvolvers(ScriptCParticleSystem* thisPtr);
		static void Internal_setLayer(ScriptCParticleSystem* thisPtr, uint64_t layer);
		static uint64_t Internal_getLayer(ScriptCParticleSystem* thisPtr);
		static bool Internal__togglePreviewMode(ScriptCParticleSystem* thisPtr, bool enabled);
	};
}
