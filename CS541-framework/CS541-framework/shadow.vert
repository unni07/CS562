/////////////////////////////////////////////////////////////////////////
// Vertex shader for the shadow pass
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
in vec4 vertex;

out vec4 position;

void main()
{      
	
    gl_Position = ProjectionMatrix * ViewMatrix * ModelMatrix * vertex;
	position = gl_Position;
}