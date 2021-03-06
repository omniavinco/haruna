﻿// Ŭnicode please 
#include "exe_stdafx.h"
#include "texture_mapping.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <GL/glew.h>
#include <GL/glfw.h>

#include <vector>

#include "sora/assert_inc.h"
#include "sora/io/filesystem.h"
#include "sora/io/low_level_c_file.h"
#include "haruna/gl/shader.h"
#include "haruna/gl/texture.h"
#include "haruna/gl/gl_env.h"
#include "haruna/primitive_mesh.h"

using sora::io::Filesystem;
using sora::io::ReadonlyCFile;

Texture2DMapping::Texture2DMapping(float width, float height)
	: AbstractLogic(width, height), y_rot_(0)
{
}

Texture2DMapping::~Texture2DMapping()
{
	tex_->Deinit();
	prog_->Deinit();
}

bool Texture2DMapping::Init()
{
	//쉐이더 
	std::string fs_path = Filesystem::GetAppPath("shader/simple_tex.fs");
	std::string vs_path = Filesystem::GetAppPath("shader/simple_tex.vs");
	ReadonlyCFile fs_file(fs_path);
	ReadonlyCFile vs_file(vs_path);
	bool fs_open_result = fs_file.Open();
	bool vs_open_result = vs_file.Open();
	if(!fs_open_result) {
		return false; 
	}
	if(!vs_open_result) {
		return false;
	}
	
	std::string fs_src(static_cast<const char*>(fs_file.GetBuffer()));
	std::string vs_src(static_cast<const char*>(vs_file.GetBuffer()));
	
	haruna::gl::VertexShader vs(vs_src);
	haruna::gl::FragmentShader fs(fs_src);

	prog_.reset(new haruna::gl::ShaderProgram(vs, fs));
	bool prog_result = prog_->Init();
	if(!prog_result) {
		return false;
	}


	//create texture
	std::string tex_path = Filesystem::GetAppPath("texture/sora2.png");
	tex_.reset(new haruna::gl::Texture2D(tex_path));
	bool tex_init_result = tex_->Init();
	if(!tex_init_result) {
		return false;
	}

	return true;
}
bool Texture2DMapping::Update(float dt)
{
	y_rot_ += 4.0f * dt;
	bool running = !glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED);
	return running;
}
void Texture2DMapping::Draw()
{
	//sample mesh
	//haruna::SolidCubeFactory cube_factory(1, 1, 1);
	//auto data = cube_factory.CreateNormalMesh();
	//haruna::SolidSphereFactory sphere_factory(1, 16, 16);
	//auto data = sphere_factory.CreateNormalMesh();
	haruna::SolidTeapotFactory teapot_factory(0.02);
	auto data = teapot_factory.CreateNormalMesh();
	
	prog_->Use();
	haruna::gl::ShaderLocation pos_loc = prog_->GetAttribLocation("a_position");
	haruna::gl::ShaderLocation texcoord_loc = prog_->GetAttribLocation("a_texcoord");
	haruna::gl::ShaderLocation mvp_loc = prog_->GetUniformLocation("u_mvp");
	haruna::gl::ShaderLocation tex_loc = prog_->GetUniformLocation("s_tex");


	//projection
	float aspect = width() / height();
	glm::mat4 proj_mat = glm::perspective(60.0f, aspect, 0.1f, 100.0f);

	float radius = 2;
	glm::vec3 eye(cos(y_rot_) * radius, 0, sin(y_rot_) * radius);
	glm::vec3 center(0, 0, 0);
	glm::vec3 up(0, 1, 0);
	glm::mat4 view_mat = glm::lookAt(eye, center, up);
		

	//model
	glm::mat4 model_mat = glm::mat4();

	glm::mat4 mvp = proj_mat * view_mat * model_mat;

	//draw
	glViewport(0, 0, (int)width(), (int)height());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glUniformMatrix4fv(mvp_loc.handle(), 1, GL_FALSE, glm::value_ptr(mvp));
	glEnableVertexAttribArray(pos_loc.handle());
	glEnableVertexAttribArray(texcoord_loc.handle());

	//binding texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_->tex());
	glUniform1i(tex_loc.handle(), 0);

	for(auto cmd : data) {
		std::vector<haruna::Vertex_1P1N1UV> &vert_list = cmd.vertex_list;
		std::vector<unsigned short> &index_list = cmd.index_list;

		int stride = sizeof(haruna::Vertex_1P1N1UV);
		glVertexAttribPointer(pos_loc.handle(), 3, GL_FLOAT, GL_FALSE, stride, &vert_list[0].p);
		glVertexAttribPointer(texcoord_loc.handle(), 2, GL_FLOAT, GL_FALSE, stride, &vert_list[0].uv);

		GLenum draw_mode = ToDrawMode(cmd.draw_mode);
		glDrawElements(draw_mode, index_list.size(), GL_UNSIGNED_SHORT, &index_list[0]);
	}	

	haruna::gl::GLEnv::CheckError("End Frame");
}
