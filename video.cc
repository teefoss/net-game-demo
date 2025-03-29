#include "video.hh"
#include "misc.hh"

#define ASSETS_DIR "assets/"
#define TEXT_BMP_PATH ASSETS_DIR"cp437_8x8.bmp"

#define FONT_SHEET_COLS 16

static SDL_Window * _window;
static SDL_Renderer * _renderer;
static SDL_Texture * _text_texture;
static bool _is_fullscreen = false;
static int render_scale;

static const SDL_Color palette[] = {
    [BLACK]             = { 0x00, 0x00, 0x00 },
    [VERY_DARK_GRAY]    = { 0x08, 0x08, 0x08 },
    [DARK_BLUE]         = { 0x00, 0x00, 0x55 },
    [DARK_GREEN]        = { 0x00, 0x55, 0x00 },
    // DARK_CYAN
    // DARK_RED
    // DARK_MAGENTA
    // DARK_BROWN
    [DARK_GRAY]         = { 0x55, 0x55, 0x55 },
    [BLUE]              = { 0x00, 0x00, 0xAA },
    [GREEN]             = { 0x00, 0xAA, 0x00 },
    [CYAN]              = { 0x00, 0xAA, 0xAA },
    [RED]               = { 0xAA, 0x00, 0x00 },
    [MAGENTA]           = { 0xAA, 0x00, 0xAA },
    [BROWN]             = { 0xAA, 0x55, 0x00 },
    [WHITE]             = { 0xAA, 0xAA, 0xAA },
    [GRAY]              = { 0x55, 0x55, 0x55 },
    [BRIGHT_BLUE]       = { 0x55, 0x55, 0xFF },
    [BRIGHT_GREEN]      = { 0x55, 0xFF, 0x55 },
    [BRIGHT_CYAN]       = { 0x55, 0xFF, 0xFF },
    [BRIGHT_RED]        = { 0xFF, 0x55, 0x55 },
    [BRIGHT_MAGENTA]    = { 0xFF, 0x55, 0xFF },
    [YELLOW]            = { 0xFF, 0xFF, 0x55 },
    [BRIGHT_WHITE]      = { 0xFF, 0xFF, 0xFF },
};

static SDL_Color saved_draw_color;

static void SaveDrawColor(void) {
    SDL_GetRenderDrawColor(_renderer,
                           &saved_draw_color.r,
                           &saved_draw_color.g,
                           &saved_draw_color.b,
                           &saved_draw_color.a);
}

static void RestoreDrawColor(void) {
    SDL_SetRenderDrawColor(_renderer,
                           saved_draw_color.r,
                           saved_draw_color.g,
                           saved_draw_color.b,
                           saved_draw_color.a);
}

void SetWindowTitle(const char * title)
{
    SDL_SetWindowTitle(_window, title);
}

void SetWindowPosition(int x, int y)
{
    SDL_SetWindowPosition(_window, x, y);
}

void SetColor(Color color)
{
    SDL_SetRenderDrawColor(_renderer,
                           palette[color].r,
                           palette[color].g,
                           palette[color].b,
                           255);
}

static void InitWindow(int width, int height, int scale)
{
    int w = width * scale;
    int h = height * scale;

    Uint32 window_flags = 0;
    window_flags |= _is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0;

    _window = SDL_CreateWindow("", w, h, window_flags);
}

static void InitRenderer(int width, int height)
{
    _renderer = SDL_CreateRenderer(_window, 0);

    if ( !SDL_SetRenderVSync(_renderer, 1) ) {
        fprintf(stderr, "SDL_SetRenderVSync failed: %s\n", SDL_GetError());
    }

    SDL_SetRenderLogicalPresentation(_renderer,
                                     width, height,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
}

static void InitTextTexture(void)
{
    SDL_Surface * surface = SDL_LoadBMP(TEXT_BMP_PATH);
    if ( surface == NULL ) {
        ERROR("failed to load font sheet '%s'", TEXT_BMP_PATH);
    }

    Uint32 key = SDL_MapSurfaceRGB(surface, 0, 0, 0); // transparency key
    SDL_SetSurfaceColorKey(surface, true, key);

    _text_texture = SDL_CreateTextureFromSurface(_renderer, surface);
    if ( _text_texture == NULL ) {
        ERROR("failed to create font texture");
    }

    SDL_SetTextureScaleMode(_text_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(_text_texture, SDL_BLENDMODE_BLEND);
}

#if 0
static void InitScreenTexture(int width, int height)
{
    screen_texture = SDL_CreateTexture(renderer,
                                       SDL_GetWindowPixelFormat(window),
                                       SDL_TEXTUREACCESS_TARGET,
                                       width,
                                       height);
    if ( screen_texture == NULL ) {
        ERROR("failed to create screen texture");
    }

    if ( !SDL_SetTextureScaleMode(text_texture, SDL_SCALEMODE_NEAREST) ) {
        fprintf(stderr, "SDL_SetTextureScaleMode failed: %s\n", SDL_GetError());
    }

    SDL_SetTextureBlendMode(screen_texture, SDL_BLENDMODE_BLEND);

    SDL_ScaleMode mode;
    SDL_GetTextureScaleMode(text_texture, &mode);
    printf("mode: %d\n", mode);
}
#endif

void InitVideo(int width, int height, int scale)
{
    if ( SDL_WasInit(SDL_INIT_VIDEO) == 0 ) {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    render_scale = scale;

    InitWindow(width, height, scale);
    InitRenderer(width, height);
    InitTextTexture();
}

void ToggleFullscreen(void)
{
    _is_fullscreen = !_is_fullscreen;
    SDL_SetWindowFullscreen(_window, _is_fullscreen);
}

void SetFullscreen(bool value)
{
    _is_fullscreen = value;
    SDL_SetWindowFullscreen(_window, _is_fullscreen);
}

void SetWindowSize(int w, int h)
{
    SDL_SetWindowSize(_window, w * render_scale, h * render_scale);
    SDL_SetRenderLogicalPresentation(_renderer,
                                     w, h,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}

void DrawChar(float x, float y, int ch, Color fg, Color bg)
{
    SDL_FRect src = {
        .x = static_cast<float>((ch % FONT_SHEET_COLS) * CHAR_WIDTH),
        .y = static_cast<float>((ch / FONT_SHEET_COLS) * CHAR_HEIGHT),
        .w = CHAR_WIDTH,
        .h = CHAR_HEIGHT
    };

    SDL_FRect dst = {
        .x = x,
        .y = y,
        .w = CHAR_WIDTH,
        .h = CHAR_HEIGHT
    };

    if ( bg != TRANSPARENT ) {
        SetColor(bg);
        SDL_RenderFillRect(_renderer, &dst);
    }

    const SDL_Color * c = &palette[fg];
    SDL_SetTextureColorMod(_text_texture, c->r, c->g, c->b);
    SDL_RenderTexture(_renderer, _text_texture, &src, &dst);
}

int DrawText(int x, int y, Color color, const char * format, ...)
{
    char buffer[256];

    size_t buf_size = sizeof(buffer);
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, buf_size, format, args);
    va_end(args);

    int len = (int)strlen(buffer);

    if ( len >= buf_size ) {
        fprintf(stderr, 
                "Warning: string '%s' has length greater than %zu",
                buffer, buf_size);
    }

    const char * ch = buffer;
    Uint32 x1 = x;
    while ( *ch ) {
        DrawChar(x1, y, *ch, color);
        x1 += CHAR_WIDTH;
        ch++;
    }

    return x1 - x;
}

void DrawCenteredText(int y, Color color, const char * format, ...)
{
    char buf[128];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    int render_w;
    SDL_GetRenderLogicalPresentation(_renderer, &render_w, NULL, NULL);
    int x = (render_w - (int)strlen(buf) * CHAR_WIDTH) / 2;
    DrawText(x, y, color, buf);
}

void SetViewport(const SDL_Rect * rect)
{
    SDL_SetRenderViewport(_renderer, rect);
}

void DrawPoint(float x, float y, Color color)
{
    SetColor(color);
    SDL_RenderPoint(_renderer, x, y);
}

void FillRect(float x, float y, int w, int h, int r, int g, int b)
{
    SaveDrawColor();
    SDL_SetRenderDrawColor(_renderer, r, g, b, 255);
    SDL_FRect rect = { x, y, static_cast<float>(w), static_cast<float>(h) };
    SDL_RenderFillRect(_renderer, &rect);
    RestoreDrawColor();
}

void DrawFrame(int x, int y, int w, int h, int r, int g, int b, int thickness)
{
    SaveDrawColor();
    SDL_SetRenderDrawColor(_renderer, r, g, b, 255);

    for ( int i = 0; i < thickness; i++ ) {
        SDL_FRect rect = {
            .x = static_cast<float>(x),
            .y = static_cast<float>(y),
            .w = static_cast<float>(w),
            .h = static_cast<float>(h)
        };

        SDL_RenderRect(_renderer, &rect);
        x--;
        y--;
        w += 2;
        h += 2;
    }
    RestoreDrawColor();
}

void DrawHorizontalLine(int x1, int x2, int y, int r, int g, int b)
{
    SDL_SetRenderDrawColor(_renderer, r, g, b, 255);
    SDL_RenderLine(_renderer, x1, y, x2, y);
}

void DrawHorizontalLine(int x, int y, int w, Color c)
{
    DrawHorizontalLine(x, x + w, y, palette[c].r, palette[c].g, palette[c].b);
}

void ClearWindow(int r, int g, int b)
{
    SaveDrawColor();

    SDL_SetRenderDrawColor(_renderer, r, g, b, 255);
    SDL_RenderClear(_renderer);

    RestoreDrawColor();
}

void RefreshWindow(void)
{
    SDL_SetRenderTarget(_renderer, NULL);
    SDL_RenderPresent(_renderer);
}
