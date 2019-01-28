#pragma once

#include "BsScriptEnginePrerequisites.h"
#include "Wrappers/BsScriptComponent.h"
#include "BsScriptCJoint.generated.h"
#include "../../../../../Foundation/bsfCore/Physics/BsJoint.h"
#include "../../../../../Foundation/bsfCore/Physics/BsSphericalJoint.h"

namespace bs
{
	struct __LimitConeRangeInterop;
	class CSphericalJoint;

	class BS_SCR_BE_EXPORT ScriptCSphericalJoint : public TScriptComponent<ScriptCSphericalJoint, CSphericalJoint, ScriptCJointBase>
	{
	public:
		SCRIPT_OBJ(ENGINE_ASSEMBLY, "bs", "SphericalJoint")

		ScriptCSphericalJoint(MonoObject* managedInstance, const GameObjectHandle<CSphericalJoint>& value);

	private:
		static void Internal_getLimit(ScriptCSphericalJoint* thisPtr, __LimitConeRangeInterop* __output);
		static void Internal_setLimit(ScriptCSphericalJoint* thisPtr, __LimitConeRangeInterop* limit);
		static void Internal_setFlag(ScriptCSphericalJoint* thisPtr, SphericalJointFlag flag, bool enabled);
		static bool Internal_hasFlag(ScriptCSphericalJoint* thisPtr, SphericalJointFlag flag);
	};
}
