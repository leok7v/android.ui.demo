#version 320 es

precision highp float;

uniform  ivec2 resolution;
in highp ivec2 position;

void main() {
    // google: "diamond exit" rule to see why + 1
    float x = float(position.x * 2 + 1);
    float y = float(position.y * 2 + 1);
    float w = float(resolution.x - 1); // to ensure -1..+1
    float h = float(resolution.y - 1); // inclusive
    gl_Position = vec4(x / w - 1.0, 1.0 - y / h, 0.0, 1.0);
}
