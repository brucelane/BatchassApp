#version 110
uniform float        iRatio;
uniform float        iZoom;               	// zoom

void main(void)
{
	// calculate glowing line strips based on texture coordinate
	const float resolution = 64.0;
	const float center = 0.5;
	//const float width = 0.02;
 
	//float f = fract( resolution * gl_TexCoord[0].s + 2.0 * gl_TexCoord[0].t );
	float f = fract( resolution * gl_TexCoord[0].s + iZoom * gl_TexCoord[0].t );
	float d = abs(center - f);
	float strips = clamp(iRatio / 100.0 / d, 0.0, 1.0);
 
	// calculate output color
	gl_FragColor.rgb = gl_Color.rgb * strips;
	gl_FragColor.a = 1.0;
}