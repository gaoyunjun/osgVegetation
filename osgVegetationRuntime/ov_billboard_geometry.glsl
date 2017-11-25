#version 120
#extension GL_ARB_geometry_shader4 : enable
#pragma import_defines (BLT_ROTATED_QUAD, BLT_CROSS_QUADS,BLT_GRASS)
in vec2 ov_te_texcoord[];
varying vec2 ov_geometry_texcoord;
varying vec4 ov_geometry_color;
flat varying int ov_geometry_tex_index;

uniform mat4 osg_ModelViewProjectionMatrix;
uniform mat4 osg_ProjectionMatrix;
uniform float osg_SimulationTime;
uniform sampler2D ov_color_texture;
uniform sampler2D ov_land_cover_texture;
uniform int ov_num_billboards;
uniform vec4 ov_billboard_data[10];
uniform float ov_billboard_max_distance;
uniform float ov_billboard_color_threshold;
uniform float ov_billboard_color_impact;
uniform float ov_billboard_land_cover_id;

float ov_range_rand(float minValue, float maxValue, vec2 co)
{
    float t = fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
    return minValue + t*(maxValue-minValue);
}

float ov_rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// Generate a pseudo-random barycentric point inside a triangle.
vec3 ov_get_random_barycentric_point(vec2 seed)
{
    vec3 b;
    b[0] = ov_rand(seed.xy);
    b[1] = ov_rand(seed.yx);
    if (b[0]+b[1] >= 1.0)
    {
        b[0] = 1.0 - b[0];
        b[1] = 1.0 - b[1];
    }
    b[2] = 1.0 - b[0] - b[1];
    return b;
}


void main(void)
{
	vec4 random_pos = vec4(0,0,0,1);
    vec2 terrain_texcoord = vec2(0,0);
    
	//gen a random point in triangle
    vec3 b = ov_get_random_barycentric_point(gl_PositionIn[0].xy);
    for(int i=0; i < 3; ++i)
    {
		random_pos.x += b[i] * gl_PositionIn[i].x;
		random_pos.y += b[i] * gl_PositionIn[i].y;
		random_pos.z += b[i] * gl_PositionIn[i].z;
        
		terrain_texcoord.x += b[i] * ov_te_texcoord[i].x;
		terrain_texcoord.y += b[i] * ov_te_texcoord[i].y;
    }
	
	
	vec4 lc_color = texture2D(ov_land_cover_texture, terrain_texcoord);
	if (lc_color.x > ov_billboard_land_cover_id)
		return;

	ov_geometry_color = texture2D(ov_color_texture, terrain_texcoord);

	if(length(ov_geometry_color.xyz) > ov_billboard_color_threshold)
		return;

	//Scale color by some constant
	ov_geometry_color.xyz *= 1.9;

	ov_geometry_color.xyz = mix(vec3(1, 1, 1), ov_geometry_color.xyz, ov_billboard_color_impact);
	

	vec4 camera_pos = gl_ModelViewMatrixInverse[3];
	
	vec3 camera_to_bb_dir = camera_pos.xyz - random_pos.xyz;
	//we are only interested in xy-plane direction
	camera_to_bb_dir.z = 0;
	camera_to_bb_dir = normalize(camera_to_bb_dir);
	vec4 mv_pos = gl_ModelViewMatrix * random_pos;


	//just pick some fade distance
	float bb_fade_dist = ov_billboard_max_distance /3.0;
	float bb_fade_start_dist = ov_billboard_max_distance - bb_fade_dist;
	float bb_scale = 1 - clamp((-mv_pos.z - bb_fade_start_dist) / bb_fade_dist, 0.0, 1.0);

	//culling
    if (bb_scale == 0.0 )
        return;
	
	float rand_scale = ov_range_rand(0.5, 1.5, random_pos.xy /* * 50*/);
	bb_scale = bb_scale*bb_scale*bb_scale*rand_scale;
	
	//get random billboard
	float rand_val = ov_range_rand(0, 1.0, random_pos.xy);
	ov_geometry_tex_index = 0;
	float acc_probablity = 0;
	for (int i = 0; i < ov_num_billboards; i++)
	{
		float bb_probablity = ov_billboard_data[i].w;
		if (rand_val > acc_probablity && rand_val < acc_probablity + bb_probablity)
		{
			ov_geometry_tex_index = i;
			break;
		}
		acc_probablity += bb_probablity;
	}
	//ov_geometry_tex_index = int( floor( float(ov_num_billboards) * ov_range_rand(0, 1.0, random_pos.xy)));
	//ov_geometry_tex_index = min(ov_geometry_tex_index, ov_num_billboards - 1);
	vec4 billboard_data = ov_billboard_data[ov_geometry_tex_index];
	vec2 bb_size = billboard_data.xy;
	float bb_intensity = billboard_data.z;
	ov_geometry_color.xyz = ov_geometry_color.xyz*bb_intensity;

#ifdef BLT_ROTATED_QUAD
	vec3 bb_left = vec3(bb_scale * bb_size.x, 0, 0);
	vec3 bb_up = (gl_ModelViewMatrix*vec4(0.0, 0, bb_scale * bb_size.y,0)).xyz;
	vec4 bb_vertex;
	bb_vertex.w = random_pos.w;
	bb_vertex.xyz = mv_pos.xyz + bb_left;   gl_Position = osg_ProjectionMatrix* bb_vertex; ov_geometry_texcoord = vec2(0.0, 0.0);  EmitVertex();
	bb_vertex.xyz = mv_pos.xyz - bb_left;   gl_Position = osg_ProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 0.0);  EmitVertex();
	bb_vertex.xyz = mv_pos.xyz + bb_left + bb_up;  gl_Position = osg_ProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(0.0, 1.0);  EmitVertex();
	bb_vertex.xyz = mv_pos.xyz - bb_left + bb_up;  gl_Position = osg_ProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 1.0);  EmitVertex();
	//EndPrimitive();
#endif
#ifdef BLT_GRASS
	//get fake random rotation in radians
	float rand_rad = mod(random_pos.x, 2 * 3.14);
	
	//calc sin and cos for random rotation
	float sin_rot = sin(rand_rad);
	float cos_rot = cos(rand_rad);

	//calc sin and cos part for billboard width
	float width_sin = bb_scale*bb_size.x*sin_rot;
	float width_cos = bb_scale*bb_size.x*cos_rot;
	float wind = (1 + sin(osg_SimulationTime * rand_rad))*0.1;
	float sin_wind = sin_rot*wind;
	float cos_wind = cos_rot*wind;
	
	//calc billboard height
	float height = bb_scale*bb_size.y;
	
	//set billboard up vector
	vec3 up = vec3(0, 0, height);

	//First quad, left vector used to offset in xy-space from anchor point 
	vec3 bb_left = vec3(-width_sin, -width_cos, 0.0);

	//Calc a adhoc offset vector used to offset top verticies (perpendicular to left vec)
	//Here we just offset by the perpendicular left vector that will give us a 45-deg tilt
	//if bb height and width are the same. On top of that we add the wind effect.
	vec3 offset = vec3(width_cos, -width_sin, 0) + vec3(sin_wind, cos_wind, 0);
	
	vec4 bb_vertex;
	bb_vertex.w = random_pos.w;
	bb_vertex.xyz = random_pos.xyz + bb_left;  gl_Position = gl_ModelViewProjectionMatrix * bb_vertex;  ov_geometry_texcoord = vec2(0.0, 0.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz - bb_left;  gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 0.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz + bb_left + up + offset;    gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(0.0, 1.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz - bb_left + up + offset;    gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 1.0); EmitVertex();
	EndPrimitive();


	//Second quad, just flip offset
	offset = -offset;

	bb_vertex.xyz = random_pos.xyz + bb_left;  gl_Position = gl_ModelViewProjectionMatrix * bb_vertex;  ov_geometry_texcoord = vec2(0.0, 0.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz - bb_left;  gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 0.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz + bb_left + up + offset;    gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(0.0, 1.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz - bb_left + up + offset;    gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 1.0); EmitVertex();
	EndPrimitive();

	//Third quad, calc new left vector (perpendicular to prev left vec)
	bb_left = vec3(-width_cos, width_sin, 0.0);
	//also recalc offset to reflect that we have a 90-deg rotatation
	offset = vec3(-width_sin, -width_cos, 0) + vec3(sin_wind, cos_wind, 0);

	bb_vertex.xyz = random_pos.xyz + bb_left;  gl_Position = gl_ModelViewProjectionMatrix * bb_vertex;  ov_geometry_texcoord = vec2(0.0, 0.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz - bb_left;  gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 0.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz + bb_left + up + offset;    gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(0.0, 1.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz - bb_left + up + offset;    gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 1.0); EmitVertex();
	EndPrimitive();

	//Last quad, just flip offset as before
	offset = -offset;
	bb_vertex.xyz = random_pos.xyz + bb_left;  gl_Position = gl_ModelViewProjectionMatrix * bb_vertex;  ov_geometry_texcoord = vec2(0.0, 0.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz - bb_left;  gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 0.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz + bb_left + up + offset;    gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(0.0, 1.0); EmitVertex();
	bb_vertex.xyz = random_pos.xyz - bb_left + up + offset;    gl_Position = gl_ModelViewProjectionMatrix * bb_vertex; ov_geometry_texcoord = vec2(1.0, 1.0); EmitVertex();
	EndPrimitive();
#endif
}