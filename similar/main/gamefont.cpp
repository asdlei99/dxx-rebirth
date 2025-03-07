/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Fonts for the game.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "gr.h"
#include "dxxerror.h"
#include <string.h>
#include "args.h"
#include "physfsx.h"
#include "gamefont.h"
#include "mission.h"
#include "ogl_init.h"
#include "config.h"

#include "d_array.h"
#include "d_enumerate.h"
#include "partial_range.h"

namespace {

constexpr std::array<std::array<char, 16>, 5> Gamefont_filenames_l{{
	{"font1-1.fnt"}, // Font 0
	{"font2-1.fnt"}, // Font 1
	{"font2-2.fnt"}, // Font 2
	{"font2-3.fnt"}, // Font 3
	{"font3-1.fnt"}  // Font 4
}};

constexpr std::array<std::array<char, 16>, 5> Gamefont_filenames_h{{
	{"font1-1h.fnt"}, // Font 0
	{"font2-1h.fnt"}, // Font 1
	{"font2-2h.fnt"}, // Font 2
	{"font2-3h.fnt"}, // Font 3
	{"font3-1h.fnt"}  // Font 4
}};

static int Gamefont_installed;

}

std::array<grs_font_ptr, MAX_FONTS> Gamefonts;
font_x_scale_proportion FNTScaleX(1);
font_y_scale_proportion FNTScaleY(1);

namespace {

//code to allow variable GAME_FONT, added 10/7/99 Matt Mueller - updated 11/18/99 to handle all fonts, not just GFONT_SMALL
//	take scry into account? how/when?
struct a_gamefont_conf
{
	int x;
	int y;
	std::array<char, 16> name;
};

struct gamefont_conf
{
	enum class font_index : std::size_t
	{
		None = SIZE_MAX,
	};
	std::size_t num;
	font_index cur;
	enumerated_array<a_gamefont_conf, 10, font_index> font;
};

static std::array<gamefont_conf, MAX_FONTS> font_conf;

static void gamefont_unloadfont(int gf)
{
	if (Gamefonts[gf]){
		font_conf[gf].cur = gamefont_conf::font_index::None;
		Gamefonts[gf].reset();
	}
}

static void gamefont_loadfont(grs_canvas &canvas, const int gf, const gamefont_conf::font_index fi)
{
	if (PHYSFS_exists(font_conf[gf].font[fi].name.data()))
	{
		gamefont_unloadfont(gf);
		Gamefonts[gf] = gr_init_font(canvas, font_conf[gf].font[fi].name);
	}else {
		if (!Gamefonts[gf]){
			font_conf[gf].cur = gamefont_conf::font_index::None;
			Gamefonts[gf] = gr_init_font(canvas, Gamefont_filenames_l[gf]);
		}
		return;
	}
	font_conf[gf].cur=fi;
}

}

void gamefont_choose_game_font(int scrx,int scry){
	if (!Gamefont_installed) return;

	int close=-1;
	auto m = gamefont_conf::font_index::None;
	for (const auto &&[gf, fc] : enumerate(font_conf))
	{
		for (const auto &&[i, f] : enumerate(partial_range(fc.font, fc.num)))
			if ((scrx >= f.x && close < f.x) && (scry >= f.y && close < f.y))
			{
				close = f.x;
				m=i;
			}
		if (m == gamefont_conf::font_index::None)
			Error("no gamefont found for %ix%i\n",scrx,scry);

#if DXX_USE_OGL
	if (!CGameArg.OglFixedFont)
	{
		// if there's no texture filtering, scale by int
		auto &f = fc.font[m];
		if (CGameCfg.TexFilt == opengl_texture_filter::classic)
		{
			FNTScaleX.reset(scrx / f.x);
			FNTScaleY.reset(scry / f.y);
		}
		else
		{
			FNTScaleX.reset(static_cast<float>(scrx) / f.x);
			FNTScaleY.reset(static_cast<float>(scry) / f.y);
		}

		// keep proportions
#if defined(DXX_BUILD_DESCENT_I)
#define DXX_FONT_SCALE_MULTIPLIER	1
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_FONT_SCALE_MULTIPLIER	100
#endif
		if (FNTScaleY*DXX_FONT_SCALE_MULTIPLIER < FNTScaleX*DXX_FONT_SCALE_MULTIPLIER)
			FNTScaleX.reset(FNTScaleY.operator float());
		else if (FNTScaleX*DXX_FONT_SCALE_MULTIPLIER < FNTScaleY*DXX_FONT_SCALE_MULTIPLIER)
			FNTScaleY.reset(FNTScaleX.operator float());
	}
#endif
		gamefont_loadfont(*grd_curcanv, gf, m);
	}
}

namespace {

static void addfontconf(int gf, int x, int y, const std::array<char, 16> &fn)
{
	if (!PHYSFS_exists(fn.data()))
		return;

	auto &fc = font_conf[gf];
	for (const auto &&[i, f] : enumerate(partial_range(fc.font, fc.num)))
		if (f.x == x && f.y == y)
		{
			if (i == fc.cur)
				gamefont_unloadfont(gf);
			f.name = fn;
			if (i == fc.cur)
				gamefont_loadfont(*grd_curcanv, gf, i);
			return;
		}
	auto &f = fc.font[(gamefont_conf::font_index{fc.num})];
	++ fc.num;
	f.x = x;
	f.y = y;
	f.name = fn;
}

}

namespace dsx {
void gamefont_init()
{
	if (Gamefont_installed)
		return;

	Gamefont_installed = 1;

	for (auto &&[i, gf] : enumerate(Gamefonts))
	{
		gf = nullptr;

		if (!CGameArg.GfxSkipHiresFNT)
			addfontconf(i,640,480,Gamefont_filenames_h[i]); // ZICO - addition to use D2 fonts if available
#if defined(DXX_BUILD_DESCENT_I)
		if (MacHog && (i != 0))
			addfontconf(i,640,480,Gamefont_filenames_l[i]); // Mac fonts are hires (except for the "big" one)
		else
#endif
			addfontconf(i,320,200,Gamefont_filenames_l[i]);
	}

	gamefont_choose_game_font(grd_curscreen->sc_canvas.cv_bitmap.bm_w,grd_curscreen->sc_canvas.cv_bitmap.bm_h);
}
}

void gamefont_close()
{
	if (!Gamefont_installed) return;
	Gamefont_installed = 0;

	for (auto &&[idx, gf] : enumerate(Gamefonts))
	{
		(void)gf;
		gamefont_unloadfont(idx);
	}
}
