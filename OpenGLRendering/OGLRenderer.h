/******************************************************************************
This file is part of the Newcastle OpenGL Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*/////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RendererBase.h"

#ifdef _WIN32
#include "windows.h"
#endif

#ifndef _WIN32
// GLAD doesn't like to be included after SDL
#include <glad/gl.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_opengl.h>
#endif

#ifdef _DEBUG
#define OPENGL_DEBUGGING
#endif

#ifndef _WIN32
namespace NCL::UnixCode {
	class SDLWindow;
}
#endif
namespace NCL::Rendering {

	class Mesh;
	class Shader;
	class Texture;

	class OGLMesh;
	class OGLShader;
	class OGLTexture;
	class OGLBuffer;

	class SimpleFont;

	class OGLRenderer : public RendererBase	{
	public:
		friend class OGLRenderer;
		OGLRenderer(Window& w);
		~OGLRenderer();

		void OnWindowResize(int w, int h)	override;
		bool HasInitialised()				const override {
			return initState;
		}

		bool SetVerticalSync(VerticalSyncState s) override;

	protected:
		void BeginFrame()	override;
		void RenderFrame()	override;
		void EndFrame()		override;
		void SwapBuffers()  override;

		void UseShader(const OGLShader& s);
		void BindTextureToShader(const OGLTexture& t, const std::string& uniform, int texUnit) const;
		void BindMesh(const OGLMesh& m);
		void BindBufferAsUBO(const OGLBuffer& b, uint32_t slotID);
		void BindBufferAsSSBO(const OGLBuffer& b, uint32_t slotID);

		void DrawBoundMesh(int subLayer = 0, int numInstances = 1);
#ifdef _WIN32
		void InitWithWin32(Window& w);
		void DestroyWithWin32();
		HDC		deviceContext;		//...Device context?
		HGLRC	renderContext;		//Permanent Rendering Context
#else
		void InitWithSDL2(Window& w);
		void DestroyWithSDL2();
		SDL_GLContext glContext;

		UnixCode::SDLWindow* window;
#endif

		const OGLMesh*		boundMesh;
		const OGLShader*	activeShader;
	private:
		bool initState;
	};
}
