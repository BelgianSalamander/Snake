#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <snake.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>

unsigned int squareBuffer;

int width, height;
float centerX, centerY, invCenterX, invCenterY;
int colourUniformLocation;

static std::string loadFile(const std::string& filename) {
	std::ifstream stream(filename);
	std::string str((std::istreambuf_iterator<char>(stream)),
		std::istreambuf_iterator<char>());
	stream.close();
	return str;
}

static unsigned int compileShader(unsigned int type, const std::string source) {
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)_malloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "shader\n";
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}

	return id;
}

static unsigned int createShader(const std::string vertexShader, const std::string fragmentShader) {

	unsigned int program = glCreateProgram();
	unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

void initAbstractions() {
	int m_viewport[4];
	glGenBuffers(1, &squareBuffer);
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	glBindBuffer(GL_ARRAY_BUFFER, squareBuffer);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

	unsigned int shader = createShader(loadFile("res/shaders/basic_vertex.shader"), loadFile("res/shaders/basic_frag.shader"));
	glUseProgram(shader);

	colourUniformLocation = glGetUniformLocation(shader, "u_Color");
	std::cout << colourUniformLocation << std::endl;
	_ASSERT(colourUniformLocation != -1);

	width = m_viewport[2];
	height = m_viewport[3];

	centerX = width / 2;
	centerY = height / 2;

	invCenterX = 1 / centerX;
	invCenterY = 1 / centerX;
}

void drawRectangle(float startX, float startY, float width, float height, float* color) {
	startX = (startX - centerX) * invCenterX;
	startY = (startY - centerY) * invCenterY;

	width *= invCenterX;
	height *= invCenterY;

	float positions[8] = {
		startX        , startY,
		startX + width, startY,
		startX + width, startY + height,
		startX        , startY + height
	};

	//glBindBuffer(GL_ARRAY_BUFFER, squareBuffer)
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), positions, GL_DYNAMIC_DRAW);
	glUniform4f(colourUniformLocation, color[0], color[1], color[2], color[3]);

	glDrawArrays(GL_POLYGON, 0, 4);
}