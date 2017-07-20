#pragma once

#include <ncl/gl/Scene.h>
#include <ncl/gl/CrossHair.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <set>

using namespace std;
using namespace ncl;
using namespace gl;
using namespace glm;

const static float _PI = pi<float>();

static enum LightType { PHONG, RAY_TRACE, NO_OF_LIGHT_TYPES };

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
		glClearColor(1, 1, 1, 0);
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
		lightType = RAY_TRACE;
	}

	~LitScene() {
		delete lightController;
	}

	virtual void init() override {
		font = Font::Arial(12);
		model = new Model("..\\media\\blocks.obj");
		initQuad();
		buildTBOs();
		initRayTraceImage();
		crossHair = new CrossHair(5);

		light[0].on = true;
		light[0].transform = true;
	//	light[0].position = vec4{ 0, 30, 0, 1.0};
		light[0].position.x = radius * cos(theta)*sin(phi);
		light[0].position.y = radius * cos(phi);
		light[0].position.z = radius * sin(theta)*sin(phi);
		light[0].position.w = 1.0;
		light[0].spotDirection = { 0, 0, 1, 0 };
		shader("render")([&](Shader& s) {
			s.send(lightModel);
		});
		lightController->setOrientation(inverse(quat_cast(cam.view)));
		
	}

	void initRayTraceImage() {
		glDeleteTextures(1, &scene_img);
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &scene_img);
		glBindTexture(GL_TEXTURE_2D, scene_img);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, _width, _height);
	}

	void initQuad() {
		Mesh mesh;
		mesh.positions = {
			{ -1.0f, -1.0f, 0.0f },
			{ 1.0f, -1.0f, 0.0f },
			{ 1.0f,  1.0f, 0.0f },
			{ -1.0f,  1.0f, 0.0f }
		};
		mesh.uvs[0] = {
			{0, 0},
			{1, 0},
			{1, 1},
			{0, 1}
		};
		mesh.primitiveType = GL_TRIANGLE_FAN;
		quad = new ProvidedMesh(mesh);

	}

	void buildTBOs() {
		vector<Mesh> meshes = model->getMeshes();
		vector<vec4> vertices;
		vector<float> indices;
		GLuint offset = 0;
		auto textures = loadMatrials(meshes);
		for (int i = 0; i < meshes.size(); i++) {
			Mesh& mesh = meshes[i];
			for (vec3& v : mesh.positions) {
				vertices.push_back(vec4(v, 0));
			}
			size_t size = mesh.indices.size();
			logger.info("size: " + to_string(size));
			for (int j = 0; j < size; j += 3) {
				indices.push_back(mesh.indices[j] + offset);
				indices.push_back(mesh.indices[j + 1] + offset);
				indices.push_back(mesh.indices[j + 2] + offset);
				indices.push_back(textures[i]);	

			}
			offset += mesh.positions.size();
		}

		vector<vec4> uniqueVertices;
		vector<int> newIndices;
		for (int i = 0; i < vertices.size(); i++) {
			vec4 vo = vertices[i];
			auto itr = find_if(uniqueVertices.begin(), uniqueVertices.end(), [&](vec4& v1) {
				return vo.x == v1.x && vo.y == v1.y && vo.z == v1.z && vo.w == v1.w;
			});
			if (itr == uniqueVertices.end()) {
				uniqueVertices.push_back(vo);
			}
		}

		for (int i = 0; i < uniqueVertices.size(); i++) {
			set<int> tIndices;
			vec4 vo = uniqueVertices[i];
			for (int j = 0; j < indices.size(); j++) {
				if ((j+1) % 4 == 0) continue;
				int idx = indices[j];
				vec4 v1 = vertices[idx];
				if ((vo.x == v1.x && vo.y == v1.y && vo.z == v1.z && vo.w == v1.w)) {
					indices[j] = i;
				}
			}
			//tIndices;
			//int minIdx = *tIndices.begin();
			//stringstream ss;
			//ss << minIdx << " -> { ";
			//for (int idx : tIndices) {
			//	ss << idx << " ";
			//}
			//ss << "}";
			//logger.info(ss.str());
			//for (int j = 0; j < indices.size(); j++) {
			//	int idx = indices[j];
			//	auto itr = find(tIndices.begin(), tIndices.end(), idx);
			//	if (itr != tIndices.end()) {
			//		indices[j] = minIdx;
			//	}
			//}
		}

		shader("raytrace_render")([&](Shader& s) {
			vertices_tbo = new TextureBuffer(&vertices[0], sizeof(vec4) * uniqueVertices.size(), GL_RGBA32F, 1);
			s.sendUniform1ui("vertices_tbo", 1);
			
			triangle_tbo = new TextureBuffer(&indices[0], sizeof(float) * indices.size(), GL_RGBA32F, 2);
			s.sendUniform1ui("triangle_tbo", 2);

			s.sendUniform4f("bgColor", 1, 1, 1, 1);
			vec3 pos = light[0].position.xyz;
			s.sendUniform3f("lightPos", pos.x, pos.y, pos.z);
			vec3 min = model->bound->min();

			s.sendUniform3f("aabb.min", min.x, min.y, min.z);
			vec3 max = model->bound->max();
			s.sendUniform3f("aabb.max", max.y, max.y, max.z);
			unsigned int noOfTriangles = indices.size() / 4;
			s.sendUniform1ui("NO_OF_TRIANGLES", noOfTriangles);
		});

	}

	map<unsigned, unsigned> loadMatrials(vector<Mesh> meshes) {
		map<string, unsigned> textures;
		map<unsigned, unsigned> meshTextureIdMap;
		set<string> paths;

		for (Mesh& mesh : meshes) {
			if (mesh.material.diffuseTexPath != "") paths.insert(mesh.material.diffuseTexPath);
		}

		int noTexture = paths.size();

		glActiveTexture(GL_TEXTURE3);
		glGenTextures(1, &textureMap);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureMap);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		int textureId = 0;
		for (int i = 0; i < meshes.size(); i++) {
			Material& material = meshes[i].material;
			if (material.diffuseMat != -1) {
				auto itr = textures.find(material.diffuseTexPath);
				if (itr == textures.end()) {
					textures.insert(make_pair(material.diffuseTexPath, textureId));
					meshTextureIdMap.insert(make_pair(i, textureId));
					Image img(material.diffuseTexPath);
					if (textureId == 0) {
						glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, img.width(), img.height(), noTexture, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, textureId, img.width(), img.height(), 1, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
					textureId++;
				}
				else {
					meshTextureIdMap.insert(make_pair(i, itr->second));
				}
			}
			else {
				meshTextureIdMap.insert(make_pair(i, 255));
			}
		}
		shader("raytrace_render")([&](Shader& s) {
			s.sendUniform1ui("textureMap", 3);
		});
		return meshTextureIdMap;
	}

	virtual void display() override{
		
		renderCrossHair();
		switch (lightType) {
		case PHONG:
			renderRaster();
			currentLightType = "Lighting Type: Phong";
			break;
		case RAY_TRACE:
			renderRayTrace();
			currentLightType = "Lighting Type: Ray Trace";
			break;
		default:
			break;
		}
		renderText();
		

	}

	void renderRaster() {
		shader("render")([&](Shader& s) {
			cam.model = mat4(1);
			s.sendUniformLight("light[0]", light[0]);
			s.sendComputed(cam);
			model->draw(s);
		});

	}

	void renderRayTrace() {
		cam.view = translate(mat4(1), { 0, 0, dist });
		cam.view = rotate(cam.view, radians(pitch), { 1, 0, 0 });
		cam.view = rotate(cam.view, radians(yaw), { 0, 1, 0 });
		mat4 invMV = inverse(cam.view * cam.model);
		mat4 invMVP = inverse(cam.projection * cam.view * cam.model);
		vec3 eyes = column(invMV, 3).xyz;
		//shader("raytrace")([&](Shader& s) {
		//	glActiveTexture(GL_TEXTURE0);
		//	glBindImageTexture(0, scene_img, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		//	s.sendUniform1ui("scene_img", scene_img);
		//	s.sendUniform3fv("eyes", 1, &eyes[0]);
		//	s.sendUniformMatrix4fv("invMVP", 1, GL_FALSE, value_ptr(invMVP));
		//	
		//	glDispatchCompute(_width / 32, _height / 32, 1);
		//});

		shader("raytrace_render")([&](Shader& s) {
			//glActiveTexture(GL_TEXTURE0);
			//glBindTexture(GL_TEXTURE_2D, scene_img);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			s.sendUniform3fv("eyes", 1, &eyes[0]);
			s.sendUniformMatrix4fv("invMVP", 1, GL_FALSE, value_ptr(invMVP));
		//	s.sendUniform1ui("scene_img", scene_img);
			quad->draw(s);
		});
	}

	void renderCrossHair() {
		shader("flat")([&](Shader& s) {
			cam.model = translate(mat4(1), vec3(light[0].position));
			s.sendComputed(cam);
			crossHair->draw(s);
		});
	}

	void renderText() {
		stringstream ss;
		ss << "FPS: " << fps << endl << currentLightType;
		font->render(ss.str(), 10, _height - 20);
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
			case ' ':
				lightType = (lightType + 1)%NO_OF_LIGHT_TYPES;
				break;
			default:
				break;
			}
		}
	}

	virtual void resized() override {
		font->resize(_width, _height);
		cam.projection = perspective(radians(60.0f), aspectRatio, 0.1f, 1000.0f);
		initRayTraceImage();
	}

private:
	Model* model;
	ProvidedMesh* quad;
	CrossHair* crossHair;
	LightController* lightController;
	Logger logger = Logger::get("LitScene");
	float pitch = 22, yaw = 116, dist = -120;
	TextureBuffer* triangle_tbo;
	TextureBuffer* vertices_tbo;
	GLuint textureMap;
	Font* font;
	int lightType;
	string currentLightType;
	GLuint scene_img;
	float theta = 0.66f;
	float phi = -1.0f;
	float radius = 70;

};