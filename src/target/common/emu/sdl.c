/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Deviation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
*/
#define _WIN32_WINNT 0x0500
#include <SDL2/SDL.h>
#include <sys/time.h>

#include "common.h"
#include "fltk.h"
#include "mixer.h"
#include "config/tx.h"
#include "fltk_resample.h"

#define FL_Left   SDLK_LEFT
#define FL_Right  SDLK_RIGHT
#define FL_Up     SDLK_UP
#define FL_Down   SDLK_DOWN
#define FL_Escape SDLK_ESCAPE

static const unsigned keymap[BUT_LAST] = BUTTON_MAP;

static struct {
    s32 xscale;
    s32 yscale;
    s32 xoffset;
    s32 yoffset;
} calibration = {0x10000, 0x10000, 0, 0};

static SDL_Window *screen;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static u16 (*timer_callback)(void);
static int changed = 0;

enum {
    TIMER_ENABLE = LAST_PRIORITY,
    NUM_MSEC_CALLBACKS,
};

void set_stick_positions()
{
    gui.throttle = 5;
    gui.elevator = 5;
    gui.aileron  = 5;
    gui.rudder   = 5;
    switch(Transmitter.mode) {
    case MODE_1:
    case MODE_3:
       gui.throttle = 0;
       break;
    case MODE_2:
    case MODE_4:
       gui.elevator = 0;
       break;
    }
	gui.aux2     = 5;
	gui.aux3     = 5;
	gui.aux4     = 5;
	gui.aux5     = 5;
	gui.aux6     = 5;
	gui.aux7     = 5;
}

void LCD_Init()
{

  SDL_CreateWindowAndRenderer(SCREEN_X, SCREEN_Y, SDL_WINDOW_OPENGL, &screen, &renderer);
  //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
  SDL_RenderSetLogicalSize(renderer, LCD_WIDTH, LCD_HEIGHT);
  texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_RGB24,
                               SDL_TEXTUREACCESS_STREAMING,
                               LCD_WIDTH, LCD_HEIGHT);
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir)
{
    gui.xstart = x0;
    gui.ystart = y0;
    gui.xend   = x1;
    gui.yend   = y1;
    gui.x = x0;
    if (dir == DRAW_NWSE) {
        gui.y = y0;
        gui.dir = 1;
    } else if (dir == DRAW_SWNE) {
        gui.y = y1;
        gui.dir = -1;
    }
}

void LCD_DrawStop(void) {
    changed = 1;
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    gui.xstart = x; //This is to emulate how the LCD behaves
    gui.x = x;
    gui.y = y;
    LCD_DrawPixel(color);
}

void LCD_ForceUpdate() {
    if (changed) {
        changed = 0;
/*
        u8 *image;
        #if (IMAGE_X < SCREEN_X && IMAGE_Y < SCREEN_Y && ((SCREEN_X / IMAGE_X) * IMAGE_X) && ((SCREEN_Y / IMAGE_Y) * IMAGE_Y))
          //ZOOM_X and ZOOM_Y are integers
          pixel_mult(gui.scaled_img, gui.image, IMAGE_X, IMAGE_Y, ZOOM_X, ZOOM_Y, 3);
          image = gui.scaled_img;
        #elif SCREEN_RESIZE
          //non-integer zoom
          resample(screen->w, screen->h, gui.image, IMAGE_X, IMAGE_Y, 3, 0, 0, gui.scaled_img);
          image = gui.scaled_img;
        #else
          image = gui.image;
        #endif
        SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
               image,
               SCREEN_X,
               SCREEN_Y,
               24,
               SCREEN_X * 3,
               0,
               0,
               0,
               0);
        if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
        SDL_BlitSurface(surface, NULL, screen, NULL);
        if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
        SDL_Flip(screen); 
*/
        SDL_UpdateTexture(texture, NULL, gui.image, LCD_WIDTH * 3);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}
uint32_t tick_callbackfunc(uint32_t interval, void *param)
{
    (void)interval;
    (void)param;
    SDL_Event event;
    SDL_UserEvent userevent;

    /* In this example, our callback pushes an SDL_USEREVENT event
    into the queue, and causes our callback to be called again at the
    same interval: */

    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = (void *)1L;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);
    return(1);
}

void CLOCK_Init()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
    SDL_AddTimer(100, tick_callbackfunc, NULL);
}

u32 msec_cbtime[NUM_MSEC_CALLBACKS];
u8 timer_enable;
void ALARMhandler()
{
    /* msecs++;
    if(msecs % 1000 == 0) printf("msecs %d\n", msecs);
    if(msecs%200==0)
    {   // In my PC, every 200 count = 1 second, so 1 unit of msecs = 5ms, which is the minimum time unit
        // so the msecs doesn't work in Windows
        struct timeb tp;
        u32 t;
        ftime(&tp);
        t = (tp.time * 1000) + tp.millitm;
        printf("%d\n",t);
    } */

    if(timer_callback && timer_enable & (1 << TIMER_ENABLE) &&
            CLOCK_getms() >= msec_cbtime[TIMER_ENABLE]) {
            //msecs == msec_cbtime[TIMER_ENABLE]) {
#ifdef TIMING_DEBUG
        debug_timing(4, 0);
#endif
        u16 us = timer_callback();
#ifdef TIMING_DEBUG
        debug_timing(4, 1);
#endif
        if (us > 0) {
            msec_cbtime[TIMER_ENABLE] += us;
        }
    }
    if(timer_enable & (1 << MEDIUM_PRIORITY) &&
            CLOCK_getms() >= msec_cbtime[MEDIUM_PRIORITY]) {
            // msecs == msec_cbtime[MEDIUM_PRIORITY]) {
        medium_priority_cb();
        priority_ready |= 1 << MEDIUM_PRIORITY;
        msec_cbtime[MEDIUM_PRIORITY] += MEDIUM_PRIORITY_MSEC;
    }
    if(timer_enable & (1 << LOW_PRIORITY) &&
            CLOCK_getms() >= msec_cbtime[LOW_PRIORITY]) {
            // msecs == msec_cbtime[LOW_PRIORITY]) {
        priority_ready |= 1 << LOW_PRIORITY;
        msec_cbtime[LOW_PRIORITY] += LOW_PRIORITY_MSEC;
    }
}

u32 ScanButtons()
{
    return gui.buttons;
}

int PWR_CheckPowerSwitch()
{
    return gui.powerdown;
}

void PWR_Shutdown() {
    SDL_Quit();
    exit(0);
}


int get_button(SDL_Keysym *key)
{
    int i = 0;
    unsigned k = key->sym;
    if(key->mod & KMOD_SHIFT)
        k -= 'a' - 'A';
    while(keymap[i] != 0) {
        if(k == keymap[i])
            return i;
        i++;
    }
    return -1;
}

void close_window()
{
    char tmp[256];
    sprintf(tmp, "%s & %s", _tr("Save"), _tr("Exit"));
    const SDL_MessageBoxButtonData buttons[] = {
        { /* .flags, .buttonid, .text */        0, 0, tmp},
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, _tr("Exit") },
    };
    const SDL_MessageBoxData messageboxdata = {
        SDL_MESSAGEBOX_INFORMATION, /* .flags */
        NULL, /* .window */
        _tr("Exit"), /* .title */
        "", /* .message */
        SDL_arraysize(buttons), /* .numbuttons */
        buttons, /* .buttons */
        NULL //&colorScheme /* .colorScheme */
    };
    int buttonid;
    if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
        SDL_Log("error displaying message box");
    }
    if (buttonid == 0) {
        CONFIG_SaveModelIfNeeded();
        CONFIG_SaveTxIfNeeded();
    }
}

void update_mouse(int x, int y)
{
   if (x < 0)
        x = 0;
   if (x >= LCD_WIDTH)
        x = LCD_WIDTH -1;
   if (y < 0)
        y = 0;
   if (y >= LCD_HEIGHT)
        y = LCD_HEIGHT -1;
   gui.mouse = 1;
   gui.mousex = calibration.xscale * x / 0x10000 + calibration.xoffset;
   gui.mousey = calibration.yscale * y / 0x10000 + calibration.yoffset;
}

void PWR_Sleep()
{
    SDL_Event event;
    if (! SDL_WaitEvent(&event)) {
        exit(1);
    }
    switch(event.type)
    {
        case SDL_USEREVENT: {
            /* and now we can call the function we wanted to call in the timer but couldn't because of the multithreading problems */
            unsigned long e = (unsigned long)event.user.data1;
            if (e == 1) {
                ALARMhandler();
            }
            break;
        }
        case SDL_KEYDOWN: {
            SDL_Keysym k = event.key.keysym;
            int key = get_button(&k);
            if (key >= 0) {
                gui.buttons |= (1 << key);
                break;
            }
            switch(k.sym) {
            case '\'':
            case '\\':
                gui.powerdown = 1;
                break;
            case 'q':
                if(++gui.elevator > 10)
                    gui.elevator = 10;
                break;
            case 'a':
                if(--gui.elevator < 0)
                    gui.elevator = 0;
                break;
            case 'w':
                if(++gui.rudder > 10)
                    gui.rudder = 10;
                break;
            case 's':
                if(--gui.rudder < 0)
                    gui.rudder = 0;
                break;
            case 'e':
                if(++gui.throttle > 10)
                    gui.throttle = 10;
                break;
            case 'd':
                if(--gui.throttle < 0)
                    gui.throttle = 0;
                break;
            case 'r':
                if(++gui.aileron > 10)
                    gui.aileron = 10;
                break;
            case 'f':
                if(--gui.aileron < 0)
                    gui.aileron = 0;
                break;
            case 'z':
                gui.gear = (gui.gear + 1) % 6;
                break;
            case 'x':
                gui.rud_dr = (gui.rud_dr + 1) % 6;
                break;
            case 'c':
                gui.ele_dr = (gui.ele_dr + 1) % 6;
                break;
            case 'v':
                gui.ail_dr = (gui.ail_dr + 1) % 6;
                gui.dr = gui.ail_dr; /* for Devo6 */
                break;
            case 'b':
                gui.mix = (gui.mix + 1) % 6;
                break;
            case 'n':
                gui.fmod = (gui.fmod + 1) % 6;
                break;
            case 'o':
                if(++gui.aux2 > 10)
                    gui.aux2 = 10;
                break;
            case 'l':
                if(--gui.aux2 < 0)
                    gui.aux2 = 0;
                break;
            case 'p':
                if(++gui.aux3 > 10)
                    gui.aux3 = 10;
                break;
#ifdef KEYBOARD_LAYOUT_QWERTZ
            case 96: //'ö'
#else
            case ';':
#endif
                if(--gui.aux3 < 0)
                    gui.aux3 = 0;
                break;
            case 't':
                if(++gui.aux4 > 10)
                    gui.aux4 = 10;
                break;
            case 'g':
                if(--gui.aux4 < 0)
                    gui.aux4 = 0;
                break;
#ifdef KEYBOARD_LAYOUT_QWERTZ
            case 'z':
#else
            case 'y':
#endif
                if(++gui.aux5 > 10)
                    gui.aux5 = 10;
                break;
            case 'h':
                if(--gui.aux5 < 0)
                    gui.aux5 = 0;
                break;
            case 'u':
                if(++gui.aux6 > 10)
                    gui.aux6 = 10;
                break;
            case 'j':
                if(--gui.aux6 < 0)
                    gui.aux6 = 0;
                break;
            case 'i':
                if(++gui.aux7 > 10)
                    gui.aux7 = 10;
                break;
            case 'k':
                if(--gui.aux7 < 0)
                    gui.aux7 = 0;
                break;
            case 'm':
                gui.hold = (gui.hold + 1) % 6;
                break;
            case ',':
                gui.trn = (gui.trn + 1) % 6;
                break;
            default: break;
            }
            break;
        }
        case SDL_KEYUP: {
            SDL_Keysym k = event.key.keysym;
            int key = get_button(&k);
            if (key >= 0) {
                gui.buttons &= ~(1 << key);
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT)
                update_mouse(event.button.x, event.button.y);
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            if (event.button.button == SDL_BUTTON_LEFT)
                gui.mouse = 0;
            break;
        }
        case SDL_MOUSEMOTION: {
            if (! event.motion.state & SDL_BUTTON_LMASK)
                break;
            int x = event.motion.x;
            int y = event.motion.y; //FIXME - image_ypos;
            update_mouse(x, y);
            break;
        }
        case SDL_QUIT: {
            close_window();
            exit(0);
            break;
        }
    }
}

void CLOCK_StartTimer(unsigned us, u16 (*cb)(void))
{
    timer_callback = cb;
    msec_cbtime[TIMER_ENABLE] = CLOCK_getms() + us;
            // msecs + us;
    timer_enable |= 1 << TIMER_ENABLE;
}

void CLOCK_StopTimer()
{
    timer_enable &= ~(1 << TIMER_ENABLE);
}
void CLOCK_SetMsecCallback(int cb, u32 msec)
{
    msec_cbtime[cb] = CLOCK_getms() + msec;
            //msecs + msec;
    timer_enable |= 1 << cb;
    priority_ready |= 1 << cb;
}
void CLOCK_ClearMsecCallback(int cb)
{
    timer_enable &= ~(1 << cb);
}

u32 CLOCK_getms()
{
    struct timeval tp;
    u32 t;
    gettimeofday(&tp, NULL);
    t = (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
    return t;
}

void SPITouch_Calibrate(s32 xscale, s32 yscale, s32 xoff, s32 yoff)
{
    calibration.xscale = xscale;
    calibration.yscale = yscale;
    calibration.xoffset = xoff;
    calibration.yoffset = yoff;
}
struct touch SPITouch_GetCoords() {
    //struct touch t = {gui.mousex * 256 / 320, gui.mousey, 0, 0};
    struct touch t = {gui.mousex, gui.mousey, 0, 0};
    return t;
}

int SPITouch_IRQ()
{
    return gui.mouse;
}
