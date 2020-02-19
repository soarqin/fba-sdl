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
 *
 * 2020-01-14: Language pack for FinalBurn Alpha by nobk @ github
 */
 
#ifndef _GUI_LANPACK_H_
#define _GUI_LANPACK_H_

#define RGBA_TRANSPARENT 0x00ffffff

typedef void (*lang_DrawString_t)( const char *cstr, uint32_t *screen, 
uint32_t x, uint32_t y, uint32_t resW, uint32_t RGBvalue );

bool gui_load_language_pack();
void lang_DrawString( uint32_t romID, uint32_t * screen,
					  uint32_t x, uint32_t y, uint32_t resW, uint32_t RGBvalue );

#endif
