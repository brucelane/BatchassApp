#pragma once

#include <string>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppNative.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rand.h"
#include "Logger.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"
#include "cinder/Capture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Camera.h"
// parameters
#include "ParameterBag.h"
// textures
#include "Textures.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace Reymenta
{
	// stores the pointer to the ABP instance
	typedef std::shared_ptr<class ABP> ABPRef;

	// brick
	struct brick {
		int shape;
		float size;
		float r;
		float g;
		float b;
		float a;
		float vx;
		float vy;
		float motionVector;
		float rotation;
	};
	const pair<float, float> CAMERA_Y_RANGE(32.0f, 80.0f);

	class ABP {
	public:
		ABP(ParameterBagRef aParameterBag, TexturesRef aTexturesRef);

		static ABPRef	create(ParameterBagRef aParameterBag, TexturesRef aTexturesRef)
		{
			return shared_ptr<ABP>(new ABP(aParameterBag, aTexturesRef));
		}
		void						update();
		void						draw();
		void						setShape(const int &aShape, const bool &pressed) { mShape = aShape; }
		void						addBrick(const bool &pressed);
		CameraPersp					mCam;

	private:
		// parameters
		ParameterBagRef				mParameterBag;
		// Textures
		TexturesRef						mTextures;

		float						mR, mG, mB, mA;
		float						mZoom;
		float						mBend;
		Vec2f						mXYVector;
		float						mRepetitions;
		int							mShape;
		float						mZPosition;
		float						mRotation;
		bool						mLockZ;
		bool						mLockRepetitions;
		bool						mLockRotation;
		bool						mLockSize;
		bool						mLockMotionVector;
		bool						mLockBend;
		bool						mGlobalMode;

		float						mSize;
		float						mMotionVector;
		// neRenderer
		float						x, y;
		float						r, g, b, a;
		float						scale;
		ColorA						kolor;
		int							shape;
		float						rotation;
		float						motion;
		float						steps;
		float						distance;
		//mat4						mRotationMatrix;
		float						mColorFactor;

		void						newRendering();
		void						updateBricks();
		vector <brick>				bricks;
		bool						alreadyCreated;
	};
}