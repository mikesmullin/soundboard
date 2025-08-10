#ifndef SHADERS_H
#define SHADERS_H

// Text rendering vertex shader
#define TEXT_VERTEX_SHADER \
"#version 330 core\n" \
"layout(location=0) in vec2 aPos;\n" \
"layout(location=1) in vec2 aUV;\n" \
"uniform mat4 uProj;\n" \
"out vec2 vUV;\n" \
"void main(){\n" \
"  gl_Position = uProj * vec4(aPos, 0.0, 1.0);\n" \
"  vUV = aUV;\n" \
"}\n"

// Text rendering fragment shader
#define TEXT_FRAGMENT_SHADER \
"#version 330 core\n" \
"in vec2 vUV;\n" \
"uniform sampler2D uTex;\n" \
"uniform vec3 uColor;\n" \
"out vec4 FragColor;\n" \
"void main(){\n" \
"  float coverage = texture(uTex, vUV).r;\n" \
"  FragColor = vec4(uColor, coverage);\n" \
"}\n"

// Rectangle rendering vertex shader
#define RECT_VERTEX_SHADER \
"#version 330 core\n" \
"layout(location=0) in vec2 aPos;\n" \
"uniform mat4 uProj;\n" \
"void main(){\n" \
"  gl_Position = uProj * vec4(aPos, 0.0, 1.0);\n" \
"}\n"

// Rectangle rendering fragment shader
#define RECT_FRAGMENT_SHADER \
"#version 330 core\n" \
"uniform vec3 uColor;\n" \
"out vec4 FragColor;\n" \
"void main(){\n" \
"  FragColor = vec4(uColor, 1.0);\n" \
"}\n"

#endif // SHADERS_H
