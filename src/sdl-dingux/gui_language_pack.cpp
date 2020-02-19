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

/*
 * 2020-01-14: Language pack for FinalBurn Alpha by nobk @ github
 * directory: .fba/lang
 * files: description.json
 *        font_{font name in description.json}.bitmap
 *        romlist_{lang as suffix in description.json}.txt
 * font_wenquanyi_16px.bitmap
 * romlist_zh_CN.txt which is the same format as gamelist.glt support by FinalBurn Alpha for Windows
 */

#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/xpressive/xpressive.hpp>

#include "burner.h"
#include "gui_main.h"
#include "gui_romlist.h"
#include "gui_language_pack.h"

using json = nlohmann::json;

#define LANG_SCREEN_W	320
#define LANG_SCREEN_H	240

lang_DrawString_t lang_DrawString_inp = NULL;
void lang_DrawGB2312String( const char *cstr, uint32_t *screen, uint32_t x, uint32_t y, uint32_t resW, uint32_t RGBvalue );

std::vector<uint8_t> pixfont;
std::unordered_map<std::string,std::string> lang_translation;

size_t pixfont_width = 0;
size_t pixfont_height = 0;
size_t pixfont_char_bitmap_bytes = 0;
size_t pixfont_total_chars = 0;

std::string langpack_lang;
std::string langpack_charset;

static bool load_gamelist_transation_file(const std::string & lang_suffix)
{
	using namespace std;
	using namespace boost::xpressive;
	assert( lang_suffix.size() > 3 );
	char filepath[MAX_PATH];
	sprintf(filepath, "%s/lang/romlist_%s.txt", szAppHomePath, lang_suffix.c_str());
	try {
		string line;
		lang_translation.clear();
		sregex rex = sregex::compile( "^([\\w]+)\\t([^\\t]+)$" );
		smatch rem;
		ifstream gamelist (filepath);
		size_t counter = 0;
		while(getline(gamelist, line)) {
			if( regex_match( line, rem, rex ) )
			{
				string v = rem[2];
				boost::trim(v);
				string k = rem[1];
				lang_translation[k] = v;
				counter++;
			}
		}
		return counter>0;
	} catch(exception const& error)
	{
		cerr << __FUNCTION__ << error.what() << endl;
	}
	return false;
}

static inline bool load_zh_CN_GB2312_lang_pack(const std::string & font_name,
		size_t pix_width, size_t pix_height, size_t char_bitmap_bytes, size_t total_chars)
{
	using namespace std;
#define RANGE_M_(A,R0,R1) (A>R0 && A<=R1)
	assert( RANGE_M_(font_name.size(), 1, 40) &&
			RANGE_M_(pix_width, 0, 16) &&
			RANGE_M_(pix_height, 0, 16) &&
			RANGE_M_(char_bitmap_bytes, 2, 32) &&
			RANGE_M_(total_chars, 16, 30000) );
#undef RANGE_M_
	pixfont.clear();
	if(total_chars>10000)
		return false;
	try {
		char filepath[MAX_PATH];
		// Load bitmap font
		sprintf(filepath, "%s/lang/font_%s.bitmap", szAppHomePath, font_name.c_str());
		ifstream ifont(filepath, ios::binary|ios::ate);
		ifstream::pos_type pos = ifont.tellg();
		if( (size_t)pos > char_bitmap_bytes * total_chars * 2 )
			return false;
		pixfont.resize((size_t)pos);

		ifont.seekg(0, ios::beg);
		ifont.read((char*)pixfont.data(), (size_t)pos);
		ifont.close();
		if( load_gamelist_transation_file("zh_CN") )
		{
			pixfont_width = pix_width;
			pixfont_height = pix_height;
			pixfont_char_bitmap_bytes = char_bitmap_bytes;
			pixfont_total_chars = total_chars;
			lang_DrawString_inp = &lang_DrawGB2312String;
			return true;
		}
		pixfont.clear();
	} catch(exception const& error)
	{
		cerr << __FUNCTION__ << error.what() << endl;
	}
	return false;
}

bool gui_load_language_pack()
{
	char filename[MAX_PATH];
	sprintf(filename, "%s/lang/description.json", szAppHomePath);
	try {
		std::ifstream f(filename);
		if(!f.is_open()) return false;
		auto j = json::parse(f);
		std::string fontname = j["font"];
		size_t wf = j["width_pixel"];
		size_t hf = j["height_pixel"];
		size_t cb = j["char_bitmap_bytes"];
		size_t tc = j["total_chars"];
		langpack_lang = j["lang"];
		langpack_charset = j["charset"];
		if( !wf || !hf || !cb || !tc || wf!=hf || !fontname.size() || !langpack_lang.size() || !langpack_charset.size() )
			return false;
		if( langpack_lang == "zh_CN" && langpack_charset == "GB2312" )
			return load_zh_CN_GB2312_lang_pack(fontname, wf, hf, cb, tc);
		return false;
	} catch (json::exception& e) {
		std::cerr << __FUNCTION__ << "json::exception:" << std::endl;
	} catch (...) {
		std::cerr << __FUNCTION__ << "any error" << std::endl;
	}
	return false;
}

void lang_DrawString( uint32_t romID, uint32_t * screen,
					  uint32_t x, uint32_t y, uint32_t resW, uint32_t RGBvalue )
{
	if(!lang_DrawString_inp) return;
	if(romID >= nBurnDrvCount) return;
	std::string gamelocalname;
	const char * zipname = romlist.zip[romID];
	if(lang_translation.count(zipname))
		gamelocalname = lang_translation[zipname];
	else
		gamelocalname = romlist.name[romID];
	
	lang_DrawString_inp( gamelocalname.c_str(), screen, x, y, resW, RGBvalue );
}


static const uint8_t font_bitmask[8] = { 0x80, 0x40, 0x20, 0x10,
									   0x08, 0x04, 0x02, 0x01};

static inline bool _isControl(uint8_t c)
{
	return c <= 0x20;
}

static inline bool _isAscii(uint8_t c)
{
	return c >= 0x21 && c <= 0x7E;
}

static inline bool _isGB2312(uint8_t c)
{
	return c >= 0xA1 && c <= 0xFE;
}

void lang_DisplayAsciiChar( uint32_t *Screen, uint8_t c, uint32_t resW,
							uint32_t x, uint32_t y, int x_limit, uint32_t RGBvalue )
{
	if(x_limit<=0 || y>=LANG_SCREEN_H) return;
	uint32_t *s = Screen;
	uint32_t *s_linestart = Screen;
	uint8_t * gb2312_bitmap = (uint8_t *) pixfont.data();
	// ascii code set id = 3
	size_t offset = ((3-1)*94+(c-0x21))*pixfont_char_bitmap_bytes;
	uint8_t * gb2312char_bitmap = gb2312_bitmap + offset;

	int w0 = 0;
	const int he = std::min(LANG_SCREEN_H-(int)y, (int)pixfont_height);
	for(int h = 0; h < he; h++,
			s_linestart += resW, s = s_linestart,
			gb2312char_bitmap += 2)
	{
		// ascii only use half width, aways in Byte0 
		w0 = std::min(((int)pixfont_width+1)/2, x_limit);
		auto byte0 = gb2312char_bitmap[0];
		for(int i = 0; i < w0; ++i, ++s) {
			if( byte0 & font_bitmask[i] )
				*s = RGBvalue;
			else
				*s = RGBA_TRANSPARENT; //Transparent
		}
	}
}

void lang_DisplayGB2312Char( uint32_t *Screen, uint8_t c0, uint8_t c1, uint32_t resW, 
							 uint32_t x, uint32_t y, int x_limit, uint32_t RGBvalue )
{
	if(x_limit<=0 || y>=LANG_SCREEN_H) return;
	uint32_t *s = Screen;
	uint32_t *s_linestart = Screen;
	uint8_t * gb2312_bitmap = (uint8_t *) pixfont.data();
	size_t offset = (((size_t)c0-0xa1)*94+(c1-0xa1))*pixfont_char_bitmap_bytes;
	if( offset > (pixfont_total_chars-1)*pixfont_char_bitmap_bytes )
		return;
	uint8_t * gb2312char_bitmap = gb2312_bitmap + offset;

	int w0 = 0, w1 = 0;
	const int he = std::min(LANG_SCREEN_H-(int)y, (int)pixfont_height);
	for(int h = 0; h < he; h++,
			s_linestart += resW, s = s_linestart,
			gb2312char_bitmap += 2)
	{
		// Byte0 all 8 bits
		w0 = std::min(8, x_limit);
		auto byte0 = gb2312char_bitmap[0];
		for(int i = 0; i < w0; ++i, ++s) {
			if( byte0 & font_bitmask[i] )
				*s = RGBvalue;
			else
				*s = RGBA_TRANSPARENT; //Transparent
		}
		// BYTE1 by pixfont_width - 8
		w1 = std::min((int)pixfont_width - 8, x_limit - 8);
		w1 = w1<0? 0: w1;
		auto byte1 = gb2312char_bitmap[1];
		for(int i = 0; i < w1; ++i, ++s) {
			if( byte1 & font_bitmask[i] )
				*s = RGBvalue;
			else
				*s = RGBA_TRANSPARENT; //Transparent
		}
	}
}

size_t calcGB2312String_pixel_width( const uint8_t *str, size_t width_limit, size_t & endpos )
{
	endpos = 0;
	size_t pix_w = 0;
	while(str[endpos]) {
		auto c0 = str[endpos];
		if( _isControl(c0) ) {
			pix_w += (pixfont_width+1) / 2;
			endpos++;
		} else if( _isAscii(c0) ) {
			pix_w += (pixfont_width+1) / 2;
			endpos++;
		} else if( _isGB2312(c0) ) {
			endpos++;
			auto c1 = str[endpos];
			if(!c1) break;
			if( _isGB2312(c1) ) {
				pix_w += pixfont_width;
				endpos++;
			} else // only half char, error
				break;
		} else { // skip unknown char
			endpos++;
		}

		if( pix_w > width_limit )
			return pix_w;
	}
	return pix_w;
}

void lang_DrawGB2312String( const char *cstr, uint32_t *screen, uint32_t x, uint32_t y, uint32_t resW, uint32_t RGBvalue )
{
	if( y>=LANG_SCREEN_H || x>=LANG_SCREEN_W ) return;
	const uint8_t * str = (const uint8_t *) cstr;
	size_t epos = 0;
	size_t xwidth = calcGB2312String_pixel_width((const uint8_t *)str, LANG_SCREEN_W-x, epos);
	if(!epos) return;
	uint32_t *Screen = screen + x + y * resW;
	for(size_t i = 0; i < epos; i++)
	{
		uint8_t c0 = str[i];
		if( _isAscii(c0) ) {
			lang_DisplayAsciiChar(Screen, c0, resW, x, y, LANG_SCREEN_W - x - 4, RGBvalue);
			x += (pixfont_width+1) / 2;
			Screen += (pixfont_width+1) / 2;
		} else if( _isGB2312(c0) ) {
			uint8_t c1 = str[++i];
			if( _isGB2312(c1) ) {
				lang_DisplayGB2312Char(Screen, c0, c1, resW, x, y, LANG_SCREEN_W - x - 4, RGBvalue);
				x += pixfont_width;
				Screen += pixfont_width;
			} else // only half char, error stop
				return;
		} else if( _isControl(c0) ) {
			x += (pixfont_width+1) / 2;
			Screen += (pixfont_width+1) / 2;
		}
	}
}
