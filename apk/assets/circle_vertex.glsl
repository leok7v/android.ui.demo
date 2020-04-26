#version 320 es

in vec4 vertex;
in vec2 value;
uniform mat4 view;
uniform mat4 projection;
out vec2 val;

void main() {
    val = value;
    gl_Position = projection * view * vertex;
}

