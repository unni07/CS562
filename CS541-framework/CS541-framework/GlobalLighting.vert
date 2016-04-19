/////////////////////////////////////////////////////////////////////////
// Vertex shader for the final pass
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

in vec4 vertex;

out vec2 texCoord;
void main()
{
	texCoord = (vertex.xy + vec2(1,1))/2.0;
	gl_Position = vertex;
}
