uniform sampler2D colorMap;
uniform float        iZoom;               	// zoom

void main(void)
{
   gl_FragColor = vec4( texture2D(colorMap, iZoom * gl_TexCoord[0].xy).xyz, 1.0 );
   //	gl_FragColor.a = 1.0;
}