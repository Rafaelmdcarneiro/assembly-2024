//#g_shader_mesh_forward
#version 450
#extension GL_NV_mesh_shader : require

#if !defined(DISPLAY_MODE)
#define DISPLAY_MODE 1080
#endif
#if !defined(RESOLUTION_DIVISOR)
#define RESOLUTION_DIVISOR 5
#endif
#define SCREEN_H ((DISPLAY_MODE > 0) ? (DISPLAY_MODE) : (-(DISPLAY_MODE)))
#define SCREEN_W (((800 == SCREEN_H) || (1200 == SCREEN_H)) ? ((SCREEN_H / 10) * 16) : (((SCREEN_H * 16) / 9) - (((SCREEN_H * 16) / 9) % 4)))
#if !defined(RESOLUTION_X)
#define RESOLUTION_X (SCREEN_W / RESOLUTION_DIVISOR)
#endif
#if !defined(RESOLUTION_Y)
#define RESOLUTION_Y (SCREEN_H / RESOLUTION_DIVISOR)
#endif
#define PRIMITIVE_MAX 96
#define TERRAIN_BATCH 64
#define RAIN_BATCH 32

layout(local_size_x=1) in;
layout(points, max_vertices=PRIMITIVE_MAX, max_primitives=PRIMITIVE_MAX) out;

layout(location=0) uniform sampler3D uniform_noise;
layout(location=1) uniform int uniform_time;
#if defined(USE_LD)
layout(location=2) uniform vec3 uniform_params[7];
#endif

/// Camera data.
///
/// 0: Duration, unused, unused.
/// 1: Position start.
/// 2: Position end.
/// 3: Lookat start.
/// 4: Lookat end.
///
/// Duration is arranged so 4 ticks is one second. Everything more finegrained in the intro uses global time byte
/// count directly, which means 352800 ticks per second.
vec3 g_camera[] =
{
    vec3(27, 0.0, 0.0), vec3(-16.5, -17.2, -11.4), vec3(-16.5, -17.2, -11.4), vec3(0.0, -1.0, 0.04), vec3(0.0, -1.0, 0.04),
    vec3(25, 0.0, 0.0), vec3(-16.5, -17.2, -11.4), vec3(-16.5, -15.0, -11.4), vec3(0.0, -1.0, 0.04), vec3(0.0, -1.0, 0.04),
    vec3(40, 0.0, 0.0), vec3(-23.60, -7.36, -8.22), vec3(-20.0, -8.0, -12.0), vec3(-0.9, 0.2, -0.4), vec3(-0.7, 0.6, -0.4),
    vec3(40, 0.0, 0.0), vec3(-18.0, -25.8, 82.2), vec3(-20.0, -69.0, 80.0), vec3(-0.97, 0.14, -0.18), vec3(-0.97, 0.14, -0.18),
    vec3(52, 0.0, 0.0), vec3(88.0, -170.0, 116.0), vec3(30.0, -166.0, 70.0), vec3(-0.6, 0.3, -0.7), vec3(-0.89, 0.1, -0.45),
    vec3(50, 0.0, 0.0), vec3(-5.5, 2.0, -7.4), vec3(10.6, 2.4, -1.0), vec3(0.2, 0.07, -1.0), vec3(0.9, 0.2, -0.4),
    vec3(60, 0.0, 0.0), vec3(11.0, 0.0, -27.0), vec3(11.0, -43.0, -26.0), vec3(0.0, -1.0, 0.1), vec3(0.0, -1.0, 0.1),
    vec3(32, 0.0, 0.0), vec3(11.0, -43.0, -26.0), vec3(13.0, -72.0, -23.0), vec3(0.0, -1.0, 0.1), vec3(0.01, 0.2, 1.0),
    vec3(26, 0.0, 0.0), vec3(-9.0, -70.0, 23.0), vec3(-10.5, -70.0, 20.0), vec3(0.7, 0.1, -0.7), vec3(0.8, 0.1, -0.6),
    vec3(24, 0.0, 0.0), vec3(-6.64, -72.63, 13.94), vec3(-6.56, -72.56, 12.40), vec3(0.92, 0.39, -0.01), vec3(0.94, 0.33, 0.07),
    vec3(70, 0.0, 0.0), vec3(21.76, -65.44, 14.64), vec3(34.34, -66.88, 21.30), vec3(-0.88, 0.12, -0.46), vec3(-0.88, 0.12, -0.46)
};

int g_idx = int(gl_WorkGroupID.x);
int g_time = uniform_time>>9;
float g_core_phase = smoothstep(57600, 59700, g_time) - smoothstep(69300, 71300, g_time) * 1.6;
mat3 g_rot = mat3(-0.8, -0.6, -0.01, -0.59, 0.78, -0.20, 0.13, -0.16, -0.98);
mat4 g_projection_modelview = mat4(0.0);
vec3 g_core_pos = vec3(14.0, -64.0, 11.0);
const vec3 g_lit = normalize(vec3(0.3, 1.0, 0.3));
const float g_aspect_ratio = float(RESOLUTION_X) / RESOLUTION_Y;
const float g_far = 99.0;
const float g_pi = 3.14159;

// Before being used as distance to the hit location, this variable holds the near plane distance.
float distance = 0.09;

out PerVertexData
{
    vec4 color;
} vertexData[];

vec3 turbulence(vec3 pos, float pmul, float smul)
{
    vec3 ret = vec3(0.0);
    float scale = 1.0;
    for(int ii = 0; ii < 4; ++ii)
    {
        pos = g_rot * pos;
        ret += texture(uniform_noise, pos).xyz * scale;
        pos *= pmul;
        scale *= smul;
    }
    return normalize(ret);
}

/// Fractal SDF function.
///
/// The fractal is based on "dust" by XorDev (which in turn is based on Menger Sponge):
/// https://www.shadertoy.com/view/cdG3Wd
/// The original goes something like this (s and v are position):
///
///     for(s=v=p+=d/length(d)*s.y, i=n; i>.1; i*=.4)
///         v.xz*=R+2.)),
///         s = max(s,min(min(v=i*.8-abs(mod(v,i+i)-i),v.x),v.z)),
///
/// This version makes transformations both in 2D and 3D to get the desired result.
/// Otherwise the algorithm is roughly the same.
///
/// \param pos Position.
/// \return Distance to edge.
float sdf_fractal(vec3 pos)
{
    vec3 modulo_pos = pos;
    float ret = pos.y - 13.0;
    for(float ii = 99.0; ii > 0.1; ii *= 0.3)
    {
        if(ii > 1)
        {
            modulo_pos.xz *= mat2(g_rot);
        }
        else
        {
            modulo_pos *= g_rot;
        }
        modulo_pos = ii * 0.77 - abs(mod(modulo_pos, ii + ii) - ii);
        float min_component = min(min(modulo_pos.y, modulo_pos.x), modulo_pos.z);
        ret = max(ret, min_component);
    }
    if (g_core_phase > 0.0)
    {
        vec3 diff_core = pos - g_core_pos;
        vec2 mod_core = mod(diff_core.xz + g_time * 0.002, 1.6) - 0.8;
        float core_y_component = length(diff_core.xz) * 1.6 - abs(diff_core.y) * 0.5 + 1;
        vec3 dd = vec3(abs(mod_core) - vec2(g_core_phase * 0.4), core_y_component);
        float ret_box = length(max(dd, 0.0)) + min(max(max(dd.x, dd.z), dd.y), 0.0);
        ret = min(ret, ret_box);
    }
    return ret;
}

float raycast(vec3 pos, vec3 dir, float max_dist, out float dist, out vec3 hit)
{
    vec3 prev = pos;
    dist = 0.0;
    for(int ii = 0; (ii < 200) && (dist < max_dist); ++ii)
    {
        hit = pos + dist * dir;
        float sdf_value_fractal = sdf_fractal(hit);
        if (sdf_value_fractal < 0.0)
        {
            for (int jj = 0; (jj < 9); ++jj)
            {
                vec3 mid = (prev + hit) * 0.5;
                if (sdf_fractal(mid) < 0.0)
                {
                    hit = mid;
                }
                else
                {
                    prev = mid;
                }
            }
            return 1.0;
        }
        // Core SDF is a sphere and it's not that important to bisect it.
        float sdf_value_core = length(hit - g_core_pos) - mix(1.5, 4.0, g_core_phase);
        if (sdf_value_core < 0.0)
        {
            return -1.0;
        }
        prev = hit;
        dist = min(max_dist, dist + min(sdf_value_fractal, sdf_value_core) * 1.005 + 0.005);
    }
    return 0.0;
}

vec4 raycast_and_shadow(vec3 pos, vec3 dir, out float dist, out vec3 hit)
{
    float color = raycast(pos, dir, g_far, dist, hit);
    if (color > 0.0)
    {
        vec3 diff = vec3(0.04, 0.0, 0.0);
        vec3 nor = normalize(normalize(vec3(sdf_fractal(hit + diff), sdf_fractal(hit + diff.yxy), sdf_fractal(hit + diff.yyx))) + turbulence(hit * 9, 0.1, 1.4) * 0.05);
        vec3 core_diff = g_core_pos - hit;
        float core_dist = length(core_diff);
        vec3 lit = normalize(mix(core_diff / core_dist, g_lit, mix(1.0, smoothstep(20.0, 30.0, core_dist), g_core_phase)));
        vec3 hh = normalize(lit - dir);

        // Oren-nayar lookalike. High roughness.
        float i_diffuse_reflectance = 1.1;
#if 0
        float rsqr = 11.0;
        float i_A = 1.0 - 0.5 * rsqr / (rsqr + 0.33);
        float i_B = 0.45 * rsqr / (rsqr + 0.09);
#else
        // Roughness squared 11 or so.
        float i_A = 0.51;
        float i_B = 0.45;
#endif
        float ndotl = max(dot(nor, lit), 0.0);
        float ndotv = max(dot(nor, -dir), 0.0);
        float anglel = acos(ndotl);
        float anglev = acos(ndotv);
        float cos_phi_diff = dot(normalize(lit - ndotl * nor), normalize(-dir - ndotv * nor));
        float i_diffuse = i_diffuse_reflectance * ndotl * (i_A + i_B * max(0.0, cos_phi_diff) * sin(max(anglel, anglev)) * tan(min(anglel, anglev)));

        // GGX specular lookalike. The fresenel component is dropped.
        float ndoth = max(dot(nor, hh), 0);
        float i_ldoth = max(dot(dir, hh), 0);
        float i_rsqr = 2.0;
        float i_alpha_sqr = i_rsqr * i_rsqr;
        float k = i_rsqr / 2;
        float denom = ndoth * ndoth * (i_alpha_sqr - 1) + 1;
        float i_D = i_alpha_sqr / (g_pi * denom * denom);
        float i_specular = ndotl * i_D / ((1 - ndotl * k + k) * (1 - ndotv * k + k));

        float shading = i_diffuse + i_specular;

        // Shadow is just a multiplier. It's good enough.
        // Reuse ndotl and nor as shadow distance and shadow hit location, they're not used anymore.
        float shadow_color = raycast(hit + lit * diff.x, lit, 33.0, ndotl, nor);
        if (shadow_color > 0.0)
        {
            shading *= 0.6;
        }
        return vec4(mix(vec3(1.5, 1.3, 1.0), vec3(color), min(core_dist * 0.009, 1.0)) * shading, color);
    }
    return vec4(color);
}

void generate_particles(vec3 pos, vec3 dir, vec3 hit, vec4 color, vec3 walk_mul, vec3 walk_add, float norm_dist, float inv_dist, int count)
{
    vec3 offset = vec3(0.0);
    for(int ii = 0; (ii < count); ++ii)
    {
        offset += texture(uniform_noise, hit * 0.03 + offset * 0.011 + vec3(0.0, 0.0, 0.1)).xyz * walk_mul + walk_add;
        vec4 spos = g_projection_modelview * vec4(hit + offset * inv_dist * inv_dist, 1.0);
        spos /= spos.w;
        vec2 add = texture(uniform_noise, (pos * 0.002 - dir * 0.003 + vec3(g_idx + ii)) * g_rot).xy * mix(0.0022, 0.0033, inv_dist);
        spos.xy += add;
        gl_MeshVerticesNV[gl_PrimitiveCountNV].gl_Position = spos;
        vec4 outColor = mix(vec4(color.rgb, 1.0) * 0.6, vec4(mix(vec3(0.5, 0.5, 0.3), vec3(0.3), inv_dist), 1.0), norm_dist);
        vertexData[gl_PrimitiveCountNV].color = vec4(outColor.xy + max(add, 0.0) * 44, outColor.zw);
        gl_PrimitiveIndicesNV[gl_PrimitiveCountNV] = gl_PrimitiveCountNV;
        gl_PrimitiveCountNV += 1;
    }
}

void main()
{
    if(g_core_phase > -0.6)
    {
        g_core_phase *= mix(g_time & 63, 63, g_core_phase) / 63.0;
    }
    gl_PrimitiveCountNV = 0;

#if defined(USE_LD)
    g_rot = mat3(uniform_params[4].x, uniform_params[4].y, uniform_params[4].z,
            uniform_params[5].x, uniform_params[5].y, uniform_params[5].z,
            uniform_params[6].x, uniform_params[6].y, uniform_params[6].z);
#endif

    int iy = g_idx / RESOLUTION_X;
    int ix = g_idx - iy * RESOLUTION_X;
    float cx = mix(-1.0, 1.0, ix / float(RESOLUTION_X));
    float nx = mix(-1.0, 1.0, (ix + 1) / float(RESOLUTION_X));
    float cy = mix(-1.0, 1.0, iy / float(RESOLUTION_Y));
    float ny = mix(-1.0, 1.0, (iy + 1) / float(RESOLUTION_Y));
    float mx = mix(cx, nx, 0.5);
    float my = mix(cy, ny, 0.5);

    // Direction settings.
    vec3 position;
    vec3 forward;
    vec3 up = vec3(0.0, 1.0, 0.0);

#if defined(USE_LD)
    if(uniform_params[3].x == 0.0)
    {
        g_rot = mat3(uniform_params[4].x, uniform_params[4].y, uniform_params[4].z,
                uniform_params[5].x, uniform_params[5].y, uniform_params[5].z,
                uniform_params[6].x, uniform_params[6].y, uniform_params[6].z);
        position = uniform_params[0];
        forward = normalize(uniform_params[1]);
        up = normalize(uniform_params[2]);
    }
    else
    {
#endif
        float ctime = uniform_time;
        float time_mul = 88000; // Second divided into almost steps.
        for(int ii = 0; true; ii += 5)
        {
            vec3 settings = g_camera[ii];
            float ftime = settings.x * time_mul;
            float ratio = ctime / ftime;
            if(ratio < 1)
            {
                position = mix(g_camera[ii + 1], g_camera[ii + 2], ratio);
                forward = normalize(mix(g_camera[ii + 3], g_camera[ii + 4], ratio));
                break;
            }
            ctime -= ftime;
        }

        //position = g_camera[1];
        //forward = g_camera[3];
#if defined(USE_LD)
    }
#endif
 
    // Raycast direction.
    // Note that hit is the right direction before use to remove one extra variable.
    vec2 aspect = vec2(mx * g_aspect_ratio, my);
    vec3 hit = normalize(cross(forward, up));
    up = normalize(cross(hit, forward));
    vec3 direction = normalize(aspect.x * hit + aspect.y * up + forward);

    // Create projection matrix.
#if defined(USE_LD)
    float fov_y = 1.3; // Radians.
    float ff = 1.0 / tan(fov_y / 2.0)/*1.33*/;
#else
    float ff = 1.315;
#endif
    g_projection_modelview[0][0] = ff / g_aspect_ratio;
    g_projection_modelview[1][1] = ff;
    g_projection_modelview[2][2] = (distance + g_far) / (distance - g_far);
    g_projection_modelview[2][3] = -1.0;
    g_projection_modelview[3][2] = (2.0 * distance * g_far) / (distance - g_far);

    mat3 camera_matrix = transpose(mat3(hit, up, -forward));
    mat4 modelview = mat4(camera_matrix);
    modelview[3] = vec4(camera_matrix * -position, 1.0);
    g_projection_modelview *= modelview;

    // Normal.
    vec4 color = raycast_and_shadow(position, direction, distance, hit);
    float norm_dist = distance / g_far;
    float inv_dist = sqrt(1 - norm_dist);
    float phase_in = smoothstep(10400, 13100, g_time);
    float phase_out = smoothstep(12700, 13100, g_time);
    int mod_duration = 1750;

    float mod_phase = mod(g_time - 200, mod_duration);
    float phase_dist = length(hit - mix(position, g_core_pos, g_core_phase)) * 11;
    if (g_core_phase <= 0)
    {
        mod_phase *= mix(g_time & 255, 255, mod_phase / mod_duration) / 255;
    }
    float phase = (g_time > 73000) ? 0.0 : smoothstep(-200, 0, -abs(mod_phase - phase_dist)) * phase_in * 0.3;
    color += vec4(0.4, 0.0, 0.1, 0.0) * phase;
    phase = phase * ((min(abs(g_time - 49500), abs(g_time - 76000)) < 4500) ? 0.03 : 1) + (phase_in - phase_out) * 0.1;

    // Reuse forward direction as walk direction.
    forward = mix(g_lit, normalize(hit - g_core_pos), g_core_phase) * min(phase, phase_out) * 0.6;

    // Reuse mod_duration for error phase.
    mod_duration = g_time / 30;
    forward += texture(uniform_noise, hit * 0.001 + my * vec3(0.11, 0.1, 0.12)).xyz * smoothstep(-0.05, 0.0, -abs(my - texture(uniform_noise, vec3(mod_duration * 0.11) + vec3(0.11, 0.1, 0.11)).y * 6.0)) * (0.001 - phase_in * 0.001 + g_core_phase * 0.05);

    if (color.w < 0.0)
    {
        color = -color;
        phase = max(phase, g_core_phase * (((g_time & 2048) > 0) ? 0.5 : 0.2));
    }
    // Reuse phase_out to fade to grey.
    phase_out = color.r * 0.3 + color.g * 0.59 + 0.11 * color.b;

    generate_particles(position, direction, hit, mix(color, vec4(phase_out * 0.3), smoothstep(73000, 76000, g_time)), vec3(phase), forward, norm_dist, inv_dist, TERRAIN_BATCH);

    // Rain.
    if (g_idx < (smoothstep(13100, 15000, g_time) - g_core_phase) * 21 * 21 * 21)
    {
        int pz = g_idx / (21 * 21);
        int py = (g_idx - pz * 21 * 21) / 21;
        int px = g_idx - pz * 21 * 21 - py * 21;
        vec3 cpos = floor(position) + vec3(px, py, pz) - 10;
        vec3 rpos = texture(uniform_noise, cpos * 0.011).xyz;
        norm_dist = min(length(rpos), 1);
        inv_dist = sqrt(1 - norm_dist);
        rpos.y = mod(rpos.y - g_time * 0.0006, 1);
        cpos += rpos * 33;
        rpos = cpos - position;

        // Reuse phase as rain position vector lenght.
        phase = length(rpos);

        raycast(position, rpos / phase, 33.0, distance, hit);
        if (distance > phase)
        {
            raycast(cpos, vec3(0.0, 1.0, 0.0), 33.0, distance, hit);
            if (distance >= 33)
            {
                generate_particles(position, direction, cpos, vec4(vec3(0.9, 0.9, 1.0), 0.01), vec3(0.01), vec3(0.0, 0.02, 0.0), norm_dist, inv_dist, RAIN_BATCH);
            }
        }
    }

#if 0
    gl_MeshVerticesNV[0].gl_Position = vec4(mx, my, 0.0, 1.0); // Center
    gl_MeshVerticesNV[1].gl_Position = vec4(cx, cy, 0.0, 1.0); // Upper Left
    gl_MeshVerticesNV[2].gl_Position = vec4(nx, cy, 0.0, 1.0); // Upper Right
    gl_MeshVerticesNV[3].gl_Position = vec4(cx, ny, 0.0, 1.0); // Bottom Left
    gl_MeshVerticesNV[4].gl_Position = vec4(nx, ny, 0.0, 1.0); // Bottom Right
    vertexData[0].color = raycast(mx, my);
    vertexData[1].color = raycast(cx, cy);
    vertexData[2].color = raycast(nx, cy);
    vertexData[3].color = raycast(cx, ny);
    vertexData[4].color = raycast(nx, ny);
    gl_PrimitiveIndicesNV[0] = 0;
    gl_PrimitiveIndicesNV[1] = 1;
    gl_PrimitiveIndicesNV[2] = 2;
    gl_PrimitiveIndicesNV[3] = 0;
    gl_PrimitiveIndicesNV[4] = 4;
    gl_PrimitiveIndicesNV[5] = 2;
    gl_PrimitiveIndicesNV[6] = 0;
    gl_PrimitiveIndicesNV[7] = 3;
    gl_PrimitiveIndicesNV[8] = 4;
    gl_PrimitiveIndicesNV[9] = 0;
    gl_PrimitiveIndicesNV[10] = 1;
    gl_PrimitiveIndicesNV[11] = 3;
    gl_PrimitiveCountNV = 4;
#endif
}
