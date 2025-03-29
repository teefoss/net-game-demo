//
//  video.hh
//  Realm
//
//  Created by Thomas Foster on 3/13/25.
//

#ifndef video_h
#define video_h

#include <SDL3/SDL.h>

#define PIXEL_SCALE 1
#define CHAR_WIDTH (8 * PIXEL_SCALE)
#define CHAR_HEIGHT (8 * PIXEL_SCALE)

enum Color {
    BLACK,
    VERY_DARK_GRAY,
    DARK_BLUE,
    DARK_GREEN,
    DARK_GRAY,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    WHITE,
    GRAY,
    BRIGHT_BLUE,
    BRIGHT_GREEN,
    BRIGHT_CYAN,
    BRIGHT_RED,
    BRIGHT_MAGENTA,
    YELLOW,
    BRIGHT_WHITE,

    NUM_COLORS,
    TRANSPARENT,
};

void InitVideo(int width, int height, int scale);
void RefreshWindow(void);

// Settings
void ToggleFullscreen(void);
void SetFullscreen(bool value);
void SetWindowSize(int w, int h);
void SetColor(Color color);
void SetViewport(const SDL_Rect * rect);
void SetWindowTitle(const char * title);
void SetWindowPosition(int x, int y);

// Drawing
void ClearWindow(int r, int g, int b);
void DrawPoint(float x, float y, Color color);
void FillRect(float x, float y, int w, int h, int r, int g, int b);
void DrawFrame(int x, int y, int w, int h, int r, int g, int b, int thickness);
void DrawHorizontalLine(int x1, int x2, int y, int r, int g, int b);
void DrawHorizontalLine(int x, int y, int w, Color c);

// Text
void DrawChar(float x, float y, int ch, Color fg, Color bg = TRANSPARENT);
int DrawText(int x, int y, Color color, const char * format, ...);
void DrawCenteredText(int y, Color color, const char * format, ...);

#endif /* video_h */
