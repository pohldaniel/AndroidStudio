#version 300 es

precision mediump float;

const int MAX_JOINTS = 50;
const int MAX_WEIGHTS = 3;

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in ivec4 aJointIds;
layout(location = 4) in vec4 aJointWeights;

out vec2 v_texCoord;
out vec3 v_normal;

uniform mat4 jointTransforms[MAX_JOINTS];

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

void main(){

    vec4 totalLocalPos = vec4(0.0);
    vec4 totalNormal = vec4(0.0);

    for (int i = 0; i< MAX_WEIGHTS; i++)
    {
        mat4 jointTransform = jointTransforms[aJointIds[i]];

        vec4 posePosition = jointTransform * aPosition;
        totalLocalPos += posePosition * aJointWeights[i];

        vec4 worldNormal = jointTransform * vec4(aNormal, 0.0);
        totalNormal += worldNormal * aJointWeights[i];
    }

    gl_Position = u_projection * u_view * u_model * totalLocalPos;
    //gl_Position = u_projection * u_view * u_model * aPosition;
    v_texCoord = aTexCoord;
    v_normal = totalNormal.xyz;
    //v_normal = (totalLocalPos * vec4(aNormal, 0.0)).xyz;
}