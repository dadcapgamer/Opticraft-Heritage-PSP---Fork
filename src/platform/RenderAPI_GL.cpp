#include "platform/RenderAPI.h"
#include "platform/Profiler.h"
#include "platform/Log.h"


#if defined(PLATFORM_PSP)
#include <GL/gl.h>
#include <GL/glu.h>
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#else
#include <glad/glad.h>
#endif
#include <SDL.h>
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#include <cstdint>
#include <cstdio>
#include <cstring>

#if PLATFORM_PSP
namespace
{
void checkPspRenderStateError(int operation, int value, const char* name)
{
    const unsigned int error = glGetError();
    if (error != 0)
    {
        platformProfilePspRenderStateError(operation, value, error);
        static bool reported = false;
        if (!reported)
        {
            reported = true;
            MC_LOG_ERROR("render", "PSP render state error op=%d value=%d name=%s code=0x%x\n",
                         operation, value, name, error);
        }
    }
}
}
#endif


#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif

#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL 0x813C
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 0x803A
#endif

#ifndef GL_SAMPLES_PASSED_ARB
#define GL_SAMPLES_PASSED_ARB 0x8914
#endif
#ifndef GL_QUERY_RESULT_ARB
#define GL_QUERY_RESULT_ARB 0x8866
#endif
#ifndef GL_QUERY_RESULT_AVAILABLE_ARB
#define GL_QUERY_RESULT_AVAILABLE_ARB 0x8867
#endif

static bool desktopHasOpenGL12()
{
#if PLATFORM_PSP
    return false;
#else
    const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    if (version == nullptr)
        return false;
    int major = 0;
    int minor = 0;
    if (std::sscanf(version, "%d.%d", &major, &minor) != 2)
        return false;
    return major > 1 || (major == 1 && minor >= 2);
#endif
}

#if PLATFORM_PSP
bool renderDrawCaptured(const RenderCapturedMesh& mesh);
bool renderCaptureInterleaved(const RenderInterleavedMesh& mesh, RenderCapturedMesh& out, bool append = false);

enum class PspDisplayCmdType : std::uint8_t
{
    DrawCaptured,
    PushMatrix,
    PopMatrix,
    Translate,
    Scale,
    Rotate,
    Color4f,
    BindTexture,
    CallList
};

struct PspDisplayCmd
{
    PspDisplayCmdType type;
    union {
        struct { float x, y, z; } translate;
        struct { float x, y, z; } scale;
        struct { float angle, x, y, z; } rotate;
        struct { float r, g, b, a; } color;
        int integer;
        std::uint32_t meshIndex;
    } data;
};

struct PspDisplayList
{
    std::vector<PspDisplayCmd> commands;
    std::vector<RenderCapturedMesh> meshes;
    bool inUse = false;

    void clear()
    {
        commands.clear();
        meshes.clear();
        inUse = false;
    }
};

static constexpr int PSP_MAX_DISPLAY_LISTS = 1024;
static PspDisplayList s_pspLists[PSP_MAX_DISPLAY_LISTS];
static bool s_pspListsInitialized = false;
static int s_pspCurrentRecordingList = 0;

static void pspEnsureDisplayListPool()
{
    if (s_pspListsInitialized)
        return;
    for (int i = 0; i < PSP_MAX_DISPLAY_LISTS; ++i)
    {
        s_pspLists[i].commands.reserve(16);
        s_pspLists[i].meshes.reserve(2);
        s_pspLists[i].inUse = false;
    }
    s_pspListsInitialized = true;
}
#endif

// SDL_GL_GetProcAddress-based runtime loaders (desktop only).
static void *getProcEither(const char *arbName, const char *coreName)
{
	void *f = SDL_GL_GetProcAddress(arbName);
	if (!f) f = SDL_GL_GetProcAddress(coreName);
	return f;
}

static void activeTextureCompat(GLenum texture)
{
	using Function = void (APIENTRYP)(GLenum);
	static Function function = reinterpret_cast<Function>(getProcEither("glActiveTextureARB", "glActiveTexture"));
	if (function) function(texture);
}

static void clientActiveTextureCompat(GLenum texture)
{
	using Function = void (APIENTRYP)(GLenum);
	static Function function = reinterpret_cast<Function>(getProcEither("glClientActiveTextureARB", "glClientActiveTexture"));
	if (function) function(texture);
}

static void multiTexCoord2fCompat(GLenum texture, GLfloat u, GLfloat v)
{
	using Function = void (APIENTRYP)(GLenum, GLfloat, GLfloat);
	static Function function = reinterpret_cast<Function>(getProcEither("glMultiTexCoord2fARB", "glMultiTexCoord2f"));
	if (function) function(texture, u, v);
}

void glGenQueriesARB(GLsizei count, GLuint *queries)
{
	using Function = void (APIENTRYP)(GLsizei, GLuint *);
	static Function function = reinterpret_cast<Function>(getProcEither("glGenQueriesARB", "glGenQueries"));
	if (function) function(count, queries);
	else for (GLsizei i = 0; i < count; ++i) queries[i] = 0;
}

void glBeginQueryARB(GLenum target, GLuint query)
{
	using Function = void (APIENTRYP)(GLenum, GLuint);
	static Function function = reinterpret_cast<Function>(getProcEither("glBeginQueryARB", "glBeginQuery"));
	if (function) function(target, query);
}

void glEndQueryARB(GLenum target)
{
	using Function = void (APIENTRYP)(GLenum);
	static Function function = reinterpret_cast<Function>(getProcEither("glEndQueryARB", "glEndQuery"));
	if (function) function(target);
}

void glGetQueryObjectuivARB(GLuint query, GLenum parameter, GLuint *value)
{
	using Function = void (APIENTRYP)(GLuint, GLenum, GLuint *);
	static Function function = reinterpret_cast<Function>(getProcEither("glGetQueryObjectuivARB", "glGetQueryObjectuiv"));
	if (function) function(query, parameter, value);
	// Fail-OPEN: sin soporte de query -> disponible y "paso 1 sample" (visible),
	// nunca 0 (que marcaria todo como ocluido y borraria el mundo).
	else *value = 1u;
}


void renderEnable(RenderCapability capability)
{
    switch (capability)
    {
        case RenderCapability::Texture2D: glEnable(GL_TEXTURE_2D); break;
#if !PLATFORM_PSP
        case RenderCapability::ColorMaterial: glEnable(GL_COLOR_MATERIAL); break;
#else
        case RenderCapability::ColorMaterial: break;
#endif
        case RenderCapability::CullFace: glEnable(GL_CULL_FACE); break;
        case RenderCapability::AlphaTest: glEnable(GL_ALPHA_TEST); break;
        case RenderCapability::Blend: glEnable(GL_BLEND); break;
        case RenderCapability::DepthTest: glEnable(GL_DEPTH_TEST); break;
#if !PLATFORM_PSP
        case RenderCapability::Fog: glEnable(GL_FOG); break;
#else
        case RenderCapability::Fog: break;
#endif
#if !PLATFORM_PSP
        case RenderCapability::Lighting: glEnable(GL_LIGHTING); break;
#else
        case RenderCapability::Lighting: break;
#endif
        case RenderCapability::Normalize: glEnable(GL_NORMALIZE); break;
#if !PLATFORM_PSP
        case RenderCapability::RescaleNormal: glEnable(GL_RESCALE_NORMAL); break;
#else
        case RenderCapability::RescaleNormal: break;
#endif
#if !PLATFORM_PSP
        case RenderCapability::Light0: glEnable(GL_LIGHT0); break;
        case RenderCapability::Light1: glEnable(GL_LIGHT1); break;
#else
        case RenderCapability::Light0: break;
        case RenderCapability::Light1: break;
#endif
#if !PLATFORM_PSP
        case RenderCapability::PolygonOffsetFill: glEnable(GL_POLYGON_OFFSET_FILL); break;
#else
        case RenderCapability::PolygonOffsetFill: break;
#endif
    }
#if PLATFORM_PSP
    if (capability != RenderCapability::ColorMaterial &&
        capability != RenderCapability::Fog &&
        capability != RenderCapability::Lighting &&
        capability != RenderCapability::Light0 &&
        capability != RenderCapability::Light1 &&
        capability != RenderCapability::RescaleNormal &&
        capability != RenderCapability::PolygonOffsetFill)
        checkPspRenderStateError(1, static_cast<int>(capability), "enable");
#endif
}
void renderDisable(RenderCapability capability)
{
    switch (capability)
    {
        case RenderCapability::Texture2D: glDisable(GL_TEXTURE_2D); break;
#if !PLATFORM_PSP
        case RenderCapability::ColorMaterial: glDisable(GL_COLOR_MATERIAL); break;
#else
        case RenderCapability::ColorMaterial: break;
#endif
        case RenderCapability::CullFace: glDisable(GL_CULL_FACE); break;
        case RenderCapability::AlphaTest: glDisable(GL_ALPHA_TEST); break;
        case RenderCapability::Blend: glDisable(GL_BLEND); break;
        case RenderCapability::DepthTest: glDisable(GL_DEPTH_TEST); break;
#if !PLATFORM_PSP
        case RenderCapability::Fog: glDisable(GL_FOG); break;
#else
        case RenderCapability::Fog: break;
#endif
#if !PLATFORM_PSP
        case RenderCapability::Lighting: glDisable(GL_LIGHTING); break;
#else
        case RenderCapability::Lighting: break;
#endif
        case RenderCapability::Normalize: glDisable(GL_NORMALIZE); break;
#if !PLATFORM_PSP
        case RenderCapability::RescaleNormal: glDisable(GL_RESCALE_NORMAL); break;
#else
        case RenderCapability::RescaleNormal: break;
#endif
#if !PLATFORM_PSP
        case RenderCapability::Light0: glDisable(GL_LIGHT0); break;
        case RenderCapability::Light1: glDisable(GL_LIGHT1); break;
#else
        case RenderCapability::Light0: break;
        case RenderCapability::Light1: break;
#endif
#if !PLATFORM_PSP
        case RenderCapability::PolygonOffsetFill: glDisable(GL_POLYGON_OFFSET_FILL); break;
#else
        case RenderCapability::PolygonOffsetFill: break;
#endif
    }
#if PLATFORM_PSP
    if (capability != RenderCapability::ColorMaterial &&
        capability != RenderCapability::Fog &&
        capability != RenderCapability::Lighting &&
        capability != RenderCapability::Light0 &&
        capability != RenderCapability::Light1 &&
        capability != RenderCapability::RescaleNormal &&
        capability != RenderCapability::PolygonOffsetFill)
        checkPspRenderStateError(2, static_cast<int>(capability), "disable");
#endif
}

void renderBlendFunc(RenderBlendFactor source, RenderBlendFactor destination)
{
    auto glFactor = [](RenderBlendFactor factor) -> GLenum
    {
        switch (factor)
        {
            case RenderBlendFactor::Zero: return GL_ZERO;
            case RenderBlendFactor::One: return GL_ONE;
            case RenderBlendFactor::SrcColor: return GL_SRC_COLOR;
            case RenderBlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
            case RenderBlendFactor::SrcAlpha: return GL_SRC_ALPHA;
            case RenderBlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
            case RenderBlendFactor::DstAlpha: return GL_DST_ALPHA;
            case RenderBlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
            case RenderBlendFactor::DstColor: return GL_DST_COLOR;
            case RenderBlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
        }
        return GL_ONE;
    };
    glBlendFunc(glFactor(source), glFactor(destination));
#if PLATFORM_PSP
    const int blendValue =
        (static_cast<int>(source) << 4) | static_cast<int>(destination);
    checkPspRenderStateError(3, blendValue, "blendFunc");
#endif
}

void renderDepthMask(bool enabled)
{
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
#if PLATFORM_PSP
    checkPspRenderStateError(4, enabled ? 1 : 0, "depthMask");
#endif
}

void renderDepthFunc(RenderCompare function)
{
    auto glCompare = [](RenderCompare compare) -> GLenum
    {
        switch (compare)
        {
            case RenderCompare::Never: return GL_NEVER;
            case RenderCompare::Less: return GL_LESS;
            case RenderCompare::Equal: return GL_EQUAL;
            case RenderCompare::LessEqual: return GL_LEQUAL;
            case RenderCompare::Greater: return GL_GREATER;
            case RenderCompare::NotEqual: return GL_NOTEQUAL;
            case RenderCompare::GreaterEqual: return GL_GEQUAL;
            case RenderCompare::Always: return GL_ALWAYS;
        }
        return GL_ALWAYS;
    };
    glDepthFunc(glCompare(function));
}

void renderAlphaFunc(RenderCompare function, float reference)
{
    auto glCompare = [](RenderCompare compare) -> GLenum
    {
        switch (compare)
        {
            case RenderCompare::Never: return GL_NEVER;
            case RenderCompare::Less: return GL_LESS;
            case RenderCompare::Equal: return GL_EQUAL;
            case RenderCompare::LessEqual: return GL_LEQUAL;
            case RenderCompare::Greater: return GL_GREATER;
            case RenderCompare::NotEqual: return GL_NOTEQUAL;
            case RenderCompare::GreaterEqual: return GL_GEQUAL;
            case RenderCompare::Always: return GL_ALWAYS;
        }
        return GL_ALWAYS;
    };
    glAlphaFunc(glCompare(function), reference);
}

void renderCullFace(RenderFace face)
{
    switch (face)
    {
        case RenderFace::Front: glCullFace(GL_FRONT); break;
        case RenderFace::Back: glCullFace(GL_BACK); break;
        case RenderFace::FrontAndBack: glCullFace(GL_FRONT_AND_BACK); break;
    }
}

void renderColorMask(bool red, bool green, bool blue, bool alpha)
{
#if PLATFORM_PSP
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
    platformProfilePspColorMaskCall();
    return;
#else
    glColorMask(red ? GL_TRUE : GL_FALSE, green ? GL_TRUE : GL_FALSE, blue ? GL_TRUE : GL_FALSE, alpha ? GL_TRUE : GL_FALSE);
#endif
}

void renderBindTexture(int texture)
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::BindTexture;
        cmd.data.integer = texture;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return;
    }
#endif
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
}

void renderSetActiveTextureUnit(int textureUnit)
{
    activeTextureCompat(static_cast<GLenum>(textureUnit));
}

void renderSetClientActiveTextureUnit(int textureUnit)
{
    clientActiveTextureCompat(static_cast<GLenum>(textureUnit));
}

void renderSetMultiTextureCoord(int textureUnit, float u, float v)
{
    multiTexCoord2fCompat(static_cast<GLenum>(textureUnit), u, v);
}

void renderSetLightmapColors(const std::uint32_t* colors, int count)
{
    (void)colors;
    (void)count;
}

void renderColor4f(float r, float g, float b, float a)
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::Color4f;
        cmd.data.color.r = r;
        cmd.data.color.g = g;
        cmd.data.color.b = b;
        cmd.data.color.a = a;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return;
    }
#endif
    glColor4f(r, g, b, a);
}

void renderColor3f(float r, float g, float b)
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        renderColor4f(r, g, b, 1.0f);
        return;
    }
#endif
    glColor3f(r, g, b);
}

void renderNormal3f(float x, float y, float z)
{
    glNormal3f(x, y, z);
}

void renderGenerateTextures(int count, int *textures)
{
    if (count <= 0 || textures == nullptr)
        return;
    glGenTextures(static_cast<GLsizei>(count), reinterpret_cast<GLuint *>(textures));
}

void renderDeleteTextures(int count, const int *textures)
{
    if (count <= 0 || textures == nullptr)
        return;
    glDeleteTextures(static_cast<GLsizei>(count), reinterpret_cast<const GLuint *>(textures));
}

void renderTextureSubImageRgba(int level, int x, int y, int width, int height, const void *pixels)
{
    glTexSubImage2D(GL_TEXTURE_2D, level, x, y, width, height,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

// Specifies one mip level of the bound texture from tightly packed RGBA8.
//
// The desktop internal format stays GL_RGBA -- the third glTexImage2D argument
// is what decides the uploaded cost, and a desktop driver stores that as 32 bits
// per texel. The console backends pick their own storage (PS2: CT16, or PSMT8
// plus a CT16 CLUT when PS2_ENABLE_PSMT8 is on), which is why they take the
// pixels rather than a format enum.
void renderTextureImageRgba(int level, int width, int height, const void *pixels)
{
#if PLATFORM_PSP
    // PSP hardware strictly requires textures to be powers of two and at most 512x512.
    auto isPot = [](int x) { return x > 0 && (x & (x - 1)) == 0; };
    auto clampPot = [](int x) {
        int p = 1;
        while (p < x && p < 512) p <<= 1;
        if (p > 512) p = 512;
        return p;
    };

    if (width > 512 || height > 512 || !isPot(width) || !isPot(height))
    {
        const int targetWidth = clampPot(width);
        const int targetHeight = clampPot(height);
        std::vector<unsigned char> resampled(static_cast<std::size_t>(targetWidth * targetHeight * 4));
        const unsigned char *src = static_cast<const unsigned char *>(pixels);

        for (int y = 0; y < targetHeight; ++y)
        {
            const int sy = (y * height) / targetHeight;
            for (int x = 0; x < targetWidth; ++x)
            {
                const int sx = (x * width) / targetWidth;
                const std::size_t srcIdx = (static_cast<std::size_t>(sy) * width + sx) * 4u;
                const std::size_t dstIdx = (static_cast<std::size_t>(y) * targetWidth + x) * 4u;
                resampled[dstIdx + 0] = src[srcIdx + 0];
                resampled[dstIdx + 1] = src[srcIdx + 1];
                resampled[dstIdx + 2] = src[srcIdx + 2];
                resampled[dstIdx + 3] = src[srcIdx + 3];
            }
        }
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, targetWidth, targetHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, resampled.data());
        return;
    }
#endif
    glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

// Only the desktop backend maps these one-to-one onto texture state. The Wii
// takes all three in wii_native_texture_begin_upload, before any level exists,
// so there is nothing left to program by the time this is called; the PS2
// resolves wrap per primitive from the UV bounds instead of per texture. See
// ps2_texture_set_parameters for why that is the better answer there.
void renderTextureParameters(bool blur, bool mipmaps, bool clamp)
{
#if PLATFORM_PSP
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, blur ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, blur ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    blur ? GL_LINEAR : (mipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, blur ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP : GL_REPEAT);
#endif
}

void renderApplyTextureQuality(bool blur, int mipmapLevel, bool mipmapLinear, int anisotropy)
{
    GLenum minFilter = blur ? GL_LINEAR : GL_NEAREST;
    if (mipmapLevel > 0)
    {
        if (blur)
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
        else
            minFilter = mipmapLinear ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, blur ? GL_LINEAR : GL_NEAREST);
    // Only the selected chain is uploaded. Clamp sampling to that chain so a
    // texture does not become incomplete by implicitly requiring lower levels.
    if (desktopHasOpenGL12())
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmapLevel > 0 ? mipmapLevel : 0);
    }

    if (SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic") == SDL_TRUE)
    {
        GLfloat maximum = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximum);
        const GLfloat requested = static_cast<GLfloat>(anisotropy < 1 ? 1 : anisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        requested < maximum ? requested : maximum);
    }
}

int renderGetMaxAnisotropy()
{
    if (SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic") != SDL_TRUE)
        return 1;
    GLfloat maximum = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximum);
    return maximum > 1.0f ? static_cast<int>(maximum) : 1;
}

int renderGetMaxSamples()
{
    int buffers = 0;
    int samples = 0;
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples);
    return buffers > 0 && samples > 0 ? samples : 0;
}

bool renderTextureBeginUpload(int, int, int, int, bool, bool, bool, bool) { return true; }
bool renderTextureIsValid(int texture) { return texture > 0; }
void renderResetResources()
{
#if PLATFORM_PSP
    s_pspCurrentRecordingList = 0;
    for (int i = 0; i < PSP_MAX_DISPLAY_LISTS; ++i)
    {
        s_pspLists[i].clear();
    }
    s_pspCurrentRecordingList = 0;
#endif
}


void renderFogf(RenderFogParameter parameter, float value)
{
    GLenum glParameter = GL_FOG_DENSITY;
    switch (parameter)
    {
        case RenderFogParameter::Density: glParameter = GL_FOG_DENSITY; break;
        case RenderFogParameter::Start: glParameter = GL_FOG_START; break;
        case RenderFogParameter::End: glParameter = GL_FOG_END; break;
        case RenderFogParameter::Mode: glParameter = GL_FOG_MODE; break;
        case RenderFogParameter::Color: glParameter = GL_FOG_COLOR; break;
        case RenderFogParameter::DistanceMode:
#ifdef GL_FOG_DISTANCE_MODE_NV
            glParameter = GL_FOG_DISTANCE_MODE_NV;
#else
            return;
#endif
            break;
    }
    glFogf(glParameter, value);
}

void renderFogi(RenderFogParameter parameter, RenderFogMode value)
{
    const GLenum glParameter = parameter == RenderFogParameter::DistanceMode ? GL_FOG_DISTANCE_MODE_NV : GL_FOG_MODE;
    GLint glMode = GL_LINEAR;
    switch (value)
    {
        case RenderFogMode::Exp: glMode = GL_EXP; break;
        case RenderFogMode::Exp2: glMode = GL_EXP2; break;
        case RenderFogMode::Linear: glMode = GL_LINEAR; break;
        case RenderFogMode::EyeRadial: glMode = GL_EYE_RADIAL_NV; break;
    }
    glFogi(glParameter, glMode);
}

void renderFogColor(const float* values)
{
    glFogfv(GL_FOG_COLOR, values);
}

void renderLightfv(int lightIndex, RenderLightParameter parameter, const float* values)
{
    GLenum glParameter = GL_POSITION;
    switch (parameter)
    {
        case RenderLightParameter::Ambient: glParameter = GL_AMBIENT; break;
        case RenderLightParameter::Diffuse: glParameter = GL_DIFFUSE; break;
        case RenderLightParameter::Specular: glParameter = GL_SPECULAR; break;
        case RenderLightParameter::Position: glParameter = GL_POSITION; break;
    }
    glLightfv(GL_LIGHT0 + lightIndex, glParameter, values);
}

void renderLightModelAmbient(const float* values)
{
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, values);
}

void renderColorMaterial(RenderFace face, RenderColorMaterialMode mode)
{
#ifndef PLATFORM_PSP
    GLenum glFace = face == RenderFace::Front ? GL_FRONT : face == RenderFace::Back ? GL_BACK : GL_FRONT_AND_BACK;
    GLenum glMode = mode == RenderColorMaterialMode::Ambient ? GL_AMBIENT : GL_AMBIENT_AND_DIFFUSE;
    glColorMaterial(glFace, glMode);
#else
    (void)face;
    (void)mode;
#endif
}

void renderShadeModel(RenderShadeModel model)
{
#if !PLATFORM_PSP
    glShadeModel(model == RenderShadeModel::Smooth ? GL_SMOOTH : GL_FLAT);
#else
    (void)model;
#endif
}

void renderClear(unsigned int mask)
{
    glClear(static_cast<GLbitfield>(mask));
}

void renderFinishGpu()
{
    glFinish();
}

// The GL backend has no separate submit step: SDL_GL_SwapWindow is the frame
// boundary and the driver already pipelines behind it.
void renderSubmitFrame()
{
}

void renderClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void renderClearDepth(double depth)
{
    glClearDepth(depth);
}

void renderPolygonOffset(float factor, float units)
{
    glPolygonOffset(factor, units);
}

void renderLineWidth(float width)
{
    glLineWidth(width);
}

void renderViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void renderGetViewport(int* values)
{
    if (values != nullptr)
        glGetIntegerv(GL_VIEWPORT, values);
}

void renderGetMatrix(RenderMatrixQuery query, float* values)
{
    GLenum glQuery = query == RenderMatrixQuery::Projection ? GL_PROJECTION_MATRIX : query == RenderMatrixQuery::Texture ? GL_TEXTURE_MATRIX : GL_MODELVIEW_MATRIX;
    glGetFloatv(glQuery, values);
}

const unsigned char* renderGetString(RenderStringQuery query)
{
    GLenum glQuery = query == RenderStringQuery::Vendor ? GL_VENDOR : query == RenderStringQuery::Renderer ? GL_RENDERER : query == RenderStringQuery::Version ? GL_VERSION : GL_EXTENSIONS;
    return glGetString(glQuery);
}

bool renderSupportsFeature(RenderFeature feature)
{
    switch (feature)
    {
        case RenderFeature::FancyFogDistance:
            return SDL_GL_ExtensionSupported("GL_NV_fog_distance") == SDL_TRUE;
        case RenderFeature::OcclusionQuery:
            return SDL_GL_ExtensionSupported("GL_ARB_occlusion_query") == SDL_TRUE;
        case RenderFeature::Mipmaps:
            return desktopHasOpenGL12();
        case RenderFeature::AnisotropicFiltering:
            return SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic") == SDL_TRUE;
        case RenderFeature::MultisampleAntialiasing:
            return renderGetMaxSamples() > 0;
    }
    return false;
}

unsigned int renderGetError()
{
    return glGetError();
}

void renderFogHint(RenderHintMode mode)
{
#ifndef PLATFORM_PSP
    glHint(GL_FOG_HINT, mode == RenderHintMode::Nicest ? GL_NICEST : GL_FASTEST);
#else
    (void)mode;
#endif
}


void renderMatrixMode(RenderMatrixMode mode)
{
    switch (mode)
    {
        case RenderMatrixMode::ModelView: glMatrixMode(GL_MODELVIEW); break;
        case RenderMatrixMode::Projection: glMatrixMode(GL_PROJECTION); break;
        case RenderMatrixMode::Texture: glMatrixMode(GL_TEXTURE); break;
    }
}

void renderLoadIdentity()
{
    glLoadIdentity();
}

void renderPushMatrix()
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::PushMatrix;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return;
    }
#endif
    glPushMatrix();
}

void renderPopMatrix()
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::PopMatrix;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return;
    }
#endif
    glPopMatrix();
}

void renderTranslate(float x, float y, float z)
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::Translate;
        cmd.data.translate.x = x;
        cmd.data.translate.y = y;
        cmd.data.translate.z = z;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return;
    }
#endif
    glTranslatef(x, y, z);
}

void renderRotate(float angle, float x, float y, float z)
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::Rotate;
        cmd.data.rotate.angle = angle;
        cmd.data.rotate.x = x;
        cmd.data.rotate.y = y;
        cmd.data.rotate.z = z;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return;
    }
#endif
    glRotatef(angle, x, y, z);
}

void renderScale(float x, float y, float z)
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::Scale;
        cmd.data.scale.x = x;
        cmd.data.scale.y = y;
        cmd.data.scale.z = z;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return;
    }
#endif
    glScalef(x, y, z);
}

void renderScaleDouble(double x, double y, double z)
{
    glScaled(x, y, z);
}

void renderFrustum(double left, double right, double bottom, double top, double nearValue, double farValue)
{
    glFrustum(left, right, bottom, top, nearValue, farValue);
}

void renderOrtho(double left, double right, double bottom, double top, double nearValue, double farValue)
{
    glOrtho(left, right, bottom, top, nearValue, farValue);
}

void renderGenerateOcclusionQueries(int count, int* queries)
{
    glGenQueriesARB(static_cast<GLsizei>(count), reinterpret_cast<GLuint*>(queries));
}

void renderBeginOcclusionQuery(int query)
{
    glBeginQueryARB(GL_SAMPLES_PASSED_ARB, static_cast<GLuint>(query));
}

void renderEndOcclusionQuery()
{
    glEndQueryARB(GL_SAMPLES_PASSED_ARB);
}

bool renderOcclusionQueryResultAvailable(int query)
{
    GLuint value = 0;
    glGetQueryObjectuivARB(static_cast<GLuint>(query), GL_QUERY_RESULT_AVAILABLE_ARB, &value);
    return value != 0;
}

unsigned int renderOcclusionQueryResult(int query)
{
    GLuint value = 0;
    glGetQueryObjectuivARB(static_cast<GLuint>(query), GL_QUERY_RESULT_ARB, &value);
    return static_cast<unsigned int>(value);
}

bool renderReadPixelsRgb(int x, int y, int width, int height, void* pixels)
{
    if (pixels == nullptr || width <= 0 || height <= 0) return false;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    return true;
}

void renderSetLegacyPresentationGamma(bool)
{
    // Desktop uses the real GLSL framebuffer gamma pass.
}

bool renderCopyFramebufferToBoundTexture(int x, int y, int width, int height)
{
    if (width <= 0 || height <= 0) return false;
#ifndef PLATFORM_PSP
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, width, height);
    return glGetError() == GL_NO_ERROR;
#else
    (void)x; (void)y;
    return true;
#endif
}



bool renderCaptureInterleaved(const RenderInterleavedMesh& mesh, RenderCapturedMesh& out, bool append)
{
    if (mesh.data == nullptr || mesh.stride <= 0 || mesh.count <= 0)
        return false;
    if (!append)
        out.clear();

#if PLATFORM_PSP
    // Convert standard 32-byte terrain quads/triangles into 24-byte native PSP GE layout:
    // [UV (8B float2), Color (4B ubyte4), Pos (12B float3)] and expand GL_QUADS to GL_TRIANGLES
    // once at capture time so PSPGL submits directly to the GE without per-frame CPU repacking.
    if (mesh.stride == 32 && mesh.hasTexture && mesh.texCoordOffset == 12 &&
        mesh.hasColor && mesh.colorOffset == 20 && !mesh.positionShort &&
        (mesh.primitive == RenderPrimitive::Quads || mesh.primitive == RenderPrimitive::Triangles))
    {
        const bool isQuads = (mesh.primitive == RenderPrimitive::Quads) && ((mesh.count % 4) == 0);
        const int outVertsToAdd = isQuads ? ((mesh.count / 4) * 6) : mesh.count;

        if (!out.empty() && (out.stride != 24 || out.primitive != RenderPrimitive::Triangles ||
            out.positionShort || !out.hasTexture || out.texCoordOffset != 0 ||
            !out.hasColor || out.colorOffset != 8 || out.hasNormals || out.hasBrightness))
            return false;

        if (out.empty())
        {
            out.stride = 24;
            out.primitive = RenderPrimitive::Triangles;
            out.positionShort = false;
            out.hasTexture = true;
            out.texCoordOffset = 0;
            out.hasColor = true;
            out.colorOffset = 8;
            out.hasNormals = false;
            out.normalOffset = 0;
            out.hasBrightness = false;
            out.brightnessOffset = 0;
        }

        const size_t oldWords = out.raw.size();
        const size_t newWords = static_cast<size_t>(outVertsToAdd) * 6u; // 24 bytes = 6 words
        out.raw.resize(oldWords + newWords);

        struct PspNativeVertex
        {
            float u, v;
            std::uint32_t col;
            float x, y, z;
        };

        auto packVertex = [](PspNativeVertex& dst, const unsigned char* srcVert)
        {
            const float* pos = reinterpret_cast<const float*>(srcVert + 0);
            const float* uv = reinterpret_cast<const float*>(srcVert + 12);
            const std::uint32_t* col = reinterpret_cast<const std::uint32_t*>(srcVert + 20);
            dst.u = uv[0];
            dst.v = uv[1];
            dst.col = *col;
            dst.x = pos[0];
            dst.y = pos[1];
            dst.z = pos[2];
        };

        PspNativeVertex* dst = reinterpret_cast<PspNativeVertex*>(
            reinterpret_cast<unsigned char*>(out.raw.data()) + oldWords * 4u);
        const unsigned char* src = static_cast<const unsigned char*>(mesh.data) +
            static_cast<size_t>(mesh.first) * 32u;

        if (isQuads)
        {
            const int quadCount = mesh.count / 4;
            int outIdx = 0;
            for (int q = 0; q < quadCount; ++q)
            {
                const unsigned char* v0 = src + (q * 4 + 0) * 32;
                const unsigned char* v1 = src + (q * 4 + 1) * 32;
                const unsigned char* v2 = src + (q * 4 + 2) * 32;
                const unsigned char* v3 = src + (q * 4 + 3) * 32;

                packVertex(dst[outIdx++], v0);
                packVertex(dst[outIdx++], v1);
                packVertex(dst[outIdx++], v2);
                packVertex(dst[outIdx++], v2);
                packVertex(dst[outIdx++], v3);
                packVertex(dst[outIdx++], v0);
            }
        }
        else
        {
            for (int i = 0; i < mesh.count; ++i)
            {
                packVertex(dst[i], src + i * 32);
            }
        }

        out.vertexCount += outVertsToAdd;
        return true;
    }
#endif

    if (!out.empty() && (out.stride != mesh.stride || out.primitive != mesh.primitive ||
        out.positionShort != mesh.positionShort ||
        out.hasTexture != mesh.hasTexture || (mesh.hasTexture && out.texCoordOffset != mesh.texCoordOffset) ||
        out.hasColor != mesh.hasColor || (mesh.hasColor && out.colorOffset != mesh.colorOffset) ||
        out.hasNormals != mesh.hasNormals || (mesh.hasNormals && out.normalOffset != mesh.normalOffset) ||
        out.hasBrightness != mesh.hasBrightness || (mesh.hasBrightness && out.brightnessOffset != mesh.brightnessOffset)))
        return false;

    if (out.empty())
    {
        out.stride = mesh.stride;
        out.primitive = mesh.primitive;
        out.positionShort = mesh.positionShort;
        out.hasTexture = mesh.hasTexture;
        out.texCoordOffset = mesh.texCoordOffset;
        out.hasColor = mesh.hasColor;
        out.colorOffset = mesh.colorOffset;
        out.hasNormals = mesh.hasNormals;
        out.normalOffset = mesh.normalOffset;
        out.hasBrightness = mesh.hasBrightness;
        out.brightnessOffset = mesh.brightnessOffset;
    }

    const unsigned char* src = static_cast<const unsigned char*>(mesh.data) + (size_t)mesh.first * mesh.stride;
    const size_t bytes = (size_t)mesh.count * mesh.stride;
    const size_t old = out.raw.size();
    out.raw.resize(old + (bytes + 3u) / 4u);
    std::memcpy(reinterpret_cast<unsigned char*>(out.raw.data()) + old * 4u, src, bytes);
    out.vertexCount += mesh.count;
    return true;
}

static bool renderDrawInterleavedInternal(const RenderInterleavedMesh& mesh, bool insideBatch)
{
    if (mesh.data == nullptr || mesh.stride <= 0 || mesh.count <= 0)
        return false;

#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0)
    {
        if (s_pspCurrentRecordingList >= PSP_MAX_DISPLAY_LISTS)
            return false;

        RenderCapturedMesh captured;
        if (!renderCaptureInterleaved(mesh, captured, false))
            return false;

        std::uint32_t meshIdx = static_cast<std::uint32_t>(s_pspLists[s_pspCurrentRecordingList].meshes.size());
        s_pspLists[s_pspCurrentRecordingList].meshes.push_back(std::move(captured));

        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::DrawCaptured;
        cmd.data.meshIndex = meshIdx;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return true;
    }
    if (mesh.stride == 24 && mesh.hasTexture && mesh.texCoordOffset == 0 &&
        mesh.hasColor && mesh.colorOffset == 8 && !mesh.hasNormals && !mesh.positionShort)
    {
        const unsigned char* base = reinterpret_cast<const unsigned char*>(mesh.data);
        glTexCoordPointer(2, GL_FLOAT, 24, base + 0);
        glColorPointer(4, GL_UNSIGNED_BYTE, 24, base + 8);
        glVertexPointer(3, GL_FLOAT, 24, base + 12);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
        glDrawArrays(static_cast<GLenum>(renderPrimitiveValue(mesh.primitive)), mesh.first, mesh.count);
        return true;
    }
    if (mesh.stride == 20 && mesh.hasTexture && mesh.texCoordOffset == 12 &&
        mesh.hasColor && mesh.colorOffset == 8 && !mesh.hasNormals && mesh.positionShort)
    {
        const unsigned char* base = reinterpret_cast<const unsigned char*>(mesh.data);
        glVertexPointer(3, GL_SHORT, 20, base + 0);
        glColorPointer(4, GL_UNSIGNED_BYTE, 20, base + 8);
        glTexCoordPointer(2, GL_FLOAT, 20, base + 12);
        if (!insideBatch)
        {
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        glDrawArrays(static_cast<GLenum>(renderPrimitiveValue(mesh.primitive)), mesh.first, mesh.count);
        return true;
    }
#endif

    auto pointerForOffset = [&](int offset) -> const void*
    {
        return reinterpret_cast<const unsigned char*>(mesh.data) + offset;
    };

    if (!insideBatch)
        clientActiveTextureCompat(GL_TEXTURE0_ARB);

    if (mesh.hasTexture)
    {
        glTexCoordPointer(2, GL_FLOAT, mesh.stride, pointerForOffset(mesh.texCoordOffset));
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    else
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
#if !PLATFORM_PSP
    if (mesh.hasBrightness)
    {
        clientActiveTextureCompat(GL_TEXTURE1_ARB);
        glTexCoordPointer(2, GL_SHORT, mesh.stride, pointerForOffset(mesh.brightnessOffset));
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        clientActiveTextureCompat(GL_TEXTURE0_ARB);
    }
#endif
    if (mesh.hasColor)
    {
        glColorPointer(4, GL_UNSIGNED_BYTE, mesh.stride, pointerForOffset(mesh.colorOffset));
        glEnableClientState(GL_COLOR_ARRAY);
    }
    else
    {
        glDisableClientState(GL_COLOR_ARRAY);
    }
    if (mesh.hasNormals)
    {
        glNormalPointer(GL_BYTE, mesh.stride, pointerForOffset(mesh.normalOffset));
        glEnableClientState(GL_NORMAL_ARRAY);
    }
    else
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    const int posOffset = (mesh.stride == 24 && mesh.texCoordOffset == 0) ? 12 : 0;
    glVertexPointer(3, mesh.positionShort ? GL_SHORT : GL_FLOAT, mesh.stride, pointerForOffset(posOffset));
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(static_cast<GLenum>(renderPrimitiveValue(mesh.primitive)), mesh.first, mesh.count);

    if (!insideBatch)
    {
        glDisableClientState(GL_VERTEX_ARRAY);

        if (mesh.hasTexture)
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#if !PLATFORM_PSP
        if (mesh.hasBrightness)
        {
            clientActiveTextureCompat(GL_TEXTURE1_ARB);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            clientActiveTextureCompat(GL_TEXTURE0_ARB);
        }
#endif
        if (mesh.hasColor)
        {
            glDisableClientState(GL_COLOR_ARRAY);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }
        if (mesh.hasNormals)
            glDisableClientState(GL_NORMAL_ARRAY);
    }

    return true;
}

bool renderDrawInterleaved(const RenderInterleavedMesh& mesh)
{
    return renderDrawInterleavedInternal(mesh, false);
}

static bool renderDrawCapturedInternal(const RenderCapturedMesh& mesh, bool insideBatch)
{
    if (mesh.empty()) return false;
    RenderInterleavedMesh view;
    view.data = mesh.raw.data(); view.stride = mesh.stride; view.count = mesh.vertexCount;
    view.primitive = mesh.primitive; view.positionShort = mesh.positionShort;
    view.hasTexture = mesh.hasTexture; view.texCoordOffset = mesh.texCoordOffset;
    view.hasColor = mesh.hasColor; view.colorOffset = mesh.colorOffset;
    view.hasNormals = mesh.hasNormals; view.normalOffset = mesh.normalOffset;
    view.hasBrightness = mesh.hasBrightness; view.brightnessOffset = mesh.brightnessOffset;
    return renderDrawInterleavedInternal(view, insideBatch);
}

void renderPreparePspTerrainMesh(RenderCapturedMesh& mesh, float offsetX, float offsetY, float offsetZ)
{
    (void)offsetX;
    (void)offsetY;
    (void)offsetZ;
    if (mesh.empty() || mesh.positionShort || mesh.stride != 24 ||
        !mesh.hasTexture || mesh.texCoordOffset != 0 ||
        !mesh.hasColor || mesh.colorOffset != 8 || mesh.hasNormals)
        return;

    if (mesh.primitive != RenderPrimitive::Quads || (mesh.vertexCount % 4) != 0)
        return;

    struct PspTerrainVertex
    {
        float u, v;
        std::uint32_t col;
        float x, y, z;
    };

    const int quadCount = mesh.vertexCount / 4;
    const int dstCount = quadCount * 6;
    mesh.raw.resize(static_cast<std::size_t>(dstCount) * 6u); // 24 bytes = 6 words per vertex

    PspTerrainVertex* verts = reinterpret_cast<PspTerrainVertex*>(mesh.raw.data());
    for (int q = quadCount - 1; q >= 0; --q)
    {
        const PspTerrainVertex v0 = verts[q * 4 + 0];
        const PspTerrainVertex v1 = verts[q * 4 + 1];
        const PspTerrainVertex v2 = verts[q * 4 + 2];
        const PspTerrainVertex v3 = verts[q * 4 + 3];

        PspTerrainVertex* tv = verts + q * 6;
        tv[0] = v0;
        tv[1] = v1;
        tv[2] = v2;
        tv[3] = v0;
        tv[4] = v2;
        tv[5] = v3;
    }

    mesh.vertexCount = dstCount;
    mesh.primitive = RenderPrimitive::Triangles;
}
bool renderDrawCaptured(const RenderCapturedMesh& mesh)
{
    return renderDrawCapturedInternal(mesh, false);
}
#if PLATFORM_PSP
bool renderRecordCapturedMesh(RenderCapturedMesh&& mesh)
{
    if (mesh.empty() || s_pspCurrentRecordingList <= 0 ||
        s_pspCurrentRecordingList >= PSP_MAX_DISPLAY_LISTS)
        return false;

    PspDisplayList& list = s_pspLists[s_pspCurrentRecordingList];
    const std::uint32_t meshIndex = static_cast<std::uint32_t>(list.meshes.size());
    list.meshes.push_back(std::move(mesh));

    PspDisplayCmd cmd;
    cmd.type = PspDisplayCmdType::DrawCaptured;
    cmd.data.meshIndex = meshIndex;
    list.commands.push_back(cmd);
    return true;
}
#endif

#if PLATFORM_PSP
void renderCompactTerrainMesh(RenderCapturedMesh& mesh)
{
    if (mesh.empty() || mesh.positionShort || mesh.stride != 24 ||
        !mesh.hasTexture || mesh.texCoordOffset != 0 ||
        !mesh.hasColor || mesh.colorOffset != 8 || mesh.hasNormals)
        return;

    struct PspSrcVertex
    {
        float u, v;
        std::uint32_t col;
        float x, y, z;
    };

    struct PspCompactVertex
    {
        std::int16_t x, y, z, pad;
        std::uint32_t col;
        float u, v;
    };

    const int count = mesh.vertexCount;
    std::vector<std::int32_t> compactRaw(static_cast<std::size_t>(count) * 5u); // 20 bytes = 5 words
    const PspSrcVertex* src = reinterpret_cast<const PspSrcVertex*>(mesh.raw.data());
    PspCompactVertex* dst = reinterpret_cast<PspCompactVertex*>(compactRaw.data());

    for (int i = 0; i < count; ++i)
    {
        dst[i].x = static_cast<std::int16_t>(src[i].x * 256.0f + (src[i].x >= 0.0f ? 0.5f : -0.5f));
        dst[i].y = static_cast<std::int16_t>(src[i].y * 256.0f + (src[i].y >= 0.0f ? 0.5f : -0.5f));
        dst[i].z = static_cast<std::int16_t>(src[i].z * 256.0f + (src[i].z >= 0.0f ? 0.5f : -0.5f));
        dst[i].pad = 0;
        dst[i].col = src[i].col;
        dst[i].u = src[i].u;
        dst[i].v = src[i].v;
    }

    mesh.raw.swap(compactRaw);
    mesh.stride = 20;
    mesh.positionShort = true;
    mesh.colorOffset = 8;
    mesh.texCoordOffset = 12;
}
#endif

int renderGenerateDisplayLists(int count)
{
#if PLATFORM_PSP
    if (count <= 0) return 0;
    pspEnsureDisplayListPool();

    int run = 0;
    int runStart = 0;
    for (int i = 1; i < PSP_MAX_DISPLAY_LISTS; ++i)
    {
        if (!s_pspLists[i].inUse)
        {
            if (run == 0) runStart = i;
            run++;
            if (run == count)
            {
                for (int j = 0; j < count; ++j)
                {
                    s_pspLists[runStart + j].clear();
                    s_pspLists[runStart + j].inUse = true;
                }
                return runStart;
            }
        }
        else
        {
            run = 0;
        }
    }
    return 0;
#else
    return static_cast<int>(glGenLists(static_cast<GLsizei>(count)));
#endif
}

void renderBeginDisplayList(int list)
{
#if PLATFORM_PSP
    if (list <= 0 || list >= PSP_MAX_DISPLAY_LISTS) return;
    pspEnsureDisplayListPool();
    s_pspLists[list].clear();
    s_pspLists[list].inUse = true;
    s_pspCurrentRecordingList = list;
#else
    glNewList(static_cast<GLuint>(list), GL_COMPILE);
#endif
}

void renderDeleteDisplayLists(int first, int count)
{
#if PLATFORM_PSP
    if (first <= 0 || count <= 0) return;
    int end = first + count;
    if (end > PSP_MAX_DISPLAY_LISTS)
        end = PSP_MAX_DISPLAY_LISTS;
    for (int i = first; i < end; ++i)
    {
        s_pspLists[i].clear();
    }
#else
    glDeleteLists(static_cast<GLuint>(first), static_cast<GLsizei>(count));
#endif
}

void renderEndDisplayList()
{
#if PLATFORM_PSP
    if (s_pspCurrentRecordingList > 0 && s_pspCurrentRecordingList < PSP_MAX_DISPLAY_LISTS)
    {
        for (RenderCapturedMesh& mesh : s_pspLists[s_pspCurrentRecordingList].meshes)
        {
            if (mesh.primitive == RenderPrimitive::Quads)
            {
                renderPreparePspTerrainMesh(mesh, 0.0f, 0.0f, 0.0f);
            }
        }
    }
    s_pspCurrentRecordingList = 0;
#else
    glEndList();
#endif
}

#if PLATFORM_PSP
static void renderCallDisplayListInternal(int list, bool insideBatch)
{
    if (list <= 0 || list >= PSP_MAX_DISPLAY_LISTS)
        return;

    if (s_pspCurrentRecordingList > 0)
    {
        PspDisplayCmd cmd;
        cmd.type = PspDisplayCmdType::CallList;
        cmd.data.integer = list;
        s_pspLists[s_pspCurrentRecordingList].commands.push_back(cmd);
        return;
    }

    const PspDisplayList& dl = s_pspLists[list];
    if (!dl.inUse)
        return;

    for (const PspDisplayCmd& cmd : dl.commands)
    {
        switch (cmd.type)
        {
            case PspDisplayCmdType::DrawCaptured:
                if (cmd.data.meshIndex < dl.meshes.size())
                {
                    renderDrawCapturedInternal(dl.meshes[cmd.data.meshIndex], insideBatch);
                }
                break;
            case PspDisplayCmdType::PushMatrix:
                glPushMatrix();
                break;
            case PspDisplayCmdType::PopMatrix:
                glPopMatrix();
                break;
            case PspDisplayCmdType::Translate:
                glTranslatef(cmd.data.translate.x, cmd.data.translate.y, cmd.data.translate.z);
                break;
            case PspDisplayCmdType::Scale:
                glScalef(cmd.data.scale.x, cmd.data.scale.y, cmd.data.scale.z);
                break;
            case PspDisplayCmdType::Rotate:
                glRotatef(cmd.data.rotate.angle, cmd.data.rotate.x, cmd.data.rotate.y, cmd.data.rotate.z);
                break;
            case PspDisplayCmdType::Color4f:
                glColor4f(cmd.data.color.r, cmd.data.color.g, cmd.data.color.b, cmd.data.color.a);
                break;
            case PspDisplayCmdType::BindTexture:
                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(cmd.data.integer));
                break;
            case PspDisplayCmdType::CallList:
                renderCallDisplayListInternal(cmd.data.integer, insideBatch);
                break;
        }
    }
}

void renderCallDisplayList(int list)
{
    renderCallDisplayListInternal(list, false);
}

void renderCallDisplayLists(int count, const int* lists)
{
    if (lists == nullptr || count <= 0) return;

    clientActiveTextureCompat(GL_TEXTURE0_ARB);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    for (int i = 0; i < count; ++i)
    {
        renderCallDisplayListInternal(lists[i], true);
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
#else
void renderCallDisplayList(int list)
{
    glCallList(static_cast<GLuint>(list));
}

void renderCallDisplayLists(int count, const int* lists)
{
    glCallLists(static_cast<GLsizei>(count), GL_INT, lists);
}
#endif
