/////////////////////////////////////////////////////////////////////////
// Vertex shader for the reflection pass
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix, ViewInverse;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform mat4 ShadowMatrix;
uniform bool isTop;

uniform vec3 lightPos;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

out vec3 tangent;
out vec2 texCoord;
out vec3 normalVec, lightVec, eyeVec;
out vec4 shadowCoord; 

void main()
{      

	texCoord = vertexTexture;
	tangent = vertexTangent;
    

    normalVec = normalize(mat3(NormalMatrix)*vertexNormal);    
    
    vec3 worldVertex = vec3(ModelMatrix * vertex);
    eyeVec = (ViewInverse*vec4(0,0,0,1)).xyz - worldVertex;
    lightVec = lightPos - worldVertex;

    vec4 position =  ModelMatrix * vertex;

	vec3 R = vec3(position);
	vec3 abcd = R / length(R);
	float d;
	if(isTop) {
		d = 1 - abcd.z;
		gl_Position = vec4(abcd.x/d, abcd.y/d, -abcd.z * length(R) /100.0 - 0.9, 1.0);
	}
	else
	{
		d = 1 + abcd.z;
		gl_Position = vec4(abcd.x/d, abcd.y/d, abcd.z * length(R) /100.0 - 0.9, 1.0);
	}

	shadowCoord = ShadowMatrix * ModelMatrix * vertex;
}
