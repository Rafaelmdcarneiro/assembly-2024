/// \file
/// Main intro code.

//######################################
// Include #############################
//######################################

#include "dnload.h"

#if defined(USE_LD)
#include "fps_counter.hpp"
#include "glsl_program.hpp"
#include "glsl_pipeline.hpp"
#include "image_png.hpp"
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
namespace fs = boost::filesystem;
namespace po = boost::program_options;
#elif !defined(DISABLE_SYNTH)
#include "synth.h"
#endif

//######################################
// Define ##############################
//######################################

#if !defined(DISPLAY_MODE)
/// Screen mode.
///
/// Negative values windowed.
/// Positive values fullscreen.
#define DISPLAY_MODE -1080
#endif

#if !defined(RESOLUTION_DIVISOR)
/// Resolution divisor for dot pattern.
/// Larger values indicate sparser pattern.
#define RESOLUTION_DIVISOR 5
#endif

/// \cond
#if (0 > (DISPLAY_MODE))
#define SCREEN_F 0
#define SCREEN_H (-(DISPLAY_MODE))
#elif (0 < (DISPLAY_MODE))
#define SCREEN_F 1
#define SCREEN_H (DISPLAY_MODE)
#else
#error "invalid display mode (pre)"
#endif
#if ((800 == SCREEN_H) || (1200 == SCREEN_H))
#define SCREEN_W ((SCREEN_H / 10) * 16)
#else
#define SCREEN_W (((SCREEN_H * 16) / 9) - (((SCREEN_H * 16) / 9) % 4))
#endif
#if !defined(RESOLUTION_X)
#define RESOLUTION_X (SCREEN_W / RESOLUTION_DIVISOR)
#endif
#if !defined(RESOLUTION_Y)
#define RESOLUTION_Y (SCREEN_H / RESOLUTION_DIVISOR)
#endif
/// \endcond

/// Size of one sample in bytes.
#define AUDIO_SAMPLE_SIZE 4

/// \cond
#if (4 == AUDIO_SAMPLE_SIZE)
#define AUDIO_SAMPLE_TYPE_SDL AUDIO_F32SYS
typedef float sample_t;
#elif (2 == AUDIO_SAMPLE_SIZE)
#define AUDIO_SAMPLE_TYPE_SDL AUDIO_S16SYS
typedef int16_t sample_t;
#elif (1 == AUDIO_SAMPLE_SIZE)
#define AUDIO_SAMPLE_TYPE_SDL AUDIO_U8
typedef uint8_t sample_t;
#else
#error "invalid audio sample size"
#endif
#define AUDIO_POSITION_SHIFT (9 - (4 / sizeof(sample_t)))
/// \endcond

/// Audio channels.
#define AUDIO_CHANNELS 2

/// Audio samplerate.
#define AUDIO_SAMPLERATE 44100

/// Audio byterate.
#define AUDIO_BYTERATE (AUDIO_CHANNELS * AUDIO_SAMPLERATE * AUDIO_SAMPLE_SIZE)

/// Intro length (in bytes of audio).
#define INTRO_LENGTH (111 * AUDIO_BYTERATE)

/// Intro start position (in seconds).
#define INTRO_START (0 * AUDIO_BYTERATE)

/// Noise volume size (one side).
#define VOLUME_SIDE 64

/// Noise volume size (elements).
#define NOISE_SIZE (VOLUME_SIDE * VOLUME_SIDE * VOLUME_SIDE * 3)

/// Noise buffer size (bytes).
#define NOISE_BUFFER_LENGTH (NOISE_SIZE * sizeof(int16_t))

#if !defined(START_PLAY_POS)
/// Unless explicitly defined, playback starts at beginning of buffer.
#define START_PLAY_POS 0
#endif

/// Audio buffer length (bytes, should be larger than intro length for safety).
#define AUDIO_BUFFER_LENGTH (INTRO_LENGTH * 9 / 8 + START_PLAY_POS)

//######################################
// Global data #########################
//######################################

/// Buffer to use as generic scratchpad for everything.
static uint8_t g_scratch_buffer[(AUDIO_BUFFER_LENGTH > NOISE_BUFFER_LENGTH) ? AUDIO_BUFFER_LENGTH : NOISE_BUFFER_LENGTH];

/// Audio buffer (offset into the scratch buffer).
static uint8_t* g_audio_buffer = g_scratch_buffer + START_PLAY_POS;

/// Current audio position.
static int g_audio_position = INTRO_START;

#if defined(USE_LD)

/// Developer mode global toggle.
static bool g_flag_developer = false;

/// Record mode global toggle.
static bool g_flag_record = false;

/// Silent mode global toggle.
static bool g_flag_silent = false;

/// Usage blurb.
static std::string_view g_usage(""
        "Usage: olkiluoto_3-2-1 <options>\n"
        "For Assembly 2024 4k intro compo.\n"
        "Release version does not pertain to any size limitations.\n");

/// Wave file to use for loading music in non-minified mode where the synth is unlikely to work.
static std::string_view g_synth_test_input_file("olkiluoto_3-2-1.wav");

#else

/// Developer mode disabled.
#define g_flag_developer 0

#endif

//######################################
// Global functions ####################
//######################################

/// Global SDL window storage.
SDL_Window *g_sdl_window;

/// Swap buffers.
///
/// Uses global data.
static void swap_buffers()
{
    dnload_SDL_GL_SwapWindow(g_sdl_window);
}

/// Tear down initialized systems.
///
/// Uses global data.
static void teardown()
{
    dnload_SDL_Quit();
}

#if defined(USE_LD)

/// OpenGL error check.
static void gl_error_check()
{
    auto err = glGetError();
    if (err != GL_NO_ERROR)
    {
        std::ostringstream sstr;
        sstr << "OpenGL error: " << err;
        BOOST_THROW_EXCEPTION(std::runtime_error(sstr.str()));
    }
}

#endif

//######################################
// Random ##############################
//######################################

/// Seed for glibc random
static int32_t g_lgc_rand_seed = 0/*1559687923*//*123456789*/;

/// Simple rand() to avoid loading libc.
///
/// This function is essentially the suggested C17 LGC random.
///
/// Linear Congruential Generator. See:
/// https://en.wikipedia.org/wiki/Linear_congruential_generator
///
/// \return next random number.
static int16_t lgc_rand()
{
    // Use constants from some old glibc implementation.
    const int32_t LGC_A = 1103515245;
    const int32_t LGC_C = 12345;

    g_lgc_rand_seed = LGC_A * g_lgc_rand_seed + LGC_C;

    return static_cast<int16_t>(g_lgc_rand_seed >> 16);
}

#if defined(USE_LD)

/// Set the random seed.
///
/// \param seed New random seed.
static void lgc_srand(int32_t seed)
{
    g_lgc_rand_seed = seed;
}

/// Random float value.
///
/// \param op Given maximum value.
/// \return Random value between 0 and given value.
static float frand(float op)
{
    return static_cast<float>(lgc_rand() & 0xFFFF) * ((1.0f / 65535.0f) * op);
}

/// Generate a ZXY rotation matrix.
///
/// \param rx Rotation x.
/// \param ry Rotation y.
/// \param rz Rotation z.
/// \param out_matrix Matrix to write into.
static void generate_rotation_matrix_zxy(float rx, float ry, float rz, float *out_matrix)
{
    float sx = sinf(rx);
    float sy = sinf(ry);
    float sz = sinf(rz);
    float cx = cosf(rx);
    float cy = cosf(ry);
    float cz = cosf(rz);

    out_matrix[0] = sx * sy * sz + cy * cz;
    out_matrix[1] = sz * cx;
    out_matrix[2] = sx * sz * cy - sy * cz;
    out_matrix[3] = sx * sy * cz - sz * cy;
    out_matrix[4] = cx * cz;
    out_matrix[5] = sx * cy * cz + sy * sz;
    out_matrix[6] = sy * cx;
    out_matrix[7] = -sx;
    out_matrix[8] = cx * cy;
}

#endif

//######################################
// Music ###############################
//######################################

/// \brief Update audio stream.
///
/// \param userdata Not used.
/// \param stream Target stream.
/// \param len Number of bytes to write.
static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;

    for(int ii = 0; (ii < len); ++ii)
    {
        stream[ii] = g_audio_buffer[g_audio_position + ii];
    }
    g_audio_position += len;
}

/// SDL audio specification struct.
static SDL_AudioSpec audio_spec =
{
    AUDIO_SAMPLERATE,
    AUDIO_SAMPLE_TYPE_SDL,
    AUDIO_CHANNELS,
    0,
#if defined(USE_LD)
    4096,
#else
    256, // ~172.3Hz, lower values seem to cause underruns
#endif
    0,
    0,
    audio_callback,
    NULL
};

#if defined(USE_LD) || (defined(DISABLE_SYNTH) && !defined(ENABLE_BYTEBEAT))

/// Silence generator stub.
/// \param output Output buffer to write to.
/// \param samples Number of bytes to write.
static void silence_func(uint8_t* output, unsigned samples)
{
    memset(output, 0, samples * sizeof(sample_t));
}

#endif

#if defined(USE_LD) || (defined(DISABLE_SYNTH) && defined(ENABLE_BYTEBEAT))

/// Bytebeat generator stub.
/// \param output Output buffer to write to.
/// \param samples Number of bytes to write.
static void bytebeat_func(uint8_t* output, unsigned samples)
{
    // Example by "bst", taken from "Music from very short programs - the 3rd iteration" by viznut.
    for(unsigned ii = 0; ((samples / AUDIO_CHANNELS / 10) > ii); ++ii)
    {
        uint8_t sample = static_cast<uint8_t>(
                static_cast<unsigned>(static_cast<int>(ii / 70000000 * ii * ii + ii) % 127) |
                ii >> 4 | ii >> 5 | (ii % 127 + (ii >> 17)) | ii
                );
        float flt_sample = (static_cast<float>(sample) / 127.5f) - 1.0f;

        float* flt_audio_buffer = reinterpret_cast<float*>(output);
        flt_audio_buffer[(ii * 10) + 0] = flt_sample;
        flt_audio_buffer[(ii * 10) + 1] = flt_sample;
        flt_audio_buffer[(ii * 10) + 2] = flt_sample;
        flt_audio_buffer[(ii * 10) + 3] = flt_sample;
        flt_audio_buffer[(ii * 10) + 4] = flt_sample;
        flt_audio_buffer[(ii * 10) + 5] = flt_sample;
        flt_audio_buffer[(ii * 10) + 6] = flt_sample;
        flt_audio_buffer[(ii * 10) + 7] = flt_sample;
        flt_audio_buffer[(ii * 10) + 8] = flt_sample;
        flt_audio_buffer[(ii * 10) + 9] = flt_sample;
    }
}

#endif

/// Synth wrapper.
static void synth_func()
{
#if defined(USE_LD) || defined(DISABLE_SYNTH)
    unsigned samples = INTRO_LENGTH / sizeof(sample_t);
#endif

#if defined(USE_LD)
    // Fill with 0 if forced silence.
    if(g_flag_silent)
    {
        silence_func(g_audio_buffer, samples);
        return;
    }

    Uint8 *wav_buffer = nullptr;
    Uint32 wav_length = 0;
    if(!g_synth_test_input_file.empty())
    {
        const fs::path REL_PATH("rel");
        const fs::path SYNTH_TEST_INPUT_FILE_PATH(std::string(g_synth_test_input_file).c_str());
        SDL_AudioSpec wav_spec;
        SDL_AudioSpec* wav_found = nullptr;

        wav_found = SDL_LoadWAV(SYNTH_TEST_INPUT_FILE_PATH.string().c_str(), &wav_spec, &wav_buffer, &wav_length);
        if(!wav_found)
        {
            fs::path fname = REL_PATH / SYNTH_TEST_INPUT_FILE_PATH;
            wav_found = SDL_LoadWAV(fname.string().c_str(), &wav_spec, &wav_buffer, &wav_length);
        }
        if(!wav_found)
        {
            fs::path fname = fs::path("..") / REL_PATH / SYNTH_TEST_INPUT_FILE_PATH;
            wav_found = SDL_LoadWAV(fname.string().c_str(), &wav_spec, &wav_buffer, &wav_length);
        }
    }

    // If successful, read floating-point data from file.
    if(wav_length > 0)
    {
        size_t copy_length = std::min(static_cast<size_t>(wav_length), static_cast<size_t>(AUDIO_BUFFER_LENGTH));
        memcpy(g_audio_buffer, wav_buffer, copy_length);
        SDL_FreeWAV(wav_buffer);
    }
    // No no audio data available, fill data with bytebeat.
    else
    {
        std::cerr << "WARNING: synth not available, filling audio with bytebeat" << std::endl;
        bytebeat_func(g_audio_buffer, samples);
    }
#else

#if defined(DISABLE_SYNTH)
#if defined(ENABLE_BYTEBEAT)
    bytebeat_func(g_audio_buffer, samples);
#else
    silence_func(g_audio_buffer, samples);
#endif
#else
    synth(reinterpret_cast<float*>(g_scratch_buffer));
#endif

#if defined(SYNTH_TEST_OUTPUT_FILE)
    // Write raw .wav file if necessary.
    {
        SF_INFO info;
        info.samplerate = AUDIO_SAMPLERATE;
        info.channels = AUDIO_CHANNELS;
        info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

        SNDFILE *outfile = dnload_sf_open(SYNTH_TEST_OUTPUT_FILE, SFM_WRITE, &info);
        sf_count_t write_count = INTRO_LENGTH / AUDIO_CHANNELS / AUDIO_SAMPLE_SIZE;
        dnload_sf_writef_float(outfile, reinterpret_cast<float*>(g_audio_buffer), write_count);
        dnload_sf_close(outfile);
    }
#endif
#endif
}

#if defined(USE_LD)

/// Set the audio position, avoiding misalignment.
///
/// \param ticks Audio position ticks.
void set_audio_position(int ticks)
{
    g_audio_position = ticks - (ticks % static_cast<int>(sizeof(sample_t) * AUDIO_CHANNELS));
}

/// Get the relative fake "start time" from current time and audio position.
///
/// \retuyrn Start ticks.
int get_start_ticks()
{
    auto current_ticks = SDL_GetTicks();
    double flt_position = static_cast<double>(g_audio_position);
    return static_cast<int>(current_ticks -
            static_cast<uint32_t>((flt_position * 1000.0) / static_cast<double>(AUDIO_BYTERATE)));
}

#endif

//######################################
// Shaders #############################
//######################################

#include "forward.mesh.glsl.hpp"
#include "forward.frag.glsl.hpp"

/// GL info arrays.
/// 1 Texture.
/// 1 Program.
static GLuint g_gl_array[2];

/// Noise texture.
static GLuint& g_texture_noise = g_gl_array[0];

/// Forward program.
static GLuint& g_program_forward = g_gl_array[1];

/// Fixed uniform location.
static const GLint g_uniform_noise = 0;

/// Fixed uniform location.
static const GLint g_uniform_time = 1;

#if defined(USE_LD)

/// Noise texture index.
static const int g_texture_index_noise = 0;

/// Fixed uniform location.
static const GLint g_uniform_params = 2;

#endif

#if !defined(USE_LD)

/// \brief Create shader.
///
/// \param source Source of the shader.
/// \return Shader ID.
static GLuint create_shader(const char* source, GLenum type)
{
    GLuint ret = dnload_glCreateShader(type);

    dnload_glShaderSource(ret, 1, &source, NULL);

    dnload_glCompileShader(ret);

#if defined(DISABLE_SYNTH)
    GLint err;
    dnload_glGetShaderiv(ret, GL_COMPILE_STATUS, &err);
    if (err == GL_FALSE)
    {
        static GLchar log[4096];
        dnload_glGetShaderInfoLog(ret, 4096, nullptr, log);
        dnload_puts(log);
    }
#endif

    return ret;
}

/// \brief Create program.
///
/// \param vertex Vertex shader.
/// \param fragment Fragment shader.
/// \return Program ID
static GLuint create_program(const char* mesh, const char* fragment)
{
    GLuint ret = dnload_glCreateProgram();

    dnload_glAttachShader(ret, create_shader(mesh, GL_MESH_SHADER_NV));
    dnload_glAttachShader(ret, create_shader(fragment, GL_FRAGMENT_SHADER));

    dnload_glLinkProgram(ret);

    return ret;
}

#endif

//######################################
// Uniform data ########################
//######################################

#if defined(USE_LD)

/// \brief Parameter uniforms.
///
/// 0: X position.
/// 1: Y position.
/// 2: Z position.
/// 3: X forward.
/// 4: Y forward.
/// 5: Z forward.
/// 6: X up.
/// 7: Y up.
/// 8: Z up.
/// 9: Playback mode on/off.
/// 10: Unused.
/// 11: Unused.
/// 12-20: Noise transformation matrix.
static float g_params[] =
{
    -11.9f, -13.4f, -2.9f,
    0.0f, 0.0f, -1.0f,
    0.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    // Matrix.
    -0.8f, -0.6f, -0.01f, -0.59f, 0.78f, -0.20f, 0.13f, -0.16f, -0.98f,
};

/// Control position X.
static float& g_pos_x = g_params[0];
/// Control position Y.
static float& g_pos_y = g_params[1];
/// Control position Z.
static float& g_pos_z = g_params[2];

/// Control forward X.
static float& g_fw_x = g_params[3];
/// Control forward Y.
static float& g_fw_y = g_params[4];
/// Control forward Z.
static float& g_fw_z = g_params[5];

/// Control up X.
static float& g_up_x = g_params[6];
/// Control up Y.
static float& g_up_y = g_params[7];
/// Control up Z.
static float& g_up_z = g_params[8];

/// Playback mode.
static float& g_playback_mode = g_params[9];

/// Offset to matrix.
static float* g_uniform_matrix = &(g_params[12]);

#endif

/// \brief Draw the world.
///
/// \param ticks Tick count.
/// \param aspec Screen aspect.
static void draw(int ticks)
{
    dnload_glClear(GL_COLOR_BUFFER_BIT);
    dnload_glUniform1i(g_uniform_noise, 0);
    dnload_glUniform1i(g_uniform_time, ticks);
#if defined(USE_LD)
    glUniform3fv(g_uniform_params, static_cast<GLsizei>(sizeof(g_params) / 3 / sizeof(float)), g_params);
#endif
    dnload_glDrawMeshTasksNV(0, RESOLUTION_X * RESOLUTION_Y);

#if defined(USE_LD)
    gl_error_check();
#endif
}

//######################################
// Utility #############################
//######################################

#if defined(USE_LD)

/// Parse resolution from string input.
///
/// \param op Resolution string.
/// \return Tuple of width and height.
std::pair<unsigned, unsigned> parse_resolution(const std::string &op)
{
    size_t cx = op.find("x");

    if(std::string::npos == cx)
    {
        cx = op.rfind("p");

        if((std::string::npos == cx) || (0 >= cx))
        {
            std::ostringstream sstr;
            sstr << "invalid resolution string '" << op << '\'';
            BOOST_THROW_EXCEPTION(std::runtime_error(sstr.str()));
        }

        std::string sh = op.substr(0, cx);

        unsigned rh = boost::lexical_cast<unsigned>(sh);
        unsigned rw = (rh * 16) / 9;
        unsigned rem4 = rw % 4;

        return std::make_pair(rw - rem4, rh);
    }

    std::string sw = op.substr(0, cx);
    std::string sh = op.substr(cx + 1);

    return std::make_pair(boost::lexical_cast<int>(sw), boost::lexical_cast<int>(sh));
}

/// \brief Audio writing callback.
///
/// \param data Raw audio data.
/// \param size Audio data size (in samples).
void write_audio(std::string_view basename, void *data, size_t size)
{
    std::string filename = std::string(basename) + ".raw";
    FILE *fd = fopen(filename.c_str(), "wb");
    if(!fd)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("could not open '" + filename + "' for writing"));
    }
    fwrite(data, size, 1, fd);
    fclose(fd);
}

/// \brief Image writing callback.
///
/// \param basename Base name of the image.
/// \param screen_w Screen width.
/// \param screen_h Screen height.
/// \param idx Frame index to write.
void write_frame(std::string_view basename, unsigned screen_w, unsigned screen_h, unsigned idx)
{
    std::unique_ptr<uint8_t[]> image(new uint8_t[screen_w * screen_h * 3]);
    std::ostringstream sstr;

    glReadPixels(0, 0, static_cast<GLsizei>(screen_w), static_cast<GLsizei>(screen_h), GL_RGB, GL_UNSIGNED_BYTE,
            image.get());

    sstr << basename << "_" << std::setfill('0') << std::setw(4) << idx << ".png";

    gfx::image_png_save(sstr.str(), screen_w, screen_h, 24, image.get());
    return;
}

#endif

//######################################
// intro / _start ######################
//######################################

/// \cond
#if defined(DNLOAD_VIDEOCORE)
#define DEFAULT_SDL_WINDOW_FLAGS SDL_WINDOW_BORDERLESS
#else
#define DEFAULT_SDL_WINDOW_FLAGS SDL_WINDOW_OPENGL
#endif
/// \endcond

#if defined(USE_LD)
/// \brief Intro body function.
///
/// \param screen_w Screen width.
/// \param screen_h Screen height.
/// \param flag_fullscreen Fullscreen toggle.
void intro(unsigned screen_w, unsigned screen_h, bool flag_fullscreen)
#else
#define screen_w static_cast<unsigned>(SCREEN_W)
#define screen_h static_cast<unsigned>(SCREEN_H)
#define flag_fullscreen static_cast<bool>(SCREEN_F)
void _start()
#endif
{
    dnload();
    dnload_SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    dnload_SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    dnload_SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    dnload_SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    g_sdl_window = dnload_SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            static_cast<int>(screen_w), static_cast<int>(screen_h),
            DEFAULT_SDL_WINDOW_FLAGS | (flag_fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
    dnload_SDL_GL_CreateContext(g_sdl_window);
    dnload_SDL_ShowCursor(g_flag_developer);

#if defined(USE_LD)
    {
        GLenum err = glewInit();
        if(GLEW_OK != err)
        {
            std::cerr << "glewInit(): " << glewGetErrorString(err) << std::endl;
            teardown();
            exit(1);
        }
    }
#endif

    // Generate textures (until start of pipelines).
    dnload_glGenTextures(1, g_gl_array);

    // Noise volumetric texture.
    {
        uint16_t* noise_array = reinterpret_cast<uint16_t*>(g_audio_buffer);

        for(size_t ii = 0; (ii < NOISE_SIZE); ++ii)
        {
            noise_array[ii] = static_cast<uint16_t>(lgc_rand());
        }

#if defined(USE_LD)
        glActiveTexture(GL_TEXTURE0 + g_texture_index_noise);
#endif
        dnload_glBindTexture(GL_TEXTURE_3D, g_texture_noise);
        dnload_glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16_SNORM, VOLUME_SIDE, VOLUME_SIDE, VOLUME_SIDE, 0, GL_RGB,
                GL_SHORT, noise_array);
        dnload_glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#if defined(USE_LD)
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
        dnload_glGenerateMipmap(GL_TEXTURE_3D);
    }

    dnload_glEnable(GL_BLEND);
    dnload_glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

#if defined(USE_LD)
#if 0
    GlslPipeline program;
    program.addShader(GL_MESH_SHADER_NV, g_shader_mesh_quad);
    program.addShader(GL_FRAGMENT_SHADER, g_shader_fragment_quad);
    if(!program.link())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("pipeline creation failure"));
    }
    glBindProgramPipeline(program.getId());
    g_program_fragment = program.getProgramId(GL_FRAGMENT_SHADER);
    gl_error_check();
#else
    GlslProgram program;
    program.addShader(GL_MESH_SHADER_NV, g_shader_mesh_forward);
    program.addShader(GL_FRAGMENT_SHADER, g_shader_fragment_forward);
    if(!program.link())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("program creation failure"));
    }
    g_program_forward = program.getId();
#endif
#else
    g_program_forward = create_program(g_shader_mesh_forward, g_shader_fragment_forward);
#endif
    dnload_glUseProgram(g_program_forward);

    // Generate audio.
    synth_func();

#if defined(USE_LD)
    if(g_flag_record)
    {
        SDL_Event event;
        unsigned frame_idx = 0;

        // audio
        SDL_PauseAudio(1);

        write_audio("intro", g_audio_buffer, INTRO_LENGTH);

        // video
        for(;;)
        {
            int ticks = static_cast<int>(static_cast<float>(frame_idx) / 60.0f * static_cast<float>(AUDIO_BYTERATE));

            if(ticks > INTRO_LENGTH)
            {
                break;
            }

            if(SDL_PollEvent(&event) && (event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_ESCAPE))
            {
                break;
            }

            draw(ticks);
            write_frame("intro", screen_w, screen_h, frame_idx);
            swap_buffers();
            ++frame_idx;
        }

        teardown();
        return;
    }
#endif

    dnload_SDL_OpenAudio(&audio_spec, NULL);
#if defined(USE_LD)
    if(!g_flag_developer)
#endif
    {
        dnload_SDL_PauseAudio(0);
    }

#if defined(USE_LD)
    FpsCounter fps_counter;
    int start_ticks = static_cast<int>(SDL_GetTicks());
    int last_ticks = start_ticks;
    int curr_ticks = 0;
#endif

    for(;;)
    {
#if defined(USE_LD)
        static uint8_t mouse_look = 0;
        static int8_t move_backward = 0;
        static int8_t move_down = 0;
        static int8_t move_forward = 0;
        static int8_t move_left = 0;
        static int8_t move_right = 0;
        static int8_t move_up = 0;
        static int8_t time_delta = 0;
        static bool fast = false;
        int time_add_seconds = 0;
        int mouse_look_x = 0;
        int mouse_look_y = 0;
        bool quit = false;
#endif
        SDL_Event event;

#if defined(USE_LD)
        while(SDL_PollEvent(&event))
        {
            if(SDL_QUIT == event.type)
            {
                quit = true;
            }
            else if(SDL_KEYDOWN == event.type)
            {
                switch(event.key.keysym.sym)
                {
                case SDLK_a:
                    move_left = 1;
                    break;

                case SDLK_d:
                    move_right = 1;
                    break;

                case SDLK_e:
                    move_up = 1;
                    break;

                case SDLK_q:
                    move_down = 1;
                    break;

                case SDLK_s:
                    move_backward = 1;
                    break;

                case SDLK_w:
                    move_forward = 1;
                    break;

                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    fast = true;
                    break;

                case SDLK_LALT:
                    SDL_PauseAudio(1);
                    time_delta = -1;
                    break;

                case SDLK_MODE:
                case SDLK_RALT:
                    SDL_PauseAudio(1);
                    time_delta = 1;
                    break;

                case SDLK_LEFT:
                    time_add_seconds = -5;
                    break;

                case SDLK_RIGHT:
                    time_add_seconds = 5;
                    break;

                case SDLK_UP:
                    time_add_seconds = 30;
                    break;

                case SDLK_DOWN:
                    time_add_seconds = -30;
                    break;

                case SDLK_F5:
                    if(!program.link())
                    {
                        BOOST_THROW_EXCEPTION(std::runtime_error("program recreation failure"));
                    }
                    g_program_forward = program.getId();
                    glUseProgram(g_program_forward);
                    break;

                case SDLK_RETURN:
                    g_playback_mode = 1.0;
                    break;

                case SDLK_p:
                    {
                        SDL_AudioStatus audio_status = SDL_GetAudioStatus();
                        if(audio_status == SDL_AUDIO_PLAYING)
                        {
                            SDL_PauseAudio(1);
                        }
                        else
                        {
                            set_audio_position(g_audio_position);
                            start_ticks = get_start_ticks();
                            SDL_PauseAudio(0);
                            g_playback_mode = 1.0;
                        }
                    }
                    break;

                case SDLK_SPACE:
                    {
                        // Not exactly 1/4 seconds but compresses nicely.
                        unsigned sec_4 = static_cast<unsigned>(static_cast<double>(curr_ticks) / 88000.0);
                        SDL_PauseAudio(1);
                        g_playback_mode = 0.0;
                        printf("vec3 pos = vec3(%1.2f, %1.2f, %1.2f);\n"
                                "vec3 fw  = vec3(%1.2f, %1.2f, %1.2f);\n"
                                "vec3 up  = vec3(%1.2f, %1.2f, %1.2f);\n"
                                "time     = %u // %u // %u\n",
                                g_pos_x, g_pos_y, g_pos_z,
                                g_fw_x, g_fw_y, g_fw_z,
                                g_up_x, g_up_y, g_up_z,
                                curr_ticks >> 9, sec_4, curr_ticks / AUDIO_BYTERATE);
                    }
                    break;

                case SDLK_INSERT:
                    {
                        lgc_srand(static_cast<int32_t>(time(0)) ^ static_cast<int32_t>(SDL_GetTicks()));

                        float rx = frand(static_cast<float>(M_PI * 2.0));
                        float ry = frand(static_cast<float>(M_PI * 2.0));
                        float rz = frand(static_cast<float>(M_PI * 2.0));

                        generate_rotation_matrix_zxy(rx, ry, rz, g_uniform_matrix);

                        printf("mat3 rot = mat3(%1.2f, %1.2f, %1.2f, %1.2f, %1.2f, %1.2f, %1.2f, %1.2f, %1.2f); // %1.2f ; %1.2f ; %1.2f\n",
                                g_uniform_matrix[0], g_uniform_matrix[3], g_uniform_matrix[6],
                                g_uniform_matrix[1], g_uniform_matrix[4], g_uniform_matrix[7],
                                g_uniform_matrix[2], g_uniform_matrix[5], g_uniform_matrix[8],
                                rx, ry, rz);
                    }
                    break;

                case SDLK_ESCAPE:
                    quit = true;
                    break;

                default:
                    break;
                }
            }
            else if(SDL_KEYUP == event.type)
            {
                switch(event.key.keysym.sym)
                {
                case SDLK_a:
                    move_left = 0;
                    break;

                case SDLK_d:
                    move_right = 0;
                    break;

                case SDLK_e:
                    move_up = 0;
                    break;

                case SDLK_q:
                    move_down = 0;
                    break;

                case SDLK_s:
                    move_backward = 0;
                    break;

                case SDLK_w:
                    move_forward = 0;
                    break;

                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    fast = false;
                    break;

                case SDLK_MODE:
                case SDLK_LALT:
                case SDLK_RALT:
                    time_delta = 0;
                    break;

                default:
                    break;
                }
            }
            else if(SDL_MOUSEBUTTONDOWN == event.type)
            {
                if(1 == event.button.button)
                {
                    SDL_PauseAudio(1);
                    g_playback_mode = 0.0;
                    mouse_look = 1;
                }
            }
            else if(SDL_MOUSEBUTTONUP == event.type)
            {
                if(1 == event.button.button)
                {
                    mouse_look = 0;
                }
            }
            else if(SDL_MOUSEMOTION == event.type)
            {
                if(0 != mouse_look)
                {
                    mouse_look_x += event.motion.xrel;
                    mouse_look_y += event.motion.yrel;
                }
            }
        }

        curr_ticks = static_cast<int>(SDL_GetTicks());
        auto print_fps = fps_counter.appendTimestamp(static_cast<unsigned>(curr_ticks));
        if(print_fps)
        {
            printf("FPS: %1.1f\n", *print_fps);
        }

        double tick_diff = static_cast<double>(curr_ticks - last_ticks) / 1000.0 * static_cast<double>(AUDIO_BYTERATE);

        if(g_flag_developer)
        {
            float move_speed = (fast ? 0.0001f : 0.00003f) * static_cast<float>(tick_diff);
            float uplen = sqrtf(g_up_x * g_up_x + g_up_y * g_up_y + g_up_z * g_up_z);
            float fwlen = sqrtf(g_fw_x * g_fw_x + g_fw_y * g_fw_y + g_fw_z * g_fw_z);
            float rt_x;
            float rt_y;
            float rt_z;
            float movement_rt = static_cast<float>(move_right - move_left) * move_speed;
            float movement_up = static_cast<float>(move_up - move_down) * move_speed;
            float movement_fw = static_cast<float>(move_forward - move_backward) * move_speed;

            g_up_x /= uplen;
            g_up_y /= uplen;
            g_up_z /= uplen;

            g_fw_x /= fwlen;
            g_fw_y /= fwlen;
            g_fw_z /= fwlen;

            rt_x = g_fw_y * g_up_z - g_fw_z * g_up_y;
            rt_y = g_fw_z * g_up_x - g_fw_x * g_up_z;
            rt_z = g_fw_x * g_up_y - g_fw_y * g_up_x;

            if(0 != mouse_look_x)
            {
                float angle = static_cast<float>(mouse_look_x) / static_cast<float>(screen_h / 4) * 0.25f;
                float ca = cosf(angle);
                float sa = sinf(angle);
                float new_rt_x = ca * rt_x + sa * g_fw_x;
                float new_rt_y = ca * rt_y + sa * g_fw_y;
                float new_rt_z = ca * rt_z + sa * g_fw_z;
                float new_fw_x = ca * g_fw_x - sa * rt_x;
                float new_fw_y = ca * g_fw_y - sa * rt_y;
                float new_fw_z = ca * g_fw_z - sa * rt_z;

                rt_x = new_rt_x;
                rt_y = new_rt_y;
                rt_z = new_rt_z;
                g_fw_x = new_fw_x;
                g_fw_y = new_fw_y;
                g_fw_z = new_fw_z;
            }
            if(0 != mouse_look_y)
            {
                float angle = static_cast<float>(mouse_look_y) / static_cast<float>(screen_h / 4) * 0.25f;
                float ca = cosf(angle);
                float sa = sinf(angle);
                float new_fw_x = ca * g_fw_x + sa * g_up_x;
                float new_fw_y = ca * g_fw_y + sa * g_up_y;
                float new_fw_z = ca * g_fw_z + sa * g_up_z;
                float new_up_x = ca * g_up_x - sa * g_fw_x;
                float new_up_y = ca * g_up_y - sa * g_fw_y;
                float new_up_z = ca * g_up_z - sa * g_fw_z;

                g_fw_x = new_fw_x;
                g_fw_y = new_fw_y;
                g_fw_z = new_fw_z;
                g_up_x = new_up_x;
                g_up_y = new_up_y;
                g_up_z = new_up_z;
            }

            g_pos_x += movement_rt * rt_x + movement_up * g_up_x + movement_fw * g_fw_x;
            g_pos_y += movement_rt * rt_y + movement_up * g_up_y + movement_fw * g_fw_y;
            g_pos_z += movement_rt * rt_z + movement_up * g_up_z + movement_fw * g_fw_z;
        }

        SDL_AudioStatus audio_status = SDL_GetAudioStatus();
        if(audio_status == SDL_AUDIO_PLAYING)
        {
            if(time_add_seconds)
            {
                SDL_PauseAudio(1);
                int apos = std::max(std::min(g_audio_position + (AUDIO_BYTERATE * time_add_seconds), INTRO_LENGTH), 0);
                set_audio_position(apos);
                start_ticks = get_start_ticks();
                curr_ticks = start_ticks;
                SDL_PauseAudio(0);
            }
            else
            {
                double seconds_elapsed = static_cast<double>(SDL_GetTicks() - static_cast<unsigned>(start_ticks)) / 1000.0;
                curr_ticks = static_cast<int>(seconds_elapsed * static_cast<double>(AUDIO_BYTERATE)) + INTRO_START;
            }
        }
        else
        {
            int dtime = static_cast<int>(time_delta * (fast ? 5.0f : 1.0f) * tick_diff) + time_add_seconds * AUDIO_BYTERATE;
            g_audio_position = std::max(std::min(g_audio_position + dtime, INTRO_LENGTH), 0);
            last_ticks = curr_ticks;
            curr_ticks = g_audio_position;
        }

        if((curr_ticks > static_cast<int>(INTRO_LENGTH)) || quit)
        {
            break;
        }
#else
        auto curr_ticks = g_audio_position;

        dnload_SDL_PollEvent(&event);

        if((curr_ticks >= INTRO_LENGTH) || (event.type == SDL_KEYDOWN))
        {
            break;
        }
#endif

        draw(curr_ticks);
        swap_buffers();
    }

    teardown();
#if !defined(USE_LD)
    asm_exit();
#endif
}

//######################################
// Main ################################
//######################################

#if defined(USE_LD)
/// Main function.
///
/// \param argc Argument count.
/// \param argv Arguments.
/// \return Program return code.
int DNLOAD_MAIN(int argc, char **argv)
{
    unsigned screen_w = SCREEN_W;
    unsigned screen_h = SCREEN_H;
    bool flag_fullscreen = false;
    bool flag_window = false;

    try
    {
        if(argc > 0)
        {
            po::options_description desc("Options");
            desc.add_options()
                ("developer,d", "Developer mode.")
                ("fullscreen,f", "Start in fullscreen mode as opposed to windowed mode.")
                ("help,h", "Print help text.")
                ("record,R", "Do not play intro normally, instead save audio as .wav and frames as .png -files.")
                ("resolution,r", po::value<std::string>(), "Resolution to use, specify as 'WIDTHxHEIGHT' or 'HEIGHTp'.")
                ("silent,s", "Don't play any sound, just silence.")
                ("window,w", "Start in windowed mode as opposed to fullscreen mode.");

            po::variables_map vmap;
            po::store(po::command_line_parser(argc, argv).options(desc).run(), vmap);
            po::notify(vmap);

            if(vmap.count("developer"))
            {
                g_flag_developer = true;
            }
            if(vmap.count("fullscreen"))
            {
                flag_window = true;
            }
            if(vmap.count("help"))
            {
                std::cout << g_usage << desc << std::endl;
                return 0;
            }
            if(vmap.count("record"))
            {
                g_flag_record = true;
            }
            if(vmap.count("resolution"))
            {
                std::pair<unsigned, unsigned> resolution = parse_resolution(vmap["resolution"].as<std::string>());
                screen_w = resolution.first;
                screen_h = resolution.second;
            }
            if(vmap.count("silent"))
            {
                g_flag_silent = true;
            }
            if(vmap.count("window"))
            {
                flag_window = true;
            }
        }

        if(flag_fullscreen && flag_window)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("both fullscreen and windowed mode specified"));
        }
        bool fullscreen = flag_fullscreen || (!flag_window && !g_flag_developer && !g_flag_record);

        intro(screen_w, screen_h, fullscreen);
    }
    catch(const boost::exception &err)
    {
        std::cerr << boost::diagnostic_information(err);
        return 1;
    }
    catch(const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << __FILE__ << ": unknown exception caught\n";
        return -1;
    }
    return 0;
}
#endif

//######################################
// End #################################
//######################################

