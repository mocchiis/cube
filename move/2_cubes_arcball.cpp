#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ディレクトリの設定ファイル
#include "common.h"

static int WIN_WIDTH = 500;                       // ウィンドウの幅
static int WIN_HEIGHT = 500;                       // ウィンドウの高さ
static const char *WIN_TITLE = "moving_cubes";     // ウィンドウのタイトル

// シェーダファイル
static std::string VERT_SHADER_FILE = std::string(SHADER_DIRECTORY) + "render.vert";
static std::string FRAG_SHADER_FILE = std::string(SHADER_DIRECTORY) + "render.frag";

// 頂点オブジェクト
struct Vertex {
	Vertex(const glm::vec3 &position_, const glm::vec3 &color_)
		: position(position_)
		, color(color_) {
	}

	glm::vec3 position;
	glm::vec3 color;
};

static const glm::vec3 positions[8] = {
	glm::vec3(-1.0f, -1.0f, -1.0f),
	glm::vec3(1.0f, -1.0f, -1.0f),
	glm::vec3(-1.0f,  1.0f, -1.0f),
	glm::vec3(-1.0f, -1.0f,  1.0f),
	glm::vec3(1.0f,  1.0f, -1.0f),
	glm::vec3(-1.0f,  1.0f,  1.0f),
	glm::vec3(1.0f, -1.0f,  1.0f),
	glm::vec3(1.0f,  1.0f,  1.0f)
};

static const glm::vec3 colors[6] = {
	glm::vec3(1.0f, 1.0f, 0.0f),  // イエロー
	glm::vec3(0.0f, 1.0f, 1.0f),  // シアン
	glm::vec3(1.0f, 0.0f, 1.0f),  // マゼンタ
	glm::vec3(1.0f, 0.0f, 0.0f),  // 赤
	glm::vec3(0.0f, 1.0f, 0.0f),  // 緑
	glm::vec3(0.0f, 0.0f, 1.0f),  // 青
	
};

static const unsigned int faces[12][3] = {
	{ 1, 6, 7 },{ 1, 7, 4 },
	{ 2, 5, 7 },{ 2, 7, 4 },
	{ 3, 5, 7 },{ 3, 7, 6 },
	{ 0, 1, 4 },{ 0, 4, 2 },
	{ 0, 1, 6 },{ 0, 6, 3 },
	{ 0, 2, 5 },{ 0, 5, 3 }
};

// バッファを参照する番号
GLuint vaoId;
GLuint vertexBufferId;
GLuint indexBufferId;

// シェーダを参照する番号
GLuint programId;

// 立方体の回転角度
static float theta = 0.0f;

// オブジェクトを選択するためのID
bool selectMode = false;
int selectobject = 0;

glm::mat4 viewMat, projMat;

//構造体の宣言
//立方体
struct drawCube {
	float scale = 1.0f;

	glm::mat4 modelMat;
	glm::mat4 acRotMat, acTransMat, acScaleMat;

	//初期化
	void initialize() {
		modelMat = glm::mat4(1.0);
		acRotMat = glm::mat4(1.0);
		acTransMat = glm::mat4(1.0);
		acScaleMat = glm::mat4(1.0);
	}

	//描画
	void draw() {

		// 座標の変換
		projMat = glm::perspective(45.0f,
			(float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 1000.0f);

		viewMat = glm::lookAt(glm::vec3(3.0f, 4.0f, 5.0f),   // 視点の位置
			glm::vec3(0.0f, 0.0f, 0.0f),   // 見ている先
			glm::vec3(0.0f, 1.0f, 0.0f));  // 視界の上方向

		glm::mat4 mvpMat = projMat * viewMat * modelMat * acTransMat * acRotMat * acScaleMat;

		// Uniform変数の転送
		GLuint uid;
		uid = glGetUniformLocation(programId, "u_mvpMat");
		glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(mvpMat));
		uid = glGetUniformLocation(programId, "u_selectID");
		glUniform1i(uid, selectMode ? cubeID : -1);//

		glDrawElements(GL_TRIANGLES, 48, GL_UNSIGNED_INT, 0);

	}

	static int n;
	int cubeID;

	drawCube() {
		n++;
		cubeID = n;
	}
};

int drawCube::n = 0;
drawCube cb[2];

//アークボールモード
//enum(列挙型): 複数の変数に一連の整数値を付けると便利な場合に使用
enum ArcballMode {
	ARCBALL_MODE_NONE = 0x00,
	ARCBALL_MODE_TRANSLATE = 0x01,
	ARCBALL_MODE_ROTATE = 0x02,
	ARCBALL_MODE_SCALE = 0x04
};

int arcballMode = ARCBALL_MODE_NONE;

glm::vec3 gravity;
glm::ivec2 oldPos;
glm::ivec2 newPos;

// VAOの初期化
void initVAO() {
	// Vertex配列の作成update
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	int idx = 0;
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 3; j++) {
			Vertex v(positions[faces[i * 2 + 0][j]], colors[i]);
			vertices.push_back(v);
			indices.push_back(idx++);
			gravity += v.position;
		}

		for (int j = 0; j < 3; j++) {
			Vertex v(positions[faces[i * 2 + 1][j]], colors[i]);
			vertices.push_back(v);
			indices.push_back(idx++);
			gravity += v.position;
		}
	}
	
	gravity /= indices.size();//gravityはindicesの要素数で割ったもの//平行移動

	// VAOの作成
	glGenVertexArrays(1, &vaoId);
	glBindVertexArray(vaoId);

	// 頂点バッファの作成
	glGenBuffers(1, &vertexBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

	// 頂点バッファの有効化
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

	// 頂点番号バッファの作成
	glGenBuffers(1, &indexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(),
		indices.data(), GL_STATIC_DRAW);

	// VAOをOFFにしておく
	glBindVertexArray(0);
}

GLuint compileShader(const std::string &filename, GLuint type) {
	// シェーダの作成
	GLuint shaderId = glCreateShader(type);

	// ファイルの読み込み
	std::ifstream reader;
	size_t codeSize;
	std::string code;

	// ファイルを開く
	reader.open(filename.c_str(), std::ios::in);
	if (!reader.is_open()) {
		// ファイルを開けなかったらエラーを出して終了
		fprintf(stderr, "Failed to load a shader: %s\n", VERT_SHADER_FILE.c_str());
		exit(1);
	}

	// ファイルをすべて読んで変数に格納 (やや難)
	reader.seekg(0, std::ios::end);             // ファイル読み取り位置を終端に移動 
	codeSize = reader.tellg();                  // 現在の箇所(=終端)の位置がファイルサイズ
	code.resize(codeSize);                      // コードを格納する変数の大きさを設定
	reader.seekg(0);                            // ファイルの読み取り位置を先頭に移動
	reader.read(&code[0], codeSize);            // 先頭からファイルサイズ分を読んでコードの変数に格納

												// ファイルを閉じる
	reader.close();

	// コードのコンパイル
	const char *codeChars = code.c_str();
	glShaderSource(shaderId, 1, &codeChars, NULL);
	glCompileShader(shaderId);

	// コンパイルの成否を判定する
	GLint compileStatus;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		// コンパイルが失敗したらエラーメッセージとソースコードを表示して終了
		fprintf(stderr, "Failed to compile a shader!\n");

		// エラーメッセージの長さを取得する
		GLint logLength;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			// エラーメッセージを取得する
			GLsizei length;
			std::string errMsg;
			errMsg.resize(logLength);
			glGetShaderInfoLog(shaderId, logLength, &length, &errMsg[0]);

			// エラーメッセージとソースコードの出力
			fprintf(stderr, "[ ERROR ] %s\n", errMsg.c_str());
			fprintf(stderr, "%s\n", code.c_str());
		}
		exit(1);
	}

	return shaderId;
}

GLuint buildShaderProgram(const std::string &vShaderFile, const std::string &fShaderFile) {
	// シェーダの作成
	GLuint vertShaderId = compileShader(vShaderFile, GL_VERTEX_SHADER);
	GLuint fragShaderId = compileShader(fShaderFile, GL_FRAGMENT_SHADER);

	// シェーダプログラムのリンク
	GLuint programId = glCreateProgram();
	glAttachShader(programId, vertShaderId);
	glAttachShader(programId, fragShaderId);
	glLinkProgram(programId);

	// リンクの成否を判定する
	GLint linkState;
	glGetProgramiv(programId, GL_LINK_STATUS, &linkState);
	if (linkState == GL_FALSE) {
		// リンクに失敗したらエラーメッセージを表示して終了
		fprintf(stderr, "Failed to link shaders!\n");

		// エラーメッセージの長さを取得する
		GLint logLength;
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0) {
			// エラーメッセージを取得する
			GLsizei length;
			std::string errMsg;
			errMsg.resize(logLength);
			glGetProgramInfoLog(programId, logLength, &length, &errMsg[0]);

			// エラーメッセージを出力する
			fprintf(stderr, "[ ERROR ] %s\n", errMsg.c_str());
		}
		exit(1);
	}

	// シェーダを無効化した後にIDを返す
	glUseProgram(0);
	return programId;
}

// シェーダの初期化
void initShaders() {
	programId = buildShaderProgram(VERT_SHADER_FILE, FRAG_SHADER_FILE);
}

// OpenGLの初期化関数
void initializeGL() {
	// 深度テストの有効化
	glEnable(GL_DEPTH_TEST);

	// 背景色の設定 (黒)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// VAOの初期化
	initVAO();

	// シェーダの用意
	initShaders();
	
	//1つ目の立方体の場所
	cb[0].initialize();
	cb[0].modelMat = glm::translate(cb[0].modelMat, glm::vec3(-2.0f, 0.0f, 0.0f));

	//2つ目の立方体の場所
	cb[1].initialize();
	cb[1].modelMat = glm::translate(cb[1].modelMat, glm::vec3(1.0f, 0.0f, 0.0f));
}

// OpenGLの描画関数
void paintGL() {
	// 背景色の描画
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	 // シェーダの有効化
	glUseProgram(programId);

	// VAOの有効化
	glBindVertexArray(vaoId);

	//1つ目の立方体描画	
	cb[0].draw();

	//2つ目の立方体描画
	cb[1].draw();

	// VAOの無効化
	glBindVertexArray(0);

	// シェーダの無効化
	glUseProgram(0);
}

void resizeGL(GLFWwindow *window, int width, int height) {
	// ユーザ管理のウィンドウサイズを変更
	WIN_WIDTH = width;
	WIN_HEIGHT = height;

	// GLFW管理のウィンドウサイズを変更
	glfwSetWindowSize(window, WIN_WIDTH, WIN_HEIGHT);

	// 実際のウィンドウサイズ (ピクセル数) を取得
	int renderBufferWidth, renderBufferHeight;
	glfwGetFramebufferSize(window, &renderBufferWidth, &renderBufferHeight);

	// ビューポート変換の更新
	glViewport(0, 0, renderBufferWidth, renderBufferHeight);
}


// Arcballコントロールのための変数
bool isDragging = false;

void mouseEvent(GLFWwindow *window, int button, int action, int mods) {
	// クリックしたボタンで処理を切り替える
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		arcballMode = ARCBALL_MODE_ROTATE;
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
		arcballMode = ARCBALL_MODE_SCALE;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		arcballMode = ARCBALL_MODE_TRANSLATE;
	}

	// クリックされた位置を取得
	double px, py;
	glfwGetCursorPos(window, &px, &py);

	if (action == GLFW_PRESS) {
		const int cx = (int)px;
		const int cy = (int)py;

		// 選択モードでの描画
		selectMode = true;
		paintGL();
		selectMode = false;

		// より適切なやり方
		unsigned char byte[4];
		glReadPixels(cx, WIN_HEIGHT - cy - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &byte);
		printf("Mouse position: %d %d\n", cx, cy);
		printf("Select object %d\n", (int)byte[0]);
		selectobject = (int)byte[0] - 1;

		if (!isDragging) {
			isDragging = true;
			oldPos = glm::ivec2(px, py);
			newPos = glm::ivec2(px, py);
		}
	}
	else {
		isDragging = false;
		oldPos = glm::ivec2(0, 0);
		newPos = glm::ivec2(0, 0);
		arcballMode = ARCBALL_MODE_NONE;
	}
}

// スクリーン上の位置をアークボール球上の位置に変換する関数
glm::vec3 getVector(double x, double y) {
	glm::vec3 pt(2.0 * x / WIN_WIDTH - 1.0, -2.0 * y / WIN_HEIGHT + 1.0, 0.0);

	//単位円の半径
	const double xySquared = pt.x * pt.x + pt.y * pt.y;

	if (xySquared <= 1.0) {
		// 単位円の内側ならz座標を計算
		pt.z = std::sqrt(1.0 - xySquared);
	}
	else {
		// 外側なら球の外枠上にあると考える
		pt = glm::normalize(pt);
	}

	return pt;
}

//回転角と回転軸の計算
void updateRotate() {
	static const double Pi = 4.0 * std::atan(1.0);

	// getVectorでスクリーン座標をアークボール球上の座標に変換
	const glm::vec3 u = glm::normalize(getVector(newPos.x, newPos.y));
	const glm::vec3 v = glm::normalize(getVector(oldPos.x, oldPos.y));

	// カメラ座標における回転量 (=オブジェクト座標における回転量)
	const double angle = std::acos(std::max(-1.0f, std::min(glm::dot(u, v), 1.0f)));

	// カメラ空間における回転軸
	const glm::vec3 rotAxis = glm::cross(v, u);

	// カメラ座標の情報をワールド座標に変換する行列
	const glm::mat4 c2oMat = glm::inverse(viewMat * cb[selectobject].modelMat);

	// オブジェクト座標における回転軸
	const glm::vec3 rotAxisObjSpace = glm::vec3(c2oMat * glm::vec4(rotAxis, 0.0f));

	// 回転行列の更新
	cb[selectobject].acRotMat = glm::rotate((float)(4.0 * angle), rotAxisObjSpace) * cb[selectobject].acRotMat;
}

//平行移動
void updateTranslate() {
	if (selectobject >= 0) {

		// オブジェクト重心のスクリーン座標を求める//クリック位置での深度値を求める 
		glm::vec4 gravityScreenSpace = (projMat * viewMat * cb[selectobject].modelMat) * glm::vec4(gravity.x, gravity.y, gravity.z, 1.0f);
		gravityScreenSpace /= gravityScreenSpace.w;

		// スクリーン座標系における移動量
		glm::vec4 newPosScreenSpace(2.0 * newPos.x / WIN_WIDTH, -2.0 * newPos.y / WIN_HEIGHT, gravityScreenSpace.z, 1.0f);
		glm::vec4 oldPosScreenSpace(2.0 * oldPos.x / WIN_WIDTH, -2.0 * oldPos.y / WIN_HEIGHT, gravityScreenSpace.z, 1.0f);

		// スクリーン座標の情報をオブジェクト座標に変換する行列
		const glm::mat4 s2oMat = glm::inverse(projMat * viewMat * cb[selectobject].modelMat);

		// スクリーン空間の座標をオブジェクト空間に変換
		glm::vec4 newPosObjSpace = s2oMat * newPosScreenSpace;
		glm::vec4 oldPosObjSpace = s2oMat * oldPosScreenSpace;
		newPosObjSpace /= newPosObjSpace.w;
		oldPosObjSpace /= oldPosObjSpace.w;

		// オブジェクト座標系での移動量
		const glm::vec3 transObjSpace = glm::vec3(newPosObjSpace - oldPosObjSpace);

		// オブジェクト空間での平行移動
		cb[selectobject].acTransMat = glm::translate(cb[selectobject].acTransMat, transObjSpace);
	}
	
}

//拡大・縮小の関数
void updateScale() {
	cb[selectobject].acScaleMat = glm::scale(glm::vec3(cb[selectobject].scale,
		cb[selectobject].scale, cb[selectobject].scale));
}

//ホイールイベント(拡大・縮小)
void wheelEvent(GLFWwindow *window, double xpos, double ypos) {
	if (selectobject >= 0) {
		cb[selectobject].scale += ypos / 10.0; //y座標の変化がホイール移動量になる

		updateScale();
	}	
}

void updateMouse() {
	switch (arcballMode) {
	case ARCBALL_MODE_ROTATE:
		updateRotate();
		break;

	case ARCBALL_MODE_TRANSLATE:
		updateTranslate();
		break;

	case ARCBALL_MODE_SCALE:
		cb[selectobject].scale += (float)(oldPos.y - newPos.y) / WIN_HEIGHT;
		updateScale();
		break;
	}
}

void mouseMoveEvent(GLFWwindow *window, double xpos, double ypos) {
	//押されている途中でかつどちらかの立方体が選択されている
	if (isDragging && selectobject >= 0) {
		// マウスの現在位置を更新
		newPos = glm::ivec2(xpos, ypos);

		// マウスがあまり動いていない時は処理をしない
		const double dx = newPos.x - oldPos.x;
		const double dy = newPos.y - oldPos.y;
		const double length = dx * dx + dy * dy;
		if (length < 2.0f * 2.0f) {
			return;
		}
		else {
			if (selectobject >= 0) {
				updateMouse();
				oldPos = glm::ivec2(xpos, ypos);

			}

		}
	}
}

int main(int argc, char **argv) {
	// OpenGLを初期化する
	if (glfwInit() == GL_FALSE) {
		fprintf(stderr, "Initialization failed!\n");
		return 1;
	}

	// OpenGLのバージョン設定 (Macの場合には必ず必要)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Windowの作成
	GLFWwindow *window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE,
		NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Window creation failed!");
		glfwTerminate();
		return 1;
	}

	// OpenGLの描画対象にWindowを追加
	glfwMakeContextCurrent(window);

	// マウスのイベントを処理する関数を登録
	glfwSetMouseButtonCallback(window, mouseEvent);
	glfwSetCursorPosCallback(window, mouseMoveEvent);
	glfwSetScrollCallback(window, wheelEvent);

	// OpenGL 3.x/4.xの関数をロードする (glfwMakeContextCurrentの後でないといけない)
	const int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0) {
		fprintf(stderr, "Failed to load OpenGL 3.x/4.x libraries!\n");
		return 1;
	}

	// ウィンドウのリサイズを扱う関数の登録
	glfwSetWindowSizeCallback(window, resizeGL);

	// OpenGLを初期化
	initializeGL();

	// メインループ
	while (glfwWindowShouldClose(window) == GL_FALSE) {
		// 描画
		paintGL();

		// 描画用バッファの切り替え
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
