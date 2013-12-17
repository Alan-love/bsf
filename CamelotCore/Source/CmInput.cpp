#include "CmInput.h"
#include "CmTime.h"
#include "CmMath.h"
#include "CmRectI.h"
#include "CmDebug.h"
#include "CmRenderWindowManager.h"

#include <boost/bind.hpp>

namespace CamelotFramework
{
	const int Input::HISTORY_BUFFER_SIZE = 10; // Size of buffer used for input smoothing
	const float Input::WEIGHT_MODIFIER = 0.5f;

	Input::Input()
		:mSmoothHorizontalAxis(0.0f), mSmoothVerticalAxis(0.0f), mCurrentBufferIdx(0), mMouseLastRel(0, 0), mRawInputHandler(nullptr)
	{ 
		mHorizontalHistoryBuffer = cm_newN<float>(HISTORY_BUFFER_SIZE);
		mVerticalHistoryBuffer = cm_newN<float>(HISTORY_BUFFER_SIZE);
		mTimesHistoryBuffer = cm_newN<float>(HISTORY_BUFFER_SIZE);

		for(int i = 0; i < HISTORY_BUFFER_SIZE; i++)
		{
			mHorizontalHistoryBuffer[i] = 0.0f;
			mVerticalHistoryBuffer[i] = 0.0f;
			mTimesHistoryBuffer[i] = 0.0f;
		}

		for(int i = 0; i < BC_Count; i++)
			mKeyState[i] = false;

		mOSInputHandler = cm_shared_ptr<OSInputHandler>();

		mOSInputHandler->onCharInput.connect(boost::bind(&Input::charInput, this, _1));
		mOSInputHandler->onCursorMoved.connect(boost::bind(&Input::cursorMoved, this, _1));
		mOSInputHandler->onCursorPressed.connect(boost::bind(&Input::cursorPressed, this, _1));
		mOSInputHandler->onCursorReleased.connect(boost::bind(&Input::cursorReleased, this, _1));
		mOSInputHandler->onDoubleClick.connect(boost::bind(&Input::cursorDoubleClick, this, _1));
		mOSInputHandler->onInputCommand.connect(boost::bind(&Input::inputCommandEntered, this, _1));

		RenderWindowManager::instance().onFocusGained.connect(boost::bind(&Input::inputWindowChanged, this, _1));
	}

	Input::~Input()
	{
		cm_deleteN(mHorizontalHistoryBuffer, HISTORY_BUFFER_SIZE);
		cm_deleteN(mVerticalHistoryBuffer, HISTORY_BUFFER_SIZE);
		cm_deleteN(mTimesHistoryBuffer, HISTORY_BUFFER_SIZE);
	}

	void Input::registerRawInputHandler(std::shared_ptr<RawInputHandler> inputHandler)
	{
		if(mRawInputHandler != inputHandler)
		{
			mRawInputHandler = inputHandler;

			if(mRawInputHandler != nullptr)
			{
				mRawInputHandler->onButtonDown.connect(boost::bind(&Input::buttonDown, this, _1));
				mRawInputHandler->onButtonUp.connect(boost::bind(&Input::buttonUp, this, _1));

				mRawInputHandler->onAxisMoved.connect(boost::bind(&Input::axisMoved, this, _1, _2));
			}
		}
	}

	void Input::update()
	{
		if(mRawInputHandler == nullptr)
		{
			LOGERR("Raw input handler not initialized!");
			return;
		}
		else
			mRawInputHandler->update();

		if(mOSInputHandler == nullptr)
		{
			LOGERR("OS input handler not initialized!");
			return;
		}
		else
			mOSInputHandler->update();

		updateSmoothInput();
	}

	void Input::inputWindowChanged(RenderWindow& win)
	{
		if(mRawInputHandler != nullptr)
			mRawInputHandler->inputWindowChanged(win);

		if(mOSInputHandler != nullptr)
			mOSInputHandler->inputWindowChanged(win);
	}

	void Input::buttonDown(ButtonCode code)
	{
		mKeyState[code & 0x0000FFFF] = true;

		if(!onButtonDown.empty())
		{
			ButtonEvent btnEvent;
			btnEvent.buttonCode = code;

			onButtonDown(btnEvent);
		}
	}

	void Input::buttonUp(ButtonCode code)
	{
		mKeyState[code & 0x0000FFFF] = false;

		if(!onButtonUp.empty())
		{
			ButtonEvent btnEvent;
			btnEvent.buttonCode = code;

			onButtonUp(btnEvent);
		}
	}

	void Input::axisMoved(const RawAxisState& state, RawInputAxis axis)
	{
		if(axis == RawInputAxis::Mouse_XY)
			mMouseLastRel = Vector2I(-state.rel.x, -state.rel.y);

		mAxes[(int)axis] = state;
	}

	void Input::cursorMoved(const PositionalInputEvent& event)
	{
		mMouseAbsPos = event.screenPos;

		if(!onCursorMoved.empty())
			onCursorMoved(event);
	}

	void Input::cursorPressed(const PositionalInputEvent& event)
	{
		mMouseAbsPos = event.screenPos;

		if(!onCursorPressed.empty())
			onCursorPressed(event);
	}

	void Input::cursorReleased(const PositionalInputEvent& event)
	{
		mMouseAbsPos = event.screenPos;

		if(!onCursorReleased.empty())
			onCursorReleased(event);
	}

	void Input::cursorDoubleClick(const PositionalInputEvent& event)
	{
		if(!onDoubleClick.empty())
			onDoubleClick(event);
	}

	void Input::inputCommandEntered(InputCommandType commandType)
	{
		if(!onInputCommand.empty())
			onInputCommand(commandType);
	}

	void Input::charInput(UINT32 chr)
	{
		if(!onCharInput.empty())
		{
			TextInputEvent textInputEvent;
			textInputEvent.textChar = chr;

			onCharInput(textInputEvent);
		}
	}

	float Input::getHorizontalAxis() const
	{
		return mSmoothHorizontalAxis;
	}

	float Input::getVerticalAxis() const
	{
		return mSmoothVerticalAxis;
	}

	bool Input::isButtonDown(ButtonCode button) const
	{
		return mKeyState[button & 0x0000FFFF];
	}

	void Input::updateSmoothInput()
	{
		float currentTime = gTime().getTime();

		mHorizontalHistoryBuffer[mCurrentBufferIdx] = (float)mMouseLastRel.x;
		mVerticalHistoryBuffer[mCurrentBufferIdx] = (float)mMouseLastRel.y;
		mTimesHistoryBuffer[mCurrentBufferIdx] = currentTime;

		int i = 0;
		int idx = mCurrentBufferIdx;
		float currentWeight = 1.0f;
		float horizontalTotal = 0.0f;
		float verticalTotal = 0.0f;
		while(i < HISTORY_BUFFER_SIZE)
		{
			float timeWeight = 1.0f - (currentTime - mTimesHistoryBuffer[idx]) * 10.0f;
			if(timeWeight < 0.0f)
				timeWeight = 0.0f;

			horizontalTotal += mHorizontalHistoryBuffer[idx] * currentWeight * timeWeight;
			verticalTotal += mVerticalHistoryBuffer[idx] * currentWeight * timeWeight;

			currentWeight *= WEIGHT_MODIFIER;
			idx = (idx + 1) % HISTORY_BUFFER_SIZE;
			i++;
		}

		mCurrentBufferIdx = (mCurrentBufferIdx + 1) % HISTORY_BUFFER_SIZE;

		mSmoothHorizontalAxis = Math::clamp(horizontalTotal / HISTORY_BUFFER_SIZE, -1.0f, 1.0f);
		mSmoothVerticalAxis = Math::clamp(verticalTotal / HISTORY_BUFFER_SIZE, -1.0f, 1.0f);

		mMouseLastRel = Vector2I(0, 0);
	}

	Input& gInput()
	{
		return Input::instance();
	}
}