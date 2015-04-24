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
	// stores the pointer to the vertexSphere instance
	typedef std::shared_ptr<class VertexSphere> VertexSphereRef;

	class VertexSphere {
	public:
		VertexSphere(ParameterBagRef aParameterBag, TexturesRef aTexturesRef, ShadersRef aShadersRef);
		//virtual					~VertexSphere();
		static VertexSphereRef	create(ParameterBagRef aParameterBag, TexturesRef aTexturesRef, ShadersRef aShadersRef)
		{
			return shared_ptr<VertexSphere>(new VertexSphere(aParameterBag, aTexturesRef, aShadersRef));
		}
		void update();
		void draw();

	private:
		// Parameters
		ParameterBagRef				mParameterBag;
		// Shaders
		ShadersRef					mShaders;
		// Textures
		TexturesRef					mTextures;
		Vec2f mMouse;
		float mAngle;
		//float mRotationSpeed;
		Vec3f mAxis;
		Quatf mQuat;

		bool						mRotate;

	};
}