#pragma once

#include <set>
#include <random>

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <ncl/gl/Scene.h>
#include <ncl/gl/CrossHair.h>
#include <ncl/gl/rayTracingUtilities.h>


using namespace std;
using namespace ncl;
using namespace gl;
using namespace glm;

const static float _PI = pi<float>();

static enum LightType { PHONG, RAY_TRACE, PATH_TRACE, NO_OF_LIGHT_TYPES };

class LightController : public _3DMotionEventHandler {
public:
	LightController(LightSource& light)
		:light(light), orientation(orientation){
	}

	virtual void onMotion(const _3DMotionEvent& event) override {
	//	vec3 t = vec3(mat4_cast(orientation) * vec4(event.translation, 1));
		if (light.spotAngle < 180.0f) {
			vec3 d = vec3(light.spotAngle);
			float lim = _PI / 2;
			float yaw = radians(event.rotation.y * 0.005f);
			float pitch = radians(event.rotation.x * 0.005f); 
			float roll = radians(event.rotation.z * 0.005f); 
			lightDirection *= Orientation({ pitch, yaw, roll });
			updatDirection();
		}
		
	//	light.position += vec4(t  * 0.001f, 0);
	//	vec4& p = light.position;

	}

	void setOrientation(Orientation orientation) {
		this->orientation = orientation;
		lightDirection = orientation;
		updatDirection();
	}
	
	void updatDirection() {

	}

	virtual void onNoMotion() override {

	}
private:
	LightSource& light;
	Orientation orientation;
	Orientation lightDirection;
	Logger logger = Logger::get("LightController");
};

class CamController : public _3DMotionEventHandler {
public:
	CamController(float& yaw, float& pitch)
	:yaw(yaw), pitch(pitch){
	}

	virtual void onMotion(const _3DMotionEvent& event) override {
		reset(pitch += event.rotation.x * 0.005f);
		reset(yaw += event.rotation.y * 0.005f);
	}

	void reset(float& angle) {
		if (angle > 360) {
			angle -= 360;
		}
		else if (angle < 0) {
			angle += 360;
		}
	}

	virtual void onNoMotion() override {

	}
private:
	float &yaw, &pitch;
};

class LitScene : public Scene {
public:
	LitScene(): Scene("Lit scene", 1280, 960){
		_useImplictShaderLoad = true;
		addShader("flat", GL_VERTEX_SHADER, identity_vert_shader);
		addShader("flat", GL_FRAGMENT_SHADER, identity_frag_shader);
		lightType = RAY_TRACE;
	}

	~LitScene() {
		delete lightController;
	}

	virtual void init() override {
		_motionEventHandler = new Chain3DMotionEventHandler({ new LightController(light[0]), new CamController(yaw, pitch) });
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
	//	lightController->setOrientation(inverse(quat_cast(cam.view)));
		vec3 min = model->bound->min();
		vec3 max = model->bound->max();
		forShaders({ "raytrace", "pathtrace" }, [&](Shader& s) {
			s.sendUniform3fv("aabb.min", 1, value_ptr(min));
			s.sendUniform3fv("aabb.max", 1, value_ptr(max));
			s.sendUniform4fv("bgColor", 1, value_ptr(bg));

		});
		glClearColor(bg.x, bg.y, bg.z, bg.w);
		
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
		mesh.indices = { 0,1,2,0,2,3 };
	//	mesh.primitiveType = GL_TRIANGLE_FAN;
		quad = new ProvidedMesh(mesh);

	}

	void buildTBOs() {
		vector<Mesh> meshes = model->getMeshes();
		vector<vec4> vertices;
		vector<int> indices;
		vector<vec3> normals;
		GLuint offset = 0;
		auto textures = loadMatrials(meshes);
		for (int i = 0; i < meshes.size(); i++) {
			Mesh& mesh = meshes[i];
			for (int k = 0; k < mesh.positions.size(); k++) {
				vec4 v = vec4(mesh.positions[k], 0);
				vertices.push_back(v);
				normals.push_back(mesh.normals[k]);
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

		Mesh mesh;
		vector<vec4> uniqueVertices;
		vector<int> newIndices;
		for (int i = 0; i < vertices.size(); i++) {
			vec4 vo = vertices[i];
			auto itr = find_if(uniqueVertices.begin(), uniqueVertices.end(), [&](vec4& v1) {
				return vo.x == v1.x && vo.y == v1.y && vo.z == v1.z && vo.w == v1.w;
			});
			if (itr == uniqueVertices.end()) {
				uniqueVertices.push_back(vo);
				mesh.positions.push_back(vo.xyz);
				mesh.normals.push_back(normals[i]);
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
		}

		for (int i = 0; i < indices.size(); i+= 4) {
			ray_tracing::Triangle tri;
			tri.v0 = uniqueVertices[indices[i]];
			tri.v1 = uniqueVertices[indices[i + 1]];
			tri.v2 = uniqueVertices[indices[i + 2]];
			tri.tid = indices[i + 3];
			triangle_ssbo.triangles.push_back(tri);
		}

		glGenBuffers(1, &tri_ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, tri_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeOf(triangle_ssbo), NULL, GL_DYNAMIC_COPY);

		model2 = new ProvidedMesh(mesh);
		vertices_tbo = new TextureBuffer("vertices_tbo", &uniqueVertices[0], sizeof(vec4) * uniqueVertices.size(), GL_RGBA32F, 1);
		triangle_tbo = new TextureBuffer("triangle_tbo", &indices[0], sizeof(int) * indices.size(), GL_RGBA32I, 2);
		forShaders({ "raytrace", "pathtrace" }, [&](Shader& s) {	
			vertices_tbo->sendTo(s);
			triangle_tbo->sendTo(s);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, tri_ssbo);
			unsigned int noOfTriangles = indices.size() / 4;
			s.sendUniform1f("NO_OF_TRIANGLES", noOfTriangles);
			s.sendUniform1f("NO_OF_VERTICES", uniqueVertices.size());
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
			if (!material.usingDefaultMat) {
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
		forShaders({ "raytrace", "pathtrace" }, [&](Shader& s) {
			s.sendUniform1ui("textureMap", 3);
		});
		return meshTextureIdMap;
	}

	virtual void display() override{
		cam.view = translate(mat4(1), { 0, 0, dist });
		cam.view = rotate(cam.view, radians(pitch), { 1, 0, 0 });
		cam.view = rotate(cam.view, radians(yaw), { 0, 1, 0 });
		stringstream ss;
		switch (lightType) {
		case PHONG:
			renderRaster();
			currentLightType = "Lighting Type: Phong";
			break;
		case RAY_TRACE:
			renderRayTrace();
			currentLightType = "Lighting Type: Ray Trace";
			break;
		case PATH_TRACE:
			pathTrace();
			ss.str("");
			ss.clear();
			ss << "Lighting Type: Path Trace" << endl;
			ss << "\tNo of samples: " << samples;
			currentLightType = ss.str();
			break;
		default:
			break;
		}
		renderText();
		glDisable(GL_DEPTH_TEST);
		renderCrossHair();
		glEnable(GL_DEPTH_TEST);
		

	}

	void renderRaster() {
		vec3 lightPos = vec3(light[0].position);
		shader("render")([&](Shader& s) {
			cam.model = mat4(1);
		//	s.sendUniformLight("light[0]", light[0]);
			s.sendUniform3fv("lightPos", 1, &lightPos[0]);
			s.sendComputed(cam);
			model->draw(s);
		});

	}

	void renderRayTrace() {
		vec3 lightPos =  vec3(light[0].position);
		mat4 invMV = inverse(cam.view);
		mat4 invMVP = inverse(cam.projection * cam.view);
		vec3 eyes = column(invMV, 3).xyz;
		shader("raytrace")([&](Shader& s) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, tri_ssbo);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeOf(triangle_ssbo), &triangle_ssbo.triangles[0]);
			
			glActiveTexture(GL_TEXTURE0);
			glBindImageTexture(0, scene_img, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			s.sendUniform1ui("scene_img", scene_img);
			s.sendUniform3fv("lightPos", 1, &lightPos[0]);
			s.sendUniform3fv("eyes", 1, &eyes[0]);
			s.sendUniformMatrix4fv("invMVP", 1, GL_FALSE, value_ptr(invMVP));
			
			glDispatchCompute(_width / 32, _height / 32, 1);
		});

		shader("raytrace_render")([&](Shader& s) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, scene_img);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			s.sendUniform1ui("scene_img", scene_img);
			quad->draw(s);
		});
	}

	void pathTrace() {
		vec3 lightPos = vec3(light[0].position);
		mat4 invMV = inverse(cam.view);
		mat4 invMVP = inverse(cam.projection * cam.view);
		vec3 eyes = column(invMV, 3).xyz;
		float currentTime = Timer::get().now();
		logger.info("currentTime: " + to_string(currentTime));
		shader("pathtrace")([&](Shader& s) {
			glActiveTexture(GL_TEXTURE0);
			glBindImageTexture(0, scene_img, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			s.sendUniform1ui("scene_img", scene_img);
			s.sendUniform3fv("lightPos", 1, &lightPos[0]);
			s.sendUniform3fv("eyes", 1, &eyes[0]);
			s.sendUniform1i("SAMPLES", samples);
			s.sendUniform1f("seed", float(currentTime));
			s.sendUniformMatrix4fv("invMVP", 1, GL_FALSE, value_ptr(invMVP));

			glDispatchCompute(_width / 32, _height / 32, 1);
		});

		shader("raytrace_render")([&](Shader& s) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, scene_img);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			s.sendUniform1ui("scene_img", scene_img);
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
	ProvidedMesh* model2;
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
	GLuint tri_ssbo;
	ray_tracing::SSBOTriangleData triangle_ssbo;
	vec4 bg = vec4(0.5, 0.5, 1, 1);
	float theta = 0.66f;
	float phi = -1.0f;
	float radius = 70;
	float samples = 3;
};