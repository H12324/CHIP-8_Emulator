#version 330 core
out vec4 FragColor;
//in vec3 ourColor;
in vec2 TexCoord;
uniform sampler2D ourTexture;

vec4 num2RGB(float R, float G, float B) { // Learning Shaders :)
	return vec4(float(R/ 255.0f) , float(G/ 255.0f) ,float(B/ 255.0f), 1.0);
}

void main()
{


//FragColor = texture(ourTexture, TexCoord).x *  vec4(1.0, 1.0, 1.0, 0)  ; // White
  //FragColor = texture(ourTexture, TexCoord).xxxx; // Also white
  //FragColor = texture(ourTexture, TexCoord).yxzw * 0.6; // Nice Matrix Green
	FragColor = texture(ourTexture, TexCoord).x == 1 ? num2RGB(0xFF, 0xCC, 0x00) : num2RGB(0x99, 0x66, 0x0); //  More Fancy way to pick colour with hex codes
};