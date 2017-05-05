/***********************************
 * exoPostSpecialF.glsl
 * Provides special post effects.
 * Copyright NiranV Dean, 2014
 ***********************************/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect exo_screen;

uniform vec2 screen_res;
uniform float exo_post_grey_str;
uniform int exo_numcolors;
uniform float exo_post_sepia_str;

VARYING vec2 vary_fragcoord;

void main ()
{
	vec4 col = texture2DRect(exo_screen, vary_fragcoord.xy);
	
	if(exo_numcolors > 2)
	{
		col.rgb = pow(col.rgb, vec3(0.6));
		col.rgb = col.rgb * exo_numcolors;
		col.rgb = floor(col.rgb);
		col.rgb = col.rgb / exo_numcolors;
		col.rgb = pow(col.rgb, vec3(1.0/0.6));
	}
	
	vec3 col_gr = vec3((0.299 * col.r) + (0.587 * col.g) + (0.114 * col.b));
	col.rgb = mix(col.rgb, col_gr, exo_post_grey_str);
	
	vec3 col_sep;
	col_sep.r = (col.r*0.3588) + (col.g*0.7044) + (col.b*0.1368);
	col_sep.g = (col.r*0.299) + (col.g*0.5870) + (col.b*0.114);
	col_sep.b = (col.r*0.2392) + (col.g*0.4696) + (col.b*0.0912);
	col.rgb = mix(col.rgb, col_sep, exo_post_sepia_str);
    
	frag_color = col;
  
}
