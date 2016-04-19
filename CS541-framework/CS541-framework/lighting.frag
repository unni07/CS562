/////////////////////////////////////////////////////////////////////////
// Pixel shader for the final pass
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform int mode;
uniform bool useTexture;

uniform vec3 phongDiffuse;
uniform vec3 phongSpecular;
uniform float phongShininess;

uniform vec3 lightValue, lightAmbient;

uniform sampler2D groundColor;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;

void main()
{
    vec3 N = normalize(normalVec);
    vec3 E = normalize(eyeVec);   
    vec3 L = normalize(lightVec);

    vec3 Kd = phongDiffuse;
    if (useTexture)
        Kd = texture(groundColor,2.0*texCoord.st).xyz;

    gl_FragColor.xyz = max(0.0, dot(L, N))*Kd;


}
