#version 330 core

in vec2 texCoord;
out vec4 color;
 
uniform sampler2D groundColor;
uniform sampler2D depthMap;
void main(){
   
    vec4 color1 = vec4(texture2D( depthMap, texCoord ).rgb,1.0);
	color = vec4(texture2D( groundColor, texCoord ).rgb,1.0);
	//color = vec4(1,0,0,1);
}