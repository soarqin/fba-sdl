/*
 * FinalBurn Alpha for Dingux/OpenDingux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <algorithm>
#include <map>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "burner.h"
#include "sdl_run.h"
#include "gui_main.h"
#include "gui_gfx.h"
#include "gui_romlist.h"
#include "gui_config.h"
#include "gui_setpath.h"
#include "gui_language_pack.h"

#define ROMLIST(A,B) romlist.A[gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone)[B]]
#define GUI_SCREEN_W 320
#define GUI_SCREEN_H 240

SDL_Event event;

SDL_Surface *gui_screen = NULL;
SDL_Surface *bg = NULL;
SDL_Surface *bgs = NULL;
SDL_Surface *bg_temp = NULL;
SDL_Surface *font = NULL;
SDL_Surface *barre = NULL;
SDL_Surface *barre18px = NULL;
SDL_Surface *preview = NULL;
SDL_Surface *title = NULL;
SDL_Surface *help = NULL;
SDL_Surface *credit = NULL;
SDL_Surface *Tmp = NULL;
static SDL_Surface * lang_fontsurf = NULL;

FILE *fp;
FILE *fp2;

unsigned int font6x[255];
int i;
unsigned char flag_preview;
char g_string[255];
char g_message[255];
unsigned char offset_x, offset_y;
bool use_language_pack = false;
typedef struct lang_t{
	uint32_t gamelist_line_height;
	uint32_t gamelist_line_count;
	uint32_t gamelist_line_count_half;
} lang_t;

lang_t gui_lang = {LINE_HEIGHT, LINES_COUNT, LINES_COUNT_HALF};

typedef struct selector {
	int y;
	int x;
	int rom;
	int ofs;
} SELECTOR;

SELECTOR sel;

unsigned char joy_speed[4]={0,1,3,7};

static char *abreviation_cf[11][7]={
	{"Disable","Enable","","","","",""},
	{"Original","Fullscreen","","","","",""},
	{"Off","Auto","Manual","","","",""},
	{"C68k","M68k","A68k","","","",""},
	{"CZ80","MAME Z80","","","","",""},
	{"Off","On","","","","",""},
	{"Off","LIBAO","SDL","SDL old","","",""},
	{"11025", "16000", "22050", "32000", "44100", "", ""},
	{"No","Yes","","","","",""},
	{"No","-90","-180","-270","","",""},
	{"Off","Aspect","Fullscr","","","",""}
};

void load_lastsel();
void save_lastsel();

void redraw_screen(void)
{
	SDL_Delay(16);
	SDL_Flip(gui_screen);
}

void free_memory(void)
{
	printf("Freeing surfaces\n");
	SDL_FreeSurface(gui_screen);
	SDL_FreeSurface(bg);
	SDL_FreeSurface(bgs);
	SDL_FreeSurface(bg_temp);
	SDL_FreeSurface(font);
	SDL_FreeSurface(barre);
	SDL_FreeSurface(preview);
	SDL_FreeSurface(title);
	SDL_FreeSurface(help);
	SDL_FreeSurface(credit);
	if(lang_fontsurf) SDL_FreeSurface(lang_fontsurf);
	if(barre18px) SDL_FreeSurface(barre18px);
	//printf("Freeing memory\n");
	/*for (ii=0;ii<romlist.nb_list[0];++ii){
		free(romlist.name[ii]);
		free(romlist.zip[ii]);
		free(romlist.manufacturer[ii]);
		free(romlist.parent[ii]);
	}*/
}

int exit_prog(void)
{
	//menage avant execution
	free_memory();

	SDL_JoystickClose(0);
	//SDL_Quit();

	return 0;

}

void put_string(char *string, unsigned int pos_x, unsigned int pos_y, unsigned char couleur, SDL_Surface *s)
{
	SDL_Rect Src;
	SDL_Rect Dest;

	Src.y = couleur;
	Src.w = 5;
	Src.h = 9;
	Dest.y = pos_y;

	while(*string)
	{
		if (font6x[*string]){
			Src.x = font6x[*string];
			Dest.x = pos_x;
			SDL_BlitSurface(font, &Src, s, &Dest);
		}
		++string;
		pos_x += 6;
	}
}

void put_stringM(char *string, unsigned int pos_x, unsigned int pos_y, unsigned int taille, unsigned char couleur)
{
	SDL_Rect Src;
	SDL_Rect Dest;
	unsigned char caratere;

	if( taille > sel.x ){

		string += sel.x ;

		Src.y = couleur;
		Src.w = 5;
		Src.h = 9;
		Dest.y = pos_y;


		if ( (taille-sel.x) > 52 ) {
			for( caratere=sel.x ; caratere<(sel.x+48) ; ++caratere)
			{
				if (font6x[*string]){
					Src.x = font6x[*string];
					Dest.x = pos_x;
					SDL_BlitSurface(font, &Src, gui_screen, &Dest);
				}
				++string;
				pos_x += 6;
			}
			for( caratere=0 ; caratere<3 ; ++caratere)
			{
				if (font6x[*string]){
					Src.x = font6x[46];
					Dest.x = pos_x;
					SDL_BlitSurface(font, &Src, gui_screen, &Dest);
				}
				pos_x += 6;
			}
		}else{
			while(*string)
			{
				if (font6x[*string]){
					Src.x = font6x[*string];
					Dest.x = pos_x;
					SDL_BlitSurface(font, &Src, gui_screen, &Dest);
				}
				++string;
				pos_x += 6;
			}
		}
	}
}

static std::unordered_map<uint32_t, uint32_t> idx2RGBAcolor = {
	{BLANC,		0xFFFFFFFF}, // white
	{ROUGE,		0xFF0000FF}, // red
	{ORANGE,	0xFF00A5FF},
	{JAUNE,		0xFF00FFFF}, // yellow
	{VERT,		0xFF00FF00}, // green
	{BLEU,		0xFFFF0000}  // blue
};

static inline bool prepare_fontsurf()
{
	// RGBA 32 bits
	if(!lang_fontsurf) {
		lang_fontsurf = SDL_CreateRGBSurface(0, GUI_SCREEN_W, GUI_SCREEN_H,
											 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		if(!lang_fontsurf) return false;
	}
	SDL_FillRect(lang_fontsurf, NULL, RGBA_TRANSPARENT);
	return true;
}

void lang_put_stringM( uint32_t romID, unsigned int pos_x, unsigned int pos_y, unsigned char coloridx )
{
	if(romID>=nBurnDrvCount || pos_x<0 || pos_y<0 || !idx2RGBAcolor.count(coloridx))
		return;

	lang_DrawString( romID, (uint32_t *)lang_fontsurf->pixels, pos_x, pos_y, GUI_SCREEN_W, idx2RGBAcolor[coloridx] );
}

static inline void lang_show_gamelist_lines()
{
	if( lang_fontsurf )
		SDL_BlitSurface(lang_fontsurf, NULL, gui_screen, NULL);
}

/*void show_screen(char * text)
{
	#define CREDIT_X	68
	#define CREDIT_Y	62
	#define CREDIT_L	184
	#define CREDIT_H	116
	Uint32 Pnoir = SDL_MapRGB(credit->format, 0, 0, 0);
	Uint32 Pblanc = SDL_MapRGB(credit->format, 255, 255, 255 );
	//capture gui_screen actuel
	drawSprite( gui_screen , credit , 0 , 0 , 0 , 0 , 320 , 240 );
	//detourage
	ligneV( credit , CREDIT_X-1 , CREDIT_Y , CREDIT_H , Pnoir );
	ligneV( credit , CREDIT_X+CREDIT_L , CREDIT_Y , CREDIT_H , Pnoir );
	ligneH( credit , CREDIT_X , CREDIT_Y-1 , CREDIT_L , Pnoir);
	ligneH( credit , CREDIT_X , CREDIT_Y+CREDIT_H , CREDIT_L , Pnoir);
	//cadre
	carre_plein( credit , CREDIT_X, CREDIT_Y, CREDIT_L, CREDIT_H, Pblanc);
	carre_plein( credit , CREDIT_X+1, CREDIT_Y+1, CREDIT_L-2, CREDIT_H-2, Pnoir);

	put_string( "Set Keys" , 142 , CREDIT_Y+8 , BLANC , credit );
	put_string( text , CREDIT_X+8 , CREDIT_Y+24 , BLEU , credit );
}*/
int last_numero = -1;

void load_preview(unsigned int numero)
{
	if (romlist.nb_list[cfg.list] == 0) {
		flag_preview = 0;
		return;
	}
	if(preview) {
		if(last_numero==numero)
			return;
		SDL_FreeSurface(preview);
		preview = NULL;
	}
	last_numero = numero;

	char *ext[2] = {"png", "bmp"};
	FILE *fp;

	for(int i = 0; i < 2; i++) {
		sprintf((char*)g_string, "%s/%s.%s", szAppPreviewPath, ROMLIST(zip, numero), ext[i]);

		// check first current rom
		if((preview = IMG_Load(g_string)) != NULL) {
			drawSprite(bg, bg_temp, 124, 3, 124, 3, 192, 112);
			drawSprite(preview, bg_temp, 0, 0, 220 - preview->w / 2, 3, 192, 112);

			flag_preview = 1;
			return;
		} 

		// then check parent rom
		if(strcmp(ROMLIST(parent, numero), "fba") != 0) {
			sprintf((char*)g_string, "%s/%s.%s", szAppPreviewPath, ROMLIST(parent, numero), ext[i]);

			if((preview = IMG_Load(g_string)) != NULL) {
				drawSprite(bg, bg_temp, 124, 3, 124, 3, 192, 112);
				drawSprite(preview, bg_temp, 0, 0, 220 - preview->w / 2, 3, 192, 112);

				flag_preview = 1;
				return;
			} 
		}
		if(0==i) for(int j=9; j>=0; --j) {
			sprintf((char*)g_string, "%s/%s%ip.png", szAppSavePath, ROMLIST(zip, numero),j);
			if((preview = IMG_Load(g_string)) != NULL) {
				drawSprite(bg, bg_temp, 124, 3, 124, 3, 192, 112);
				drawSprite(preview, bg_temp, 0, 0, 220 - preview->w / 2, 3, 192, 112);

				flag_preview = 1;
				return;
			}
		}
	}

	// nothing is found
	drawSprite(bg, bg_temp, 124, 3, 124, 3, 192, 112);

	sprintf((char*)g_string, "PREVIEW %s.bmp", ROMLIST(zip, numero));
	put_string(g_string, 160, 49, ROUGE, bg_temp);
	put_string("NOT AVAILABLE", 181, 59, ROUGE, bg_temp);

	flag_preview = 0;
}

char ss_prg_credit(void)
{
	#define CREDIT_X	68
	#define CREDIT_Y	62
	#define CREDIT_L	184
	#define CREDIT_H	116

	unsigned int compteur = 1;
	Uint32 Pnoir = SDL_MapRGB(credit->format, 0, 0, 0);
	Uint32 Pblanc = SDL_MapRGB(credit->format, 255, 255, 255);

	drawSprite(gui_screen, credit, 0, 0, 0, 0, 320, 240);

	ligneV(credit, CREDIT_X - 1, CREDIT_Y, CREDIT_H, Pnoir);
	ligneV(credit, CREDIT_X + CREDIT_L, CREDIT_Y, CREDIT_H, Pnoir);
	ligneH(credit, CREDIT_X, CREDIT_Y - 1, CREDIT_L, Pnoir);
	ligneH(credit, CREDIT_X, CREDIT_Y + CREDIT_H, CREDIT_L, Pnoir);

	carre_plein( credit , CREDIT_X, CREDIT_Y, CREDIT_L, CREDIT_H, Pblanc);
	carre_plein( credit , CREDIT_X+1, CREDIT_Y+1, CREDIT_L-2, CREDIT_H-2, Pnoir);

	put_string("CREDIT", 142, CREDIT_Y + 8, BLANC, credit);
	put_string("Final Burn Alpha v" VERSION, CREDIT_X + 8, CREDIT_Y + 24, BLEU, credit);
	put_string("for OpenDingux/RG350", CREDIT_X + 28, CREDIT_Y + 34, BLEU, credit);
	put_string("Frontend is based on Capex", CREDIT_X + 8, CREDIT_Y + 54, VERT, credit);
	put_string("by JyCet and Juanvvc", CREDIT_X + 32, CREDIT_Y + 64, VERT, credit);
	put_string("Skin by HiBan", CREDIT_X + 56, CREDIT_Y + 88, VERT, credit);

	put_string("Press (SELECT) to quit", 85, CREDIT_Y + 100, BLANC, credit);

	while(1)
	{
		drawSprite( credit , gui_screen , 0 , 0 , 0 , 0 , 320 , 240 );
		redraw_screen();

		SDL_PollEvent(&event);
		if(event.type == SDL_KEYDOWN) {
			if(compteur == 0 || (compteur > cfg.delayspeed && ((compteur & joy_speed[cfg.repeatspeed]) == 0))) {
				if(event.key.keysym.sym == SDLK_ESCAPE ) {
					return 1;
				} else return 0;
			}
			++compteur;
		}else if(event.type == SDL_KEYUP) {
			compteur=0;
		}
	}
}

void ss_prg_help(void)
{
	#define HELP_X  20
	#define HELP_Y	32
	#define HELP_L	280
	#define HELP_H	176

	unsigned int compteur = 1;
	Uint32 Pnoir = SDL_MapRGB(help->format, 0, 0, 0);
	Uint32 Pblanc = SDL_MapRGB(help->format, 255, 255, 255);

	drawSprite(gui_screen, help, 0, 0, 0, 0, 320, 240);

	ligneV(help, HELP_X - 1, HELP_Y , HELP_H , Pnoir);
	ligneV(help, HELP_X + HELP_L, HELP_Y, HELP_H, Pnoir);
	ligneH(help, HELP_X, HELP_Y - 1, HELP_L, Pnoir);
	ligneH(help, HELP_X, HELP_Y + HELP_H, HELP_L, Pnoir);

	carre_plein(help, HELP_X, HELP_Y, HELP_L, HELP_H, Pblanc);
	carre_plein(help, HELP_X + 1, HELP_Y + 1, HELP_L - 2, HELP_H - 2, Pnoir);

	put_string("COLOR HELP", 130, HELP_Y + 8, VERT, help);
	put_string("RED      missing", HELP_X + 82, HELP_Y + 22, ROUGE, help);
	put_string("YELLOW   parent rom", HELP_X + 82, HELP_Y + 32, JAUNE, help);
	put_string("ORANGE   clone rom", HELP_X + 82, HELP_Y + 42, ORANGE, help);
	//put_string("         & parent detected", HELP_X + 8, HELP_Y + 54, JAUNE, help);
	//put_string("GREEN    clone & parent & cache", HELP_X + 8, HELP_Y + 64, VERT, help);
	//put_string("         detected", HELP_X + 8, HELP_Y + 74, VERT, help);
	//put_string("BLUE     parent & cache detected", HELP_X + 8, HELP_Y + 84, BLEU, help);
	put_string("WHITE    selected flag", HELP_X + 82, HELP_Y + 52, BLANC, help);
	
	put_string("HOTKEY HELP", 130, HELP_Y + 72, VERT, help);
#ifdef GCW0_BTN_LAYOUT
#define HELP_X_LEFTRIGHT	"X+LEFT and X+RIGHT   filter by hardware"
#define HELP_X_UPDOWN		"X+UP and X+DOWN      filter by availability"
#define HELP_X_LR			"X+L and X+R          filter by genre"
#define HELP_X_SELETE		"X+SELECT    set/unset selected flag to ROM"
#define HELP_X_A			"X+A     add selected ROM to favorites list"
#define HELP_X_START		"X+START NO comfirm delete all selected ROM"
#else
#define HELP_X_LEFTRIGHT	"Y+LEFT and Y+RIGHT   filter by hardware"
#define HELP_X_UPDOWN		"Y+UP and Y+DOWN      filter by availability"
#define HELP_X_LR			"Y+L and Y+R          filter by genre"
#define HELP_X_SELETE		"Y+SELECT    set/unset selected flag to ROM"
#define HELP_X_A			"Y+A     add selected ROM to favorites list"
#define HELP_X_START		"Y+START NO comfirm delete all selected ROM"
#endif
	put_string(HELP_X_LEFTRIGHT, HELP_X + 10, HELP_Y + 86, BLEU, help);
	put_string(HELP_X_UPDOWN, HELP_X + 10, HELP_Y + 96, BLEU, help);
	put_string(HELP_X_LR, HELP_X + 10, HELP_Y + 106, BLEU, help);
	put_string(HELP_X_SELETE, HELP_X + 10, HELP_Y + 116, BLANC, help);
	put_string(HELP_X_A, HELP_X + 10, HELP_Y + 126, BLEU, help);
	put_string(HELP_X_START, HELP_X + 10, HELP_Y + 136, ROUGE, help);

	put_string("Any button to return", 100, HELP_Y + 156, VERT, help);


	int Hquit = 0;
	while(!Hquit) {
		drawSprite(help, gui_screen, 0, 0, 0, 0, 320, 240);
		redraw_screen();

		SDL_PollEvent(&event);
		if (event.type==SDL_KEYDOWN){
			//if (compteur==0 || (compteur>cfg.delayspeed && ((compteur&joy_speed[cfg.repeatspeed])==0))){
				//if ( event.key.keysym.sym==SDLK_A ){
				if ( event.key.keysym.sym > 0)
					if (compteur==0) Hquit = 1 ;
				//}
			//}
			++compteur;
		}else if (event.type==SDL_KEYUP){
			compteur = 0;
		}
	}
}

void init_title(void)
{
	assert(!bg && !barre && !barre18px && !title && !font);

	if( use_language_pack ) {
		char fbapic[MAX_PATH];
		sprintf(fbapic, "%s/lang/skin/fba_bg.png", szAppHomePath);
		bg = IMG_Load(fbapic);
		sprintf(fbapic, "%s/lang/skin/fba_title.png", szAppHomePath);
		title = IMG_Load(fbapic);
		sprintf(fbapic, "%s/lang/skin/fba_selector_18px.png", szAppHomePath);
		barre18px = IMG_Load(fbapic);
	}
	if(!bg) bg = IMG_Load("./skin/fba_bg.png");
	if(!title) title = IMG_Load("./skin/fba_title.png");
	barre = IMG_Load("./skin/fba_selector.png");

	font = IMG_Load("./skin/fba_font.png");

	bg_temp = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0xf800, 0x07e0, 0x001f, 0x0000);
	bgs = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0xf800, 0x07e0, 0x001f, 0x0000);
	help = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0xf800, 0x07e0, 0x001f, 0x0000);
	credit = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0xf800, 0x07e0, 0x001f, 0x0000);

	drawSprite( bg , bgs , 0 , 0 , 0 , 0 , 320 , 240 );
	if (cfg.FXshadow < 100) gui_set_gamma(bgs, cfg.FXshadow);

	preparation_fenetre( bgs , bg , 124 , 3 , 192 , 112 );
	preparation_fenetre( bgs , bg , 4 , 53 , 116 , 62 );

	drawSprite( title , bg, 0, 0, 0, 0, 123, 52);

	// precalculate font6 x coordinate
	for(i = 0; i < 32; ++i) font6x[i] = 0;
	for(i = 32; i < 255; ++i) font6x[i] = (i - 32) * 6;
}

void prep_bg()
{
	drawSprite(bg, bg_temp, 0, 0, 0, 0, 320, 240);
	if( use_language_pack ) {
		const int win_x=4, win_y=119, win_l=312, win_h=121;
		Uint32 Pnoir = SDL_MapRGB(bg_temp->format, 0, 0, 0);
		drawSprite(bgs, bg_temp, win_x, win_y, win_x, win_y, win_l, win_h);
		ligneV(bg_temp, win_x-1, win_y, win_h, Pnoir);
		ligneV(bg_temp, win_x+win_l, win_y, win_h, Pnoir);
		ligneH(bg_temp, win_x, win_y-1, win_l, Pnoir);
	} else {
		preparation_fenetre(bgs, bg_temp, 4, 119, 312, 118);
	}
	if(flag_preview && romlist.nb_list[cfg.list] != 0) {
		drawSprite(preview, bg_temp, 0, 0, 220 - preview->w / 2, 3, 192, 112);
	}

	sprintf((char*)g_string, "%s: %d roms", filter[cfg.list].label, unfiltered_nb_list[cfg.list]);
	put_string( g_string , 6 , 95 , BLANC , bg_temp );
	
	sprintf((char*)g_string, "Show: %d roms", romlist.nb_list[cfg.list]);
	put_string( g_string , 6 , 105 , BLANC , bg_temp );
}

void prep_bg_main(void)
{
	gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone); // update romlist counters
	prep_bg();
	sprintf((char*)g_string, "Hardware %s; Genre: %s", hardwares[cfg.hardware].label, genres[cfg.genre].label);
	put_string( g_string , 6 , 120 , BLEU , bg_temp );
}

void prep_bg_opt(int first)
{
	gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone); // update romlist counters
	if (g_message[0] != 0) {
		prep_bg();
		put_string( g_message , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_MAIN_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "Main settings:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_FILTER_FIRST) {
		prep_bg_main();
	} else if (first == OPTION_PATHS_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "ROMs Paths:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_DEF_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "Default ROM settings:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_DEF_KEY_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "Default Key mappings:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_DEF_AF_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "Default Autofire settings:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_DEF_RUN_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "Default Run Game settings:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_DEF_VKEY_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "Default Vertical Key mappings:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_DEF_VAF_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "Default Vertial Autofire settings:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	} else if (first == OPTION_DEF_VRUN_FIRST) {
		prep_bg();
		sprintf((char*)g_string, "Default Vertial Run Game settings:");
		put_string( g_string , 6 , 120 , BLEU , bg_temp );
	}
}

void prep_bg_run(void)
{
	prep_bg();
	preparation_fenetre(bgs, bg_temp, 4, 119, 260, 118);
	preparation_fenetre(bgs, bg_temp, 269, 119, 47, 118);
}

void affiche_BG(int rom = -1)
{
	drawSprite(bg_temp, gui_screen, 0, 0, 0, 0, 320, 240);

	if (romlist.nb_list[cfg.list] == 0) {
		return;
	}

	if (rom == -1) {
		rom = gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone)[sel.rom];
	}

	sprintf((char*)g_string, "Rom: %s", romlist.zip[rom]);
	put_string(g_string, 6, 55, BLANC, gui_screen);

	if(strcmp(romlist.parent[rom], "fba") == 0) {
		//put_string("Parent rom", 6, 65, BLANC, gui_screen);
	} else {
		sprintf((char*)g_string, "Clone of %s", romlist.parent[rom]);
		put_string(g_string, 6, 65, BLANC, gui_screen);
	}

	if(romlist.year[rom] != NULL) {
		put_string(romlist.year[rom], 6, 75, BLANC, gui_screen);
	}

	if(romlist.manufacturer[rom] != NULL) {
		put_string(romlist.manufacturer[rom], 6, 85, BLANC, gui_screen);
	}

}

std::vector<int> gui_menu_key_values;
std::map<int, char *> gui_menu_key_labels;
std::vector<int> gui_menu_fps_values;
std::map<int, char *> gui_menu_fps_labels;

void gui_menu_key_init()
{
	if (gui_menu_key_values.size() != 0) {
		return;
	}

	gui_menu_key_values.push_back(SDLK_LCTRL);
	gui_menu_key_values.push_back(SDLK_LALT);
	gui_menu_key_values.push_back(SDLK_SPACE);
	gui_menu_key_values.push_back(SDLK_LSHIFT);
	gui_menu_key_values.push_back(SDLK_TAB);
	gui_menu_key_values.push_back(SDLK_BACKSPACE);
	gui_menu_key_labels[SDLK_LCTRL]     = "A";
	gui_menu_key_labels[SDLK_LALT]      = "B";
#ifdef GCW0_BTN_LAYOUT
	gui_menu_key_labels[SDLK_SPACE]     = "Y";
	gui_menu_key_labels[SDLK_LSHIFT]    = "X";
#else
	gui_menu_key_labels[SDLK_SPACE]     = "X";
	gui_menu_key_labels[SDLK_LSHIFT]    = "Y";
#endif
	gui_menu_key_labels[SDLK_TAB]       = "L";
	gui_menu_key_labels[SDLK_BACKSPACE] = "R";
}
void gui_menu_fps_init()
{
	if (gui_menu_fps_values.size() != 0) {
		return;
	}

	gui_menu_fps_values.push_back(0);
	gui_menu_fps_values.push_back(6);
	gui_menu_fps_values.push_back(10);
	gui_menu_fps_values.push_back(16);
	gui_menu_fps_values.push_back(30);
	gui_menu_fps_labels[0]  = "off";
	gui_menu_fps_labels[6]  = "6 fps";
	gui_menu_fps_labels[10] = "10 fps";
	gui_menu_fps_labels[16] = "16 fps";
	gui_menu_fps_labels[30] = "30 fps";
}
int gui_menu_get_next(std::vector<int>& values, int current)
{
	int result = current;

	std::vector<int>::iterator it = std::find(values.begin(), values.end(), current);
	if (it != values.end()) {
		it++;
		if (it == values.end()) {
			it = values.begin();
		}
		result = *it;
	}

	return result;
}

int gui_menu_get_prev(std::vector<int>& values, int current)
{
	int result = current;

	std::vector<int>::iterator it = std::find(values.begin(), values.end(), current);
	if (it != values.end()) {
		if (it == values.begin()) {
			it = values.end();
		}
		it--;
		result = *it;
	}

	return result;
}

void gui_clear_last_romlist()
{
	char filename[MAX_PATH];
	sprintf(filename, "%s/lastromlist.txt", szAppHomePath);
	remove(filename);
}

void put_option_line(int first, unsigned char num, unsigned char y)
{
	switch(first + num) {
	// MAIN OPTIONS
	case OPTION_GUI_ROMLIST_CACHE_CLEAR:
		put_string( "Clear ROMs list cache and rescaning" , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DELAYSPEED:
		sprintf((char*)g_string, "Keyrepeat delay: %d" , cfg.delayspeed );
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_REPEATSPEED:
		sprintf((char*)g_string, "Keyrepeat speed: %d" , cfg.repeatspeed );
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_FILTER:
		sprintf((char*)g_string, "Filter settings -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_ROM_PATHS:
		sprintf((char*)g_string, "ROMs Paths -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEFAULTS:
		sprintf((char*)g_string, "Default ROM settings -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_SHADOW:
		sprintf((char*)g_string, "Background shadow: %d%c" , cfg.FXshadow, 37 );
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_SKIN:
		if (cfg.skin) put_string( "External skin: Enable" , OPTIONS_START_X , y , BLANC , gui_screen );
		else put_string( "External skin: Disable" , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_FAV_CLEAR:
		put_string( "Clear missing roms in favorites list" , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_RETURN:
		put_string( "Return to the rom list" , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	// FILTER OPTIONS
	case OPTION_GUI_FILTER_LIST:
		sprintf((char*)g_string, "Show roms: %s only",  filter[cfg.list].label );
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_FILTER_HARDWARE:
		sprintf((char*)g_string, "Hardware: %s",  hardwares[cfg.hardware].label);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_FILTER_GENRE:
		sprintf((char*)g_string, "Genre: %s", genres[cfg.genre].label);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_FILTER_CLONE:
		sprintf((char*)g_string, "Show clones: %s", clones[cfg.clone].label);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_FILTER_RETURN:
		put_string( "Return to the main settings" , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	// PATHS OPTIONS
	case OPTION_GUI_PATH0:
	case OPTION_GUI_PATH1:
	case OPTION_GUI_PATH2:
	case OPTION_GUI_PATH3:
	case OPTION_GUI_PATH4:
	case OPTION_GUI_PATH5:
	case OPTION_GUI_PATH6:
	case OPTION_GUI_PATH7:
	case OPTION_GUI_PATH8:
	case OPTION_GUI_PATH9:
	case OPTION_GUI_PATH10:
	case OPTION_GUI_PATH11:
	case OPTION_GUI_PATH12:
	case OPTION_GUI_PATH13:
	case OPTION_GUI_PATH14:
	case OPTION_GUI_PATH15:
	case OPTION_GUI_PATH16:
	case OPTION_GUI_PATH17:
	case OPTION_GUI_PATH18:
	case OPTION_GUI_PATH19:
		sprintf((char*)g_string, "Rom path %d: %s" , first + num - OPTION_GUI_PATH0 + 1,szAppRomPaths[first + num - OPTION_GUI_PATH0]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_PATH_RETURN:
		put_string( "Return to the main settings" , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	// DEFAULTS
	case OPTION_GUI_DEF_KEYS:
		sprintf((char*)g_string, "Default Key mappings -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_AUTOFIRE:
		sprintf((char*)g_string, "Default Autofire settings -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RUN:
		sprintf((char*)g_string, "Default Run Game settings -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_VKEYS:
		sprintf((char*)g_string, "Default Vertical Key mappings -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_VAUTOFIRE:
		sprintf((char*)g_string, "Default Vertical Autofire settings -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_VRUN:
		sprintf((char*)g_string, "Default Vertical Run Game settings -->");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RESET:
		sprintf((char*)g_string, "Reset settings");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_DELETE:
		sprintf((char*)g_string, "Delete all saved ROM configs");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RETURN:
		sprintf((char*)g_string, "Return to the main settings");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	// KEY MAPPINGS
	case OPTION_GUI_DEF_KEY1:
	case OPTION_GUI_DEF_KEY2:
	case OPTION_GUI_DEF_KEY3:
	case OPTION_GUI_DEF_KEY4:
	case OPTION_GUI_DEF_KEY5:
	case OPTION_GUI_DEF_KEY6:
		{
			int idx = first + num - OPTION_GUI_DEF_KEY1;
			sprintf((char*)g_string, "Fire %d: %s", idx + 1, gui_menu_key_labels[(&keymap.fire1)[idx]]);
			put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		}
		break;
	case OPTION_GUI_DEF_KEY_RETURN:
		sprintf((char*)g_string, "Return back");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	// AUTOFIRE SETTINGS
	case OPTION_GUI_DEF_AF_FPS1:
	case OPTION_GUI_DEF_AF_FPS2:
	case OPTION_GUI_DEF_AF_FPS3:
	case OPTION_GUI_DEF_AF_FPS4:
	case OPTION_GUI_DEF_AF_FPS5:
	case OPTION_GUI_DEF_AF_FPS6:
		{
			int idx = (first + num - OPTION_GUI_DEF_AF_FPS1) / 2;
			sprintf((char*)g_string, "Autofire %d fps: %s", idx + 1, gui_menu_fps_labels[(&autofire.fire1)[idx].fps]);
			put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		}
		break;
	case OPTION_GUI_DEF_AF_KEY1:
	case OPTION_GUI_DEF_AF_KEY2:
	case OPTION_GUI_DEF_AF_KEY3:
	case OPTION_GUI_DEF_AF_KEY4:
	case OPTION_GUI_DEF_AF_KEY5:
	case OPTION_GUI_DEF_AF_KEY6:
		{
			int idx = (first + num - OPTION_GUI_DEF_AF_KEY1) / 2;
			sprintf((char*)g_string, "Autofire %d key: %s", idx + 1, gui_menu_key_labels[(&autofire.fire1)[idx].key]);
			put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		}
		break;
	case OPTION_GUI_DEF_AF_RETURN:
		sprintf((char*)g_string, "Return back");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	// RUN GAME DEFAULTS
	case OPTION_GUI_DEF_RUN_SOUND:
	case OPTION_GUI_DEF_VRUN_SOUND:
		sprintf((char*)g_string, "Sound: %s", abreviation_cf[6][options.sound]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RUN_SAMPLERATE:
	case OPTION_GUI_DEF_VRUN_SAMPLERATE:
		sprintf((char*)g_string, "Audio sample rate: %sHz", abreviation_cf[7][options.samplerate]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_VRUN_ROTATE:
		sprintf((char*)g_string, "Rotate vertical game: %s", abreviation_cf[9][options.rotate]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RUN_HWSCALING:
	case OPTION_GUI_DEF_VRUN_HWSCALING:
		sprintf((char*)g_string, "Hardware scaling: %s", abreviation_cf[10][options.hwscaling]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RUN_VSYNC:
	case OPTION_GUI_DEF_VRUN_VSYNC:
		sprintf((char*)g_string, "Vertical sync: %s", abreviation_cf[5][options.vsync]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RUN_SHOWFPS:
	case OPTION_GUI_DEF_VRUN_SHOWFPS:
		sprintf((char*)g_string, "Show FPS: %s", abreviation_cf[0][options.showfps]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RUN_CORE:
	case OPTION_GUI_DEF_VRUN_CORE:
		sprintf((char*)g_string, "68K Emu Core: %s", abreviation_cf[3][options.m68kcore]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RUN_ANALOG:
	case OPTION_GUI_DEF_VRUN_ANALOG:
		sprintf((char*)g_string, "Analogue Sensitivity: %d%%", options.sense);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	case OPTION_GUI_DEF_RUN_RETURN:
	case OPTION_GUI_DEF_VRUN_RETURN:
		sprintf((char*)g_string, "Return back");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	// KEY MAPPINGS
	case OPTION_GUI_DEF_VKEY1:
	case OPTION_GUI_DEF_VKEY2:
	case OPTION_GUI_DEF_VKEY3:
	case OPTION_GUI_DEF_VKEY4:
	case OPTION_GUI_DEF_VKEY5:
	case OPTION_GUI_DEF_VKEY6:
	{
		int idx = first + num - OPTION_GUI_DEF_VKEY1;
		sprintf((char*)g_string, "Vertical Fire %d: %s", idx + 1, gui_menu_key_labels[(&keymap.fire1)[idx]]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
	}
	break;
	case OPTION_GUI_DEF_VKEY_RETURN:
		sprintf((char*)g_string, "Return back");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	// AUTOFIRE SETTINGS
	case OPTION_GUI_DEF_VAF_FPS1:
	case OPTION_GUI_DEF_VAF_FPS2:
	case OPTION_GUI_DEF_VAF_FPS3:
	case OPTION_GUI_DEF_VAF_FPS4:
	case OPTION_GUI_DEF_VAF_FPS5:
	case OPTION_GUI_DEF_VAF_FPS6:
	{
		int idx = (first + num - OPTION_GUI_DEF_VAF_FPS1) / 2;
		sprintf((char*)g_string, "Vertical Autofire %d fps: %s", idx + 1, gui_menu_fps_labels[(&autofire.fire1)[idx].fps]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
	}
	break;
	case OPTION_GUI_DEF_VAF_KEY1:
	case OPTION_GUI_DEF_VAF_KEY2:
	case OPTION_GUI_DEF_VAF_KEY3:
	case OPTION_GUI_DEF_VAF_KEY4:
	case OPTION_GUI_DEF_VAF_KEY5:
	case OPTION_GUI_DEF_VAF_KEY6:
	{
		int idx = (first + num - OPTION_GUI_DEF_VAF_KEY1) / 2;
		sprintf((char*)g_string, "Vertical Autofire %d key: %s", idx + 1, gui_menu_key_labels[(&autofire.fire1)[idx].key]);
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
	}
	break;
	case OPTION_GUI_DEF_VAF_RETURN:
		sprintf((char*)g_string, "Return back");
		put_string( g_string , OPTIONS_START_X , y , BLANC , gui_screen );
		break;
	}
}

void gui_reset_selection()
{
	sel.y = START_Y - 1;
	sel.x = 0;
	sel.rom = 0;
	sel.ofs = 0;
}

void gui_validate_selection()
{
	gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone); // apply filters
	if (sel.rom < 0 || sel.ofs < 0 ||
		sel.y < START_Y - 1 || sel.y > START_Y + (gui_lang.gamelist_line_count - 1) * gui_lang.gamelist_line_height ||
		sel.rom > romlist.nb_list[cfg.list] - 1 ||
		sel.ofs > romlist.nb_list[cfg.list] - 1)
	{
		gui_reset_selection();	
	}
}

void ss_prg_options(int first, int last)
{
	int options_y, options_num, options_off, options_start_y, options_last_line;
	int Quit;
	unsigned int compteur = 1;

	options_last_line = OPTIONS_LINE_COUNT - 1;
	options_start_y = START_Y + LINE_HEIGHT;
	options_y = options_start_y - 1;
	options_num = 0;
	options_off = 0;

	last = last - first;

	unsigned option_start;

	gui_load_cfg();

	prep_bg_opt(first);
	Quit=0;
	while(!Quit) {
		affiche_BG();

		drawSprite(barre, gui_screen, 0, 0, 4, options_y, 312, 10);

		option_start = options_start_y;
		for(int y = options_off; y < (options_off + OPTIONS_LINE_COUNT); y++) {
			put_option_line(first, y, option_start);
			option_start += LINE_HEIGHT;
		}

		redraw_screen();

		SDL_PollEvent(&event);
		if(event.type == SDL_KEYDOWN) {
			if(compteur == 0 || (compteur > cfg.delayspeed && ((compteur & joy_speed[cfg.repeatspeed]) == 0))) {
				if(event.key.keysym.sym == SDLK_DOWN) {
					if (g_message[0] != 0) {
						g_message[0] = 0;
						prep_bg_opt(first);
					}

					if(options_num == last && compteur == 0) {
						options_y = options_start_y - 1;
						options_num = 0;
						options_off = 0;
					} else {
						if(options_num < options_last_line || options_off == ( last - options_last_line)) {
							if(options_num < last) {
								options_y += LINE_HEIGHT;
								++options_num;
							}
						} else {
							++options_off;
							++options_num;
						}
					}
				} else if(event.key.keysym.sym == SDLK_UP) {
					if (g_message[0] != 0) {
						g_message[0] = 0;
						prep_bg_opt(first);
					}

					if(options_num == 0 && compteur == 0) {
						options_y = options_start_y - 1 + ((last < options_last_line ? last : options_last_line) * LINE_HEIGHT);
						options_num = last;
						options_off = last < options_last_line ? 0 : last - options_last_line;
					} else {
						if(options_num > (last - options_last_line / 2) || options_off == 0) {
							if(options_num > 0) {
								options_y -= LINE_HEIGHT;
								--options_num;
							}
						} else {
							--options_off;
							--options_num;
						}
					}
				// PROCESS OPTIONS
				} else if (event.key.keysym.sym == SDLK_LEFT) {
					switch(first + options_num) {
						case OPTION_GUI_DELAYSPEED:
							--cfg.delayspeed;
							if(cfg.delayspeed == 9) cfg.delayspeed = 50;
							break;
						case OPTION_GUI_REPEATSPEED:
							--cfg.repeatspeed;
							if(cfg.repeatspeed == -1) cfg.repeatspeed = 3;
							break;
						case OPTION_GUI_SHADOW:
							--cfg.FXshadow;
							if(cfg.FXshadow == -1) cfg.FXshadow = 100;
							break;
						case OPTION_GUI_SKIN:
							cfg.skin ^= 1;
							break;
						case OPTION_GUI_FILTER_LIST:
							// prev filter
							cfg.list--;
							if (cfg.list < 0) cfg.list = NB_FILTERS - 1;
							gui_reset_selection();
							break;
						case OPTION_GUI_FILTER_HARDWARE:
							// prev hardware
							cfg.hardware--;
							if (cfg.hardware < 0) cfg.hardware = NB_HARDWARES - 1;
							gui_reset_selection();
							break;
						case OPTION_GUI_FILTER_GENRE:
							// prev genre
							cfg.genre--;
							if (cfg.genre < 0) cfg.genre = NB_GENRES - 1;
							gui_reset_selection();
							break;
						case OPTION_GUI_FILTER_CLONE:
							// prev clone option
							cfg.clone--;
							if (cfg.clone < 0) cfg.clone = NB_CLONES - 1;
							gui_reset_selection();
							break;
						case OPTION_GUI_DEF_KEY1:
						case OPTION_GUI_DEF_KEY2:
						case OPTION_GUI_DEF_KEY3:
						case OPTION_GUI_DEF_KEY4:
						case OPTION_GUI_DEF_KEY5:
						case OPTION_GUI_DEF_KEY6:
							{
								int idx = first + options_num - OPTION_GUI_DEF_KEY1;
								int * val = &keymap.fire1 + idx;
								*val = gui_menu_get_prev(gui_menu_key_values, *val);
							}
							break;
						case OPTION_GUI_DEF_VKEY1:
						case OPTION_GUI_DEF_VKEY2:
						case OPTION_GUI_DEF_VKEY3:
						case OPTION_GUI_DEF_VKEY4:
						case OPTION_GUI_DEF_VKEY5:
						case OPTION_GUI_DEF_VKEY6:
							{
								int idx = first + options_num - OPTION_GUI_DEF_VKEY1;
								int * val = &keymap.fire1 + idx;
								*val = gui_menu_get_prev(gui_menu_key_values, *val);
							}
							break;
						case OPTION_GUI_DEF_AF_FPS1:
						case OPTION_GUI_DEF_AF_FPS2:
						case OPTION_GUI_DEF_AF_FPS3:
						case OPTION_GUI_DEF_AF_FPS4:
						case OPTION_GUI_DEF_AF_FPS5:
						case OPTION_GUI_DEF_AF_FPS6:
							{
								int idx = (first + options_num - OPTION_GUI_DEF_AF_FPS1) / 2;
								int * val = (&(&autofire.fire1 + idx)->fps);
								*val = gui_menu_get_prev(gui_menu_fps_values, *val);
							}
							break;
						case OPTION_GUI_DEF_VAF_FPS1:
						case OPTION_GUI_DEF_VAF_FPS2:
						case OPTION_GUI_DEF_VAF_FPS3:
						case OPTION_GUI_DEF_VAF_FPS4:
						case OPTION_GUI_DEF_VAF_FPS5:
						case OPTION_GUI_DEF_VAF_FPS6:
							{
								int idx = (first + options_num - OPTION_GUI_DEF_VAF_FPS1) / 2;
								int * val = (&(&autofire.fire1 + idx)->fps);
								*val = gui_menu_get_prev(gui_menu_fps_values, *val);
							}
							break;
						case OPTION_GUI_DEF_AF_KEY1:
						case OPTION_GUI_DEF_AF_KEY2:
						case OPTION_GUI_DEF_AF_KEY3:
						case OPTION_GUI_DEF_AF_KEY4:
						case OPTION_GUI_DEF_AF_KEY5:
						case OPTION_GUI_DEF_AF_KEY6:
							{
								int idx = (first + options_num - OPTION_GUI_DEF_AF_KEY1) / 2;
								int * val = (&(&autofire.fire1 + idx)->key);
								*val = gui_menu_get_prev(gui_menu_key_values, *val);
							}
							break;
						case OPTION_GUI_DEF_VAF_KEY1:
						case OPTION_GUI_DEF_VAF_KEY2:
						case OPTION_GUI_DEF_VAF_KEY3:
						case OPTION_GUI_DEF_VAF_KEY4:
						case OPTION_GUI_DEF_VAF_KEY5:
						case OPTION_GUI_DEF_VAF_KEY6:
							{
								int idx = (first + options_num - OPTION_GUI_DEF_VAF_KEY1) / 2;
								int * val = (&(&autofire.fire1 + idx)->key);
								*val = gui_menu_get_prev(gui_menu_key_values, *val);
							}
							break;
						case OPTION_GUI_DEF_RUN_SOUND:
						case OPTION_GUI_DEF_VRUN_SOUND:
							options.sound--;
							if(options.sound < 0) options.sound = 3;
							break;
						case OPTION_GUI_DEF_RUN_SAMPLERATE:
						case OPTION_GUI_DEF_VRUN_SAMPLERATE:
							options.samplerate--;
							if(options.samplerate < 0) options.samplerate = 4;
							break;
						case OPTION_GUI_DEF_VRUN_ROTATE:
							options.rotate --;// ^= 1;
							if(options.rotate<0) {
								options.rotate = 0;
							}
							break;
						case OPTION_GUI_DEF_RUN_HWSCALING:
						case OPTION_GUI_DEF_VRUN_HWSCALING:
							options.hwscaling--;
							if(options.hwscaling < 0) options.hwscaling = 2;
							break;
						case OPTION_GUI_DEF_RUN_VSYNC:
						case OPTION_GUI_DEF_VRUN_VSYNC:
							options.vsync ^= 1;
							break;
						case OPTION_GUI_DEF_RUN_SHOWFPS:
						case OPTION_GUI_DEF_VRUN_SHOWFPS:
							options.showfps ^= 1;
							break;
						case OPTION_GUI_DEF_RUN_CORE:
						case OPTION_GUI_DEF_VRUN_CORE:
							options.m68kcore--;
							if(options.m68kcore < 0) options.m68kcore = 2;
							break;
						case OPTION_GUI_DEF_RUN_ANALOG:
						case OPTION_GUI_DEF_VRUN_ANALOG:
							options.sense--;
							if(options.sense < 0) options.sense = 100;
							break;
					}
				} else if(event.key.keysym.sym == SDLK_RIGHT) {
					switch(first + options_num) {
						case OPTION_GUI_DELAYSPEED:
							++cfg.delayspeed;
							if(cfg.delayspeed == 51) cfg.delayspeed = 10;
							break;
						case OPTION_GUI_REPEATSPEED:
							++cfg.repeatspeed;
							if(cfg.repeatspeed == 4) cfg.repeatspeed = 0;
							break;
						case OPTION_GUI_SHADOW:
							++cfg.FXshadow;
							if(cfg.FXshadow == 101) cfg.FXshadow = 0;
							break;
						case OPTION_GUI_SKIN:
							cfg.skin ^= 1;
							break;
						case OPTION_GUI_FILTER_LIST:
							// next filter
							cfg.list = (cfg.list + 1) % NB_FILTERS;
							gui_reset_selection();
							break;
						case OPTION_GUI_FILTER_HARDWARE:
							// next hardware
							cfg.hardware = (cfg.hardware + 1) % NB_HARDWARES;
							gui_reset_selection();
							break;
						case OPTION_GUI_FILTER_GENRE:
							// next hardware
							cfg.genre = (cfg.genre + 1) % NB_GENRES;
							gui_reset_selection();
							break;
						case OPTION_GUI_FILTER_CLONE:
							// next clone option
							cfg.clone = (cfg.clone + 1) % NB_CLONES;
							gui_reset_selection();
							break;
						case OPTION_GUI_DEF_KEY1:
						case OPTION_GUI_DEF_KEY2:
						case OPTION_GUI_DEF_KEY3:
						case OPTION_GUI_DEF_KEY4:
						case OPTION_GUI_DEF_KEY5:
						case OPTION_GUI_DEF_KEY6:
							{
								int idx = first + options_num - OPTION_GUI_DEF_KEY1;
								int * val = &keymap.fire1 + idx;
								*val = gui_menu_get_next(gui_menu_key_values, *val);
							}
							break;
						case OPTION_GUI_DEF_VKEY1:
						case OPTION_GUI_DEF_VKEY2:
						case OPTION_GUI_DEF_VKEY3:
						case OPTION_GUI_DEF_VKEY4:
						case OPTION_GUI_DEF_VKEY5:
						case OPTION_GUI_DEF_VKEY6:
							{
								int idx = first + options_num - OPTION_GUI_DEF_VKEY1;
								int * val = &keymap.fire1 + idx;
								*val = gui_menu_get_next(gui_menu_key_values, *val);
							}
							break;
						case OPTION_GUI_DEF_AF_FPS1:
						case OPTION_GUI_DEF_AF_FPS2:
						case OPTION_GUI_DEF_AF_FPS3:
						case OPTION_GUI_DEF_AF_FPS4:
						case OPTION_GUI_DEF_AF_FPS5:
						case OPTION_GUI_DEF_AF_FPS6:
							{
								int idx = (first + options_num - OPTION_GUI_DEF_AF_FPS1) / 2;
								int * val = (&(&autofire.fire1 + idx)->fps);
								*val = gui_menu_get_next(gui_menu_fps_values, *val);
							}
							break;
						case OPTION_GUI_DEF_VAF_FPS1:
						case OPTION_GUI_DEF_VAF_FPS2:
						case OPTION_GUI_DEF_VAF_FPS3:
						case OPTION_GUI_DEF_VAF_FPS4:
						case OPTION_GUI_DEF_VAF_FPS5:
						case OPTION_GUI_DEF_VAF_FPS6:
							{
								int idx = (first + options_num - OPTION_GUI_DEF_VAF_FPS1) / 2;
								int * val = (&(&autofire.fire1 + idx)->fps);
								*val = gui_menu_get_next(gui_menu_fps_values, *val);
							}
							break;
						case OPTION_GUI_DEF_AF_KEY1:
						case OPTION_GUI_DEF_AF_KEY2:
						case OPTION_GUI_DEF_AF_KEY3:
						case OPTION_GUI_DEF_AF_KEY4:
						case OPTION_GUI_DEF_AF_KEY5:
						case OPTION_GUI_DEF_AF_KEY6:
							{
								int idx = (first + options_num - OPTION_GUI_DEF_AF_KEY1) / 2;
								int * val = (&(&autofire.fire1 + idx)->key);
								*val = gui_menu_get_next(gui_menu_key_values, *val);
							}
							break;
						case OPTION_GUI_DEF_VAF_KEY1:
						case OPTION_GUI_DEF_VAF_KEY2:
						case OPTION_GUI_DEF_VAF_KEY3:
						case OPTION_GUI_DEF_VAF_KEY4:
						case OPTION_GUI_DEF_VAF_KEY5:
						case OPTION_GUI_DEF_VAF_KEY6:
							{
								int idx = (first + options_num - OPTION_GUI_DEF_VAF_KEY1) / 2;
								int * val = (&(&autofire.fire1 + idx)->key);
								*val = gui_menu_get_next(gui_menu_key_values, *val);
							}
							break;
						case OPTION_GUI_DEF_RUN_SOUND:
						case OPTION_GUI_DEF_VRUN_SOUND:
							options.sound++;
							if(options.sound > 3) options.sound = 0;
							break;
						case OPTION_GUI_DEF_RUN_SAMPLERATE:
						case OPTION_GUI_DEF_VRUN_SAMPLERATE:
							options.samplerate++;
							if(options.samplerate >= 5) options.samplerate = 0;
							break;
						case OPTION_GUI_DEF_VRUN_ROTATE:
							options.rotate ++;// ^= 1;
							if(options.rotate>3) {
								options.rotate = 3;
							}
							break;
						case OPTION_GUI_DEF_RUN_HWSCALING:
						case OPTION_GUI_DEF_VRUN_HWSCALING:
							options.hwscaling++;
							if(options.hwscaling > 2) options.hwscaling = 0;
							break;
						case OPTION_GUI_DEF_RUN_VSYNC:
						case OPTION_GUI_DEF_VRUN_VSYNC:
							options.vsync ^= 1;
							break;
						case OPTION_GUI_DEF_RUN_SHOWFPS:
						case OPTION_GUI_DEF_VRUN_SHOWFPS:
							options.showfps ^= 1;
							break;
						case OPTION_GUI_DEF_RUN_CORE:
						case OPTION_GUI_DEF_VRUN_CORE:
							options.m68kcore++;
							if(options.m68kcore > 2) options.m68kcore = 0;
							break;
						case OPTION_GUI_DEF_RUN_ANALOG:
						case OPTION_GUI_DEF_VRUN_ANALOG:
							options.sense++;
							if(options.sense > 100) options.sense = 0;
							break;
					}
				} else if(event.key.keysym.sym == SDLK_LCTRL) {
					int option = first + options_num;		
					if(option >= OPTION_GUI_PATH0 && option <= OPTION_GUI_PATH19) {
						// call set path menu
						gui_setpath(szAppRomPaths[option - OPTION_GUI_PATH0]);
						prep_bg_opt(first);
					} else if(option == OPTION_GUI_RETURN) {
						prep_bg_main();
						Quit = 1;
					} else if(option == OPTION_GUI_FILTER_RETURN ||
						option == OPTION_GUI_PATH_RETURN ||
						option == OPTION_GUI_DEF_RETURN ||
						option == OPTION_GUI_DEF_KEY_RETURN ||
						option == OPTION_GUI_DEF_AF_RETURN ||
						option == OPTION_GUI_DEF_RUN_RETURN ||
						option == OPTION_GUI_DEF_VKEY_RETURN ||
						option == OPTION_GUI_DEF_VAF_RETURN ||
						option == OPTION_GUI_DEF_VRUN_RETURN ) {
						Quit = 1;
					} else if (option == OPTION_GUI_FILTER) {
						ss_prg_options(OPTION_FILTER_FIRST, OPTION_FILTER_LAST);
						gui_write_cfg();
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_ROM_PATHS) {
						ss_prg_options(OPTION_PATHS_FIRST, OPTION_PATHS_LAST);
						gui_write_cfg();
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEFAULTS) {
						ConfigGameDefault();
						ss_prg_options(OPTION_DEF_FIRST, OPTION_DEF_LAST);
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEF_RESET) {
						ConfigGameDefaultDelete();
						ConfigGameAllDefault();
						sprintf((char*)g_message, "Default setting successfully reseted");
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEF_DELETE) {
						ConfigGameDelete();
						sprintf((char*)g_message, "All ROM config files successfully deleted");
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEF_KEYS) {
						gui_menu_key_init();
						ConfigGameLoadDefault();
						ss_prg_options(OPTION_DEF_KEY_FIRST, OPTION_DEF_KEY_LAST);
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEF_AUTOFIRE) {
						gui_menu_fps_init();
						gui_menu_key_init();
						ConfigGameLoadDefault();
						ss_prg_options(OPTION_DEF_AF_FIRST, OPTION_DEF_AF_LAST);
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEF_VKEYS) {
						gui_menu_key_init();
						ConfigGameLoadVDefault();
						ss_prg_options(OPTION_DEF_VKEY_FIRST, OPTION_DEF_VKEY_LAST);
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEF_VAUTOFIRE) {
						gui_menu_fps_init();
						gui_menu_key_init();
						ConfigGameLoadVDefault();
						ss_prg_options(OPTION_DEF_VAF_FIRST, OPTION_DEF_VAF_LAST);
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEF_RUN) {
						ConfigGameLoadDefault();
						ss_prg_options(OPTION_DEF_RUN_FIRST, OPTION_DEF_RUN_LAST);
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_DEF_VRUN) {
						ConfigGameLoadVDefault();
						ss_prg_options(OPTION_DEF_VRUN_FIRST, OPTION_DEF_VRUN_LAST);
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_FAV_CLEAR) {
						gui_clear_favorite(true);
						gui_reset_selection();
						sprintf((char*)g_message, "Favorites list successfully cleared");
						prep_bg_opt(first);
					} else if (option == OPTION_GUI_ROMLIST_CACHE_CLEAR) {
						gui_clear_last_romlist();
						gui_reset_selection();
						sprintf((char*)g_message, "ROMs list cache successfully cleared, rescaning...");
						prep_bg_opt(first);
						affiche_BG();
						drawSprite(barre, gui_screen, 0, 0, 4, options_y, 312, 10);
						redraw_screen();

						gui_sort_romlist();

						prep_bg_main();
						Quit = 1;
					}
				} else if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
					int option = first + options_num;
					if(option >= OPTION_GUI_PATH0 && option <= OPTION_GUI_PATH19) {
						strcpy(szAppRomPaths[option - OPTION_GUI_PATH0], "");
					}
				} else if(event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_LALT) {
					prep_bg_main();
					Quit = 1;
				}
			}
			++compteur;
		}else if(event.type == SDL_KEYUP){
			compteur = 0;// reinitialisation joystick
		}

		if (!Quit && first == OPTION_FILTER_FIRST) {
			prep_bg_opt(first);
		}
		if(Quit){
			switch(first) {
				case OPTION_DEF_KEY_FIRST:
				case OPTION_DEF_AF_FIRST:
				case OPTION_DEF_RUN_FIRST:
					ConfigGameSaveDefault();
					break;
				case OPTION_DEF_VKEY_FIRST:
				case OPTION_DEF_VAF_FIRST:
				case OPTION_DEF_VRUN_FIRST:
					ConfigGameSaveVDefault();
					break;
			}
		}
	}
	gui_write_cfg();
}

static int run_options_static[] = {
	OPTION_FBA_RUN,
	OPTION_FBA_SOUND,
	OPTION_FBA_SAMPLERATE,
	OPTION_FBA_ROTATE,
#ifdef OPTIONS_FOR_GCW0
	OPTION_FBA_HWSCALING,
#endif
	OPTION_FBA_VSYNC,
	OPTION_FBA_SHOWFPS,
	OPTION_FBA_68K,
#ifdef OPTIONS_FOR_A320
	OPTION_FBA_Z80,
#endif
#ifdef OPTIONS_FOR_GCW0
	OPTION_FBA_ANALOG,
#endif
	OPTION_FBA_FAVORITE,
	0
};

void put_run_option_line(unsigned char num, unsigned char y)
{
	#define OPTIONS_START_X		8
	#define CONF_START_X		272

	switch (num) {
	case OPTION_FBA_RUN:
		put_string("Run game", OPTIONS_START_X, y, BLANC, gui_screen);
		break;
	case OPTION_FBA_SOUND:
		put_string( "Sound" , OPTIONS_START_X , y , BLANC , gui_screen );
		put_string(abreviation_cf[6][options.sound], CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_SAMPLERATE:
		put_string("Audio sample rate", OPTIONS_START_X , y , BLANC , gui_screen );
		sprintf((char*)g_string, "%sHz" , abreviation_cf[7][options.samplerate]);
		put_string(g_string, CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_ROTATE:
		put_string("Rotate vertical game", OPTIONS_START_X, y, BLANC, gui_screen);
		put_string(abreviation_cf[9][options.rotate], CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_HWSCALING:
		put_string("Hardware scaling", OPTIONS_START_X, y, BLANC, gui_screen);
		put_string(abreviation_cf[10][options.hwscaling], CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_VSYNC:
		put_string("Vertical sync", OPTIONS_START_X, y, BLANC, gui_screen);
		put_string(abreviation_cf[5][options.vsync], CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_SHOWFPS:
		put_string("Show FPS", OPTIONS_START_X, y, BLANC, gui_screen);
		put_string(abreviation_cf[0][options.showfps], CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_68K:
		put_string("68K Emu Core", OPTIONS_START_X, y, BLANC, gui_screen);
		put_string(abreviation_cf[3][options.m68kcore], CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_Z80:
		put_string("Z80 Emu Core", OPTIONS_START_X, y, BLANC, gui_screen);
		put_string(abreviation_cf[4][options.z80core], CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_ANALOG:
		put_string("Analogue Sensitivity", OPTIONS_START_X, y, BLANC, gui_screen);
		sprintf((char*)g_string, "%d%%" , options.sense);
		put_string(g_string, CONF_START_X, y, VERT, gui_screen);
		break;
	case OPTION_FBA_FAVORITE:
		put_string("Rom in the favorites list", OPTIONS_START_X, y, BLANC, gui_screen);
		put_string(abreviation_cf[8][gui_in_favorite(nBurnDrvActive)], CONF_START_X, y, VERT, gui_screen);
		break;
	}
}

void ss_prog_run(void)
{

	int run_y, run_num, run_off;
	int Quit;
	unsigned int compteur = 1;
	
	int nb_rom = nBurnDrvActive = gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone)[sel.rom];
	bool bVertical = BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL;
	
	int opt_first = 0, opt_last = (sizeof(run_options_static) / sizeof(int)) - 2;
	int run_options[opt_last + 1];
	memcpy(run_options, run_options_static, sizeof(int) * (opt_last + 1));
	if (!bVertical) {
		opt_last--;
		for (int i = 3; i <= opt_last; i++) {
			run_options[i] = run_options[i + 1];
		}
	}
#undef OPTION_FBA_FIRST
#undef OPTION_FBA_LAST

#define OPTION_FBA_FIRST opt_first
#define OPTION_FBA_LAST opt_last

#define RUN_START_X	8

	if(!ConfigGameLoad()) ConfigGameSave();

	run_y = START_Y-1;
	run_num = 0;
	run_off = 0;

	prep_bg_run();

	Quit=0;

	while(!Quit) {
		affiche_BG(nb_rom);

		drawSprite(barre, gui_screen, 0, 0, 4, run_y, 260, 10);
		drawSprite(barre, gui_screen, 0, 0, 269, run_y, 47, 10);

		int option_start = START_Y;

		for (int i = run_off; i <= opt_last; i++) {
			if(i <= opt_last)
				put_run_option_line(run_options[i], option_start);
			option_start += 9;
		}

		redraw_screen();

		SDL_PollEvent(&event);
		if(event.type == SDL_KEYDOWN) {
			if(compteur == 0 || (compteur > cfg.delayspeed && ((compteur & joy_speed[cfg.repeatspeed]) == 0))) {
				if(event.key.keysym.sym == SDLK_DOWN) {
					run_num++;
					if(run_num > OPTION_FBA_LAST) {
						run_y = START_Y - 1;
						run_num = OPTION_FBA_FIRST;
						run_off = run_num;
					} else {
						if(run_num < 12 || run_off > OPTION_FBA_LAST - 12) {
							run_y += 9;
						} else run_off++;
					}
				} else if(event.key.keysym.sym == SDLK_UP) {
					run_num--;
					if(run_num < OPTION_FBA_FIRST){
						run_y = START_Y - 1 + ((OPTION_FBA_LAST < 12 ? OPTION_FBA_LAST : 12) * 9);
						run_num = OPTION_FBA_LAST;
						run_off = OPTION_FBA_LAST < 12 ? 0 : OPTION_FBA_LAST - 12;
					} else {
						if(run_num >= OPTION_FBA_LAST - 7 || run_off == 0) run_y -= 9; else run_off--;
					}
				} else if (event.key.keysym.sym == SDLK_LEFT) {
					switch(run_options[run_num]) {
						case OPTION_FBA_SOUND:
							options.sound--;
							if(options.sound < 0) options.sound = 3;
							break;
						case OPTION_FBA_SAMPLERATE:
							options.samplerate--;
							if(options.samplerate < 0) options.samplerate = 4;
							break;
						case OPTION_FBA_ROTATE:
							options.rotate --;// ^= 1;
							if(options.rotate<0) {
								options.rotate = 0;
							}
							break;
						case OPTION_FBA_HWSCALING:
							options.hwscaling--;
							if(options.hwscaling < 0) options.hwscaling = 2;
							break;
						case OPTION_FBA_VSYNC:
							options.vsync ^= 1;
							break;
						case OPTION_FBA_SHOWFPS:
							options.showfps ^= 1;
							break;
						case OPTION_FBA_68K:
							options.m68kcore--;
							if(options.m68kcore < 0) options.m68kcore = 2;
							break;
						case OPTION_FBA_Z80:
							options.z80core ^= 1;
							break;
						case OPTION_FBA_ANALOG:
							options.sense--;
							if(options.sense < 0) options.sense = 100;
							break;
						case OPTION_FBA_FAVORITE:
							gui_favorite_change(nBurnDrvActive);
							if (cfg.list == 4) {
								gui_reset_selection();
							}
							break;
					}
				} else if (event.key.keysym.sym == SDLK_RIGHT) {
					switch(run_options[run_num]) {
						case OPTION_FBA_SOUND:
							options.sound++;
							if(options.sound > 3) options.sound = 0;
							break;
						case OPTION_FBA_SAMPLERATE:
							options.samplerate++;
							if(options.samplerate >= 5) options.samplerate = 0;
							break;
						case OPTION_FBA_ROTATE:
							options.rotate ++;// ^= 1;
							if(options.rotate>3) {
								options.rotate = 3;
							}
							break;
						case OPTION_FBA_HWSCALING:
							options.hwscaling++;
							if(options.hwscaling > 2) options.hwscaling = 0;
							break;
						case OPTION_FBA_VSYNC:
							options.vsync ^= 1;
							break;
						case OPTION_FBA_SHOWFPS:
							options.showfps ^= 1;
							break;
						case OPTION_FBA_68K:
							options.m68kcore++;
							if(options.m68kcore > 2) options.m68kcore = 0;
							break;
						case OPTION_FBA_Z80:
							options.z80core ^= 1;
							break;
						case OPTION_FBA_ANALOG:
							options.sense++;
							if(options.sense > 100) options.sense = 0;
							break;
						case OPTION_FBA_FAVORITE:
							gui_favorite_change(nBurnDrvActive);
							if (cfg.list == 4) {
								gui_reset_selection();
							}
							break;
					}
				} else if (event.key.keysym.sym == SDLK_LCTRL) {
					save_lastsel();

					nBurnDrvActive = nBurnDrvSelect[0] = nb_rom;
					bool bVertical = BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL;
					if(!bVertical)
						options.rotate = 0;
					ConfigGameSave();

					SDL_QuitSubSystem(SDL_INIT_VIDEO);

					// run emulator here
					RunEmulator(nBurnDrvActive);

					if(!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) {
						SDL_InitSubSystem(SDL_INIT_VIDEO);
					}

					gui_screen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
					SDL_ShowCursor(0);

					prep_bg();
					Quit = 1;
				} else if(event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_ESCAPE) {
					prep_bg();
					Quit = 1;
				}
			}
			++compteur;
		} else if(event.type == SDL_KEYUP){
			compteur=0;// reinitialisation joystick
		}
	}

	nBurnDrvActive = nb_rom;
	ConfigGameSave();
}

int findfirst(int l, int s)
{
	l -= 32;

	int z=0;

	while(z < romlist.nb_list[cfg.list]) {
		if(ROMLIST(name, z)[0] == l) {
			s = z;
			z = romlist.nb_list[cfg.list];
		}
		z++;
	}

	if(s > romlist.nb_list[cfg.list] - 13) s = romlist.nb_list[cfg.list] - 13;
	return s;
}

void save_lastsel()
{
	FILE *fp;
	char filename[512];

	sprintf(filename, "%s/lastsel.cap", szAppHomePath);
	fp = fopen(filename, "w");
	if(fp) {
		fwrite(&sel.rom, 1, sizeof(int), fp);
		fwrite(&sel.ofs, 1, sizeof(int), fp);
		fwrite(&sel.y, 1, sizeof(int), fp);
		fclose(fp);
	}
}

void load_lastsel()
{
	FILE *fp;
	char filename[512];

	sprintf(filename, "%s/lastsel.cap", szAppHomePath);
	fp = fopen(filename, "r");
	if(fp) {
		fread(&sel.rom, 1, sizeof(int), fp);
		fread(&sel.ofs, 1, sizeof(int), fp);
		fread(&sel.y, 1, sizeof(int), fp);
		fclose(fp);
	}
	gui_validate_selection();
}

void set_language()
{
	// default lang English
	gui_lang.gamelist_line_height = LINE_HEIGHT;
	gui_lang.gamelist_line_count = LINES_COUNT;
	gui_lang.gamelist_line_count_half = LINES_COUNT_HALF;
	if(!use_language_pack) return;

	extern size_t pixfont_height;
	if(!pixfont_height)
		return;
	gui_lang.gamelist_line_height = pixfont_height;
	gui_lang.gamelist_line_count = (LINE_HEIGHT * LINES_COUNT+pixfont_height-1) / gui_lang.gamelist_line_height;
	gui_lang.gamelist_line_count_half = gui_lang.gamelist_line_count / 2;
}

void gui_menu_main()
{
	int Quit;
	unsigned int romlist_y;
	unsigned int compteur = 0;
	std::unordered_set<int> selected_flag_roms;

	sel.y = START_Y-1;
	sel.x=0;
	sel.rom = 0;
	sel.ofs = 0;

	load_lastsel();

	prep_bg_main();
	flag_preview = 0;
	load_preview(sel.rom);

	//load_cf();
	int barre_offset = 0;
	int barre_y = 0;
	int barre_height = 10;
	if(use_language_pack) {
		if(barre18px) {
			switch(gui_lang.gamelist_line_height) {
			case 12: barre_y = 2; barre_height = 14; break;
			case 13: barre_y = 1; barre_height = 15; break;
			case 14: barre_y = 1; barre_height = 16; break;
			case 15: barre_y = 0; barre_height = 17; break;
			case 16:
			default: barre_y = 0; barre_height = 18; break;
			}
		} else {
			barre_offset = (gui_lang.gamelist_line_height-10)/2;
		}
	}

	Quit = 0;
	while(!Quit) {
		affiche_BG();

		if (romlist.nb_list[cfg.list] != 0) {
			drawSprite((use_language_pack && barre18px)? barre18px: barre, gui_screen,
					   0, barre_y, 4, sel.y + barre_offset, 312, barre_height);
		}

		// show rom list
		if(use_language_pack) 
			prepare_fontsurf();
		romlist_y = START_Y;
		if(romlist.nb_list[cfg.list] <= gui_lang.gamelist_line_count) {
			for(unsigned int fID = 0; fID < romlist.nb_list[cfg.list]; ++fID) {
				const int romID = gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone)[fID];
				const bool df_color = selected_flag_roms.count(romID);
				if(!use_language_pack)
					put_stringM(ROMLIST(name, fID), // string
						START_X, // x
						romlist_y, // y
						ROMLIST(longueur, fID), // length
						df_color? BLANC : ROMLIST(etat, fID)); // color
				else
					lang_put_stringM(romID,
						START_X, // x
						romlist_y, // y
						df_color? BLANC : ROMLIST(etat, fID)); // color
				romlist_y += gui_lang.gamelist_line_height;
			}
		} else {
			for(unsigned int fID = sel.ofs; fID < sel.ofs + gui_lang.gamelist_line_count; ++fID) {
				const int romID = gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone)[fID];
				const bool df_color = selected_flag_roms.count(romID);
				if(!use_language_pack)
					put_stringM(ROMLIST(name, fID), 
						START_X, 
						romlist_y, 
						ROMLIST(longueur, fID), 
						df_color? BLANC : ROMLIST(etat, fID));
				else
					lang_put_stringM(romID,
						START_X, // x
						romlist_y, // y
						df_color? BLANC : ROMLIST(etat, fID)); // color
				romlist_y += gui_lang.gamelist_line_height;
			}
		}
		if(use_language_pack) 
			lang_show_gamelist_lines();

		redraw_screen();

		SDL_PollEvent(&event);
		if(event.type == SDL_KEYDOWN) {
			if(compteur == 0 || (compteur > cfg.delayspeed && ((compteur & joy_speed[cfg.repeatspeed]) == 0))) {
				if((event.key.keysym.sym >= SDLK_a) && (event.key.keysym.sym <= SDLK_z) &&
					(romlist.nb_list[cfg.list] != 0)) {
					sel.rom = findfirst(event.key.keysym.sym,sel.rom);
					sel.ofs = sel.rom;
					sel.y = START_Y - 1;
				} else if(event.key.keysym.sym == SDLK_LSHIFT) { // X button
					// no need to increment compteur
					continue;
				} else if(event.key.keysym.sym == SDLK_TAB) { // page up
pageup:
					Uint8* keystate = SDL_GetKeyState(NULL);
					if (keystate[SDLK_LSHIFT]) {
						if (compteur == 0) {
							// prev genre
							cfg.genre--;
							if (cfg.genre < 0) cfg.genre = NB_GENRES - 1;
							gui_reset_selection();
							prep_bg_main();
							gui_write_cfg();
						}
					} else if (romlist.nb_list[cfg.list] > gui_lang.gamelist_line_count) {
						sel.rom -= gui_lang.gamelist_line_count;
						if(sel.rom > gui_lang.gamelist_line_count_half) {
							sel.ofs = sel.rom - gui_lang.gamelist_line_count_half;
							sel.y = START_Y - 1 + gui_lang.gamelist_line_count_half * gui_lang.gamelist_line_height;
						} else {
							sel.ofs = sel.rom = 0;
							sel.y = START_Y - 1;
						}
						gui_validate_selection();
					}
				} else if(event.key.keysym.sym == SDLK_BACKSPACE) { // page down
pagedown:
					Uint8* keystate = SDL_GetKeyState(NULL);
					if (keystate[SDLK_LSHIFT]) {
						if (compteur == 0) {
							// next genre
							cfg.genre = (cfg.genre + 1) % NB_GENRES;
							gui_reset_selection();
							prep_bg_main();
							gui_write_cfg();
						}
					} else if (romlist.nb_list[cfg.list] > gui_lang.gamelist_line_count) {
						sel.rom += gui_lang.gamelist_line_count;
						if(sel.rom < romlist.nb_list[cfg.list] - gui_lang.gamelist_line_count_half) {
							sel.ofs = sel.rom - gui_lang.gamelist_line_count_half;
							sel.y = START_Y - 1 + gui_lang.gamelist_line_count_half * gui_lang.gamelist_line_height;
						} else {
							sel.rom = romlist.nb_list[cfg.list] - 1;
							sel.ofs = sel.rom - (gui_lang.gamelist_line_count - 1);
							sel.y = START_Y - 1 + (gui_lang.gamelist_line_count - 1) * gui_lang.gamelist_line_height;
						}
					}
				} else if(event.key.keysym.sym == SDLK_DOWN) {
					if (SDL_GetKeyState(NULL)[SDLK_LSHIFT]) {
						if (compteur == 0) {
							// next filter
							cfg.list = (cfg.list + 1) % NB_FILTERS;
							gui_reset_selection();
							prep_bg_main();
							gui_write_cfg();
						}
					} else if (romlist.nb_list[cfg.list] == 0) {
					}
					// if in the end of list, reset to the beginning
					else if (sel.rom == romlist.nb_list[cfg.list] - 1 && compteur == 0) {
						sel.y = START_Y - 1;
						sel.rom = 0;
						sel.ofs = 0;
					} else {
movedown:
						if (romlist.nb_list[cfg.list] <= gui_lang.gamelist_line_count + 1) {
							// if rom number in list <= gui_lang.gamelist_line_count + 1
								if (sel.rom < romlist.nb_list[cfg.list] - 1) {
									sel.y += gui_lang.gamelist_line_height;
									++sel.rom;
									if(compteur == 0) {
										load_preview(sel.rom);
									}
								}
						}else{
							if (sel.rom < gui_lang.gamelist_line_count_half ||
									sel.ofs == (romlist.nb_list[cfg.list] - gui_lang.gamelist_line_count)) {
								if (sel.rom < (romlist.nb_list[cfg.list] - 1)) {
									sel.y += gui_lang.gamelist_line_height;
									++sel.rom;
									if(compteur == 0) {
										load_preview(sel.rom);
									}
								}
							} else {
								++sel.ofs;
								++sel.rom;
								if(compteur == 0) {
									load_preview(sel.rom);
								}
							}
						}
					}
				} else if(event.key.keysym.sym == SDLK_UP) {
					Uint8* keystate = SDL_GetKeyState(NULL);
					if (keystate[SDLK_LSHIFT]) {
						if (compteur == 0) {
							// prev filter
							cfg.list--;
							if (cfg.list < 0) cfg.list = NB_FILTERS - 1;
							gui_reset_selection();
							prep_bg_main();
							gui_write_cfg();
						}
					} else if (romlist.nb_list[cfg.list] == 0) {
					} else if (sel.rom == 0 && compteur == 0) {
						sel.rom = romlist.nb_list[cfg.list] - 1;
						if (romlist.nb_list[cfg.list] < gui_lang.gamelist_line_count) {
							sel.y = START_Y - 1 + ((romlist.nb_list[cfg.list] - 1) * gui_lang.gamelist_line_height);
							//sel.ofs = 0;
						} else {
							sel.y = START_Y - 1 + ((gui_lang.gamelist_line_count - 1) * gui_lang.gamelist_line_height);
							sel.ofs = romlist.nb_list[cfg.list] - gui_lang.gamelist_line_count;
						}
					} else {
						if(sel.rom > romlist.nb_list[cfg.list] - gui_lang.gamelist_line_count_half || sel.ofs == 0) {
							if(sel.rom > 0) {
								sel.y -= gui_lang.gamelist_line_height;
								--sel.rom;
								if(compteur == 0) {
									load_preview(sel.rom);
								}
							}
						} else {
							--sel.ofs;
							--sel.rom;
							if(compteur == 0) {
								load_preview(sel.rom);
							}
						}
					}
				} else if(event.key.keysym.sym == SDLK_LEFT) {
					Uint8* keystate = SDL_GetKeyState(NULL);
					if (keystate[SDLK_LSHIFT]) {
						if (compteur == 0) {
							// prev hardware
							cfg.hardware--;
							if (cfg.hardware < 0) cfg.hardware = NB_HARDWARES - 1;
							gui_reset_selection();
							prep_bg_main();
							gui_write_cfg();
						}
					} else
						goto pageup;
#if 0
					} else if (romlist.nb_list[cfg.list] == 0) {
					} else if (sel.x > 0) {
						--sel.x;
					}
#endif
				} else if(event.key.keysym.sym == SDLK_RIGHT) {
					Uint8* keystate = SDL_GetKeyState(NULL);
					if (keystate[SDLK_LSHIFT]) {
						if (compteur == 0) {
							// next hardware
							cfg.hardware = (cfg.hardware + 1) % NB_HARDWARES;
							gui_reset_selection();
							prep_bg_main();
							gui_write_cfg();
						}
					} else
						goto pagedown;
#if 0
					} else if (romlist.nb_list[cfg.list] == 0) {
					} else if (sel.x < romlist.long_max - 53) {
						++sel.x;
					}
#endif
				} else if(event.key.keysym.sym == SDLK_ESCAPE ) { // SELECT button
					Uint8* keystate = SDL_GetKeyState(NULL);
					if (keystate[SDLK_LSHIFT]) {
						if(romlist.nb_list[cfg.list] != 0 && ROMLIST(etat, sel.rom) != ROUGE) {
							int sel_rom = gui_get_filtered_romsort(cfg.list, cfg.hardware, cfg.genre, cfg.clone)[sel.rom];
							if (0 == selected_flag_roms.count(sel_rom))
								selected_flag_roms.insert(sel_rom);
							else
								selected_flag_roms.erase(sel_rom);
							if (sel.rom == romlist.nb_list[cfg.list] - 1 && compteur == 0) {
								sel.y = START_Y - 1;
								sel.rom = 0;
								sel.ofs = 0;
							} else
								goto movedown;
						}
					}
					else if(ss_prg_credit()) Quit = 1;
				} else if( event.key.keysym.sym == SDLK_LALT) { // B button
					if(ss_prg_credit()) Quit = 1;
				} else if(event.key.keysym.sym == SDLK_LCTRL){ // A button
					if (SDL_GetKeyState(NULL)[SDLK_LSHIFT]) {
						for(auto onerom : selected_flag_roms) {
							if (4 != cfg.list)
								add_to_favorite(onerom);
							else
								remove_from_favorite(onerom);
						}
						save_favorite();
						selected_flag_roms.clear();
						if (4 == cfg.list )
						{
							sel.y = START_Y - 1;
							sel.rom = 0;
							sel.ofs = 0;
						}
						// change filtered romsort to other list force update then change back in prep_bg_main()
						gui_get_filtered_romsort((cfg.list+1)%NB_FILTERS, cfg.hardware, cfg.genre, cfg.clone);
						prep_bg_main();
					}
					else {
						// executer l'emu
						if(romlist.nb_list[cfg.list] != 0 && ROMLIST(etat, sel.rom) != ROUGE) {
							ss_prog_run();
							prep_bg_main();
							last_numero = -1; // force reload preview picture
							load_preview(sel.rom);

							// flush event queue
							while(SDL_PollEvent(&event));
							// simulate key unpress
							// important
							event.type = SDL_KEYUP;
							event.key.keysym.sym = SDLK_LCTRL;
							SDL_PushEvent(&event);
							compteur = 0;
							continue;
						}
					}
				} else if(event.key.keysym.sym == SDLK_SPACE ){
					if(compteur == 0) ss_prg_help();
				} else if(event.key.keysym.sym == SDLK_RETURN ){ // START button
					if (SDL_GetKeyState(NULL)[SDLK_LSHIFT]) {
					// apply delete flag roms delete permanently 
						char romfilepath[MAX_PATH];
						for(auto onerom : selected_flag_roms) {
							for(int idr = 0; idr < DIRS_MAX; idr++) {
								if(!strlen(szAppRomPaths[idr]))
									continue;
								sprintf(romfilepath, "%s%s.zip", szAppRomPaths[idr], romlist.zip[onerom]);
								remove(romfilepath);
							}
							romlist.etat[onerom] = ROUGE;
						}
						selected_flag_roms.clear();
						prep_bg_main();
					}
					else
						ss_prg_options(OPTION_MAIN_FIRST, OPTION_MAIN_LAST);
				}
			}
			++compteur;
		} else if(event.type == SDL_KEYUP) {
			if(compteur) {
				load_preview(sel.rom);
			}
			compteur = 0; // reinitialisation joystick
		}
	}

	save_lastsel();
}

void GuiRun()
{
	// fill data with data
	gui_sort_romlist();
	use_language_pack = gui_load_language_pack();
	set_language();

	gui_screen = SDL_SetVideoMode(GUI_SCREEN_W, GUI_SCREEN_H, 16, SDL_SWSURFACE);

	SDL_ShowCursor(0);
	SDL_JoystickOpen(0);

	gui_load_cfg();
	init_title();

	gui_menu_main();

	exit_prog();
}

/*EOF*/
