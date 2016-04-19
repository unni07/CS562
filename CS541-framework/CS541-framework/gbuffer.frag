#version 330

in vec2 texCoord;
in vec3 normalVec, lightVec, eyeVec; 
in vec4 position;
in vec4 shadowCoord; 

layout (location = 0) out vec3 WorldPosOut; 
layout (location = 1) out vec3 DiffuseOut; 
layout (location = 2) out vec3 NormalOut; 
layout (location = 3) out vec3 TexCoordOut;
layout (location = 4) out vec3 EyeVecOut;
layout (location = 5) out vec3 LightVecOut;
layout (location = 6) out vec4 ShadowCordOut; 

uniform bool useTexture;
uniform sampler2D groundColor; 
uniform vec3 phongDiffuse;
uniform bool useSkyDome = false;
void main()
{
	WorldPosOut = position.xyz;

	vec3 Kd = phongDiffuse;
	if (useTexture) {
	    Kd = texture(groundColor,texCoord.st).xyz * phongDiffuse;
	}

	
    NormalOut = normalize(normalVec); 
    TexCoordOut = vec3(texCoord, 0.0); 
	DiffuseOut = Kd;
    EyeVecOut = normalize(eyeVec);
    LightVecOut = normalize(lightVec);
	ShadowCordOut = shadowCoord;
}