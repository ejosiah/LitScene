// LitScene.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#ifndef CONNECT_3D
#define CONNECT_3D
#endif
#define GLM_SWIZZLE 

#include <ncl/gl/GlfwApp.h>
#include "LitScene.h"
using namespace std;
using namespace ncl;
using namespace gl;

static ConsoleAppender CONSOLE_APPENDER;
vector<Appender*> Logger::appenders = { &CONSOLE_APPENDER };
const unsigned int Logger::level = LEVEL_DEBUG;

int main()
{
	Scene* scene = new LitScene;
	start(scene);
	return 0;
}
