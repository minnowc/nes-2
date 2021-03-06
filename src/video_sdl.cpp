#include "video_sdl.h"
#include <SDL2/SDL.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <gl.h>
#include <glext.h>

SDLVideoDevice::SDLVideoDevice():
  window(
    SDL_CreateWindow(
      "",
      SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
      512,480,
      SDL_WINDOW_OPENGL
    )
  ),
  glcon(
    new SDL_GLContext(SDL_GL_CreateContext(window))
  )
{

  glMatrixMode(GL_PROJECTION|GL_MODELVIEW);
  glLoadIdentity();
  glOrtho(0,256,240,0,0,1);
  glClearColor(0,0.6,0,1);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);
  
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(
    GL_TEXTURE_2D, 
    0, 
    GL_RGBA, 
    256,
    240,
    0,
    GL_RGBA,
    GL_UNSIGNED_INT_8_8_8_8, 
    nullptr
  );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

}

SDLVideoDevice::~SDLVideoDevice() {
  glDeleteTextures(1, &texture);
  SDL_GL_DeleteContext(*static_cast<SDL_GLContext*>(glcon));
  delete static_cast<SDL_GLContext*>(glcon);
  SDL_DestroyWindow(window);
}


void SDLVideoDevice::set_buffer(Framebuffer const& buffer) {
  glTexSubImage2D(
    GL_TEXTURE_2D,
    0,
    0,
    0,
    256,
    240,
    GL_RGBA, 
    GL_UNSIGNED_INT_8_8_8_8, 
    (const GLvoid*)buffer.data()
  );
  glBegin(GL_TRIANGLE_STRIP);
  glTexCoord2f(0.0, 0.0f);  glVertex2i(0,0);
  glTexCoord2f(1.0, 0.0f);  glVertex2i(256,0);
  glTexCoord2f(0.0, 1.0f);  glVertex2i(0,240);
  glTexCoord2f(1.0, 1.0f);  glVertex2i(256,240);
  glEnd();
  SDL_GL_SwapWindow(window);
  glClear(GL_COLOR_BUFFER_BIT);
}

