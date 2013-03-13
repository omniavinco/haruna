﻿// Ŭnicofde please 
#include "stdafx.h"
#include "debug_draw.h"

#include <memory>

#include "sora/low_level_c_file.h"
#include "sora/filesystem.h"
#include "sora/math_helper.h"
#include "haruna/primitive_mesh.h"
#include "shader.h"
#include "gl_env.h"
#include "sys_font.h"
#include "texture.h"

using glm::vec4;
using glm::vec3;
using sora::Filesystem;

namespace haruna {;
namespace gl {
	glm::vec4 ConvertColor(const haruna::vec4ub &orig) 
	{
		vec4 color;
		for(int i = 0 ; i < 4 ; i++) {
			color[i] = (float)orig[i] / 255.0f;
		}
		return color;
	}

	class ColorShader {
	public:
		ColorShader() {}
		~ColorShader() {}

		bool Init()
		{
			std::string fs_path = sora::Filesystem::GetAppPath("shader/color_shader.fs");
			std::string vs_path = sora::Filesystem::GetAppPath("shader/color_shader.vs");
			sora::ReadonlyCFile fs_file = sora::ReadonlyCFile(fs_path);
			sora::ReadonlyCFile vs_file = sora::ReadonlyCFile(vs_path);
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

			pos_loc_ = prog_->GetAttribLocation("a_position");
		
			mvp_loc_ = prog_->GetUniformLocation("u_mvp");
			color_loc_ = prog_->GetUniformLocation("u_color");

			return true;
		}

		bool Deinit()
		{
			if(prog_.get() != nullptr) {
				return prog_->Deinit();
			}
			return false;
		}

		void ApplyColor(const haruna::vec4ub &color)
		{
			vec4 colorf = ConvertColor(color);
			glUniform4fv(color_loc(), 1, glm::value_ptr(colorf));
		}

		const haruna::gl::ShaderLocation &mvp_loc() const { return mvp_loc_; }
		const haruna::gl::ShaderLocation &pos_loc() const { return pos_loc_; }
		const haruna::gl::ShaderLocation &color_loc() const { return color_loc_; }

		const ShaderProgram* prog() const { return prog_.get(); }
		ShaderProgram* prog() { return prog_.get(); }
	private:
		std::unique_ptr<haruna::gl::ShaderProgram> prog_;

		haruna::gl::ShaderLocation mvp_loc_;
		haruna::gl::ShaderLocation pos_loc_;
		haruna::gl::ShaderLocation color_loc_;
	};

	class TextShader {
	public:
		TextShader() {}
		~TextShader() {}

		bool Init()
		{
			std::string fs_path = sora::Filesystem::GetAppPath("shader/text.fs");
			std::string vs_path = sora::Filesystem::GetAppPath("shader/text.vs");
			sora::ReadonlyCFile fs_file = sora::ReadonlyCFile(fs_path);
			sora::ReadonlyCFile vs_file = sora::ReadonlyCFile(vs_path);
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

			pos_loc_ = prog_->GetAttribLocation("a_position");
			texcoord_loc_ = prog_->GetAttribLocation("a_texcoord");
		
			mvp_loc_ = prog_->GetUniformLocation("u_mvp");
			color_loc_ = prog_->GetUniformLocation("u_color");
			font_tex_loc_ = prog_->GetUniformLocation("s_tex");

			return true;
		}
		bool Deinit()
		{
			if(prog_.get() != nullptr) {
				return prog_->Deinit();
			}
			return false;
		}

		void ApplyColor(const haruna::vec4ub &color)
		{
			vec4 colorf = ConvertColor(color);
			glUniform4fv(color_loc(), 1, glm::value_ptr(colorf));
		}

		const ShaderProgram* prog() const { return prog_.get(); }
		ShaderProgram* prog() { return prog_.get(); }

		const haruna::gl::ShaderLocation &mvp_loc() const { return mvp_loc_; }
		const haruna::gl::ShaderLocation &pos_loc() const { return pos_loc_; }
		const haruna::gl::ShaderLocation &color_loc() const { return color_loc_; }
		const haruna::gl::ShaderLocation &texcoord_loc() const { return texcoord_loc_; }
		const haruna::gl::ShaderLocation &font_tex_loc() const { return font_tex_loc_; }

	private:
		std::unique_ptr<haruna::gl::ShaderProgram> prog_;

		haruna::gl::ShaderLocation mvp_loc_;
		haruna::gl::ShaderLocation pos_loc_;
		haruna::gl::ShaderLocation texcoord_loc_;
		haruna::gl::ShaderLocation color_loc_;
		haruna::gl::ShaderLocation font_tex_loc_;
	};

	std::vector<DrawCmdData<Vertex_1P>> wire_sphere_mesh;
	std::unique_ptr<ColorShader> color_shader;
	std::unique_ptr<TextShader> text_shader;
	std::unique_ptr<SysFont> sys_font;

	////////////////

	bool init_debug_draw()
	{
		WireSphereFactory factory(1, 8, 8);
		wire_sphere_mesh = factory.CreateSimpleMesh();

		sys_font.reset(new SysFont());
		if(!sys_font->Init()) {
			return false;
		}

		color_shader.reset(new ColorShader());
		if(!color_shader->Init()) {
			return false;
		}

		text_shader.reset(new TextShader());
		if(!text_shader->Init()) {
			return false;
		}
		return true;
	}
	bool deinit_debug_draw()
	{
		if(sys_font.get() != nullptr) {
			bool retval = sys_font->Deinit();
			sys_font.reset(nullptr);
			if(!retval) {
				return false;
			}
		}

		if(color_shader.get() != nullptr) {
			bool retval = color_shader->Deinit();
			color_shader.reset(nullptr);
			if(!retval) {
				return false;
			}
		}

		if(text_shader.get() != nullptr) {
			bool retval = text_shader->Deinit();
			text_shader.reset(nullptr);
			if(!retval) {
				return false;
			}
		}
		return true;
	}

	
	struct LineWidthReplacer {
		LineWidthReplacer(float width) : width_(width) 
		{
			if(width_ != 1.0f) {
				glLineWidth(width);
			}
		}
		~LineWidthReplacer()
		{
			if(width_ != 1.0f) {
				glLineWidth(1.0f);
			}
		}
	private:
		float width_;
	};

	struct PointSizeReplacer {
		PointSizeReplacer(float size) : size_(size) 
		{
			if(size != 1.0f) {
				glPointSize(size_);
			}
		}
		~PointSizeReplacer()
		{
			if(size_ != 1.0f) {
				glPointSize(1.0f);
			}
		}
	private:
		float size_;
	};

		

	DebugDrawer3D::DebugDrawer3D()
	{
	}

	DebugDrawer3D::~DebugDrawer3D()
	{
	}

	void DebugDrawer3D::Draw(const DebugDrawManager &mgr) 
	{
		color_shader->prog()->Use();

		auto pos_loc = color_shader->pos_loc();
		glEnableVertexAttribArray(pos_loc);

		/*
		RenderState *dev = &Device::GetInstance()->render_state();
		dev->Set3D();
		DrawCmdList(mgr);
		*/

		// 깊이씹고 렌더링하는것과 깊이고려 렌더링을 분리해서 그리자
		// 굳이 순서대로 그릴 이유가 없겟더라
		//glDisable(GL_DEPTH_TEST);
		//glEnable(GL_DEPTH_TEST);
	}

	void DebugDrawer3D::DrawElem(DebugDraw3D_Line *cmd) 
	{
		/*		
		mat4 mvp = cmd->GetMVPMatrix();
		ShaderVariable mvp_var = shader->uniform_var(kMVPHandleName);
		SR_ASSERT(mvp_var.location != kInvalidShaderVarLocation);
		SetUniformMatrix(mvp_var, mvp);		
		*/

		color_shader->ApplyColor(cmd->color);

		std::array<vec3, 2> vertex_list;
		vertex_list[0] = cmd->p1;
		vertex_list[1] = cmd->p2;
		auto pos_loc = color_shader->pos_loc();
		glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 0, &vertex_list[0]);

		LineWidthReplacer replacer(cmd->line_width);
		glDrawArrays(GL_LINES, 0, 2);
	}
	void DebugDrawer3D::DrawElem(DebugDraw3D_Cross *cmd) 
	{
		/*
		mat4 mvp = cmd->GetMVPMatrix();
		ShaderVariable mvp_var = shader.uniform_var(kMVPHandleName);
		SetUniformMatrix(mvp_var, mvp);
		*/

		color_shader->ApplyColor(cmd->color);

		std::array<glm::vec3, 0> vert_list;
		vert_list[0] = cmd->pos;
		auto pos_loc = color_shader->pos_loc();
		glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, 0, &vert_list[0]);

		PointSizeReplacer replacer(cmd->size);
		glDrawArrays(GL_POINTS, 0, 1);
	}

	void DebugDrawer3D::DrawElem(DebugDraw3D_Sphere *cmd) 
	{
		/*
		ShaderManager *shader_mgr = Device::GetInstance()->shader_mgr();
		Shader &shader = *(shader_mgr->Get(ShaderManager::kConstColor));
		RenderState *dev = &Device::GetInstance()->render_state();
		dev->UseShader(shader);

		vec4 color = Draw2DPolicy::ConvertColor(cmd->color);
		shader.SetUniformVector(kConstColorHandleName, color);

		static bool init_mesh = false;
		static GeometricObject<glm::vec3> mesh;
		if(init_mesh == false) {
		init_mesh = true;
		mesh.WireSphere(1.0f, 16, 16);
		}

		mat4 mvp = cmd->GetMVPMatrix();
		mvp = glm::translate(mvp, cmd->pos);
		mvp = glm::scale(mvp, vec3(cmd->radius, cmd->radius, cmd->radius));
		ShaderVariable mvp_var = shader.uniform_var(kMVPHandleName);
		SetUniformMatrix(mvp_var, mvp);
		*/

		color_shader->ApplyColor(cmd->color);

				/*
		//resize! cmd->size
		auto it = wire_sphere_mesh.begin();
		auto endit = wire_sphere_mesh.end();
		for( ; it != endit ; ++it) {
			const DrawCmdData<Vertex_1P> &cmd = *it;
			auto index_list = cmd.index_list;
			auto vert_list = cmd.vertex_list;

			SR_ASSERT(index_list.empty() == false);

			glVertexAttribPointer(color_shader->pos_loc(), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_1P), &vert_list[0]);
			glDrawElements(GL_LINES, index_list.size(), GL_UNSIGNED_SHORT, &index_list[0]);
		}
		*/
	}

	void DebugDrawer3D::DrawElem(DebugDraw3D_String *cmd) 
	{
		/*
		//billboard 같은 느낌으로 글자 쓰기
		//기울어지는거 없이 항상 글자가 뜨도록 적절히 만들기
		mat4 mvp = cmd->GetMVPMatrix();
		vec4 cliping_pos = mvp * vec4(cmd->pos, 1);   // -1~1에 적절히 위치한 좌표(로 아마도 보정되엇을거다)
		cliping_pos /= cliping_pos.w;
		cliping_pos.z = -cliping_pos.z; //보정된 좌표계는 z방향 다르다

		RenderState *dev = &Device::GetInstance()->render_state();
		float win_w = (float)dev->win_width();
		float win_h = (float)dev->win_height();
		// -1~+1로 보정된 좌표를 윈도우좌표로 변환
		vec3 win_coord(
		(cliping_pos.x + 1) * win_w/2.0f,
		(cliping_pos.y + 1.0f) * win_h/2.0f,
		cliping_pos.z);

		ShaderManager *shader_mgr = Device::GetInstance()->shader_mgr();
		Shader &shader = *(shader_mgr->Get(ShaderManager::kText));
		dev->UseShader(shader);

		vec4 color = Draw2DPolicy::ConvertColor(cmd->color);
		shader.SetUniformVector(kConstColorHandleName, color);

		sora::SysFont *font = Device::GetInstance()->sys_font();
		dev->UseTexture(font->font_texture(), 0);

		mat4 mvp_mat = glm::ortho(0.0f, win_w, 0.0f, win_h);
		mvp_mat = glm::translate(mvp_mat, win_coord);
		mvp_mat = glm::scale(mvp_mat, vec3(cmd->scale));
		ShaderVariable mvp_var = shader.uniform_var(kMVPHandleName);
		SetUniformMatrix(mvp_var, mvp_mat);

		//2d처럼 렌더링하기
		glEnable(GL_BLEND);

		sora::Label label(font, cmd->msg);
		shader.SetVertexList(label.vertex_list());
		shader.DrawElements(kDrawTriangles, label.index_list());

		//3d용으로 복구
		glDisable(GL_BLEND);

		//점하나 적절히 찍기
		DebugDraw3D_Cross cross_cmd;
		cross_cmd.color = cmd->color;
		cross_cmd.depth_enable = cmd->depth_enable;
		cross_cmd.duration = cmd->duration;
		cross_cmd.model_mat = cmd->model_mat;
		cross_cmd.view_mat = cmd->view_mat;
		cross_cmd.projection_mat = cmd->projection_mat;
		cross_cmd.pos = cmd->pos;
		cross_cmd.size = 5;
		DrawElem(&cross_cmd);
		*/

		color_shader->ApplyColor(cmd->color);
	}

	void DebugDrawer3D::DrawElem(DebugDraw3D_Axis *cmd) 
	{
		/*
		ShaderManager *shader_mgr = Device::GetInstance()->shader_mgr();
		Shader *shader = shader_mgr->Get(ShaderManager::kVertexColor);
		RenderState *render_state = &Device::GetInstance()->render_state();
		render_state->UseShader(*shader);

		vector<ColorVertex3D> vertex_list;
		vertex_list.reserve(6);

		vec4ub red(255, 0, 0, 255);
		vec4ub green(0, 255, 0, 255);
		vec4ub blue(0, 0, 255, 255);

		// x = red
		vertex_list.push_back(CreateColorVertex3D(0, 0, 0, red));
		vertex_list.push_back(CreateColorVertex3D(cmd->size, 0, 0, red));

		// y = green
		vertex_list.push_back(CreateColorVertex3D(0, 0, 0, green));
		vertex_list.push_back(CreateColorVertex3D(0, cmd->size, 0, green));

		// z = blue
		vertex_list.push_back(CreateColorVertex3D(0, 0, 0, blue));
		vertex_list.push_back(CreateColorVertex3D(0, 0, cmd->size, blue));

		mat4 mvp = cmd->GetMVPMatrix() * cmd->xf;
		ShaderVariable mvp_var = shader->uniform_var(kMVPHandleName);
		SetUniformMatrix(mvp_var, mvp);

		shader->SetVertexList(vertex_list);
		shader->DrawArrays(kDrawLines, vertex_list.size());  
		*/

		color_shader->ApplyColor(cmd->color);
	}

	void DebugDrawer2D::DrawElem(DebugDraw2D_Line *cmd) 
	{
		//TODO
		float width = 640;
		float height = 480;

		glm::mat4 model_mat;
		glm::mat4 view_mat;
		glm::mat4 proj_mat = glm::ortho(0.0f, width, 0.0f, height);

		glm::mat4 mvp_mat = proj_mat * view_mat * model_mat;
		glUniformMatrix4fv(color_shader->mvp_loc(), 1, GL_FALSE, glm::value_ptr(mvp_mat));

		color_shader->ApplyColor(cmd->color);

		std::array<glm::vec3, 2> vert_list;
		vert_list[0] = vec3(cmd->p1.x, cmd->p1.y, 0);
		vert_list[1] = vec3(cmd->p2.x, cmd->p2.y, 0);
		glVertexAttribPointer(color_shader->pos_loc(), 3, GL_FLOAT, GL_FALSE, 0, &vert_list[0]);

		LineWidthReplacer replacer(cmd->line_width);
		glDrawArrays(GL_LINES, 0, 2);
		
	}
	void DebugDrawer2D::DrawElem(DebugDraw2D_Cross *cmd) 
	{
		//TODO
		float width = 640;
		float height = 480;

		glm::mat4 model_mat;
		glm::mat4 view_mat;
		glm::mat4 proj_mat = glm::ortho(0.0f, width, 0.0f, height);

		glm::mat4 mvp_mat = proj_mat * view_mat * model_mat;
		glUniformMatrix4fv(color_shader->mvp_loc(), 1, GL_FALSE, glm::value_ptr(mvp_mat));


		color_shader->ApplyColor(cmd->color);

		std::array<glm::vec3, 1> vert_list;
		vert_list[0] = vec3(cmd->pos.x, cmd->pos.y, 0);
		glVertexAttribPointer(color_shader->pos_loc(), 3, GL_FLOAT, GL_FALSE, 0, &vert_list[0]);

		PointSizeReplacer replacer(cmd->size);
		glDrawArrays(GL_POINTS, 0, 1);
	}

	void DebugDrawer2D::DrawElem(DebugDraw2D_Circle *cmd) 
	{
		//TODO
		float width = 640;
		float height = 480;

		glm::mat4 model_mat;
		model_mat = glm::translate(model_mat, vec3(cmd->pos.x, cmd->pos.y, 0));
		model_mat = glm::scale(model_mat, glm::vec3(cmd->radius, cmd->radius, 1));
		glm::mat4 view_mat;
		glm::mat4 proj_mat = glm::ortho(0.0f, width, 0.0f, height);

		glm::mat4 mvp_mat = proj_mat * view_mat * model_mat;
		glUniformMatrix4fv(color_shader->mvp_loc(), 1, GL_FALSE, glm::value_ptr(mvp_mat));

		color_shader->ApplyColor(cmd->color);

		std::vector<glm::vec3> point_list;
		point_list.push_back(glm::vec3(0, 0, 0));

		float split = 32;
		for(int i = 0 ; i < split + 1 ; i++) {
			float rad_seg = sora::kPi / split  * 2;
			float rad = rad_seg * i;
			float x = cos(rad);
			float y = sin(rad);

			point_list.push_back(glm::vec3(x, y, 0));
		}

		glVertexAttribPointer(color_shader->pos_loc(), 3, GL_FLOAT, GL_FALSE, 0, &point_list[0]);
		glDrawArrays(GL_LINE_STRIP, 0, point_list.size());
	}

	void DebugDrawer2D::DrawElem(DebugDraw2D_String *cmd) 
	{
		//TODO 
		float width = 640;
		float height = 480;

		//폰트의 왼쪽위를 원점으로 사용하기 위해서 약간 올린다
		float base_line = SysFont::kFontSize * cmd->scale;

		glm::mat4 model_mat;
		model_mat = glm::translate(model_mat, vec3(cmd->pos.x, cmd->pos.y + base_line, 0));
		model_mat = glm::scale(model_mat, glm::vec3(cmd->scale, cmd->scale, 1));
		glm::mat4 view_mat;
		glm::mat4 proj_mat = glm::ortho(0.0f, width, 0.0f, height);
		glm::mat4 mvp_mat = proj_mat * view_mat * model_mat;
		glUniformMatrix4fv(text_shader->mvp_loc(), 1, GL_FALSE, glm::value_ptr(mvp_mat));

		text_shader->ApplyColor(cmd->color);

		Label label(sys_font.get(), cmd->msg);
		auto vert_list = label.vertex_list();
		auto index_list = label.index_list();

		int stride = sizeof(vert_list[0]);
		glVertexAttribPointer(text_shader->pos_loc(), 3, GL_FLOAT, GL_FALSE, stride, &vert_list[0].p);
		glVertexAttribPointer(text_shader->texcoord_loc(), 2, GL_FLOAT, GL_FALSE, stride, &vert_list[0].uv);
		glDrawElements(GL_TRIANGLES, index_list.size(), GL_UNSIGNED_SHORT, &index_list[0]);
	}

	
	
	void DebugDrawer2D::Draw(const DebugDrawManager &mgr) 
	{
		//string은 쉐이더가 다르니 따로 그린다
		//종류를 나눠서 그리다보니까 높이 정보는 씹는다
		//어차피 2차원 디버깅은 통계 관련에서만 쓸데니까 별 문제 없을거다
		std::vector<DebugDraw2D*> color_list;
		std::vector<DebugDraw2D*> text_list;

		auto it = mgr.begin_2d();
		auto endit = mgr.end_2d();
		for( ; it != endit ; ++it) {
			DebugDraw2D *cmd = it->get();
			if(cmd->type == kDebugDraw2DString) {
				text_list.push_back(cmd);
			} else {
				color_list.push_back(cmd);
			}
		}

		//2차원 환경에서의 렌더링 기본 설정
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		//RenderState *dev = &Device::GetInstance()->render_state();
		//dev->Set2D();
		color_shader->prog()->Use();
		glEnableVertexAttribArray(color_shader->pos_loc());
		for(DebugDraw2D *cmd : color_list) {
			AbstractDebugDrawer2D::DrawElem(cmd);
		}
		
		text_shader->prog()->Use();
		glEnableVertexAttribArray(text_shader->pos_loc());
		glEnableVertexAttribArray(text_shader->texcoord_loc());

		// 폰트 텍스쳐는 1번만 쓰면 된다
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sys_font->font_texture()->tex());
		glUniform1i(text_shader->font_tex_loc(), 0);

		for(DebugDraw2D *cmd : text_list) {
			AbstractDebugDrawer2D::DrawElem(cmd);
		}
	}

	DebugDrawer2D::DebugDrawer2D()
	{
	}
	DebugDrawer2D::~DebugDrawer2D()
	{
	}
}	// namespace gl
}	// namespace haruna