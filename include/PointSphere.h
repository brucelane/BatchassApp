#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/AppNative.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Camera.h"
#include "Logger.h"

// textures
#include "Textures.h"
// shaders
#include "Shaders.h"
// Parameters
#include "ParameterBag.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace Reymenta
{
	// stores the pointer to the WarpWrapper instance
	typedef std::shared_ptr<class PointSphere> PointSphereRef;

	class PointSphere {
	public:
		PointSphere(ParameterBagRef aParameterBag, TexturesRef aTexturesRef, ShadersRef aShadersRef);
		virtual					~PointSphere();
		static PointSphereRef	create(ParameterBagRef aParameterBag, TexturesRef aTexturesRef, ShadersRef aShadersRef)
		{
			return shared_ptr<PointSphere>(new PointSphere(aParameterBag, aTexturesRef, aShadersRef));
		}
		void						update();
		void						resize();
		void						mouseMove(MouseEvent event);
		void						mouseDown(MouseEvent event);
		void						mouseDrag(MouseEvent event);
		void						mouseUp(MouseEvent event);
		void						keyDown(KeyEvent event);
		void						keyUp(KeyEvent event);
		void						draw();

	private:
		// Logger
		LoggerRef					log;

		// Parameters
		ParameterBagRef				mParameterBag;
		// Shaders
		ShadersRef					mShaders;
		// Textures
		TexturesRef					mTextures;

		float						mRotation;

		float						mSphereAngle;
		Vec3f						mSphereAxis;
		Quatf						mSphereQuat;
		gl::GlslProg				mSphereShader;
		gl::Texture					mSphereTexture;
		int							multFactor;
	};
}