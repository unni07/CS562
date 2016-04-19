/////////////////////////////////////////////////////////////////////////
// Pixel shader for the final pass
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 330

uniform int mode;
uniform bool useTexture;
uniform bool useNormalMap = false;
uniform bool useReflection = false;
uniform bool useSkyDome = false;
uniform	float PI = 3.1415926;

uniform vec3 phongDiffuse;
uniform vec3 phongSpecular;
uniform float phongShininess;

uniform vec3 lightValue, lightAmbient;

uniform sampler2D groundColor, shadowMap, reflection_top, reflection_bottom, normal_map;

in vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;
in vec4 shadowCoord;
in vec3 tangent;
in float zVal;
vec3 info;

 vec3 N;
 vec3 E;
 vec3 L;
 vec3 H;
 vec3 T;
 float I;

vec3 FresnelValue()
{
	float value = 0.05f;
	vec3 Ks = vec3(value,value,value);
	vec3 f = Ks + (1-Ks) * pow((1-max(dot(L,H),1.0f)),5.0);
	return f;
}

float Distribution()
{
	float alpha = pow(8192.0,0.6);
	float k = (alpha + 2.0) / (2.0*PI);
	float d = k * pow(max(dot(N,H),0.0f),alpha);
	return d;
} 

float Shadowing()
{
	float t1 = 2*max(dot(N,H),0.0f)*max(dot(N,E),0.0f)/max(dot(H,E),0.0f);
	float t2 = 2*max(dot(N,H),0.0f)*max(dot(N,L),0.0f)/max(dot(H,L),0.0f);
	float s = max(min(1, min(t1,t2)),0.0f);
	return s;
}

vec3 BRDF(vec3 Kd)
{
	vec3 c = Kd / PI;

	vec3 product = FresnelValue() * Shadowing() * Distribution();
	
	vec3 brdf = c + (product/(4.0 * max(dot(N,L),0.0f) * max(dot(E,N),0.0f)));

	return brdf;
}

void main()
{
    N = normalize(normalVec);
    E = normalize(eyeVec);
	T = normalize(tangent);
    L = normalize(lightVec);

	//half vector
	H = normalize(E+L);

	I = 3.5;

	if(useNormalMap)
	{
		info = texture(normal_map,texCoord).rgb;
		vec3 abc = info.rgb * 2 - vec3(1);
		vec3 B = cross(T, N);
		N = abc.r * T + abc.g * B + abc.b * N;
	}

	float diffuse = max(dot(N,L),0.0f);

	float otherPart = diffuse * I;

    vec3 Kd = phongDiffuse;



	if(useReflection)
	{
		vec3 R = 2.0f * max(dot(E,N), 0.0f) *  N - E;
		vec3 abc = R / length(R);
		vec2 tex;
		if( R.z < 0) {
			float d = 1 - abc.z;
			tex = vec2(abc.x/d, abc.y/d) * vec2(1/2.0,1/2.0) + vec2(1/2.0,1/2.0);
			Kd += texture(reflection_top, tex.st ).xyz * phongDiffuse;
		}
		else
		{
			float d = 1 + abc.z;
			tex = vec2(abc.x/d, abc.y/d) * vec2(1/2.0,1/2.0) + vec2(1/2.0,1/2.0);
			Kd += texture(reflection_bottom, tex.st ).xyz * phongDiffuse;
		}
		
	}

    if (useTexture) {
	    Kd = texture(groundColor,texCoord.st).xyz * phongDiffuse;
	}
	vec3 ambient = diffuse * lightAmbient * Kd;
	vec3 finalLight = BRDF(Kd) * otherPart;

	vec2 shadowIndex = (shadowCoord.xy/shadowCoord.w);

	float bias = 0.005*tan(acos(diffuse));
	bias = clamp(bias, 0,0.01);

	float lightDepth = 0.0;
	float pixelDepth = 0.0;
	if(shadowIndex.x > 0.0 && shadowIndex.x < 1.0 && shadowIndex.y > 0.0 && shadowIndex.y < 1.0 && shadowCoord.w > 0.0) {
		lightDepth =  texture(shadowMap, shadowIndex).w;
		pixelDepth = shadowCoord.w;
	}

	if(pixelDepth - bias  > lightDepth) {
		finalLight =  ambient ;
	}

	vec3 shadowPart = Kd * vec3(0.16);
	vec3 final_color = max(finalLight,shadowPart);

	gl_FragColor.rgb = final_color;

	if(useSkyDome)
	{
		Kd = texture(groundColor,-texCoord.st).xyz;
		gl_FragColor.rgb = Kd;

	} else{
		gl_FragColor.rgb = final_color;

	}
 }