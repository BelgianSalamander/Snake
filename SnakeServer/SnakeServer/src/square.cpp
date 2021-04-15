#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <ft2build.h>
#include <queue>;
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

FT_Library library;
FT_Face face;
FT_GlyphSlot g;

GLuint tex;
GLuint solid;

unsigned int squareBuffer;
unsigned char data[4][4] = {
	{255, 255, 255, 255},
	{255, 255, 255, 255},
	{255, 255, 255, 255},
	{255, 255, 255, 255}
};

int width, height;
float centerX, centerY, invCenterX, invCenterY;
int colourUniformLocation;
int textureUniformLocation;
int textureColourUniformLocation;

unsigned int textureShader;

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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const void*)(sizeof(float) * 2));

	textureShader = createShader(loadFile("res/shaders/texture_vertex.shader"), loadFile("res/shaders/texture_frag.shader"));
	textureUniformLocation = glGetUniformLocation(textureShader, "u_Texture");
	textureColourUniformLocation = glGetUniformLocation(textureShader, "u_Color");
	glUseProgram(textureShader);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(textureUniformLocation, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &solid);
	glBindTexture(GL_TEXTURE_2D, solid);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RED,
		4,
		4,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		data
	);

	width = m_viewport[2];
	height = m_viewport[3];

	centerX = width / 2;
	centerY = height / 2;

	invCenterX = 1 / centerX;
	invCenterY = 1 / centerY;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Load Freetype
	FT_Error error;
	
	
	if (error = FT_Init_FreeType(&library)) {
		std::cout << "Error while initialising freetype! (" << error << ")" << std::endl;
	}

	error = FT_New_Face(library, "C:/Windows/Fonts/arial.ttf", 0, &face);
	if (error == FT_Err_Unknown_File_Format) {
		std::cout << "Font format is unsupported" << std::endl;
	}
	else if (error) {
		std::cout << "Error while loading font! (" << error << ")" << std::endl;
	}

	g = face->glyph;

	FT_Set_Pixel_Sizes(face, 0, 48);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void drawRectangle(float startX, float startY, float width, float height, float* color) {
	glBindTexture(GL_TEXTURE_2D, solid);
	startX = (startX - centerX) * invCenterX;
	startY = (startY - centerY) * invCenterY;

	width *= invCenterX;
	height *= invCenterY;

	float positions[16] = {
		startX        , startY, 0.0, 0.0,
		startX + width, startY, 1.0, 0.0,
		startX + width, startY + height, 1.0, 1.0,
		startX        , startY + height, 0.0, 1.0
	};

	//glBindBuffer(GL_ARRAY_BUFFER, squareBuffer)
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), positions, GL_DYNAMIC_DRAW);
	glUniform4f(textureColourUniformLocation, color[0], color[1], color[2], color[3]);

	glDrawArrays(GL_POLYGON, 0, 4);
}
void render_text(const char* text, float startX, float startY, float* color, float scale = 1.0) {
	glBindTexture(GL_TEXTURE_2D, tex);
	startX = (startX - centerX) * invCenterX;
	startY = (startY - centerY) * invCenterY;
	const char* p;
	glUniform4f(textureColourUniformLocation, color[0], color[1], color[2], color[3]);

	for (p = text; *p; p++) {
		if (FT_Load_Char(face, *p, FT_LOAD_RENDER))
			continue;

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			g->bitmap.width,
			g->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			g->bitmap.buffer
		);

		float y2 = startY - (g->bitmap.rows - g->bitmap_top) * invCenterY * scale;
		float w = g->bitmap.width * invCenterX * scale;
		float h = g->bitmap.rows * invCenterY * scale;

		GLfloat box[16] = {
			startX, y2 + h   , 0, 0,
			startX + w, y2 + h    , 1, 0,
			startX,     y2, 0, 1,
			startX + w, y2, 1, 1
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		startX += (g->advance.x / 64) * invCenterX * scale;
		startY += (g->advance.y / 64) * invCenterY * scale;
	}
}
