#pragma once

#include "BsScriptEnginePrerequisites.h"
#include "BsScriptObject.h"
#include "../../../../../Foundation/bsfUtility/Math/BsVector3I.h"

namespace bs
{
	class BS_SCR_BE_EXPORT ScriptVector3I : public ScriptObject<ScriptVector3I>
	{
	public:
		SCRIPT_OBJ(ENGINE_ASSEMBLY, "bs", "Vector3I")

		static MonoObject* box(const Vector3I& value);
		static Vector3I unbox(MonoObject* value);

	private:
		ScriptVector3I(MonoObject* managedInstance);

	};
}
