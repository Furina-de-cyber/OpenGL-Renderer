#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 uv;

uniform sampler2D DirectLight;
uniform sampler2D SSR;
uniform sampler2D GI;

bool highlightedSSR;

const float DirectLightFade = 0.6f;
const float SSGIEnhanced = 1.5f;
const float SSGIFade = 0.75f;

void main() {
	vec4 directLightColor = texture(DirectLight, uv);
	vec4 SSR = texture(SSR, uv);
	vec4 GI = texture(GI, uv);
	float directLightColorFactor = 1.0f;
	if(highlightedSSR){
		bvec4 SSRExist = greaterThan(SSR, vec4(0.15f));
		if(SSRExist.x || SSRExist.y || SSRExist.z){
			directLightColorFactor = DirectLightFade;
		}
	}
	vec4 outputColor = directLightColor * directLightColorFactor + SSR + GI * 0.15f;
	FragColor = vec4(outputColor.rgb, 1.0f);
}