#include "BsScriptGUIScrollArea.h"
#include "BsScriptMeta.h"
#include "BsMonoField.h"
#include "BsMonoClass.h"
#include "BsMonoManager.h"
#include "BsSpriteTexture.h"
#include "BsMonoUtil.h"
#include "BsGUILayout.h"
#include "BsGUIScrollArea.h"
#include "BsGUIOptions.h"
#include "BsScriptSpriteTexture.h"
#include "BsScriptGUIElementStyle.h"
#include "BsScriptGUILayout.h"
#include "BsScriptGUIArea.h"
#include "BsScriptHString.h"
#include "BsScriptGUIContent.h"

using namespace CamelotFramework;

namespace BansheeEngine
{
	ScriptGUIScrollArea::ScriptGUIScrollArea(GUIScrollArea* scrollArea, GUIWidget& parentWidget)
		:mScrollArea(scrollArea), mParentWidget(parentWidget)
	{

	}

	void ScriptGUIScrollArea::initMetaData()
	{
		metaData = ScriptMeta("MBansheeEngine", "BansheeEngine", "GUIScrollArea", &ScriptGUIScrollArea::initRuntimeData);

		MonoManager::registerScriptType(&metaData);
	}

	void ScriptGUIScrollArea::initRuntimeData()
	{
		metaData.scriptClass->addInternalCall("Internal_CreateInstance", &ScriptGUIScrollArea::internal_createInstance);
		metaData.scriptClass->addInternalCall("Internal_DestroyInstance", &ScriptGUIScrollArea::internal_destroyInstance);
	}

	GUIWidget& ScriptGUIScrollArea::getParentWidget() const
	{
		return mParentWidget;
	}

	void ScriptGUIScrollArea::internal_createInstance(MonoObject* instance, MonoObject* parentLayout, MonoObject* style, MonoArray* guiOptions)
	{
		ScriptGUILayout* scriptLayout = ScriptGUILayout::toNative(parentLayout);
		GUIOptions options;

		UINT32 arrayLen = (UINT32)mono_array_length(guiOptions);
		for(UINT32 i = 0; i < arrayLen; i++)
			options.addOption(mono_array_get(guiOptions, GUIOption, i));

		GUIElementStyle* elemStyle = nullptr;

		if(style != nullptr)
			elemStyle = ScriptGUIElementStyle::toNative(style)->getInternalValue();

		GUIScrollArea* guiScrollArea = GUIScrollArea::create(scriptLayout->getParentWidget(), options, elemStyle);
		GUILayout* nativeLayout = scriptLayout->getInternalValue();
		nativeLayout->addElement(guiScrollArea);

		ScriptGUIScrollArea* nativeInstance = new (cm_alloc<ScriptGUIScrollArea>()) ScriptGUIScrollArea(guiScrollArea, scriptLayout->getParentWidget());
		nativeInstance->createInstance(instance);

		metaData.thisPtrField->setValue(instance, nativeInstance);
	}

	void ScriptGUIScrollArea::internal_destroyInstance(ScriptGUIScrollArea* nativeInstance)
	{
		nativeInstance->destroyInstance();
		cm_delete(nativeInstance);
	}
}