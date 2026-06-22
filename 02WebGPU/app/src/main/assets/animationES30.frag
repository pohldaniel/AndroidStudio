#version 300 es

precision mediump float;

uniform sampler2D u_texture;

in  vec2 v_texCoord;
in  vec3 v_normal;
out vec4 color;

void main(){
     color = texture(u_texture, v_texCoord);
    //color = vec4(normal, 1.0);
}