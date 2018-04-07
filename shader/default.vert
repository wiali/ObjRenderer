in vec3 attribPosition;
in vec4 attribColor;
in vec2 attribTexcoord;
in vec3 attribNormal;
in vec3 attribTangent;


uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

uniform mat4 uShadowMVP;
uniform mat4 uModel;
uniform vec3 lightPos;
uniform vec3 viewPos;

out vec4 color;
out vec2 textureCoord;
out vec3 normal;
out vec3 fragPos;
out vec4 shadowCoord;

out vec3 T;

out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;


void main(void)
{
	color = attribColor;
	textureCoord = attribTexcoord;
	normal = attribNormal;
	vec3 tangent=attribTangent;


	mat3 normalMatrix = transpose(inverse(mat3(uModel)));
	T = normalize(normalMatrix * tangent);
	vec3 N = normalize(normalMatrix * normal);
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(T, N);

	fragPos=vec3(uModel*vec4(attribPosition, 1.0));
	mat3 TBN = transpose(mat3(T, B, N)); 
	TangentLightPos = TBN * lightPos;
	TangentViewPos  = TBN * viewPos;
	TangentFragPos  = TBN * fragPos;

	shadowCoord = uShadowMVP * uModel * vec4(attribPosition, 1.0);
	gl_Position =  proj*view*model * vec4(attribPosition, 1.0);
}