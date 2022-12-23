#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

// The OPENGL portion of the emulator

// How modular should I make this?
// Just what I need or something to extend to the NES emulator

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec2 aTex;\n"
"out vec2 TexCoord;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos, 1.0);\n"
"	TexCoord = aTex;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"//in vec3 ourColor;\n"
"in vec2 TexCoord;\n"
"uniform sampler2D ourTexture;\n"
"void main()\n"
"{\n"
"FragColor = texture(ourTexture, TexCoord);\n"
"}\0";

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}


GLFWwindow* createWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "CHIP-8 Interpreter", NULL, NULL);
	if (window == NULL)
	{
		printf("Failed to create GLFW window\n");;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Init GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize GLAD\n");
		return NULL;
	}

	return window;
}

char* loadShader(char* shaderSource, char buf[4096]) {
	// For now just assuming file is gonna be less than 4KB
	FILE* fp;

	if (fopen_s(&fp, shaderSource, "rb")) {
		fprintf(stderr, "Failed to load shader: %s\n", shaderSource);
		return NULL;
	}

	size_t len = fread(buf, 1, 4096, fp); // Won't work if > 4KB shader
	buf[len++] = '\0';

 	printf("SHADER LOADED: %s\n", shaderSource);
	fclose(fp);
	return buf;

}

unsigned int shaderID;
void setupShaders() {
	//  Compile Shaders
	unsigned int vertex, fragment, ID;
	int success;
	char infoLog[512];
	char* vertSource = (char*)malloc(sizeof(char) * 4096);
	char* fragSource = (char*)malloc(sizeof(char) * 4096);
	
	// vertex Shader 
	vertex = glCreateShader(GL_VERTEX_SHADER);
	loadShader("shader.vert", vertSource);
	//printf("%s\n", vertSource);
	glShaderSource(vertex, 1, &vertSource, NULL);
	glCompileShader(vertex);
	// print compile errors if any
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
	};
	
	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	loadShader("shader.frag", fragSource);
	//printf("%s\n", fragSource);
	glShaderSource(fragment, 1, &fragSource, NULL);
	glCompileShader(fragment);
	// print compile errors if any
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
	};
	
	// shader Program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	// print linking errors if any
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n" , infoLog);
	}

	// delete the shaders as they're linked into our program now and no longer necessary
	free(vertSource);
	free(fragSource);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	// Maybe should return error if it fails
	shaderID = ID;
}


// Vars here?
float vertices[] = {
	// positions          // texture coords
	 1.0f,  1.0f, 0.0f,   1.0f, 0.0f,   // top right // Note make these positions the corners of the screen
	 1.0f, -1.0f, 0.0f,   1.0f, 1.0f,   // bottom right
	-1.0f, -1.0f, 0.0f,   0.0f, 1.0f,   // bottom left
	-1.0f,  1.0f, 0.0f,   0.0f, 0.0f    // top left 
};

unsigned int indices[] = {
0, 1, 3, // First Triangle
1, 2, 3  // Second Triangle
};

unsigned int VAO, VBO, EBO, tex;

void setupObjects(const unsigned char* gfx) { // All the setup for VBO's and stuff
	uint8_t test[64 * 32];
	for (int i = 0; i < 64 * 32; i++) {
		test[i] = gfx[i] * 0xFF;
	}
	// Note may want to incorporate framebuffers after learning more about them
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // remove?
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // remove? 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Default so idk if its needed
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, 0, GL_RED, GL_UNSIGNED_BYTE, gfx);
	
	// VBOS N Stuff (Move before texture stuff)
	//unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind the vertex array object then buffer(s)
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Vertex Attrib Things
	// pos attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// tex attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	// Use Shader Program
	glUseProgram(shaderID);

}

void drawSquare(const unsigned char* gfx) { // could probably have come up with better name...
	uint8_t test[64 * 32];
	for (int i = 0; i < 64 * 32; i++) {
			test[i] = gfx[i]*0xFF;
	}
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 32, GL_RED, GL_UNSIGNED_BYTE, test); // update the previous texture
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 32, 0, GL_RED, GL_UNSIGNED_BYTE, gfx);
	glBindTexture(GL_TEXTURE_2D, tex); //not sure if i need to rebind it
	glUseProgram(shaderID);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}