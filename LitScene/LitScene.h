#pragma once

#include <ncl/gl/Scene.h>
#include <ncl/gl/CrossHair.h>
#include <glm/gtx/euler_angles.hpp>

using namespace std;
using namespace ncl;
using namespace gl;
using namespace glm;

const static float _PI = pi<float>();

class LightController : public _3DMotionEventHandler {
public:
	LightController(LightSource& light)
		:light(light), orientation(orientation){
	}

	virtual void onMotion(const _3DMotionEvent& event) override {
		vec3 t = vec3(mat4_cast(orientation) * vec4(event.translation, 1));
		if (light.spotAngle < 180.0f) {
			vec3 d = vec3(light.spotAngle);
			float lim = _PI / 2;
			float yaw = radians(event.rotation.y * 0.005f);
			float pitch = radians(event.rotation.x * 0.005f); 
			float roll = radians(event.rotation.z * 0.005f); 
			lightDirection *= Orientation({ pitch, yaw, roll });
			updatDirection();
		}
		
		light.position += vec4(t  * 0.001f, 0);
		vec4& p = light.position;
		stringstream ss;
		ss << "light(" << p.x << ", " << p.y << ", " << p.z << ")";
		logger.info(ss.str());
	}

	void setOrientation(Orientation orientation) {
		this->orientation = orientation;
		lightDirection = orientation;
		updatDirection();
	}
	
	void updatDirection() {
		vec3 d = normalize(mat3_cast(lightDirection) * vec3(0, -1, 0));
		light.spotDirection = vec4(d, 0);
	}

	virtual void onNoMotion() override {

	}
private:
	LightSource& light;
	Orientation orientation;
	Orientation lightDirection;
	Logger logger = Logger::get("LightController");
};

class LitScene : public Scene {
public:
	LitScene(): Scene("Lit scene", 1280, 960){
		_useImplictShaderLoad = true;
		addShader("flat", GL_VERTEX_SHADER, identity_vert_shader);
		addShader("flat", GL_FRAGMENT_SHADER, identity_frag_shader);
		lightController = new LightController(light[0]);
		_motionEventHandler = lightController;
	}

	~LitScene() {
		delete lightController;
	}

	virtual void init() override {
		model = new Model("..\\media\\blocks.obj");
		buildTBOs();
		crossHair = new CrossHair(5);
		cam.view = translate(mat4(1), { 0, 0, dist });
		cam.view = rotate(cam.view, radians(pitch), { 1, 0, 0 });
		cam.view = rotate(cam.view, radians(yaw), { 0, 1, 0 });
		light[0].on = true;
		light[0].transform = true;
		light[0].position = vec4{ 0, 30, 0, 1.0};
		light[0].spotDirection = { 0, 0, 1, 0 };
		shader("render")([&](Shader& s) {
			s.send(lightModel);
		});
		lightController->setOrientation(inverse(quat_cast(cam.view)));
		
	}

	void buildTBOs() {
		vector<Mesh> meshes = model->getMeshes();
		vector<vec3> vertices;
		vector<GLuint> indices;
		GLuint offset = 0;
		for (Mesh& mesh : meshes) {
			for (vec3& v : mesh.positions) {
				vertices.push_back(v);
			}
			size_t size = mesh.indices.size();
			for (int i = 0; i < size; i += 3) {
				indices.push_back(mesh.indices[i] + offset);
				indices.push_back(mesh.indices[i + 1] + offset);
				indices.push_back(mesh.indices[i + 2] + offset);
				indices.push_back(255);	// texture id

			}
			offset += mesh.indices.size();
		}
		vertices_tbo = new TextureBuffer(&vertices[0], sizeof(vec3) * vertices.size(), GL_RGBA32F, 0);
		triangle_tbo = new TextureBuffer(&indices[0], sizeof(GLuint) * indices.size(), GL_RGBA32UI, 1);
	}

	virtual void display() override{
		shader("render")([&](Shader& s) {
			cam.model = mat4(1);
			s.sendUniformLight("light[0]", light[0]);
			s.sendComputed(cam);
			model->draw(s);
		});

		shader("flat")([&](Shader& s) {
			cam.model = translate(mat4(1), vec3(light[0].position));
			s.sendComputed(cam);
			crossHair->draw(s);
		});
	}

	virtual void update(float dt) override {

	}

	virtual void processInput(const Key& key) override {
		if (key.pressed()) {
			switch (key.value()) {
			case 's':
				if (light[0].spotAngle == 180.0f) {
					light[0].spotAngle = 45.0f;
				}
				else {
					light[0].spotAngle = 180.0f;
				}
				break;
			default:
				break;
			}
		}
	}

	virtual void resized() override {
		cam.projection = perspective(radians(60.0f), aspectRatio, 0.1f, 1000.0f);
	}

private:
	Model* model;
	Teapot* teapot;
	CrossHair* crossHair;
	LightController* lightController;
	Logger logger = Logger::get("LitScene");
	float pitch = 22, yaw = 116, dist = -120;
	TextureBuffer* triangle_tbo;
	TextureBuffer* vertices_tbo;
	GLuint scene_img;

};