/**
 * @file gaussianBlurF.glsl
 * @brief Post processing Gaussian blur shader
 *
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (C) 2016 Doug Falta
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * The Polarity Viewer Project
 * http://www.polarityviewer.org
 * $/LicenseInfo$
 */


#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect exo_screen;
uniform vec2 screen_res;
uniform vec2 blur_direction;
VARYING vec2 vary_fragcoord;

void main ()
{
	vec4 new_color = vec4(0.0);
	vec2 offset1 = vec2(1.411764705882353) * blur_direction;
	vec2 offset2 = vec2(3.2941176470588234) * blur_direction;
	vec2 offset3 = vec2(5.176470588235294) * blur_direction;
	
	new_color += texture2DRect(exo_screen, vary_fragcoord.xy) * 0.1964825501511404;
	new_color += texture2DRect(exo_screen, vary_fragcoord.xy + offset1) * 0.2969069646728344;
	new_color += texture2DRect(exo_screen, vary_fragcoord.xy - offset1) * 0.2969069646728344;
	new_color += texture2DRect(exo_screen, vary_fragcoord.xy + offset2) * 0.09447039785044732;
	new_color += texture2DRect(exo_screen, vary_fragcoord.xy - offset2) * 0.09447039785044732;
	new_color += texture2DRect(exo_screen, vary_fragcoord.xy + offset3) * 0.010381362401148057;
	new_color += texture2DRect(exo_screen, vary_fragcoord.xy - offset3) * 0.010381362401148057;
	
	frag_color = new_color;
}
