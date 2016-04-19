/////////////////////////////////////////////////////////////////////////
// Vertex shader for the final pass
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix, ViewInverse;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform mat4 ShadowMatrix;

out vec2 texCoord;

void main()
{

	texCoord = (vertex.xy+vec2(1,1))/2.0;
	gl_Position = vertex;

	//texCoord = vertexTexture;

    //gl_Position = ViewMatrix * ModelMatrix * vertex;
	//gl_Position = ProjectionMatrix * ViewMatrix * ModelMatrix * vertex;
}
