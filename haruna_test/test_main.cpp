﻿// Ŭnicode please 
#include <gtest/gtest.h>

#include <GL/glew.h>
#include <GL/glfw.h>

#include <iostream>

#include "sora/filesystem.h"
#include "sora/low_level_c_file.h"

int main(int argc, char *argv[])
{
	if(!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	if(!glfwOpenWindow(640, 480, 0, 0, 0, 0, 0, 0, GLFW_WINDOW)) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	GLenum err = glewInit();
	if(GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));		
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	::testing::InitGoogleTest(&argc, argv);
	int result = RUN_ALL_TESTS();

	glfwTerminate();

	getchar();
	return 0;	
}