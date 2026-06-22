#version 100

precision mediump float;

const int MAX_JOINTS = 50;
const int MAX_WEIGHTS = 3;

attribute vec4 aPosition;
attribute vec2 aTexCoord;
attribute vec3 aNormal;
attribute vec4 aJointIds;
attribute vec4 aJointWeights;

varying vec2 texCoord;
varying vec3 normal;

uniform mat4 jointTransforms[MAX_JOINTS];

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

void main(){

    vec4 totalLocalPos = vec4(0.0);
    vec4 totalNormal = vec4(0.0);

    for (int i = 0; i< MAX_WEIGHTS; i++)
    {
        mat4 jointTransform = jointTransforms[int(aJointIds[i])];

        vec4 posePosition = jointTransform * aPosition;
        totalLocalPos += posePosition * aJointWeights[i];

        vec4 worldNormal = jointTransform * vec4(aNormal, 0.0);
        totalNormal += worldNormal * aJointWeights[i];
    }


    gl_Position = u_projection * u_view * u_model * totalLocalPos;
    texCoord = aTexCoord;
    normal = totalNormal.xyz;
    //normal = (totalLocalPos * vec4(aNormal, 0.0)).xyz;
}