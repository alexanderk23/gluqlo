/*
* Gluqlo: Fliqlo for Linux
* https://github.com/alexanderk23/gluqlo
*
* Copyright (c) 2010-2012 Kuźniarski Jacek
* Copyright (c) 2014 Alexander Kovalenko
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <time.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_syswm.h"
#include "SDL_framerate.h"
#include "SDL_gfxPrimitives.h"
#include "SDL_rotozoom.h"

#ifndef FONT
#define FONT "/usr/share/gluqlo/gluqlo.ttf"
#endif

const char* TITLE = "Gluqlo 1.1";
const int DEFAULT_WIDTH = 1024;
const int DEFAULT_HEIGHT = 768;

bool twentyfourh = true;
bool fullscreen = false;

int past_h = -1, past_m = -1;

int width = DEFAULT_WIDTH;
int height = DEFAULT_HEIGHT;

TTF_Font *font_time = NULL;
TTF_Font *font_mode = NULL;

const SDL_Color FONT_COLOR = { 0xb7, 0xb7, 0xb7 };
const SDL_Color BACKGROUND_COLOR = { 0x0f, 0x0f, 0x0f };

SDL_Surface *screen;

SDL_Rect hourBackground;
SDL_Rect minBackground;

SDL_Rect bgrect;
SDL_Surface *bg;

// draw rounded box
// see http://lists.libsdl.org/pipermail/sdl-libsdl.org/2006-December/058868.html
void fill_rounded_box_b(SDL_Surface* dst, SDL_Rect *coords, int r, SDL_Color color) {
	Uint32 pixcolor = SDL_MapRGB(dst->format, color.r, color.g, color.b);

	int i, j;
	int rpsqrt2 = (int) (r / sqrt(2));
	int yd = dst->pitch / dst->format->BytesPerPixel;
	int w = coords->w / 2 - 1;
	int h = coords->h / 2 - 1;
	int xo = coords->x + w;
	int yo = coords->y + h;

	w -= r;
	h -= r;

	if(w <= 0 || h <= 0) return;

	SDL_LockSurface(dst);
	Uint32 *pixels = (Uint32*)(dst->pixels);

	int sy = (yo - h) * yd;
	int ey = (yo + h) * yd;
	int sx = xo - w;
	int ex = xo + w;

	for(i = sy; i <= ey; i += yd)
		for(j = sx - r; j <= ex + r; j++)
			pixels[i + j] = pixcolor;

	int d = -r;
	int x2m1 = -1;
	int y = r;

	for(int x = 0; x <= rpsqrt2; x++) {
		x2m1 += 2;
		d += x2m1;
		if(d >= 0) {
			y--;
			d -= y * 2;
		}

		for(i = sx - x; i <= ex + x; i++) {
			pixels[sy - y * yd + i] = pixcolor;
		}

		for(i = sx - y; i <= ex + y; i++) {
			pixels[sy - x * yd + i] = pixcolor;
		}

		for(i = sx - y; i <= ex + y; i++) {
			pixels[ey + x * yd + i] = pixcolor;
		}

		for(i = sx - x; i <= ex + x; i++) {
			pixels[ey + y * yd + i] = pixcolor;
		}
	}

	SDL_UnlockSurface(dst);
}

void render_ampm(SDL_Surface *surface, SDL_Rect *background, int pm) {
	char mode[3];
	SDL_Rect coords;
	snprintf(mode, 3, "%cM", pm ? 'P' : 'A');
	SDL_Surface *ampm = TTF_RenderText_Blended(font_mode, mode, FONT_COLOR);
	int offset = background->h * 0.12;
	coords.x = background->x + background->h * 0.07;
	coords.y = background->y + (pm ? background->h - offset - ampm->h : offset);
	SDL_BlitSurface(ampm, 0, surface, &coords);
	SDL_FreeSurface(ampm);
}


void render_digits(SDL_Surface *surface, SDL_Rect *background, char digits[], char prevdigits[], int maxsteps, int step) {
	SDL_Rect line;
	SDL_Rect srccoords, coords;
	SDL_Surface *glyph;

	int min_x, max_x, min_y, max_y, advance;
	int spc = surface->h * .0125;
	int adjust = digits[0] == '1'; // special case
	int center_x = 0;
	int halfsteps = maxsteps / 2;

	// draw digits
	if(digits[1]) {
		double scale;
		Uint8 c;
		SDL_Color color;
		int upperhalf = (step+1) <= halfsteps;
		if(upperhalf) {
			scale = 1.0 - (1.0 * (step+0.0) / (halfsteps-1));
			c = 0xb7 - ((1.0 * 0xb7) * (step+0.0) / (halfsteps-1));
		} else {
			scale = 1.0 * (step+1.0 - halfsteps) / halfsteps;
			c = (1.0 * 0xb7) * (step+1.0 - halfsteps) / halfsteps;
		}
		color.r = color.g = color.b = c;

		// blit upper halves of current digits
		coords.x = background->x;
		coords.y = background->y;
		coords.w = background->w;
		coords.h = background->h/2;
		SDL_SetClipRect(surface, &coords);
		// blit background
		SDL_BlitSurface(bg, 0, surface, &coords);

		if(adjust) center_x = -2.5 * spc;

		// first digit
		TTF_GlyphMetrics(font_time, digits[0], &min_x, &max_x, &min_y, &max_y, &advance);
		glyph = TTF_RenderGlyph_Blended(font_time, digits[0], FONT_COLOR);
		coords.x = background->x + background->w / 2 - max_x + min_x - spc - (adjust * spc) + center_x;
		coords.y = background->y + ((background->h - glyph->h) / 2);
		SDL_BlitSurface(glyph, 0, surface, &coords);
		SDL_FreeSurface(glyph);

		// second digit
		TTF_GlyphMetrics(font_time, digits[1], &min_x, &max_x, &min_y, &max_y, &advance);
		glyph = TTF_RenderGlyph_Blended(font_time, digits[1], FONT_COLOR);
		coords.x = background->x + background->w / 2 + spc / 2 + center_x;
		coords.y = background->y + ((background->h - glyph->h) / 2);
		SDL_BlitSurface(glyph, 0, surface, &coords);
		SDL_FreeSurface(glyph);

		SDL_SetClipRect(surface, NULL);

		// blit flipping half
		SDL_Surface *bgcopy = SDL_ConvertSurface(bg, bg->format, bg->flags);

		adjust = (upperhalf ? prevdigits[0] : digits[0]) == '1'; // special case
		if(adjust) center_x = -2.5 * spc; else center_x = 0;

		TTF_GlyphMetrics(font_time, upperhalf ? prevdigits[0] : digits[0], &min_x, &max_x, &min_y, &max_y, &advance);
		glyph = TTF_RenderGlyph_Blended(font_time, upperhalf ? prevdigits[0] : digits[0], color);
		coords.x = bg->w / 2 - max_x + min_x - spc - (adjust * spc) + center_x;
		coords.y = (bg->h - glyph->h) / 2;
		SDL_BlitSurface(glyph, 0, bgcopy, &coords);
		SDL_FreeSurface(glyph);

		TTF_GlyphMetrics(font_time, upperhalf ? prevdigits[1] : digits[1], &min_x, &max_x, &min_y, &max_y, &advance);
		glyph = TTF_RenderGlyph_Blended(font_time, upperhalf ? prevdigits[1] : digits[1], color);
		coords.x = bg->w / 2 + spc / 2 + center_x;
		coords.y = (bg->h - glyph->h) / 2;
		SDL_BlitSurface(glyph, 0, bgcopy, &coords);
		SDL_FreeSurface(glyph);

		SDL_Surface *z = zoomSurface(bgcopy, 1.0, scale, 1);
		srccoords.x = 0;
		srccoords.y = upperhalf ? 0 : z->h / 2;
		srccoords.w = z->w;
		srccoords.h = z->h / 2;
		coords.x = background->x;
		coords.y = background->y + ( upperhalf ? ((background->h - z->h) / 2) : background->h / 2);
		SDL_BlitSurface(z, &srccoords, surface, &coords);
		SDL_FreeSurface(z);
		SDL_FreeSurface(bgcopy);

	} else {
		// single digit
		if(adjust) center_x = -2.5 * spc;
		// blit background
		SDL_BlitSurface(bg, 0, surface, background);
		glyph = TTF_RenderGlyph_Blended(font_time, digits[0], FONT_COLOR);
		coords.x = background->x + background->w / 2 - glyph->w / 2 + center_x;
		coords.y = background->y + ((background->h - glyph->h) / 2);
		SDL_BlitSurface(glyph, 0, surface, &coords);
		SDL_FreeSurface(glyph);
	}

	// draw divider
	line.h = surface->h * 0.005;
	line.w = background->w;
	line.x = background->x;
	line.y = background->y + (background->h - line.h) / 2;
	SDL_FillRect(surface, &line, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
	line.y += line.h;
	line.h = 1;
	SDL_FillRect(surface, &line, SDL_MapRGBA(surface->format, 0x1a, 0x1a, 0x1a, 0));
}

void render_clock(int maxsteps, int step) {

	char buffer[3], buffer2[3];
	struct tm *_time;
	time_t rawtime;

	time(&rawtime);
	_time = localtime(&rawtime);

	// draw hours
	if(_time->tm_hour != past_h) {
		int h = twentyfourh ? _time->tm_hour : (_time->tm_hour + 11) % 12 + 1;
		snprintf(buffer, 3, "%d", h);
		snprintf(buffer2, 3, "%d", h);
		render_digits(screen, &hourBackground, buffer, buffer2, maxsteps, step);
		// draw am/pm
		if(!twentyfourh) render_ampm(screen, &hourBackground, _time->tm_hour > 12);
	}

	// draw minutes
	if(_time->tm_min != past_m) {
		snprintf(buffer, 3, "%02d", _time->tm_min);
		snprintf(buffer2, 3, "%02d", past_m);
		render_digits(screen, &minBackground, buffer, buffer2, maxsteps, step);
	}

	// flip backbuffer
	SDL_Flip(screen);

	if(step == maxsteps-1) {
		past_h = _time->tm_hour;
		past_m = _time->tm_min;
	}
}

Uint32 update_time(Uint32 interval, void *param) {
	SDL_Event e;
	time_t rawtime;
	struct tm *time_i;

	time(&rawtime);
	time_i = localtime(&rawtime);

	if(time_i->tm_min != past_m) {
		e.type = SDL_USEREVENT;
		e.user.code = 0;
		e.user.data1 = NULL;
		e.user.data2 = NULL;
		SDL_PushEvent(&e);
		interval = 1000 * (60 - time_i->tm_sec) - 250;
	} else {
		interval = 250;
	}

	return interval;
}

int main(int argc, char** argv ) {
	char *wid_env;
	static char sdlwid[100];
	Uint32 wid = 0;
	Display *display;
	XWindowAttributes windowAttributes;

	for(int i = 1; i < argc; i++) {
		if(strcmp("--help",argv[i]) == 0 || strcmp("-help", argv[i]) == 0) {
			printf("Usage: %s [OPTION...]\nOptions:\n", argv[0]);
			printf("  -help\t\tDisplay this\n");
			printf("  -root, -f\tFullscreen\n");
			printf("  -ampm\t\tUse 12-hour clock format (AM/PM)\n");
			printf("  -w\t\tCustom width\n");
			printf("  -h\t\tCustom height\n");
			printf("  -r\t\tCustom resolution in WxH format\n");
			return 0;
		} else if(strcmp("-root", argv[i]) == 0 || strcmp("-f", argv[i]) == 0 || strcmp("--fullscreen", argv[i]) == 0) {
			fullscreen = true;
		} else if(strcmp("-ampm", argv[i]) == 0) {
			twentyfourh = false;
		} else if(strcmp("-r", argv[i]) == 0 || strcmp("--resolution", argv[i]) == 0) {
			char *resolution = argv[i+1];
			char *val = strtok(resolution, "x");
			width = atoi(val);
			val = strtok(NULL, "x");
			height = atoi(val);
			i++;
		} else if(strcmp("-w", argv[i]) == 0) {
			width = atoi(argv[i+1]);
			i++;
		} else if(strcmp("-h", argv[i]) == 0) {
			height = atoi(argv[i+1]);
			i++;
		} else if(strcmp("-window-id", argv[i]) == 0) {
			wid = strtol(argv[i+1], (char **) NULL, 0);
			i++;
		} else {
			printf("Invalid option -- %s\n", argv[i]);
			printf("Try --help for more information.\n");
			return 0;
		}
	}

	/* If no window argument, check environment */
	if(wid == 0) {
		if ((wid_env = getenv("XSCREENSAVER_WINDOW")) != NULL ) {
			wid = strtol(wid_env, (char **) NULL, 0); /* Base 0 autodetects hex/dec */
		}
	}

	/* Get win attrs if we've been given a window, otherwise we'll use our own */
	if(wid != 0) {
		if ((display = XOpenDisplay(NULL)) != NULL) { /* Use the default display */
			XGetWindowAttributes(display, (Window) wid, &windowAttributes);
			XCloseDisplay(display);
			snprintf(sdlwid, 100, "SDL_WINDOWID=0x%X", wid);
			putenv(sdlwid); /* Tell SDL to use this window */
			width = windowAttributes.width;
			height = windowAttributes.height;
		}
	}

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);

	if(fullscreen && (!wid)) {
		screen = SDL_SetVideoMode(0, 0, 32, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN);
	} else {
		screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE|SDL_DOUBLEBUF);
	}

	if (!screen) {
		fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		return 1;
	}

	if(fullscreen || wid) {
		SDL_ShowCursor(SDL_DISABLE);
	}

	SDL_WM_SetCaption(TITLE, TITLE);

	width = screen->w;
	height = screen->h;

	TTF_Init();
	atexit(TTF_Quit);
	font_time = TTF_OpenFont(FONT, height / 1.68);
	font_mode = TTF_OpenFont(FONT, height / 16.5);
	if (!font_time || !font_mode) {
		fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
		return 1;
	}

	// clear screen
	SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));

	// calculate box coordinates
	int rectsize = height * 0.6;
	int spacing = width * .031;
	int radius =  height * .05714;

	hourBackground.x = 0.5 * (width - (0.031 * width) - (1.2 * height));
	hourBackground.y = 0.2 * height;
	hourBackground.w = rectsize;
	hourBackground.h = rectsize;

	minBackground.x = hourBackground.x + (0.6 * height) + spacing;
	minBackground.y = hourBackground.y;
	minBackground.w = rectsize;
	minBackground.h = rectsize;

	// create background surface
	bgrect.x = 0;
	bgrect.y = 0;
	bgrect.w = rectsize;
	bgrect.h = rectsize;
	bg = SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCALPHA, rectsize, rectsize, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	fill_rounded_box_b(bg, &bgrect, radius, BACKGROUND_COLOR);

	// draw current time
	render_clock(20, 19);

	// main loop
	bool done = false;
	SDL_Event event;
	SDL_TimerID timer = SDL_AddTimer(60, update_time, NULL);
	int maxsteps = 12;

	FPSmanager fpsManager;
	SDL_initFramerate(&fpsManager);
	SDL_setFramerate(&fpsManager, 50);

	while(!done && SDL_WaitEvent(&event)) {
		switch(event.type) {
			case SDL_USEREVENT:
				for(int s=0; s<maxsteps; s++) {
					render_clock(maxsteps,s);
					SDL_framerateDelay(&fpsManager);
				}
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE:
					case SDLK_q:
						done = true;
					default:
						break;
				}
				break;
			case SDL_QUIT:
				done = true;
				break;
		}
	}

	SDL_RemoveTimer(timer);

	SDL_FreeSurface(bg);
	SDL_FreeSurface(screen);

	TTF_CloseFont(font_time);
	TTF_CloseFont(font_mode);

	return 0;
}
