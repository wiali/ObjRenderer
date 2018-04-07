uniform sampler2D _MainTexture;
uniform sampler2D _ShadowMap;
uniform sampler2D _NormalTexture;
uniform sampler2D _RoughnessTexture;
uniform vec3 directionalLight;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform mat4 uModel;
uniform float shadowBias;
uniform vec4 ColorMode;
uniform float uFinalized;
uniform bool showColor;
uniform bool showPBR;
uniform bool hasNormalTexture;
uniform bool hasRoughnessTexture;


in vec4 color;
in vec2 textureCoord;
in vec3 normal;
in vec3 fragPos;
in vec4 shadowCoord;

in vec3 T;

in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

out vec4 fragColor;

float getVisibility(sampler2D shadowMap, vec4 shadowCoordinateWdivide, float shadowCoordW) {
	
	float distanceFromLight = texture2D(shadowMap, shadowCoordinateWdivide.st).z + shadowBias;
		
	if (shadowCoordW > 0.0) {
		return distanceFromLight < shadowCoordinateWdivide.z ? 0.75 : 1.0;
	}
	
	return 1.0;
}

float DiscBlur64(sampler2D shadowMap, vec4 shadowCoord, float blurSize) {		
	vec4 inUV = shadowCoord / shadowCoord.w;
	vec2 poissonDisk[64];
	poissonDisk[0] = vec2(-0.613392, 0.617481);
	poissonDisk[1] = vec2(0.170019, -0.040254);
	poissonDisk[2] = vec2(-0.299417, 0.791925);
	poissonDisk[3] = vec2(0.645680, 0.493210);
	poissonDisk[4] = vec2(-0.651784, 0.717887);
	poissonDisk[5] = vec2(0.421003, 0.027070);
	poissonDisk[6] = vec2(-0.817194, -0.271096);
	poissonDisk[7] = vec2(-0.705374, -0.668203);
	poissonDisk[8] = vec2(0.977050, -0.108615);
	poissonDisk[9] = vec2(0.063326, 0.142369);
	poissonDisk[10] = vec2(0.203528, 0.214331);
	poissonDisk[11] = vec2(-0.667531, 0.326090);
	poissonDisk[12] = vec2(-0.098422, -0.295755);
	poissonDisk[13] = vec2(-0.885922, 0.215369);
	poissonDisk[14] = vec2(0.566637, 0.605213);
	poissonDisk[15] = vec2(0.039766, -0.396100);
	poissonDisk[16] = vec2(0.751946, 0.453352);
	poissonDisk[17] = vec2(0.078707, -0.715323);
	poissonDisk[18] = vec2(-0.075838, -0.529344);
	poissonDisk[19] = vec2(0.724479, -0.580798);
	poissonDisk[20] = vec2(0.222999, -0.215125);
	poissonDisk[21] = vec2(-0.467574, -0.405438);
	poissonDisk[22] = vec2(-0.248268, -0.814753);
	poissonDisk[23] = vec2(0.354411, -0.887570);
	poissonDisk[24] = vec2(0.175817, 0.382366);
	poissonDisk[25] = vec2(0.487472, -0.063082);
	poissonDisk[26] = vec2(-0.084078, 0.898312);
	poissonDisk[27] = vec2(0.488876, -0.783441);
	poissonDisk[28] = vec2(0.470016, 0.217933);
	poissonDisk[29] = vec2(-0.696890, -0.549791);
	poissonDisk[30] = vec2(-0.149693, 0.605762);
	poissonDisk[31] = vec2(0.034211, 0.979980);
	poissonDisk[32] = vec2(0.503098, -0.308878);
	poissonDisk[33] = vec2(-0.016205, -0.872921);
	poissonDisk[34] = vec2(0.385784, -0.393902);
	poissonDisk[35] = vec2(-0.146886, -0.859249);
	poissonDisk[36] = vec2(0.643361, 0.164098);
	poissonDisk[37] = vec2(0.634388, -0.049471);
	poissonDisk[38] = vec2(-0.688894, 0.007843);
	poissonDisk[39] = vec2(0.464034, -0.188818);
	poissonDisk[40] = vec2(-0.440840, 0.137486);
	poissonDisk[41] = vec2(0.364483, 0.511704);
	poissonDisk[42] = vec2(0.034028, 0.325968);
	poissonDisk[43] = vec2(0.099094, -0.308023);
	poissonDisk[44] = vec2(0.693960, -0.366253);
	poissonDisk[45] = vec2(0.678884, -0.204688);
	poissonDisk[46] = vec2(0.001801, 0.780328);
	poissonDisk[47] = vec2(0.145177, -0.898984);
	poissonDisk[48] = vec2(0.062655, -0.611866);
	poissonDisk[49] = vec2(0.315226, -0.604297);
	poissonDisk[50] = vec2(-0.780145, 0.486251);
	poissonDisk[51] = vec2(-0.371868, 0.882138);
	poissonDisk[52] = vec2(0.200476, 0.494430);
	poissonDisk[53] = vec2(-0.494552, -0.711051);
	poissonDisk[54] = vec2(0.612476, 0.705252);
	poissonDisk[55] = vec2(-0.578845, -0.768792);
	poissonDisk[56] = vec2(-0.772454, -0.090976);
	poissonDisk[57] = vec2(0.504440, 0.372295);
	poissonDisk[58] = vec2(0.155736, 0.065157);
	poissonDisk[59] = vec2(0.391522, 0.849605);
	poissonDisk[60] = vec2(-0.620106, -0.328104);
	poissonDisk[61] = vec2(0.789239, -0.419965);
	poissonDisk[62] = vec2(-0.545396, 0.538133);
	poissonDisk[63] = vec2(-0.178564, -0.596057);
					
	vec2 texelSize = vec2(2048.0 / 1.0);
					
	float max_siz = 0.005* blurSize*(inUV.z+0.1);
	vec2 uv = inUV.xy;
	float sum = 0;
	for (int i = 0; i < 64; i++)
	{
		vec2 texcoord = uv + max_siz * poissonDisk[i];
		sum += getVisibility(shadowMap, vec4(texcoord.xy, inUV.zw), shadowCoord.w);
	}
	return sum / 64;
}
				
void main(void)
{
	float visibility = DiscBlur64(_ShadowMap, shadowCoord, 1.0);
	mat3 normalMatrix = transpose(inverse(mat3(uModel)));
	vec3 nl;
	if(!gl_FrontFacing)
		nl=-normal;
	else
		nl=normal;
	vec3 transformedNormal = normalize(normalMatrix * nl);
	float NdotL = 1.0;
	vec4 tex = texture(_MainTexture, textureCoord);
	vec3 nt=texture(_NormalTexture, textureCoord).rgb;
	float rn=texture(_RoughnessTexture, textureCoord).r;


	vec3 diffuse_tangent=vec3(0.0,0.0,0.0);
	vec3 specular_tangent=vec3(0.0,0.0,0.0); 

	vec3 colorTex=vec3(1.0,1.0,1.0);
	/*Ambient*/
	float ambientStrength=0.2;
	vec3 ambient=ambientStrength*colorTex;
	
	vec3 lightColor=vec3(0.8,0.8,0.8);
	vec3 lightColor_tangent=vec3(0.8,0.8,0.8);
	/*Diffuse*/
	vec3 lightDir=normalize(lightPos-fragPos);
	float diff=max(dot(transformedNormal,lightDir), 0.0);
	vec3 diffuse=diff*lightColor;
	

	float shininess=4.0; 
	vec3 light_specular=vec3(1.0,1.0,1.0);
	/*Specular*/
    /*vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, nl);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = ligit_specular * spec;*/
	


	vec3 nt2=normalize(nt*2.0-1.0);
	bool hasPBR=false;
	if(showPBR)
	{
	  
		
		vec3 lightDir_tangent=normalize(TangentLightPos-TangentFragPos);

		if(hasNormalTexture)
		{
			/*Tangent Diffuse*/
			float diff_tangent=abs(dot(nt2,lightDir_tangent));
			diffuse_tangent=diff_tangent*lightColor_tangent;
			hasPBR=true;
		}
		else
		{
			nt2=vec3(0.0,0.0,1.0);
		}
		if(hasRoughnessTexture)
		{
			/*Tangent Specular*/
			shininess=max(1.0,(1.0-rn)*16);
			vec3 viewDir_tangent = normalize(TangentViewPos - TangentFragPos);	
			vec3 reflectDir_tangent=reflect(-lightDir_tangent,nt2);
			float spec_tangent = pow(max(dot(viewDir_tangent, reflectDir_tangent), 0.0), shininess);
			specular_tangent =light_specular * spec_tangent*shininess/16;
			hasPBR=true;
		}
	}

	vec3 modelColor=vec3(0.8,0.8,0.8);
	/*vec3 modelColor=specular_tangent+0.6;*/

	vec3 result; 
	vec4 col;
	vec3 balance_color=vec3(1.0,1.0,1.0);
	if(showColor)
	{
		/*result= (ambient+diffuse) * vec3(modelColor);*/
		result=(ambient+mix(diffuse,diffuse_tangent,float(hasNormalTexture))+specular_tangent*float(hasRoughnessTexture))*modelColor;
		col=vec4(result*0.25,1.0);
		tex=vec4(result,1.0);
	}
	if(!showColor&&showPBR&&hasPBR)
	{	vec3 PBR=mix(0.9*balance_color,diffuse_tangent,float(hasNormalTexture))+mix(0.6*balance_color,specular_tangent,float(hasRoughnessTexture));
		PBR=PBR+0.5*balance_color*float(hasNormalTexture)*float(hasRoughnessTexture);
		tex=vec4(PBR*vec3(tex),1.0);
		col=tex*color;
	}
	if(!showColor&&!(showPBR&&hasPBR))
		col=tex*color;
	
	
	fragColor = (NdotL * visibility * mix(col*4.0, tex, uFinalized)) * ColorMode;


}