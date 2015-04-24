/*
*
* Copyright (c) 2013, Ban the Rewind
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or
* without modification, are permitted provided that the following
* conditions are met:
*
* Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in
* the documentation and/or other materials provided with the
* distribution.
*
* Neither the name of the Ban the Rewind nor the names of its
* contributors may be used to endorse or promote products
* derived from this software without specific prior written
* permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
#include "Meshes.h"

using namespace Reymenta;

Meshes::Meshes(ParameterBagRef aParameterBag, TexturesRef aTexturesRef)
{
	mSetup = false;
	mParameterBag = aParameterBag;
	mTextures = aTexturesRef;

	setup();
}

MeshesRef Meshes::create(ParameterBagRef aParameterBag, TexturesRef aTexturesRef)
{
	return shared_ptr<Meshes>(new Meshes(aParameterBag, aTexturesRef));
}

void Meshes::setup()
{
	// Set up OpenGL to work with default lighting
	glShadeModel(GL_SMOOTH);
	// creates triangles: gl::enable(GL_POLYGON_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	gl::enable(GL_NORMALIZE);

	// Load shader
	try {
		mShader = gl::GlslProg(loadAsset("meshes.vert"), loadAsset("meshes.frag"));
	}
	catch (gl::GlslProgCompileExc ex) {
		console() << ex.what() << endl;
	}

	// Define properties
	mDivision = 1;
	mDivisionPrev = mDivision;
	mResolution = Vec3i::one() * 12;
	mResolutionPrev = mResolution;
	mScale = Vec3f::one();

	// Set up the arcball
	mArcball = Arcball(getWindowSize());
	mArcball.setRadius((float)getWindowHeight() * 0.5f);

	// Set up the instancing grid
	mGridSize = Vec2i(16, 16);
	mGridSpacing = Vec2f(2.5f, 2.5f);

	// Set up the light
	mLight = new gl::Light(gl::Light::DIRECTIONAL, 0);
	mLight->setAmbient(ColorAf::white());
	mLight->setDiffuse(ColorAf::white());
	mLight->setDirection(Vec3f::one());
	mLight->setPosition(Vec3f::one() * -1.0f);
	mLight->setSpecular(ColorAf::white());
	mLight->enable();

	// Define the mesh titles for the params GUI
	mMeshTitles.push_back("Cube");
	mMeshTitles.push_back("Sphere");
	mMeshTitles.push_back("Cylinder");
	mMeshTitles.push_back("Cone");
	mMeshTitles.push_back("Torus");
	mMeshTitles.push_back("Icosahedron");
	mMeshTitles.push_back("Circle");
	mMeshTitles.push_back("Square");
	mMeshTitles.push_back("Ring");
	mMeshTitles.push_back("Custom");

	// Generate meshes
	createMeshes();

	mSetup = true;
}

void Meshes::update()
{
	mGridSize = Vec2i(mParameterBag->mRenderXY.x + 2.01 * 16, mParameterBag->mRenderXY.y + 2.01 * 16);
	mGridSpacing = Vec2f(mParameterBag->mRenderXY.x + 2.01 * 2.5f, mParameterBag->mRenderXY.y + 2.01 * 2.5f);
	mScale = Vec3f::one() * mParameterBag->controlValues[22];
	// Reset the meshes if the segment count changes
	if (mDivisionPrev != mDivision ||
		mResolutionPrev != mResolution) {
		createMeshes();
		mDivisionPrev = mDivision;
		mResolutionPrev = mResolution;
	}

	// Update light on every frame
	mLight->update(mParameterBag->mCamera);
}

void Meshes::draw()
{
	mTextures->getFbo(mParameterBag->mMeshFboIndex).bindFramebuffer();
	// Set up window
	gl::setViewport(mTextures->getFbo(mParameterBag->mMeshFboIndex).getBounds());
	gl::setMatrices(mParameterBag->mCamera);
	gl::clear(Color(mParameterBag->controlValues[5], mParameterBag->controlValues[6], mParameterBag->controlValues[7]));
	// Use arcball to rotate model view
	glMultMatrixf(mArcball.getQuat());

	// Enabled lighting, texture mapping, wireframe
	if (mParameterBag->iLight) {
		gl::enable(GL_LIGHTING);
	}

	gl::enable(GL_TEXTURE_2D);
	mTextures->getFboTexture(mParameterBag->mMixFboIndex).bind(0);
	// Bind and configure shader
	if (mShader) {
		mShader.bind();
		mShader.uniform("eyePoint", mParameterBag->mCamera.getEyePoint());
		mShader.uniform("lightingEnabled", mParameterBag->iLight);
		mShader.uniform("size", Vec2f(mGridSize));
		mShader.uniform("spacing", mGridSpacing);//mParameterBag->controlValues[12]);//speedSlider 
		mShader.uniform("tex", 0);
		mShader.uniform("textureEnabled", true);
	}
	// Apply scale
	gl::pushMatrices();
	gl::scale(mScale);
	// Instance count
	size_t instanceCount = (size_t)(mGridSize.x * mGridSize.y);
	
	// Draw selected mesh
	switch ((MeshType)mParameterBag->mMeshIndex)
	{
	case MESH_TYPE_CIRCLE:
		drawInstanced(mCircle, instanceCount);
		break;
	case MESH_TYPE_CONE:
		drawInstanced(mCone, instanceCount);
		break;
	case MESH_TYPE_CUBE:
		drawInstanced(mCube, instanceCount);
		break;
	case MESH_TYPE_CUSTOM:
		drawInstanced(mCustom, instanceCount);
		break;
	case MESH_TYPE_CYLINDER:
		drawInstanced(mCylinder, instanceCount);
		break;
	case MESH_TYPE_ICOSAHEDRON:
		drawInstanced(mIcosahedron, instanceCount);
		break;
	case MESH_TYPE_RING:
		drawInstanced(mRing, instanceCount);
		break;
	case MESH_TYPE_SQUARE:
		drawInstanced(mSquare, instanceCount);
		break;
	case MESH_TYPE_TORUS:
		drawInstanced(mTorus, instanceCount);
		break;
	default://case MESH_TYPE_SPHERE,
		drawInstanced(mSphere, instanceCount);
		break;
	}

	// End scale
	gl::popMatrices();
	mTextures->getFboTexture(mParameterBag->mMixFboIndex).unbind();
	gl::disable(GL_TEXTURE_2D);

	if (mParameterBag->iLight) {
		gl::disable(GL_LIGHTING);
	}
	// Unbind shader
	if (mShader) {
		mShader.unbind();
	}
	mTextures->getFbo(mParameterBag->mMeshFboIndex).unbindFramebuffer();
}
// Draw VBO instanced
void Meshes::drawInstanced(const gl::VboMesh& vbo, size_t count)
{
	vbo.enableClientStates();
	vbo.bindAllData();
	glDrawElementsInstancedARB(vbo.getPrimitiveType(), vbo.getNumIndices(), GL_UNSIGNED_INT, (GLvoid*)(sizeof(uint32_t) * 0), count);
	gl::VboMesh::unbindBuffers();
	vbo.disableClientStates();
}
void Meshes::mouseDown(MouseEvent event)
{
	// Rotate with arcball
	//mArcball.mouseDown(event.getPos());
	mArcball.mouseDown(mParameterBag->mRenderPosXY);
}

void Meshes::mouseDrag(MouseEvent event)
{
	// Rotate with arcball
	//mArcball.mouseDrag(event.getPos());
	mArcball.mouseDrag(mParameterBag->mRenderPosXY);
}

void Meshes::mouseWheel(MouseEvent event)
{
	// Zoom in/out with mouse wheel
	Vec3f eye = mParameterBag->mCamera.getEyePoint();
	eye.z += event.getWheelIncrement() * 0.1f;
	mParameterBag->mCamera.setEyePoint(eye);
}
// Creates VboMeshes
void Meshes::createMeshes()
{
	// Use the MeshHelper to generate primitives
	mCircle = gl::VboMesh(MeshHelper::createCircle(mResolution.xy()));
	mCone = gl::VboMesh(MeshHelper::createCylinder(mResolution.xy(), 0.0f, 1.0f, false, true));
	mCube = gl::VboMesh(MeshHelper::createCube(mResolution));
	mCylinder = gl::VboMesh(MeshHelper::createCylinder(mResolution.xy()));
	mIcosahedron = gl::VboMesh(MeshHelper::createIcosahedron(mDivision));
	mRing = gl::VboMesh(MeshHelper::createRing(mResolution.xy()));
	mSphere = gl::VboMesh(MeshHelper::createSphere(mResolution.xy()));
	mSquare = gl::VboMesh(MeshHelper::createSquare(mResolution.xy()));
	mTorus = gl::VboMesh(MeshHelper::createTorus(mResolution.xy()));

	/////////////////////////////////////////////////////////////////////////////
	// Custom mesh

	// Declare vectors
	vector<uint32_t> indices;
	vector<Vec3f> normals;
	vector<Vec3f> positions;
	vector<Vec2f> texCoords;

	// Mesh dimensions
	float halfHeight = (float)mResolution.x * 0.5f;
	float halfWidth = (float)mResolution.y * 0.5f;
	float unit = 3.0f / (float)mResolution.x;
	Vec3f scale(unit, 0.5f, unit);
	Vec3f offset(-0.5f, 0.5f, 0.0f);

	// Iterate through rows and columns using segment count
	for (int32_t y = 0; y < mResolution.y; y++) {
		for (int32_t x = 0; x < mResolution.x; x++) {

			// Set texture coordinate in [ 0 - 1, 0 - 1 ] range
			Vec2f texCoord((float)x / (float)mResolution.x, (float)y / (float)mResolution.y);
			texCoords.push_back(texCoord);

			// Use random value for Y position
			float value = randFloat();

			// Set vertex position
			Vec3f position((float)x - halfWidth, value, (float)y - halfHeight);
			positions.push_back(position * scale + offset);

			// Add a default normal for now (we'll calculate this down below)
			normals.push_back(Vec3f::zero());

			// Add indices to form quad from two triangles
			int32_t xn = x + 1 >= mResolution.x ? 0 : 1;
			int32_t yn = y + 1 >= mResolution.y ? 0 : 1;
			indices.push_back(x + mResolution.x * y);
			indices.push_back((x + xn) + mResolution.x * y);
			indices.push_back((x + xn) + mResolution.x * (y + yn));
			indices.push_back(x + mResolution.x * (y + yn));
			indices.push_back((x + xn) + mResolution.x * (y + yn));
			indices.push_back(x + mResolution.x * y);
		}
	}

	// Iterate through again to set normals
	for (int32_t y = 0; y < mResolution.y - 1; y++) {
		for (int32_t x = 0; x < mResolution.x - 1; x++) {
			Vec3f vert0 = positions[indices[(x + mResolution.x * y) * 6]];
			Vec3f vert1 = positions[indices[((x + 1) + mResolution.x * y) * 6]];
			Vec3f vert2 = positions[indices[((x + 1) + mResolution.x * (y + 1)) * 6]];
			normals[x + mResolution.x * y] = Vec3f((vert1 - vert0).cross(vert1 - vert2).normalized());
		}
	}

	// Use the MeshHelper to create a VboMesh from our vectors
	mCustom = gl::VboMesh(MeshHelper::create(indices, positions, normals, texCoords));
}

void Meshes::shutdown()
{
	// Clean up
	if (mLight != 0) {
		mLight = 0;
	}
}
