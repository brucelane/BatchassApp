#pragma once

#include <string>
#include <vector>

#include "cinder/app/AppNative.h"
#include "cinder/Timeline.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "ParameterBag.h"
// textures
#include "Textures.h"
#include "MeshHelper.h"
#include "cinder/Arcball.h"
#include "cinder/Camera.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Vbo.h"
#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace Reymenta
{
	typedef std::shared_ptr<class Meshes> MeshesRef;

	class Meshes
	{
	public:
		Meshes(ParameterBagRef aParameterBag, TexturesRef aTexturesRef);
		static MeshesRef create(ParameterBagRef aParameterBag, TexturesRef aTexturesRef);
		void setup();
		void update();
		void draw();
		void shutdown();
		bool isSetup() { return mSetup; }
		void						mouseDown(ci::app::MouseEvent event);
		void						mouseDrag(ci::app::MouseEvent event);
		void						mouseWheel(ci::app::MouseEvent event);

	private:
		bool mSetup;
		ParameterBagRef				mParameterBag;
		// Textures
		TexturesRef					mTextures;
		// Enumerate mesh types
		enum {
			MESH_TYPE_CUBE,
			MESH_TYPE_SPHERE,
			MESH_TYPE_CYLINDER,
			MESH_TYPE_CONE,
			MESH_TYPE_TORUS,
			MESH_TYPE_ICOSAHEDRON,
			MESH_TYPE_CIRCLE,
			MESH_TYPE_SQUARE,
			MESH_TYPE_RING,
			MESH_TYPE_CUSTOM
		} typedef MeshType;

		// The VboMeshes
		void						createMeshes();
		ci::gl::VboMesh				mCircle;
		ci::gl::VboMesh				mCone;
		ci::gl::VboMesh				mCube;
		ci::gl::VboMesh				mCustom;
		ci::gl::VboMesh				mCylinder;
		ci::gl::VboMesh				mIcosahedron;
		ci::gl::VboMesh				mRing;
		ci::gl::VboMesh				mSphere;
		ci::gl::VboMesh				mSquare;
		ci::gl::VboMesh				mTorus;

		// For selecting mesh type from params
		//int32_t						mMeshIndex;
		std::vector<std::string>	mMeshTitles;

		// Mesh resolution
		int32_t						mDivision;
		int32_t						mDivisionPrev;
		ci::Vec3i					mResolution;
		ci::Vec3i					mResolutionPrev;

		// Mesh scale
		ci::Vec3f					mScale;

		// Camera
		ci::Arcball					mArcball;
		//ci::CameraPersp				mCamera;

		// Lighting
		ci::gl::Light				*mLight;
		//bool						mLightEnabled;
		// Instancing shader
		ci::gl::GlslProg			mShader;

		// Instance grid
		void						drawInstanced(const ci::gl::VboMesh& vbo, size_t count = 1);
		ci::Vec2i					mGridSize;
		ci::Vec2f					mGridSpacing;

	};
}