#pragma once

#include "cinder/app/AppNative.h"
#include "cinder/Timeline.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"

// Parameters
#include "ParameterBag.h"
#include "UIController.h"
#include "UIElement.h"
// shaders
#include "Shaders.h"
// textures
#include "Textures.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace MinimalUI;

namespace Reymenta
{

	typedef std::shared_ptr<class UI> UIRef;

	class UI
	{

	public:
		UI(ParameterBagRef aParameterBag, ShadersRef aShadersRef, TexturesRef aTexturesRef, ci::app::WindowRef aWindow);
		static UIRef create(ParameterBagRef aParameterBag, ShadersRef aShadersRef, TexturesRef aTexturesRef, ci::app::WindowRef aWindow);

		void setup();
		void draw();
		void update();
		void resize();

		//void InstantBlack(const bool &pressed);
		void toggleUseTimeWithTempo(const bool &pressed);

		//void channelsSelector(const bool &pressed);
		void setTimeFactor(const int &aTimeFactor, const bool &pressed);
		//void setLayer(const int &aLayer, const bool &pressed);

		void lockFR(const bool &pressed) { mParameterBag->mLockFR = pressed; };
		void lockFG(const bool &pressed) { mParameterBag->mLockFG = pressed; };
		void lockFB(const bool &pressed) { mParameterBag->mLockFB = pressed; };
		void lockFA(const bool &pressed) { mParameterBag->mLockFA = pressed; };
		void lockBR(const bool &pressed) { mParameterBag->mLockBR = pressed; };
		void lockBG(const bool &pressed) { mParameterBag->mLockBG = pressed; };
		void lockBB(const bool &pressed) { mParameterBag->mLockBB = pressed; };
		void lockBA(const bool &pressed) { mParameterBag->mLockBA = pressed; };
		void tempoFR(const bool &pressed);
		void resetFR(const bool &pressed);
		void tempoFG(const bool &pressed);
		void resetFG(const bool &pressed);
		void tempoFB(const bool &pressed);
		void resetFB(const bool &pressed);
		void tempoFA(const bool &pressed);
		void resetFA(const bool &pressed);
		void tempoBR(const bool &pressed);
		void resetBR(const bool &pressed);
		void tempoBG(const bool &pressed);
		void resetBG(const bool &pressed);
		void tempoBB(const bool &pressed);
		void resetBB(const bool &pressed);
		void tempoBA(const bool &pressed);
		void resetBA(const bool &pressed);
		// tempo
		//void tapTempo(const bool &pressed);
		//void calculateTempo();

		MinimalUI::UIElementRef				sliderRed, sliderGreen, sliderBlue, sliderAlpha;
		MinimalUI::UIElementRef				sliderBackgroundRed, sliderBackgroundGreen, sliderBackgroundBlue, sliderBackgroundAlpha;
		//MinimalUI::UIElementRef				labelXY, labelPosXY;
		//MinimalUI::UIElementRef				sliderRenderXY, sliderRenderPosXY, sliderXSpeed, sliderYSpeed;
		//MinimalUI::UIElementRef				sliderLeftRenderXY, sliderRightRenderXY, sliderMixRenderXY, sliderPreviewRenderXY;

		//MinimalUI::UIElementRef				labelLayer;
		//MinimalUI::UIElementRef				buttonLayer[8];
		//MinimalUI::UIElementRef				fpsMvg, tempoMvg;
		//MinimalUI::UIElementRef				sliderTimeFactor;
		void toggleVisibility() { mVisible ? hide() : show(); }

		void show();
		void hide();
	private:
		void setupMiniControl();

		void mouseDown(ci::app::MouseEvent &event);
		void keyDown(ci::app::KeyEvent &event);

		// windows mgmt
		MinimalUI::UIControllerRef mMiniControl;

		vector<MinimalUI::UIControllerRef> mPanels;

		ci::app::WindowRef mWindow;
		ci::signals::scoped_connection mCbMouseDown, mCbKeyDown;
		int mPrevState;
		bool mVisible;
		bool mSetupComplete;

		// Parameters
		ParameterBagRef				mParameterBag;
		// Shaders
		ShadersRef					mShaders;
		// Textures
		TexturesRef					mTextures;

		Anim<float>					mTimer;

		// math
		string						formatNumber(float f);
		// tempo
		std::deque <double> buffer;
		ci::Timer timer;
		int counter;
		double averageTime;
		double currentTime;
		double startTime;
		float previousTime;
		int beatIndex; //0 to 3

	};
}