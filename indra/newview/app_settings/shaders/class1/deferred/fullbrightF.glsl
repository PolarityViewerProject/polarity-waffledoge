#endif

VARYING vec3 vary_position;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

//vec3 fullbrightAtmosTransport(vec3 light);
//vec3 fullbrightScaleSoftClip(vec3 light);

vec3 srgb_to_linear(vec3 cs)
{
	vec3 low_range = cs / vec3(12.92);
	vec3 high_range = pow((cs+vec3(0.055))/vec3(1.055), vec3(2.4));
	bvec3 lte = lessThanEqual(cs,vec3(0.04045));
	result.b = lt.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lt);
#endif



}

#ifdef HAS_ALPHA_MASK
uniform float minimum_alpha;
#endif

	{
		discard;
	}
#endif

	color.rgb *= vertex_color.rgb;
	//color.rgb = fullbrightAtmosTransport(color.rgb);
	//color.rgb = fullbrightScaleSoftClip(color.rgb);

#ifdef WATER_FOG
	vec3 pos = vary_position;
	color = applyWaterFogDeferred(pos, vec4(color.rgb, final_alpha));
#else
	color.a   = final_alpha;
#endif

	frag_color = color;
}

