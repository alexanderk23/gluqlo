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
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <time.h>

#include <SDL.h>
#include <SDL_ttf.h>

#ifdef XSCREENSAVER
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#ifndef FONT
#define FONT "/usr/share/gluqlo/gluqlo.ttf"
#endif

const char* TITLE = "Gluqlo 1.2";
const int DEFAULT_WIDTH = 1024;
const int DEFAULT_HEIGHT = 768;

bool twentyfourh = true;
bool leadingzero = false;
bool fullscreen = false;
bool animate = true;
bool anykeyclose = false;

int past_h = -1, past_m = -1;

int width = DEFAULT_WIDTH;
int height = DEFAULT_HEIGHT;

TTF_Font *font_time = NULL;
TTF_Font *font_mode = NULL;

const SDL_Color FONT_COLOR = { 0xb7, 0xb7, 0xb7, 0xff };
const SDL_Color BACKGROUND_COLOR = { 0x0f, 0x0f, 0x0f, 0xff };

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *screen_texture = NULL;
SDL_Surface *screen = NULL;

#ifdef XSCREENSAVER
Display *xscr_display = NULL;
Window xscr_window = 0;
GC xscr_gc = NULL;
XImage *xscr_image = NULL;
#endif

SDL_Rect hourBackground;
SDL_Rect minBackground;

SDL_Rect bgrect;
SDL_Surface *bg;

volatile sig_atomic_t quit_flag = 0;

static void signal_handler(int sig) {
	(void)sig;
	quit_flag = 1;
}

#ifdef XSCREENSAVER
static int x_error_handler(Display *dpy, XErrorEvent *event) {
	(void)dpy;
	(void)event;
	return 0;
}
#endif

static void present_screen() {
#ifdef XSCREENSAVER
	if(xscr_display) {
		xscr_image->data = (char*)screen->pixels;
		xscr_image->bytes_per_line = screen->pitch;
		XPutImage(xscr_display, xscr_window, xscr_gc, xscr_image,
			0, 0, 0, 0, screen->w, screen->h);
		XFlush(xscr_display);
		return;
	}
#endif
	SDL_UpdateTexture(screen_texture, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

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

void render_ampm(SDL_Surface *surface, SDL_Rect *rect, int pm) {
	char mode[3];
	SDL_Rect coords;
	snprintf(mode, 3, "%cM", pm ? 'P' : 'A');
	SDL_Surface *ampm = TTF_RenderText_Blended(font_mode, mode, FONT_COLOR);
	int offset = rect->h * 0.127;
	coords.x = rect->x + rect->h * 0.07;
	coords.y = rect->y + (pm ? rect->h - offset - ampm->h : offset);
	SDL_BlitSurface(ampm, 0, surface, &coords);
	SDL_FreeSurface(ampm);
}



void blit_digits(SDL_Surface *surface, SDL_Rect *rect, int spc, char digits[], SDL_Color color) {
	SDL_Surface *glyph;
	SDL_Rect coords;

	if(digits[1]) {
		SDL_Surface *glyph0 = TTF_RenderGlyph_Blended(font_time, digits[0], color);
		SDL_Surface *glyph1 = TTF_RenderGlyph_Blended(font_time, digits[1], color);

		int total_w = glyph0->w + spc + glyph1->w;
		int start_x = rect->x + (rect->w - total_w) / 2;

		coords.x = start_x;
		coords.y = rect->y + (rect->h - glyph0->h) / 2;
		SDL_BlitSurface(glyph0, 0, surface, &coords);

		coords.x = start_x + glyph0->w + spc;
		coords.y = rect->y + (rect->h - glyph1->h) / 2;
		SDL_BlitSurface(glyph1, 0, surface, &coords);

		SDL_FreeSurface(glyph0);
		SDL_FreeSurface(glyph1);
	} else {
		glyph = TTF_RenderGlyph_Blended(font_time, digits[0], color);
		coords.x = rect->x + (rect->w - glyph->w) / 2;
		coords.y = rect->y + (rect->h - glyph->h) / 2;
		SDL_BlitSurface(glyph, 0, surface, &coords);
		SDL_FreeSurface(glyph);
	}
}


void render_digits(SDL_Surface *surface, SDL_Rect *background, char digits[], char prevdigits[], int maxsteps, int step) {
	SDL_Rect rect, dstrect;
	SDL_Color color;
	double scale;
	Uint8 c;

	bool is_h = surface->h < surface->w;
	int spc = is_h ? surface->h * .0125 : surface->w * .0125;

	// blit upper halves of current digits
	rect.x = background->x;
	rect.y = background->y;
	rect.w = background->w;
	rect.h = background->h/2;
	SDL_SetClipRect(surface, &rect);
	SDL_BlitSurface(bg, 0, surface, &rect);
	blit_digits(surface, background, spc, digits, FONT_COLOR);
	SDL_SetClipRect(surface, NULL);

	int halfsteps = maxsteps / 2;
	int upperhalf = (step+1) <= halfsteps;
	if(upperhalf) {
		scale = 1.0 - (1.0 * step) / (halfsteps - 1);
		c = 0xb7 - 0xb7 * (1.0 * step) / (halfsteps - 1);
	} else {
		scale = ((1.0 * step) - halfsteps + 1) / halfsteps;
		c = 0xb7 * ((1.0 * step) - halfsteps + 1) / halfsteps;
	}
	color.r = color.g = color.b = c;
	color.a = 0xff;

	SDL_Surface *bgcopy = SDL_ConvertSurface(bg, surface->format, 0);
	rect.x = 0;
	rect.y = 0;
	rect.w = bgcopy->w;
	rect.h = bgcopy->h;
	blit_digits(bgcopy, &rect, spc, upperhalf ? prevdigits : digits, color);

	// scale vertically using SDL_BlitScaled
	int scaled_h = (int)(bgcopy->h * scale);
	if(scaled_h < 1) scaled_h = 1;
	SDL_Surface *scaled = SDL_CreateRGBSurface(0, bgcopy->w, scaled_h,
		surface->format->BitsPerPixel,
		surface->format->Rmask, surface->format->Gmask,
		surface->format->Bmask, surface->format->Amask);

	SDL_Rect src_all = {0, 0, bgcopy->w, bgcopy->h};
	SDL_Rect dst_all = {0, 0, bgcopy->w, scaled_h};
	SDL_BlitScaled(bgcopy, &src_all, scaled, &dst_all);

	rect.x = 0;
	rect.y = upperhalf ? 0 : scaled->h / 2;
	rect.w = scaled->w;
	rect.h = scaled->h / 2;
	dstrect.x = background->x;
	dstrect.y = background->y + ( upperhalf ? ((background->h - scaled->h) / 2) : background->h / 2);
	dstrect.w = rect.w;
	dstrect.h = rect.h;
	SDL_SetClipRect(surface, &dstrect);
	SDL_BlitSurface(scaled, &rect, surface, &dstrect);
	SDL_SetClipRect(surface, NULL);
	SDL_FreeSurface(scaled);
	SDL_FreeSurface(bgcopy);

	if(!animate) return;
	// draw divider
	rect.h = (is_h ? surface->h : surface->w) * 0.005;
	rect.w = background->w;
	rect.x = background->x;
	rect.y = background->y + (background->h - rect.h) / 2;
	SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 0, 0, 0));
	rect.y += rect.h;
	rect.h = 1;
	SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 0x1a, 0x1a, 0x1a));
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
		if(leadingzero) {
			snprintf(buffer, 3, "%02d", h);
			snprintf(buffer2, 3, "%02d", past_h);
		} else {
			snprintf(buffer, 3, "%d", h);
			snprintf(buffer2, 3, "%d", past_h);
		}
		render_digits(screen, &hourBackground, buffer, buffer2, maxsteps, step);
		// draw am/pm
		if(!twentyfourh) render_ampm(screen, &hourBackground, _time->tm_hour >= 12);
	}

	// draw minutes
	if(_time->tm_min != past_m) {
		snprintf(buffer, 3, "%02d", _time->tm_min);
		snprintf(buffer2, 3, "%02d", past_m);
		render_digits(screen, &minBackground, buffer, buffer2, maxsteps, step);
	}

	present_screen();

	if(step == maxsteps-1) {
		past_h = _time->tm_hour;
		past_m = _time->tm_min;
	}
}

void render_animation() {
	if(!animate) {
		render_clock(20, 19);
		return;
	}

	const int DURATION = 260;
	int start_tick = SDL_GetTicks();
	int end_tick = start_tick + DURATION;
	int current_tick;
	int frame;
	int done = 0;

	while(!done) {
		current_tick = SDL_GetTicks();
		if(current_tick >= end_tick) {
			done = 1;
			current_tick = end_tick;
		}
		frame = 99 * (current_tick-start_tick) / (end_tick-start_tick);
		render_clock(100, frame);
	}
}

Uint32 update_time(Uint32 interval, void *param) {
	(void)param;
	SDL_Event e;
	time_t rawtime;
	struct tm *time_i;

	if(quit_flag) {
		e.type = SDL_QUIT;
		SDL_PushEvent(&e);
		return 0;
	}

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
	double display_scale_factor = 1;
	unsigned long wid = 0;

	for(int i = 1; i < argc; i++) {
		if(strcmp("--help",argv[i]) == 0 || strcmp("-help", argv[i]) == 0) {
			printf("Usage: %s [OPTION...]\nOptions:\n", argv[0]);
			printf("  -help\t\tDisplay this\n");
			printf("  -root, -f\tFullscreen\n");
			printf("  -noflip\tDisable the flip animation (change time in one frame)\n");
			printf("  -anykeyclose\tClose app when mouse move or any key pressed\n");
			printf("  -ampm\t\tUse 12-hour clock format (AM/PM)\n");
			printf("  -leadingzero\tAlways display hour with two digits\n");
			printf("  -w\t\tCustom width\n");
			printf("  -h\t\tCustom height\n");
			printf("  -r\t\tCustom resolution in WxH format\n");
			printf("  -s\t\tCustom display scale factor\n");
			return 0;
		} else if(strcmp("-root", argv[i]) == 0 || strcmp("--root", argv[i]) == 0 || strcmp("-f", argv[i]) == 0 || strcmp("--fullscreen", argv[i]) == 0) {
			fullscreen = true;
		} else if(strcmp("-noflip", argv[i]) == 0) {
			animate = false;
		} else if(strcmp("-anykeyclose", argv[i]) == 0) {
			anykeyclose = true;
		} else if(strcmp("-ampm", argv[i]) == 0) {
			twentyfourh = false;
		} else if(strcmp("-leadingzero", argv[i]) == 0) {
			leadingzero = true;
		} else if(strcmp("-r", argv[i]) == 0 || strcmp("--resolution", argv[i]) == 0) {
			if(i+1 >= argc) { fprintf(stderr, "Missing argument for %s\n", argv[i]); return 1; }
			char *resolution = argv[i+1];
			char *val = strtok(resolution, "x");
			width = atoi(val);
			val = strtok(NULL, "x");
			height = atoi(val);
			i++;
		} else if(strcmp("-w", argv[i]) == 0) {
			if(i+1 >= argc) { fprintf(stderr, "Missing argument for %s\n", argv[i]); return 1; }
			width = atoi(argv[i+1]);
			i++;
		} else if(strcmp("-h", argv[i]) == 0) {
			if(i+1 >= argc) { fprintf(stderr, "Missing argument for %s\n", argv[i]); return 1; }
			height = atoi(argv[i+1]);
			i++;
		} else if(strcmp("-s", argv[i]) == 0) {
			if(i+1 >= argc) { fprintf(stderr, "Missing argument for %s\n", argv[i]); return 1; }
			display_scale_factor = atof(argv[i+1]);
			i++;
		} else if(strcmp("-window-id", argv[i]) == 0 || strcmp("--window-id", argv[i]) == 0) {
			if(i+1 >= argc) { fprintf(stderr, "Missing argument for %s\n", argv[i]); return 1; }
			wid = strtoul(argv[i+1], (char **) NULL, 0);
			i++;
		} else {
			printf("Invalid option -- %s\n", argv[i]);
			printf("Try --help for more information.\n");
			return 0;
		}
	}

#ifdef XSCREENSAVER
	char *wid_env;
	XWindowAttributes windowAttributes;

	XSetErrorHandler(x_error_handler);

	if(wid == 0) {
		if ((wid_env = getenv("XSCREENSAVER_WINDOW")) != NULL) {
			wid = strtoul(wid_env, (char **) NULL, 0);
		}
	}
#endif

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGHUP, signal_handler);

#ifdef XSCREENSAVER
	if(wid != 0) {
		xscr_display = XOpenDisplay(NULL);
		if(xscr_display) {
			if(XGetWindowAttributes(xscr_display, (Window)wid, &windowAttributes)) {
				width = windowAttributes.width;
				height = windowAttributes.height;
				xscr_window = (Window)wid;
				xscr_gc = XCreateGC(xscr_display, xscr_window, 0, NULL);
			} else {
				XCloseDisplay(xscr_display);
				xscr_display = NULL;
				wid = 0;
			}
		} else {
			wid = 0;
		}
	}

	if(xscr_display) {
		if(SDL_Init(SDL_INIT_TIMER) < 0) {
			fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
			return 1;
		}
		xscr_image = XCreateImage(xscr_display,
			DefaultVisual(xscr_display, DefaultScreen(xscr_display)),
			DefaultDepth(xscr_display, DefaultScreen(xscr_display)),
			ZPixmap, 0, NULL, width, height, 32, 0);
		if(!xscr_image) {
			fprintf(stderr, "Unable to create XImage\n");
			if(xscr_gc) XFreeGC(xscr_display, xscr_gc);
			XCloseDisplay(xscr_display);
			return 1;
		}

		Uint32 rmask, gmask, bmask, amask;
		if(xscr_image->bits_per_pixel == 32) {
			Visual *v = DefaultVisual(xscr_display, DefaultScreen(xscr_display));
			rmask = v->red_mask;
			gmask = v->green_mask;
			bmask = v->blue_mask;
			amask = ~(rmask | gmask | bmask);
		} else {
			rmask = 0x00FF0000;
			gmask = 0x0000FF00;
			bmask = 0x000000FF;
			amask = 0xFF000000;
		}
		screen = SDL_CreateRGBSurface(0, width, height, 32,
			rmask, gmask, bmask, amask);
		if(!screen) {
			fprintf(stderr, "Unable to create screen surface\n");
			if(xscr_gc) XFreeGC(xscr_display, xscr_gc);
			XCloseDisplay(xscr_display);
			return 1;
		}
	} else
#endif
	{
		if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0) {
			fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
			return 1;
		}
		if(fullscreen) {
			window = SDL_CreateWindow(TITLE,
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
		} else {
			window = SDL_CreateWindow(TITLE,
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				width, height, SDL_WINDOW_SHOWN);
		}

		if(!window) {
			fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
			return 1;
		}

		renderer = SDL_CreateRenderer(window, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if(!renderer) {
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
		}
		if(!renderer) {
			fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
			SDL_DestroyWindow(window);
			return 1;
		}
	}
	atexit(SDL_Quit);

	if(window && (fullscreen || wid)) {
		SDL_ShowCursor(SDL_DISABLE);
	}

	int render_w, render_h;
	if(renderer) {
		SDL_GetRendererOutputSize(renderer, &render_w, &render_h);
		screen = SDL_CreateRGBSurface(0, render_w, render_h, 32,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	} else {
		render_w = width;
		render_h = height;
	}
	if(!screen) {
		fprintf(stderr, "Unable to create screen surface: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		return 1;
	}

	if(renderer) {
		screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING, render_w, render_h);
		if(!screen_texture) {
			fprintf(stderr, "Unable to create screen texture: %s\n", SDL_GetError());
			SDL_FreeSurface(screen);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			return 1;
		}
	}

	width = render_w * display_scale_factor;
	height = render_h * display_scale_factor;

	bool is_horizontal = width > height;

	TTF_Init();
	atexit(TTF_Quit);
	font_time = TTF_OpenFont(FONT, (is_horizontal ? height : width) / 1.68 );
	font_mode = TTF_OpenFont(FONT, (is_horizontal ? height : width) / 16.5);
	if (!font_time || !font_mode) {
		fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
		return 1;
	}

	// clear screen
	SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));

	// calculate box coordinates
	int rectsize;
	int spacing;
	int radius;

	if (is_horizontal) {
		rectsize = height * 0.6;
		spacing = width * .031;
		radius =  height * .05714;
	}
	else {
		rectsize = width * 0.6;
		spacing = height * .031;
		radius =  width * .05714;
	}

	int jitter_width  = 1;
	int jitter_height = 1;
	if (display_scale_factor != 1) {
		jitter_width  = (render_w - width) * 0.5;
		jitter_height = (render_h - height) * 0.5;
	}

	hourBackground.w = rectsize;
	hourBackground.h = rectsize;
	minBackground.w = rectsize;
	minBackground.h = rectsize;

	if (is_horizontal) {
		hourBackground.x = 0.5 * (width - (0.031 * width) - (1.2 * height))
										+ jitter_width;
		hourBackground.y = 0.2 * height + jitter_height;

		minBackground.x = hourBackground.x + (0.6 * height) + spacing;
		minBackground.y = hourBackground.y;
	}
	else {
		hourBackground.y = 0.5 * (height - (0.031 * height) - (1.2 * width))
										+ jitter_height;
		hourBackground.x = 0.2 * width + jitter_width;

		minBackground.y = hourBackground.y + (0.6 * width) + spacing;
		minBackground.x = hourBackground.x;
	}

	// create background surface
	bgrect.x = 0;
	bgrect.y = 0;
	bgrect.w = rectsize;
	bgrect.h = rectsize;
	bg = SDL_CreateRGBSurface(0, rectsize, rectsize, 32,
		screen->format->Rmask, screen->format->Gmask,
		screen->format->Bmask, screen->format->Amask);
	fill_rounded_box_b(bg, &bgrect, radius, BACKGROUND_COLOR);

	// draw current time
	render_clock(20, 19);

	// main loop
#ifdef XSCREENSAVER
	if(xscr_display) {
		struct timespec ts = {0, 250000000};
		while(!quit_flag) {
			time_t rawtime;
			struct tm *time_i;
			time(&rawtime);
			time_i = localtime(&rawtime);
			if(time_i->tm_min != past_m) {
				render_animation();
			}
			nanosleep(&ts, NULL);
		}
	} else
#endif
	{
		bool done = false;
		SDL_Event event;
		SDL_TimerID timer = SDL_AddTimer(60, update_time, NULL);

		int mouse_x = -1;
		int mouse_y = -1;

		while(!done && !quit_flag && SDL_WaitEvent(&event)) {
			switch(event.type) {
				case SDL_USEREVENT:
					render_animation();
					break;
				case SDL_KEYDOWN:
					if(anykeyclose){
						done = true;
						break;
					}
					switch(event.key.keysym.sym) {
						case SDLK_ESCAPE:
						case SDLK_q:
							done = true;
							break;
						default:
							break;
					}
					break;

				case SDL_MOUSEMOTION:
					if ( (mouse_x == -1) || (mouse_y == -1) )
						{
							mouse_x = event.motion.x;
							mouse_y = event.motion.y;

						}

					if(((mouse_x != event.motion.x) || (mouse_y != event.motion.y)) && anykeyclose)
						done = true;
					break;

				case SDL_QUIT:
					done = true;
					break;
			}
		}

		SDL_RemoveTimer(timer);
	}

	SDL_FreeSurface(bg);
	SDL_FreeSurface(screen);

#ifdef XSCREENSAVER
	if(xscr_display) {
		if(xscr_image) {
			xscr_image->data = NULL;
			XDestroyImage(xscr_image);
		}
		if(xscr_gc) XFreeGC(xscr_display, xscr_gc);
		XCloseDisplay(xscr_display);
	} else
#endif
	{
		if(screen_texture) SDL_DestroyTexture(screen_texture);
		if(renderer) SDL_DestroyRenderer(renderer);
		if(window) SDL_DestroyWindow(window);
	}

	TTF_CloseFont(font_time);
	TTF_CloseFont(font_mode);

	return 0;
}
