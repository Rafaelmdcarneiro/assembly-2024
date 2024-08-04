#ifndef __g_shader_fragment_forward_header__
#define __g_shader_fragment_forward_header__
static const char *g_shader_fragment_forward = ""
#if defined(USE_LD)
"forward.frag.glsl"
#else
"#version 450\n"
"layout(location=0)out vec4 e;"
"in C"
"{"
"vec4 e;"
"}"
"p;"
"void main()"
"{"
"e=p.e;"
"}"
#endif
"";
#if !defined(DNLOAD_RENAME_UNUSED)
#if defined(__GNUC__)
#define DNLOAD_RENAME_UNUSED __attribute__((unused))
#else
#define DNLOAD_RENAME_UNUSED
#endif
#endif
#endif
