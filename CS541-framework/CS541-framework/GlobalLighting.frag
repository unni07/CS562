#version 330 core

uniform	float PI = 3.1415926;

in vec2 texCoord;
out vec4 color;
 

uniform sampler2D GpositionMap;
uniform sampler2D GnormalMap;
uniform sampler2D groundColor;
uniform sampler2D GuvMap;
uniform sampler2D EyeVecMap;
uniform sampler2D LightVecMap;
uniform sampler2D depthMap;
uniform sampler2D ShadowCordMap;
uniform int RenderMode = 1;
uniform float Time;

 vec3 normalVec, lightVec, eyeVec;
 vec4 shadowCoord;
 vec3 N;
 vec3 E;
 vec3 L;
 vec3 H;
 vec3 T;
 float I;


 vec3 Kd = vec3(0);
vec3 Ks = vec3(0);

vec3 FresnelValue()
{
	float value = 0.05f;
	Ks = vec3(value,value,value);
	vec3 f = Ks + (1-Ks) * pow((1-max(dot(L,H),1.0f)),5.0);
	Ks = f;
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

vec3 RenderPonintLights(vec3 worldPosition)
{
	vec3 pointLights = vec3(0);
	
	float Yposition = -2.0;
	float constant = 0.5;
	float linear = 0.1;
	float exponential = 5.0f;
	float dts = clamp(sin(Time/10),0.0,1);
	float dtc = clamp(cos(Time/10),0.0,1);
	for(int j=0; j< 20; j++) {
	for(int i=0; i< 20; i++)
	{
		vec3 pointLightPos  = vec3(45 - i*5.0, 45 - j*5.0, Yposition);
		vec3 pointLightDir = worldPosition - pointLightPos;
		float distance = length(pointLightDir);
		pointLightDir = normalize(pointLightDir);
		//vec3 pointColor = vec3(1.0f,1.0f,1.0f);
		vec3 pointColor = vec3(dts,dtc,dtc);
		float attinuation = constant + linear * distance + exponential * distance * distance;
		pointLights += pointColor / attinuation;//  * max(dot(N,pointLightPos),0.0);
	}
	}
	return pointLights;
}


void main() {
   
    normalVec = texture(GnormalMap, texCoord.st).rgb;
	vec3 worldPosition = texture(GpositionMap, texCoord.st).rgb;
    eyeVec = texture(EyeVecMap, texCoord.st).rgb;
	lightVec = texture(LightVecMap, texCoord.st).rgb;
	 Kd = texture(groundColor, texCoord.st ).rgb;
	shadowCoord = texture(ShadowCordMap, texCoord.st );

	N = normalVec;
    E = eyeVec;
    L = lightVec;
	H = normalize(E+L);
	I = 2.5;

	float diffuse = max(dot(N,L),0.0f);
	float otherPart = diffuse * I;
	
	vec3 finalPointLight = vec3(0);
	finalPointLight = RenderPonintLights(worldPosition) * 5.7;


	vec3 ambient = diffuse * Kd + finalPointLight;
	vec3 finalLight = BRDF(Kd + finalPointLight) * otherPart;


	float bias = 0.005 * tan(acos(diffuse));
	bias = clamp(bias, 0,0.01);

	float lightDepth = 0.0;
	float pixelDepth = 0.0;

	if(RenderMode == 1) {
		color = vec4(finalLight,1);
	}
	else if(RenderMode == 2) {
			color = vec4(worldPosition,1);
	}
	else if(RenderMode == 3) {
			color = vec4(normalVec,1);
	}
	else if(RenderMode == 4) {
			color = vec4(Kd,1);
	}
	else if(RenderMode == 5) {
			color = vec4(lightVec,1);
	}
	else if(RenderMode == 6) {
			color =shadowCoord;
	}
	
}