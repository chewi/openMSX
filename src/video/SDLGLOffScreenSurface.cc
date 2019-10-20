#include "SDLGLOffScreenSurface.hh"
#include "SDLGLVisibleSurface.hh"
#include "GLUtil.hh"

namespace openmsx {

SDLGLOffScreenSurface::SDLGLOffScreenSurface(const SDLGLVisibleSurface& output)
	: fboTex(true) // enable interpolation   TODO why?
{
	// only used for width and height
	setSDLSurface(const_cast<SDL_Surface*>(output.getSDLSurface()));
	setSDLRenderer(output.getSDLRenderer());
	calculateViewPort(output.getPhysicalSize());

	gl::ivec2 physSize = getPhysicalSize();
	fboTex.bind();
	glTexImage2D(GL_TEXTURE_2D,    // target
	             0,                // level
	             GL_RGB8,          // internal format
	             physSize[0],      // width
	             physSize[1],      // height
	             0,                // border
	             GL_RGB,           // format
	             GL_UNSIGNED_BYTE, // type
	             nullptr);         // data
	fbo = gl::FrameBufferObject(fboTex);
	fbo.push();

	SDLGLOutputSurface::init(*this);
}

void SDLGLOffScreenSurface::saveScreenshot(const std::string& filename)
{
	SDLGLOutputSurface::saveScreenshot(filename, *this);
}

} // namespace openmsx
