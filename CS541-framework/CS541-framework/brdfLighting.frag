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

uniform sampler2D groundColor, groundInfo;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;

void main()
{
    vec3 N = normalize(normalVec);
    vec3 E = normalize(eyeVec);   
    vec3 L = normalize(lightVec);

	//half vector
	vec3 H = normalize(E+L);

	//original deffused light intensity
	float deffused_intensity = max(dot(L, N),0.0f);
	
    vec3 Kd = phongDiffuse;
	vec3 info = texture(groundInfo,texCoord).rgb;

    if (useTexture)
        Kd = texture(groundColor,texCoord.st).xyz * phongDiffuse;

	//original deffused light 
	vec3 deffused_final = deffused_intensity*Kd;

	//Specular
	float spec_intensity = pow(dot(N,H),phongShininess);
	vec3 spec_final = spec_intensity * phongSpecular * info.r;

	vec3 daytime = (deffused_final) + (spec_final);

	vec3 nighttime = vec3(info.g, info.g, 0.0);

	vec3 final_color = daytime;

	if(deffused_intensity < 0.11f)
	    final_color = mix(nighttime, deffused_final, (deffused_intensity + 0.1) * 5);
		


	gl_FragColor.xyz = final_color;
}

void FresnelValue(vec3 L, vec3 H)
{

}

void Distribution(vec3 H, vec3 N)
{
} 

void Shadowing(vec3 H, vec3 L)
{
}