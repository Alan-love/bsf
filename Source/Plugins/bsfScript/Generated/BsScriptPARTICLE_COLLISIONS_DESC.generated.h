#pragma once

#include "BsScriptEnginePrerequisites.h"
#include "BsScriptObject.h"
#include "../../../../../Foundation/bsfCore/Particles/BsParticleEvolver.h"

namespace bs
{
	class BS_SCR_BE_EXPORT ScriptPARTICLE_COLLISIONS_DESC : public ScriptObject<ScriptPARTICLE_COLLISIONS_DESC>
	{
	public:
		SCRIPT_OBJ(ENGINE_ASSEMBLY, "bs", "ParticleCollisionsOptions")

		static MonoObject* box(const PARTICLE_COLLISIONS_DESC& value);
		static PARTICLE_COLLISIONS_DESC unbox(MonoObject* value);

	private:
		ScriptPARTICLE_COLLISIONS_DESC(MonoObject* managedInstance);

	};
}
