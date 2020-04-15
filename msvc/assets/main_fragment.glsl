#version 320 es

precision highp float;
uniform ivec4 color;
out highp vec4 fc; // "fragment" (== "pixel") color

void main() {
    const float scale = 1.0 / 255.0;
    float r = float(color[0] + 1) * scale;
    float g = float(color[1] + 1) * scale;
    float b = float(color[2] + 1) * scale;
    float a = float(color[3] + 1) * scale;
    fc = vec4(r, g, b, a);
}
