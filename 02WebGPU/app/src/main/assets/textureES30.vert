#version 300 es

precision mediump float;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 texCoord;

out vec2 v_texCoord;

uniform mat4 projection;
uniform mat4 modelView;

void main(){
    gl_Position = projection * modelView * vec4( vertexPosition, 1.0);
    v_texCoord = texCoord;
}