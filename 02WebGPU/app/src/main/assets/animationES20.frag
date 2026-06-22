#version 100

precision mediump float;

uniform sampler2D texture;

varying vec2 texCoord;
varying vec3 normal;

void main(){
    gl_FragColor = texture2D(texture, texCoord);
    //gl_FragColor = vec4(normal, 1.0);
}