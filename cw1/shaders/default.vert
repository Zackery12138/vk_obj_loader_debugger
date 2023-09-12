#version 450
layout( location = 0 ) in vec3 iPosition;
layout( location = 1 ) in vec3 iColor;
layout( location = 2 ) in vec2 iTexCoord;

layout(set = 0, binding = 0) uniform UScene
{
	mat4 camera;
	mat4 projection;
	mat4 projCam;
}uScene;

layout( location = 0 ) out vec2 v2gTexCoords;
layout( location = 1 ) out vec3 v2gColor;
layout( location = 2 ) out vec3 v2gPos;

void main()
{
	v2gPos = iPosition;
	v2gTexCoords = iTexCoord;
	v2gColor = iColor;
	gl_Position = uScene.projCam * vec4(iPosition, 1.0f);
}