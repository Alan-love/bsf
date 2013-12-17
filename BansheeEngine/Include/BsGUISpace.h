#pragma once

#include "BsPrerequisites.h"
#include "BsGUIElementBase.h"

namespace BansheeEngine
{
	class BS_EXPORT GUIFixedSpace : public GUIElementBase
	{
	public:
		GUIFixedSpace(CM::UINT32 size)
			:mSize(size)
		{ }

		CM::UINT32 getSize() const { return mSize; }
		Type _getType() const { return GUIElementBase::Type::FixedSpace; }

		virtual CM::Vector2I _getOptimalSize() const { return CM::Vector2I(getSize(), getSize()); }

	protected:
		CM::UINT32 mSize;
	};

	class BS_EXPORT GUIFlexibleSpace : public GUIElementBase
	{
	public:
		GUIFlexibleSpace() {}

		Type _getType() const { return GUIElementBase::Type::FlexibleSpace; }

		virtual CM::Vector2I _getOptimalSize() const { return CM::Vector2I(0, 0); }
	};
}