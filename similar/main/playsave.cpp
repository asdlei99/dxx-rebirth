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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Functions to load & save player's settings (*.plr file)
 *
 */

#include <stdexcept>
#include <stdio.h>
#include <string.h>
#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

#include "dxxerror.h"
#include "strutil.h"
#include "game.h"
#include "player.h"
#include "playsave.h"
#include "joy.h"
#include "digi.h"
#include "newmenu.h"
#include "palette.h"
#include "config.h"
#include "text.h"
#include "gauges.h"
#include "powerup.h"
#include "makesig.h"
#include "u_mem.h"
#include "args.h"
#include "vers_id.h"
#include "newdemo.h"
#include "gauges.h"
#include "nvparse.h"

#include "compiler-range_for.h"
#include "d_range.h"
#include "d_zip.h"
#include "partial_range.h"

#define PLAYER_EFFECTIVENESS_FILENAME_FORMAT	PLAYER_DIRECTORY_STRING("%s.eff")

#define GameNameStr "game_name"
#define GameModeStr "gamemode"
#define RefusePlayersStr "RefusePlayers"
#define DifficultyStr "difficulty"
#define GameFlagsStr "game_flags"
#define AllowedItemsStr "AllowedItems"
#define SpawnGrantedItemsStr "SpawnGrantedItems"
#define DuplicatePrimariesStr "DuplicatePrimaries"
#define DuplicateSecondariesStr "DuplicateSecondaries"
#define DuplicateAccessoriesStr "DuplicateAccessories"
#define ShufflePowerupsStr "ShufflePowerups"
#define AlwaysLightingStr "AlwaysLighting"
#define ShowEnemyNamesStr "ShowEnemyNames"
#define BrightPlayersStr "BrightPlayers"
#define InvulAppearStr "InvulAppear"
#define KillGoalStr "KillGoal"
#define PlayTimeAllowedStr "PlayTimeAllowed"
#define ControlInvulTimeStr "control_invul_time"
#define PacketsPerSecStr "PacketsPerSec"
#define NoFriendlyFireStr "NoFriendlyFire"
#define MouselookFlagsStr "Mouselook"
#define PitchLockFlagsStr "PitchLockRelease"
#define AutosaveIntervalStr	"AutosaveInterval"
#define TrackerStr "Tracker"
#define TrackerNATHPStr "trackernat"
#define NGPVersionStr "ngp version"

#if defined(DXX_BUILD_DESCENT_I)
#define PLX_OPTION_HEADER_TEXT	"[D1X Options]"
#define WEAPON_REORDER_HEADER_TEXT "[weapon reorder]"
#define WEAPON_REORDER_PRIMARY_NAME_TEXT	"primary"
#define WEAPON_REORDER_PRIMARY_VALUE_TEXT	"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x"
#define WEAPON_REORDER_SECONDARY_NAME_TEXT	"secondary"
#define WEAPON_REORDER_SECONDARY_VALUE_TEXT	"0x%x,0x%x,0x%x,0x%x,0x%x,0x%x"
//version 5  ->  6: added new highest level information
//version 6  ->  7: stripped out the old saved_game array.
//version 7 -> 8: readded the old saved_game array since this is needed
//                for shareware saved games
//the shareware is level 4

#define SAVED_GAME_VERSION 8 //increment this every time saved_game struct changes
#define COMPATIBLE_SAVED_GAME_VERSION 4
#define COMPATIBLE_PLAYER_STRUCT_VERSION 16
#elif defined(DXX_BUILD_DESCENT_II)
#define PLX_OPTION_HEADER_TEXT	"[D2X OPTIONS]"
//version 5  ->  6: added new highest level information
//version 6  ->  7: stripped out the old saved_game array.
//version 7  ->  8: added reticle flag, & window size
//version 8  ->  9: removed player_struct_version
//version 9  -> 10: added default display mode
//version 10 -> 11: added all toggles in toggle menu
//version 11 -> 12: added weapon ordering
//version 12 -> 13: added more keys
//version 13 -> 14: took out marker key
//version 14 -> 15: added guided in big window
//version 15 -> 16: added small windows in cockpit
//version 16 -> 17: ??
//version 17 -> 18: save guidebot name
//version 18 -> 19: added automap-highres flag
//version 19 -> 20: added kconfig data for windows joysticks
//version 20 -> 21: save seperate config types for DOS & Windows
//version 21 -> 22: save lifetime netstats 
//version 22 -> 23: ??
//version 23 -> 24: add name of joystick for windows version.

#define PLAYER_FILE_VERSION 24 //increment this every time the player file changes
#define COMPATIBLE_PLAYER_FILE_VERSION 17
#define AllowMarkerViewStr "Allow_marker_view"
#define ThiefAbsenceFlagStr	"ThiefAbsent"
#define ThiefNoEnergyWeaponsFlagStr	"ThiefNoEnergyWeapons"
#define AllowGuidebotStr "AllowGuidebot"
#endif
#define KEYBOARD_HEADER_TEXT	"[keyboard]"
#define SENSITIVITY_NAME_TEXT	"sensitivity"
#define SENSITIVITY_VALUE_TEXT	"%d"
#define LINEAR_NAME_TEXT	"linearity"
#define LINEAR_VALUE_TEXT	"%d"
#define SPEED_NAME_TEXT	        "speed"
#define SPEED_VALUE_TEXT	"%d"
#define DEADZONE_NAME_TEXT	"deadzone"
#define DEADZONE_VALUE_TEXT	"%d"
#define JOYSTICK_HEADER_TEXT	"[joystick]"
#define MOUSE_HEADER_TEXT	"[mouse]"
#define MOUSE_FLIGHTSIM_NAME_TEXT	"flightsim"
#define MOUSE_FLIGHTSIM_VALUE_TEXT	"%d"
#define MOUSE_FSDEAD_NAME_TEXT	"fsdead"
#define MOUSE_FSDEAD_VALUE_TEXT	"%d"
#define MOUSE_FSINDICATOR_NAME_TEXT	"fsindi"
#define MOUSE_FSINDICATOR_VALUE_TEXT	"%d"
#define MOUSE_OVERRUN_NAME_TEXT	"overrun"
#define WEAPON_KEYv2_HEADER_TEXT	"[weapon keys v2]"
#define WEAPON_KEYv2_VALUE_TEXT	"0x%x,0x%x,0x%x"
#define COCKPIT_HEADER_TEXT "[cockpit]"
#define COCKPIT_MODE_NAME_TEXT "mode"
#define COCKPIT_HUD_NAME_TEXT "hud"
#define COCKPIT_RETICLE_TYPE_NAME_TEXT "rettype"
#define COCKPIT_RETICLE_COLOR_NAME_TEXT "retrgba"
#define COCKPIT_RETICLE_SIZE_NAME_TEXT "retsize"
#define TOGGLES_HEADER_TEXT "[toggles]"
#define TOGGLES_BOMBGAUGE_NAME_TEXT "bombgauge"
#define TOGGLES_ESCORTHOTKEYS_NAME_TEXT "escorthotkeys"
#define TOGGLES_PERSISTENTDEBRIS_NAME_TEXT "persistentdebris"
#define TOGGLES_PRSHOT_NAME_TEXT "prshot"
#define TOGGLES_NOREDUNDANCY_NAME_TEXT "noredundancy"
#define TOGGLES_MULTIMESSAGES_NAME_TEXT "multimessages"
#define TOGGLES_MULTIPINGHUD_NAME_TEXT "multipinghud"
#define TOGGLES_NORANKINGS_NAME_TEXT "norankings"
#define TOGGLES_AUTOMAPFREEFLIGHT_NAME_TEXT "automapfreeflight"
#define TOGGLES_NOFIREAUTOSELECT_NAME_TEXT "nofireautoselect"
#define TOGGLES_CYCLEAUTOSELECTONLY_NAME_TEXT "cycleautoselectonly"
#define TOGGLES_FRIENDMISSILEVIEW_NAME_TEXT "friendmissileview"
#define TOGGLES_CLOAKINVULTIMER_NAME_TEXT "cloakinvultimer"
#define TOGGLES_RESPAWN_ANY_KEY	"respawnkey"
#define TOGGLES_MOUSELOOK	"mouselook"
#define TOGGLES_PITCH_LOCK	"pitchlock"
#define TOGGLES_THIEF_ABSENCE_SP	"thiefabsent"
#define TOGGLES_THIEF_NO_ENERGY_WEAPONS_SP	"thiefnoenergyweapons"
#define TOGGLES_AUTOSAVE_INTERVAL_SP	"autosaveinterval"
#define GRAPHICS_HEADER_TEXT "[graphics]"
#define GRAPHICS_ALPHAEFFECTS_NAME_TEXT "alphaeffects"
#define GRAPHICS_DYNLIGHTCOLOR_NAME_TEXT "dynlightcolor"
#define PLX_VERSION_HEADER_TEXT "[plx version]"
#define END_TEXT	"[end]"

#define SAVE_FILE_ID MAKE_SIG('D','P','L','R')

struct player_config PlayerCfg;
namespace dsx {
namespace {
#if defined(DXX_BUILD_DESCENT_I)
static void plyr_read_stats();
static std::array<saved_game_sw, N_SAVE_SLOTS> saved_games;
#elif defined(DXX_BUILD_DESCENT_II)
static inline void plyr_read_stats() {}
static int get_lifetime_checksum (int a,int b);
#endif

template <typename weapon_type_out, typename weapon_type_from_file, std::size_t N>
static void check_weapon_reorder(std::array<weapon_type_out, N> &wo, const std::array<weapon_type_from_file, N> &in)
{
	/* Examine the input array and update `m` to represent which elements were
	 * found.  A well-formed input array will have exactly one of each value in
	 * the range [0..N-1], and one occurrence of the sentinel `255`.  Since
	 * only `N` elements are present, any duplicate value means some other
	 * value will be missing.  If an out of range value is found, the loop
	 * breaks early, because that also guarantees that at least one required
	 * element will be missing.
	 */
	constexpr weapon_type_from_file cycle_never_autoselect_below{255};
	uint_fast32_t m = 0;
	for (auto &&[w, i] : zip(wo, in))
	{
		if (i == cycle_never_autoselect_below)
			m |= 1 << N;
		else if (i < N - 1)
			m |= 1 << i;
		else
			break;
		w = i;
	}
	/* If `m` is equal to the value below, then every desired element appeared
	 * in the input sequence.  If the input sequence contained duplicates or
	 * out of range values, then the test on `m` will fail, and the output
	 * array will be reset to a sane default.
	 */
	if (m != ((1 << N) | ((1 << (N - 1)) - 1)))
	{
		wo[0] = cycle_never_autoselect_below;
		range_for (const uint_fast32_t i, xrange(1u, N))
			wo[i] = i - 1;
	}
}
}

void new_player_config()
{
#if defined(DXX_BUILD_DESCENT_I)
	range_for (auto &i, saved_games)
		i.name[0] = 0;
#endif
	InitWeaponOrdering (); //setup default weapon priorities
	PlayerCfg.ControlType=0; // Assume keyboard
	PlayerCfg.RespawnMode = RespawnPress::Any;
	PlayerCfg.MouselookFlags = 0;
	PlayerCfg.PitchLockFlags = 0;
	PlayerCfg.KeySettings = DefaultKeySettings;
	PlayerCfg.KeySettingsRebirth = DefaultKeySettingsRebirth;
	kc_set_controls();

	PlayerCfg.DefaultDifficulty = DEFAULT_DIFFICULTY;
	PlayerCfg.AutoLeveling = 1;
	PlayerCfg.NHighestLevels = 1;
	PlayerCfg.HighestLevels[0].Shortname[0] = 0; //no name for mission 0
	PlayerCfg.HighestLevels[0].LevelNum = 1; //was highest level in old struct
	{
		auto &k = PlayerCfg.KeyboardSens;
		k[player_config_keyboard_index::turn_lr] = k[player_config_keyboard_index::pitch_ud] = k[player_config_keyboard_index::slide_lr] = k[player_config_keyboard_index::slide_ud] = k[player_config_keyboard_index::bank_lr] = 16;
	}
	{
		auto &j = PlayerCfg.JoystickSens;
		j[player_config_joystick_index::turn_lr] = j[player_config_joystick_index::pitch_ud] = j[player_config_joystick_index::slide_lr] = j[player_config_joystick_index::slide_ud] = j[player_config_joystick_index::bank_lr] = j[player_config_joystick_index::throttle] = 8;
	}
	{
		auto &j = PlayerCfg.JoystickDead;
		j[player_config_joystick_index::turn_lr] = j[player_config_joystick_index::pitch_ud] = j[player_config_joystick_index::slide_lr] = j[player_config_joystick_index::slide_ud] = j[player_config_joystick_index::bank_lr] = j[player_config_joystick_index::throttle] = 0;
	}
	{
		auto &j = PlayerCfg.JoystickLinear;
		j[player_config_joystick_index::turn_lr] = j[player_config_joystick_index::pitch_ud] = j[player_config_joystick_index::slide_lr] = j[player_config_joystick_index::slide_ud] = j[player_config_joystick_index::bank_lr] = j[player_config_joystick_index::throttle] = 0;
	}
	{
		auto &j = PlayerCfg.JoystickSpeed;
		j[player_config_joystick_index::turn_lr] = j[player_config_joystick_index::pitch_ud] = j[player_config_joystick_index::slide_lr] = j[player_config_joystick_index::slide_ud] = j[player_config_joystick_index::bank_lr] = j[player_config_joystick_index::throttle] = 16;
	}
	PlayerCfg.MouseFlightSim = 0;
	{
		auto &m = PlayerCfg.MouseSens;
		m[player_config_mouse_index::turn_lr] = m[player_config_mouse_index::pitch_ud] = m[player_config_mouse_index::slide_lr] = m[player_config_mouse_index::slide_ud] = m[player_config_mouse_index::bank_lr] = m[player_config_mouse_index::throttle] = 8;
	}
	{
		auto &m = PlayerCfg.MouseOverrun;
		m[player_config_mouse_index::turn_lr] = m[player_config_mouse_index::pitch_ud] = m[player_config_mouse_index::slide_lr] = m[player_config_mouse_index::slide_ud] = m[player_config_mouse_index::bank_lr] = m[player_config_mouse_index::throttle] = 0;
	}
	PlayerCfg.MouseFSDead = 0;
	PlayerCfg.MouseFSIndicator = 1;
	PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = cockpit_mode_t::full_cockpit;
	PlayerCfg.ReticleType = RET_TYPE_CLASSIC;
	PlayerCfg.ReticleRGBA[0] = RET_COLOR_DEFAULT_R; PlayerCfg.ReticleRGBA[1] = RET_COLOR_DEFAULT_G; PlayerCfg.ReticleRGBA[2] = RET_COLOR_DEFAULT_B; PlayerCfg.ReticleRGBA[3] = RET_COLOR_DEFAULT_A;
	PlayerCfg.ReticleSize = 0;
	PlayerCfg.HudMode = HudType::Standard;
#if defined(DXX_BUILD_DESCENT_I)
	PlayerCfg.BombGauge = 1;
#elif defined(DXX_BUILD_DESCENT_II)
	PlayerCfg.Cockpit3DView = {};
	PlayerCfg.ThiefModifierFlags = 0;
	PlayerCfg.MissileViewEnabled = MissileViewMode::EnabledSelfOnly;
	PlayerCfg.HeadlightActiveDefault = 1;
	PlayerCfg.GuidedInBigWindow = 0;
	PlayerCfg.GuidebotName = "GUIDE-BOT";
	PlayerCfg.GuidebotNameReal = PlayerCfg.GuidebotName;
	PlayerCfg.EscortHotKeys = 1;
#endif
	PlayerCfg.PersistentDebris = 0;
	PlayerCfg.PRShot = 0;
	PlayerCfg.NoRedundancy = 0;
	PlayerCfg.MultiMessages = 0;
        PlayerCfg.MultiPingHud = 0;
	PlayerCfg.NoRankings = 0;
	PlayerCfg.AutomapFreeFlight = 0;
	PlayerCfg.NoFireAutoselect = FiringAutoselectMode::Immediate;
	PlayerCfg.CycleAutoselectOnly = 0;
        PlayerCfg.CloakInvulTimer = 0;
	PlayerCfg.AlphaEffects = 0;
	PlayerCfg.DynLightColor = 0;

	// Default taunt macros
#if defined(DXX_BUILD_DESCENT_I)
	PlayerCfg.NetworkMessageMacro[0].copy_if(TXT_DEF_MACRO_1);
	PlayerCfg.NetworkMessageMacro[1].copy_if(TXT_DEF_MACRO_2);
	PlayerCfg.NetworkMessageMacro[2].copy_if(TXT_DEF_MACRO_3);
	PlayerCfg.NetworkMessageMacro[3].copy_if(TXT_DEF_MACRO_4);
#elif defined(DXX_BUILD_DESCENT_II)
	PlayerCfg.NetworkMessageMacro[0] = "Why can't we all just get along?";
	PlayerCfg.NetworkMessageMacro[1] = "Hey, I got a present for ya";
	PlayerCfg.NetworkMessageMacro[2] = "I got a hankerin' for a spankerin'";
	PlayerCfg.NetworkMessageMacro[3] = "This one's headed for Uranus";
#endif
	PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
}
}

namespace {

static int convert_pattern_array(const std::span<const char> name, const std::span<int> array, const char *word, const char *line)
{
	if (memcmp(word, name.data(), name.size() - 1))
		return 0;
	char *p;
	const unsigned long which = strtoul(word + name.size() - 1, &p, 10);
	if (*p || which >= array.size())
		return 0;
	array[which] = strtol(line, NULL, 10);
	return 1;
}

static void print_pattern_array(PHYSFS_File *fout, const char *name, const std::span<const int> array)
{
	for (std::size_t i = 0; i < array.size(); ++i)
		PHYSFSX_printf(fout,"%s%" DXX_PRI_size_type "=%d\n", name, i, array[i]);
}

static const char *splitword(char *line, char c)
{
	char *p = strchr(line, c);
	if (p)
	{
		*p = 0;
		return p + 1;
	}
	return p;
}
}

namespace dsx {
namespace {
static void read_player_dxx(const char *filename)
{
	plyr_read_stats();

	auto f = PHYSFSX_openReadBuffered(filename).first;
	if (!f)
		return;

	PlayerCfg.SPGameplayOptions.AutosaveInterval = std::chrono::minutes(10);
	for (PHYSFSX_gets_line_t<50> line; PHYSFSX_fgets(line,f);)
	{
#if defined(DXX_BUILD_DESCENT_I)
		if (!strcmp(line, WEAPON_REORDER_HEADER_TEXT))
		{
			while(PHYSFSX_fgets(line, f) && strcmp(line, END_TEXT))
			{
				const char *value=splitword(line,'=');
				if (!value)
					continue;
#define CONVERT_WEAPON_REORDER_VALUE(A,F)	\
	if (std::array<unsigned, 6> wo; sscanf(value, F, &wo[0], &wo[1], &wo[2], &wo[3], &wo[4], &wo[5]) == 6)	\
		check_weapon_reorder(A, wo)
				if(!strcmp(line,WEAPON_REORDER_PRIMARY_NAME_TEXT))
				{
					CONVERT_WEAPON_REORDER_VALUE(PlayerCfg.PrimaryOrder, WEAPON_REORDER_PRIMARY_VALUE_TEXT);
				}
				else if(!strcmp(line,WEAPON_REORDER_SECONDARY_NAME_TEXT))
				{
					CONVERT_WEAPON_REORDER_VALUE(PlayerCfg.SecondaryOrder, WEAPON_REORDER_SECONDARY_VALUE_TEXT);
				}
			}
		}
		else
#endif
		if (!strcmp(line,KEYBOARD_HEADER_TEXT))
		{
			while(PHYSFSX_fgets(line, f) && strcmp(line, END_TEXT))
			{
				const char *value=splitword(line,'=');
				if (!value)
					continue;
				convert_pattern_array(SENSITIVITY_NAME_TEXT, PlayerCfg.KeyboardSens, line, value);
			}
		}
		else if (!strcmp(line,JOYSTICK_HEADER_TEXT))
		{
			while(PHYSFSX_fgets(line, f) && strcmp(line, END_TEXT))
			{
				const char *value=splitword(line,'=');
				if (!value)
					continue;
				convert_pattern_array(SENSITIVITY_NAME_TEXT, PlayerCfg.JoystickSens, line, value) ||
				convert_pattern_array(LINEAR_NAME_TEXT, PlayerCfg.JoystickLinear, line, value) ||
				convert_pattern_array(SPEED_NAME_TEXT, PlayerCfg.JoystickSpeed, line, value) ||
				convert_pattern_array(DEADZONE_NAME_TEXT, PlayerCfg.JoystickDead, line, value);
			}
		}
		else if (!strcmp(line,MOUSE_HEADER_TEXT))
		{
			while(PHYSFSX_fgets(line, f) && strcmp(line, END_TEXT))
			{
				const char *value=splitword(line,'=');
				if (!value)
					continue;
				if(!strcmp(line,MOUSE_FLIGHTSIM_NAME_TEXT))
					PlayerCfg.MouseFlightSim = atoi(value);
				else if (convert_pattern_array(SENSITIVITY_NAME_TEXT, PlayerCfg.MouseSens, line, value))
					;
				else if (convert_pattern_array(MOUSE_OVERRUN_NAME_TEXT, PlayerCfg.MouseOverrun, line, value))
					;
				else if(!strcmp(line,MOUSE_FSDEAD_NAME_TEXT))
					PlayerCfg.MouseFSDead = atoi(value);
				else if(!strcmp(line,MOUSE_FSINDICATOR_NAME_TEXT))
					PlayerCfg.MouseFSIndicator = atoi(value);
			}
		}
		else if (!strcmp(line,WEAPON_KEYv2_HEADER_TEXT))
		{
			while(PHYSFSX_fgets(line,f) && strcmp(line,END_TEXT))
			{
				const char *value=splitword(line,'=');
				if (!value)
					continue;
				unsigned int kc1=0,kc2=0,kc3=0;
				int i=atoi(line);
				
				if(i==0) i=10;
					i=(i-1)*3;
		
				if (sscanf(value,WEAPON_KEYv2_VALUE_TEXT,&kc1,&kc2,&kc3) != 3)
					continue;
				PlayerCfg.KeySettingsRebirth[i]   = kc1;
				PlayerCfg.KeySettingsRebirth[i+1] = kc2;
				PlayerCfg.KeySettingsRebirth[i+2] = kc3;
			}
		}
		else if (!strcmp(line,COCKPIT_HEADER_TEXT))
		{
			while(PHYSFSX_fgets(line,f) && strcmp(line,END_TEXT))
			{
				const char *value=splitword(line,'=');
				if (!value)
					continue;
#if defined(DXX_BUILD_DESCENT_I)
				if(!strcmp(line,COCKPIT_MODE_NAME_TEXT))
					PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = static_cast<cockpit_mode_t>(atoi(value));
				else
#endif
				if(!strcmp(line,COCKPIT_HUD_NAME_TEXT))
					PlayerCfg.HudMode = static_cast<HudType>(atoi(value));
				else if(!strcmp(line,COCKPIT_RETICLE_TYPE_NAME_TEXT))
					PlayerCfg.ReticleType = atoi(value);
				else if(!strcmp(line,COCKPIT_RETICLE_COLOR_NAME_TEXT))
					sscanf(value,"%i,%i,%i,%i",&PlayerCfg.ReticleRGBA[0],&PlayerCfg.ReticleRGBA[1],&PlayerCfg.ReticleRGBA[2],&PlayerCfg.ReticleRGBA[3]);
				else if(!strcmp(line,COCKPIT_RETICLE_SIZE_NAME_TEXT))
					PlayerCfg.ReticleSize = atoi(value);
			}
		}
		else if (!strcmp(line,TOGGLES_HEADER_TEXT))
		{
			PlayerCfg.MouselookFlags = 0;
			PlayerCfg.PitchLockFlags = 0;
#if defined(DXX_BUILD_DESCENT_II)
			PlayerCfg.ThiefModifierFlags = 0;
#endif
			while(PHYSFSX_fgets(line,f) && strcmp(line,END_TEXT))
			{
				const char *value=splitword(line,'=');
				if (!value)
					continue;
#if defined(DXX_BUILD_DESCENT_I)
				if(!strcmp(line,TOGGLES_BOMBGAUGE_NAME_TEXT))
					PlayerCfg.BombGauge = atoi(value);
#elif defined(DXX_BUILD_DESCENT_II)
				if(!strcmp(line,TOGGLES_ESCORTHOTKEYS_NAME_TEXT))
					PlayerCfg.EscortHotKeys = atoi(value);
				else if (!strcmp(line, TOGGLES_THIEF_ABSENCE_SP))
				{
					if (strtoul(value, 0, 10))
						PlayerCfg.ThiefModifierFlags |= ThiefModifier::Absent;
				}
				else if (!strcmp(line, TOGGLES_THIEF_NO_ENERGY_WEAPONS_SP))
				{
					if (strtoul(value, 0, 10))
						PlayerCfg.ThiefModifierFlags |= ThiefModifier::NoEnergyWeapons;
				}
#endif
				else if (!strcmp(line, TOGGLES_AUTOSAVE_INTERVAL_SP))
				{
					const auto l = strtoul(value, 0, 10);
					PlayerCfg.SPGameplayOptions.AutosaveInterval = std::chrono::seconds(l);
				}
				if(!strcmp(line,TOGGLES_PERSISTENTDEBRIS_NAME_TEXT))
					PlayerCfg.PersistentDebris = atoi(value);
				if(!strcmp(line,TOGGLES_PRSHOT_NAME_TEXT))
					PlayerCfg.PRShot = atoi(value);
				if(!strcmp(line,TOGGLES_NOREDUNDANCY_NAME_TEXT))
					PlayerCfg.NoRedundancy = atoi(value);
				if(!strcmp(line,TOGGLES_MULTIMESSAGES_NAME_TEXT))
					PlayerCfg.MultiMessages = atoi(value);
				if(!strcmp(line,TOGGLES_MULTIPINGHUD_NAME_TEXT))
					PlayerCfg.MultiPingHud = atoi(value);
				if(!strcmp(line,TOGGLES_NORANKINGS_NAME_TEXT))
					PlayerCfg.NoRankings = atoi(value);
				if(!strcmp(line,TOGGLES_AUTOMAPFREEFLIGHT_NAME_TEXT))
					PlayerCfg.AutomapFreeFlight = atoi(value);
				if(!strcmp(line,TOGGLES_NOFIREAUTOSELECT_NAME_TEXT))
					PlayerCfg.NoFireAutoselect = static_cast<FiringAutoselectMode>(atoi(value));
				if(!strcmp(line,TOGGLES_CYCLEAUTOSELECTONLY_NAME_TEXT))
					PlayerCfg.CycleAutoselectOnly = atoi(value);
				if(!strcmp(line,TOGGLES_CLOAKINVULTIMER_NAME_TEXT))
					PlayerCfg.CloakInvulTimer = atoi(value);
				else if (!strcmp(line, TOGGLES_RESPAWN_ANY_KEY))
					PlayerCfg.RespawnMode = static_cast<RespawnPress>(atoi(value));
				else if (!strcmp(line, TOGGLES_MOUSELOOK))
					PlayerCfg.MouselookFlags = strtoul(value, 0, 10);
				else if (!strcmp(line, TOGGLES_PITCH_LOCK))
					PlayerCfg.PitchLockFlags = strtoul(value, 0, 10);
			}
		}
		else if (!strcmp(line,GRAPHICS_HEADER_TEXT))
		{
			while(PHYSFSX_fgets(line,f) && strcmp(line,END_TEXT))
			{
				const char *value=splitword(line,'=');
				if (!value)
					continue;
				if(!strcmp(line,GRAPHICS_ALPHAEFFECTS_NAME_TEXT))
					PlayerCfg.AlphaEffects = atoi(value);
				if(!strcmp(line,GRAPHICS_DYNLIGHTCOLOR_NAME_TEXT))
					PlayerCfg.DynLightColor = atoi(value);
			}
		}
		else if (!strcmp(line,PLX_VERSION_HEADER_TEXT)) // know the version this pilot was used last with - allow modifications
		{
#if 0
			int v1=0,v2=0,v3=0;
#endif
			while(PHYSFSX_fgets(line,f) && strcmp(line,END_TEXT))
			{
#if 0
				const char *value=splitword(line,'=');
				if (!value)
					continue;
				sscanf(value,"%i.%i.%i",&v1,&v2,&v3);
#endif
			}
		}
		else if (!strcmp(line,END_TEXT))
		{
			break;
		}
		else if (!strcmp(line,PLX_OPTION_HEADER_TEXT))
		{
			// No action needed
		}
		else
		{
			while(PHYSFSX_fgets(line,f) && strcmp(line,END_TEXT))
			{
			}
		}
	}
}
}

#if defined(DXX_BUILD_DESCENT_I)
namespace {
constexpr char effcode1[]="d1xrocks_SKCORX!D";
constexpr char effcode2[]="AObe)7Rn1 -+/zZ'0";
constexpr char effcode3[]="aoeuidhtnAOEUIDH6";
constexpr char effcode4[]="'/.;]<{=,+?|}->[3";

static const uint8_t *decode_stat(const uint8_t *p,int *v,const char *effcode)
{
	unsigned char c;
	int neg,i;

	if (p[0]==0)
		return NULL;
	else if (p[0]>='a'){
		neg=1;/*I=p[0]-'a';*/
	}else{
		neg=0;/*I=p[0]-'A';*/
	}
	i=0;*v=0;
	p++;
	while (p[i*2] && p[i*2+1] && p[i*2]!=' '){
		c=(p[i*2]-33)+((p[i*2+1]-33)<<4);
		c^=effcode[i+neg];
		*v+=c << (i*8);
		i++;
	}
	if (neg)
	     *v *= -1;
	if (!p[i*2])
		return NULL;
	return p+(i*2);
}

static void plyr_read_stats_v(int *k, int *d)
{
	char filename[PATH_MAX];
	int k1=-1,k2=0,d1=-1,d2=0;
	*k=0;*d=0;//in case the file doesn't exist.
	memset(filename, '\0', PATH_MAX);
	snprintf(filename,sizeof(filename),PLAYER_EFFECTIVENESS_FILENAME_FORMAT,static_cast<const char *>(get_local_player().callsign));
	if (auto f = PHYSFSX_openReadBuffered(filename).first)
	{
		PHYSFSX_gets_line_t<256> line;
		if (!PHYSFS_eof(f) && PHYSFSX_fgets(line, f))
		{
			 const char *value=splitword(line,':');
			 if(!strcmp(line,"kills") && value)
				*k=atoi(value);
		}
		if (!PHYSFS_eof(f) && PHYSFSX_fgets(line, f))
                {
			 const char *value=splitword(line,':');
			 if(!strcmp(line,"deaths") && value)
				*d=atoi(value);
		 }
		if (!PHYSFS_eof(f) && PHYSFSX_fgets(line, f))
		{
			 const char *value=splitword(line,':');
			 if(value && !strcmp(line,"key") && strlen(value)>10){
				 if (value[0]=='0' && value[1]=='1'){
					 const uint8_t *p;
					 if ((p=decode_stat(reinterpret_cast<const unsigned char *>(value + 3), &k1, effcode1))&&
					     (p=decode_stat(p+1,&k2,effcode2))&&
					     (p=decode_stat(p+1,&d1,effcode3))){
						 decode_stat(p+1,&d2,effcode4);
					 }
				 }
			 }
		}
		if (k1!=k2 || k1!=*k || d1!=d2 || d1!=*d)
		{
			*k=0;*d=0;
		}
	}
}

static void plyr_read_stats()
{
	plyr_read_stats_v(&PlayerCfg.NetlifeKills,&PlayerCfg.NetlifeKilled);
}
}

void plyr_save_stats()
{
	int kills = PlayerCfg.NetlifeKills,deaths = PlayerCfg.NetlifeKilled, neg, i;
	char filename[PATH_MAX];
	std::array<uint8_t, 16> buf, buf2;
	uint8_t a;
	memset(filename, '\0', PATH_MAX);
	snprintf(filename,sizeof(filename),PLAYER_EFFECTIVENESS_FILENAME_FORMAT,static_cast<const char *>(get_local_player().callsign));
	auto f = PHYSFSX_openWriteBuffered(filename).first;
	if(!f)
		return; //broken!

	PHYSFSX_printf(f,"kills:%i\n",kills);
	PHYSFSX_printf(f,"deaths:%i\n",deaths);
	PHYSFSX_puts_literal(f, "key:01 ");

	if (kills < 0)
	{
		neg=1;
		kills*=-1;
	}
	else
		neg=0;

	for (i=0;kills;i++)
	{
		a=(kills & 0xFF) ^ effcode1[i+neg];
		buf[i*2]=(a&0xF)+33;
		buf[i*2+1]=(a>>4)+33;
		a=(kills & 0xFF) ^ effcode2[i+neg];
		buf2[i*2]=(a&0xF)+33;
		buf2[i*2+1]=(a>>4)+33;
		kills>>=8;
	}

	buf[i*2]=0;
	buf2[i*2]=0;

	if (neg)
		i+='a';
	else
		i+='A';

	PHYSFSX_printf(f,"%c%s %c%s ",i,buf.data(),i,buf2.data());

	if (deaths < 0)
	{
		neg=1;
		deaths*=-1;
	}else
		neg=0;

	for (i=0;deaths;i++)
	{
		a=(deaths & 0xFF) ^ effcode3[i+neg];
		buf[i*2]=(a&0xF)+33;
		buf[i*2+1]=(a>>4)+33;
		a=(deaths & 0xFF) ^ effcode4[i+neg];
		buf2[i*2]=(a&0xF)+33;
		buf2[i*2+1]=(a>>4)+33;
		deaths>>=8;
	}

	buf[i*2]=0;
	buf2[i*2]=0;

	if (neg)
		i+='a';
	else
		i+='A';

	PHYSFSX_printf(f, "%c%s %c%s\n", i, buf.data(), i, buf2.data());
}
#endif

namespace {
static int write_player_dxx(const char *filename)
{
	int rc=0;
	char tempfile[PATH_MAX];

	strcpy(tempfile,filename);
	tempfile[strlen(tempfile)-4]=0;
	strcat(tempfile,".pl$");
	auto fout = PHYSFSX_openWriteBuffered(tempfile).first;
	if (!fout && CGameArg.SysUsePlayersDir)
	{
		PHYSFS_mkdir(PLAYER_DIRECTORY_STRING(""));	//try making directory
		fout = PHYSFSX_openWriteBuffered(tempfile).first;
	}
	
	if(fout)
	{
		PHYSFSX_puts_literal(fout,
							PLX_OPTION_HEADER_TEXT "\n"
#if defined(DXX_BUILD_DESCENT_I)
							WEAPON_REORDER_HEADER_TEXT "\n"
#endif
			);
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(fout,WEAPON_REORDER_PRIMARY_NAME_TEXT "=" WEAPON_REORDER_PRIMARY_VALUE_TEXT "\n",PlayerCfg.PrimaryOrder[0], PlayerCfg.PrimaryOrder[1], PlayerCfg.PrimaryOrder[2],PlayerCfg.PrimaryOrder[3], PlayerCfg.PrimaryOrder[4], PlayerCfg.PrimaryOrder[5]);
		PHYSFSX_printf(fout,WEAPON_REORDER_SECONDARY_NAME_TEXT "=" WEAPON_REORDER_SECONDARY_VALUE_TEXT "\n",PlayerCfg.SecondaryOrder[0], PlayerCfg.SecondaryOrder[1], PlayerCfg.SecondaryOrder[2],PlayerCfg.SecondaryOrder[3], PlayerCfg.SecondaryOrder[4], PlayerCfg.SecondaryOrder[5]);
#endif
		PHYSFSX_puts_literal(fout,
#if defined(DXX_BUILD_DESCENT_I)
							END_TEXT "\n"
#endif
							KEYBOARD_HEADER_TEXT "\n");
		print_pattern_array(fout, SENSITIVITY_NAME_TEXT, PlayerCfg.KeyboardSens);
		PHYSFSX_puts_literal(fout,
							END_TEXT "\n"
							JOYSTICK_HEADER_TEXT "\n"
							);
		print_pattern_array(fout, SENSITIVITY_NAME_TEXT, PlayerCfg.JoystickSens);
		print_pattern_array(fout, LINEAR_NAME_TEXT, PlayerCfg.JoystickLinear);
		print_pattern_array(fout, SPEED_NAME_TEXT, PlayerCfg.JoystickSpeed);
		print_pattern_array(fout, DEADZONE_NAME_TEXT, PlayerCfg.JoystickDead);
		PHYSFSX_puts_literal(fout,
							END_TEXT "\n"
							MOUSE_HEADER_TEXT "\n"
							);
		PHYSFSX_printf(fout,MOUSE_FLIGHTSIM_NAME_TEXT "=" MOUSE_FLIGHTSIM_VALUE_TEXT "\n",PlayerCfg.MouseFlightSim);
		print_pattern_array(fout, SENSITIVITY_NAME_TEXT, PlayerCfg.MouseSens);
                print_pattern_array(fout, MOUSE_OVERRUN_NAME_TEXT, PlayerCfg.MouseOverrun);
		PHYSFSX_printf(fout,MOUSE_FSDEAD_NAME_TEXT "=" MOUSE_FSDEAD_VALUE_TEXT "\n",PlayerCfg.MouseFSDead);
		PHYSFSX_printf(fout,MOUSE_FSINDICATOR_NAME_TEXT "=" MOUSE_FSINDICATOR_VALUE_TEXT "\n",PlayerCfg.MouseFSIndicator);
		PHYSFSX_puts_literal(fout,
							END_TEXT "\n"
							WEAPON_KEYv2_HEADER_TEXT "\n"
							);
		PHYSFSX_printf(fout,"1=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[0],PlayerCfg.KeySettingsRebirth[1],PlayerCfg.KeySettingsRebirth[2]);
		PHYSFSX_printf(fout,"2=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[3],PlayerCfg.KeySettingsRebirth[4],PlayerCfg.KeySettingsRebirth[5]);
		PHYSFSX_printf(fout,"3=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[6],PlayerCfg.KeySettingsRebirth[7],PlayerCfg.KeySettingsRebirth[8]);
		PHYSFSX_printf(fout,"4=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[9],PlayerCfg.KeySettingsRebirth[10],PlayerCfg.KeySettingsRebirth[11]);
		PHYSFSX_printf(fout,"5=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[12],PlayerCfg.KeySettingsRebirth[13],PlayerCfg.KeySettingsRebirth[14]);
		PHYSFSX_printf(fout,"6=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[15],PlayerCfg.KeySettingsRebirth[16],PlayerCfg.KeySettingsRebirth[17]);
		PHYSFSX_printf(fout,"7=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[18],PlayerCfg.KeySettingsRebirth[19],PlayerCfg.KeySettingsRebirth[20]);
		PHYSFSX_printf(fout,"8=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[21],PlayerCfg.KeySettingsRebirth[22],PlayerCfg.KeySettingsRebirth[23]);
		PHYSFSX_printf(fout,"9=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[24],PlayerCfg.KeySettingsRebirth[25],PlayerCfg.KeySettingsRebirth[26]);
		PHYSFSX_printf(fout,"0=" WEAPON_KEYv2_VALUE_TEXT "\n",PlayerCfg.KeySettingsRebirth[27],PlayerCfg.KeySettingsRebirth[28],PlayerCfg.KeySettingsRebirth[29]);
		PHYSFSX_puts_literal(fout,
							END_TEXT "\n"
							COCKPIT_HEADER_TEXT "\n"
							);
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(fout, COCKPIT_MODE_NAME_TEXT "=%i\n", underlying_value(PlayerCfg.CockpitMode[0]));
#endif
		PHYSFSX_printf(fout,COCKPIT_HUD_NAME_TEXT "=%u\n", static_cast<unsigned>(PlayerCfg.HudMode));
		PHYSFSX_printf(fout,COCKPIT_RETICLE_TYPE_NAME_TEXT "=%i\n",PlayerCfg.ReticleType);
		PHYSFSX_printf(fout,COCKPIT_RETICLE_COLOR_NAME_TEXT "=%i,%i,%i,%i\n",PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2],PlayerCfg.ReticleRGBA[3]);
		PHYSFSX_printf(fout,COCKPIT_RETICLE_SIZE_NAME_TEXT "=%i\n",PlayerCfg.ReticleSize);
		PHYSFSX_puts_literal(fout,
							END_TEXT "\n"
							TOGGLES_HEADER_TEXT "\n"
							);
#if defined(DXX_BUILD_DESCENT_I)
		PHYSFSX_printf(fout,TOGGLES_BOMBGAUGE_NAME_TEXT "=%i\n",PlayerCfg.BombGauge);
#elif defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_printf(fout,TOGGLES_ESCORTHOTKEYS_NAME_TEXT "=%i\n",PlayerCfg.EscortHotKeys);
		PHYSFSX_printf(fout, TOGGLES_THIEF_ABSENCE_SP "=%i\n", PlayerCfg.ThiefModifierFlags & ThiefModifier::Absent);
		PHYSFSX_printf(fout, TOGGLES_THIEF_NO_ENERGY_WEAPONS_SP "=%i\n", PlayerCfg.ThiefModifierFlags & ThiefModifier::NoEnergyWeapons);
#endif
		PHYSFSX_printf(fout, TOGGLES_AUTOSAVE_INTERVAL_SP "=%i\n", PlayerCfg.SPGameplayOptions.AutosaveInterval.count());
		PHYSFSX_printf(fout,TOGGLES_PERSISTENTDEBRIS_NAME_TEXT "=%i\n",PlayerCfg.PersistentDebris);
		PHYSFSX_printf(fout,TOGGLES_PRSHOT_NAME_TEXT "=%i\n",PlayerCfg.PRShot);
		PHYSFSX_printf(fout,TOGGLES_NOREDUNDANCY_NAME_TEXT "=%i\n",PlayerCfg.NoRedundancy);
		PHYSFSX_printf(fout,TOGGLES_MULTIMESSAGES_NAME_TEXT "=%i\n",PlayerCfg.MultiMessages);
		PHYSFSX_printf(fout,TOGGLES_MULTIPINGHUD_NAME_TEXT "=%i\n",PlayerCfg.MultiPingHud);
		PHYSFSX_printf(fout,TOGGLES_NORANKINGS_NAME_TEXT "=%i\n",PlayerCfg.NoRankings);
		PHYSFSX_printf(fout,TOGGLES_AUTOMAPFREEFLIGHT_NAME_TEXT "=%i\n",PlayerCfg.AutomapFreeFlight);
		PHYSFSX_printf(fout,TOGGLES_NOFIREAUTOSELECT_NAME_TEXT "=%i\n",static_cast<unsigned>(PlayerCfg.NoFireAutoselect));
		PHYSFSX_printf(fout,TOGGLES_CYCLEAUTOSELECTONLY_NAME_TEXT "=%i\n",PlayerCfg.CycleAutoselectOnly);
                PHYSFSX_printf(fout,TOGGLES_CLOAKINVULTIMER_NAME_TEXT "=%i\n",PlayerCfg.CloakInvulTimer);
		PHYSFSX_printf(fout,TOGGLES_RESPAWN_ANY_KEY "=%i\n",static_cast<unsigned>(PlayerCfg.RespawnMode));
		PHYSFSX_printf(fout, TOGGLES_MOUSELOOK "=%i\n", PlayerCfg.MouselookFlags);
		PHYSFSX_printf(fout, TOGGLES_PITCH_LOCK "=%i\n", PlayerCfg.PitchLockFlags);
		PHYSFSX_puts_literal(fout,
							END_TEXT "\n"
							GRAPHICS_HEADER_TEXT "\n"
							);
		PHYSFSX_printf(fout,GRAPHICS_ALPHAEFFECTS_NAME_TEXT "=%i\n",PlayerCfg.AlphaEffects);
		PHYSFSX_printf(fout,GRAPHICS_DYNLIGHTCOLOR_NAME_TEXT "=%i\n",PlayerCfg.DynLightColor);
		PHYSFSX_puts_literal(fout, END_TEXT "\n"
							PLX_VERSION_HEADER_TEXT "\n"
							"plx version=" DXX_VERSION_STR "\n"
							END_TEXT "\n"
							END_TEXT "\n"
							);
		fout.reset();
		if(rc==0)
		{
			PHYSFS_delete(filename);
			rc = PHYSFSX_rename(tempfile,filename);
		}
		return rc;
	}
	else
		return errno;
}
}

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	char filename[PATH_MAX];
#if defined(DXX_BUILD_DESCENT_I)
	int shareware_file = -1;
	int player_file_size;
#elif defined(DXX_BUILD_DESCENT_II)
	int rewrite_it=0;
	int swap = 0;
	short player_file_version;
#endif

	Assert(Player_num < MAX_PLAYERS);

	memset(filename, '\0', PATH_MAX);
	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.plr"), static_cast<const char *>(InterfaceUniqueState.PilotName));
	if (!PHYSFSX_exists(filename,0))
		return ENOENT;
	auto &&[file, physfserr] = PHYSFSX_openReadBuffered(filename);
	if (!file)
	{
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Failed to open PLR file\n%s\n\n%s", filename, PHYSFS_getErrorByCode(physfserr));
		return -1;
	}

	new_player_config(); // Set defaults!

#if defined(DXX_BUILD_DESCENT_I)
	// Unfortunatly d1x has been writing both shareware and registered
	// player files with a saved_game_version of 7 and 8, whereas the
	// original decent used 4 for shareware games and 7 for registered
	// games. Because of this the player files didn't get properly read
	// when reading d1x shareware player files in d1x registered or
	// vica versa. The problem is that the sizeof of the taunt macros
	// differ between the share and registered versions, causing the
	// reading of the player file to go wrong. Thus we now determine the
	// sizeof the player file to determine what kinda player file we are
	// dealing with so that we can do the right thing
	PHYSFS_seek(file, 0);
	player_file_size = PHYSFS_fileLength(file);
#endif
	int id;
	PHYSFS_readSLE32(file, &id);
#if defined(DXX_BUILD_DESCENT_I)
	short saved_game_version, player_struct_version;
	saved_game_version = PHYSFSX_readShort(file);
	player_struct_version = PHYSFSX_readShort(file);
	PlayerCfg.NHighestLevels = PHYSFSX_readInt(file);
	{
		const unsigned u = PHYSFSX_readInt(file);
		PlayerCfg.DefaultDifficulty = cast_clamp_difficulty(u);
	}
	PlayerCfg.AutoLeveling = PHYSFSX_readInt(file);
#elif defined(DXX_BUILD_DESCENT_II)
	player_file_version = PHYSFSX_readShort(file);
#endif

	if (id!=SAVE_FILE_ID) {
		struct error_invalid_player_file_magic : passive_messagebox
		{
			error_invalid_player_file_magic() :
				passive_messagebox(menu_title{TXT_ERROR}, menu_subtitle{"Invalid player file"}, TXT_OK, grd_curscreen->sc_canvas)
			{
			}
		};
		run_blocking_newmenu<error_invalid_player_file_magic>();
		return -1;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (saved_game_version < COMPATIBLE_SAVED_GAME_VERSION || player_struct_version < COMPATIBLE_PLAYER_STRUCT_VERSION) {
		struct error_invalid_player_file_version : passive_messagebox
		{
			error_invalid_player_file_version() :
				passive_messagebox(menu_title{TXT_ERROR}, menu_subtitle{TXT_ERROR_PLR_VERSION}, TXT_OK, grd_curscreen->sc_canvas)
			{
			}
		};
		run_blocking_newmenu<error_invalid_player_file_version>();
		return -1;
	}

	/* determine if we're dealing with a shareware or registered playerfile */
	switch (saved_game_version)
	{
		case 4:
			shareware_file = 1;
			break;
		case 5:
		case 6:
			shareware_file = 0;
			break;
		case 7:
			/* version 7 doesn't have the saved games array */
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2212 - sizeof(saved_games)))
				shareware_file = 1;
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2252 - sizeof(saved_games)))
				shareware_file = 0;
			break;
		case 8:
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == 2212)
				shareware_file = 1;
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == 2252)
				shareware_file = 0;
			/* d1x-rebirth v0.31 to v0.42 broke things by adding stuff to the
			   player struct without thinking (sigh) */
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2212 + 2*sizeof(int)))
			{

				shareware_file = 1;
				/* skip the cruft added to the player_info struct */
				PHYSFS_seek(file, PHYSFS_tell(file)+2*sizeof(int));
			}
			if ((player_file_size - (sizeof(hli)*PlayerCfg.NHighestLevels)) == (2252 + 2*sizeof(int)))
			{
				shareware_file = 0;
				/* skip the cruft added to the player_info struct */
				PHYSFS_seek(file, PHYSFS_tell(file)+2*sizeof(int));
			}
	}

	if (shareware_file == -1) {
		struct error_invalid_player_file_size : passive_messagebox
		{
			error_invalid_player_file_size() :
				passive_messagebox(menu_title{TXT_ERROR}, menu_subtitle{"Error invalid or unknown\nplayerfile-size"}, TXT_OK, grd_curscreen->sc_canvas)
			{
			}
		};
		run_blocking_newmenu<error_invalid_player_file_size>();
		return -1;
	}

	if (saved_game_version <= 5) {

		//deal with old-style highest level info

		PlayerCfg.HighestLevels[0].Shortname[0] = 0;							//no name for mission 0
		PlayerCfg.HighestLevels[0].LevelNum = PlayerCfg.NHighestLevels;	//was highest level in old struct

		//This hack allows the player to start on level 8 if he's made it to
		//level 7 on the shareware.  We do this because the shareware didn't
		//save the information that the player finished level 7, so the most
		//we know is that he made it to level 7.
		if (PlayerCfg.NHighestLevels==7)
			PlayerCfg.HighestLevels[0].LevelNum = 8;
		
	}
	else {	//read new highest level info
		if (PHYSFS_read(file,PlayerCfg.HighestLevels,sizeof(hli),PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)
			goto read_player_file_failed;
	}

	if ( saved_game_version != 7 ) {	// Read old & SW saved games.
		if (PHYSFS_read(file,saved_games,sizeof(saved_games),1) != 1)
			goto read_player_file_failed;
	}

#elif defined(DXX_BUILD_DESCENT_II)
	if (player_file_version > 255) // bigendian file?
		swap = 1;

	if (swap)
		player_file_version = SWAPSHORT(player_file_version);

	if (player_file_version < COMPATIBLE_PLAYER_FILE_VERSION) {
		struct error_invalid_player_file_version : passive_messagebox
		{
			error_invalid_player_file_version() :
				passive_messagebox(menu_title{TXT_ERROR}, menu_subtitle{TXT_ERROR_PLR_VERSION}, TXT_OK, grd_curscreen->sc_canvas)
			{
			}
		};
		run_blocking_newmenu<error_invalid_player_file_version>();
		return -1;
	}

	PHYSFS_seek(file,PHYSFS_tell(file)+2*sizeof(short)); //skip Game_window_w,Game_window_h
	PlayerCfg.DefaultDifficulty = cast_clamp_difficulty(PHYSFSX_readByte(file));
	PlayerCfg.AutoLeveling       = PHYSFSX_readByte(file);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); // skip ReticleOn
	PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = static_cast<cockpit_mode_t>(PHYSFSX_readByte(file));
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); //skip Default_display_mode
	{
		auto i = PHYSFSX_readByte(file);
		switch (i)
		{
			case static_cast<unsigned>(MissileViewMode::None):
			case static_cast<unsigned>(MissileViewMode::EnabledSelfOnly):
			case static_cast<unsigned>(MissileViewMode::EnabledSelfAndAllies):
				break;
			default:
				i = 0;
				break;
		}
		PlayerCfg.MissileViewEnabled = static_cast<MissileViewMode>(i);
	}
	PlayerCfg.HeadlightActiveDefault  = PHYSFSX_readByte(file);
	PlayerCfg.GuidedInBigWindow      = PHYSFSX_readByte(file);
	if (player_file_version >= 19)
		PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); //skip Automap_always_hires

	//read new highest level info

	PlayerCfg.NHighestLevels = PHYSFSX_readShort(file);
	if (swap)
		PlayerCfg.NHighestLevels = SWAPSHORT(PlayerCfg.NHighestLevels);
	Assert(PlayerCfg.NHighestLevels <= MAX_MISSIONS);

	if (PHYSFS_read(file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)
		goto read_player_file_failed;
#endif

	//read taunt macros
	{
		int len;

#if defined(DXX_BUILD_DESCENT_I)
		len = shareware_file? 25:35;
#elif defined(DXX_BUILD_DESCENT_II)
		len = MAX_MESSAGE_LEN;
#endif

		for (auto &i : PlayerCfg.NetworkMessageMacro)
			if (PHYSFS_read(file, i, len, 1) != 1)
				goto read_player_file_failed;
	}

	//read kconfig data
	{
		ubyte dummy_joy_sens;

		if (PHYSFS_read(file, &PlayerCfg.KeySettings.Keyboard, sizeof(PlayerCfg.KeySettings.Keyboard), 1) != 1)
			goto read_player_file_failed;
#if DXX_MAX_JOYSTICKS
		auto &KeySettingsJoystick = PlayerCfg.KeySettings.Joystick;
#else
		std::array<uint8_t, MAX_CONTROLS> KeySettingsJoystick;
#endif
		if (PHYSFS_read(file, &KeySettingsJoystick, sizeof(KeySettingsJoystick), 1) != 1)
			goto read_player_file_failed;
		PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS*3) ); // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
		if (PHYSFS_read(file, &PlayerCfg.KeySettings.Mouse, sizeof(PlayerCfg.KeySettings.Mouse), 1) != 1)
			goto read_player_file_failed;
		PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS) ); // Skip obsolete Cyberman map field
#if defined(DXX_BUILD_DESCENT_I)
		if (PHYSFS_read(file, &PlayerCfg.ControlType, sizeof(ubyte), 1 )!=1)
#elif defined(DXX_BUILD_DESCENT_II)
		if (player_file_version>=20)
			PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS) ); // Skip obsolete Winjoy map field
		uint8_t control_type_dos, control_type_win;
		if (PHYSFS_read(file, &control_type_dos, sizeof(ubyte), 1) != 1)
			goto read_player_file_failed;
		else if (player_file_version >= 21 && PHYSFS_read(file, &control_type_win, sizeof(ubyte), 1) != 1)
#endif
			goto read_player_file_failed;
		else if (PHYSFS_read(file, &dummy_joy_sens, sizeof(ubyte), 1) !=1 )
			goto read_player_file_failed;

#if defined(DXX_BUILD_DESCENT_II)
		PlayerCfg.ControlType = control_type_dos;
	
		std::array<uint8_t, 22> weapon_file_order{};
		std::array<uint8_t, 11> primary_order, secondary_order;
		PHYSFS_read(file, weapon_file_order.data(), weapon_file_order.size(), 1);
		range_for (const unsigned i, xrange(11u))
		{
			primary_order[i] = weapon_file_order[i * 2];
			secondary_order[i] = weapon_file_order[(i * 2) + 1];
		}
		check_weapon_reorder(PlayerCfg.PrimaryOrder, primary_order);
		check_weapon_reorder(PlayerCfg.SecondaryOrder, secondary_order);

		if (player_file_version>=16)
		{
			PHYSFS_sint32 view_primary, view_secondary;
			PHYSFS_readSLE32(file, &view_primary);
			PHYSFS_readSLE32(file, &view_secondary);
			if (swap)
			{
				view_primary = SWAPINT(view_primary);
				view_secondary = SWAPINT(view_secondary);
			}
			if (view_primary <= static_cast<unsigned>(cockpit_3d_view::Marker))
				PlayerCfg.Cockpit3DView[gauge_inset_window_view::primary] = static_cast<cockpit_3d_view>(view_primary);
			if (view_secondary <= static_cast<unsigned>(cockpit_3d_view::Marker))
				PlayerCfg.Cockpit3DView[gauge_inset_window_view::secondary] = static_cast<cockpit_3d_view>(view_secondary);
		}
#endif
	}

#if defined(DXX_BUILD_DESCENT_I)
	if ( saved_game_version != 7 ) 	{
		int i, found=0;
		
		Assert( N_SAVE_SLOTS == 10 );

		for (i=0; i < N_SAVE_SLOTS; i++ )	{
			if ( saved_games[i].name[0] )	{
				throw std::runtime_error("old save game not supported");
				// make sure we do not do this again, which would possibly overwrite
				// a new newstyle savegame
				saved_games[i].name[0] = 0;
				found++;
			}
		}
		if (found)
			write_player_file();
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (player_file_version>=22)
	{
		PHYSFS_readSLE32(file, &PlayerCfg.NetlifeKills);
		PHYSFS_readSLE32(file, &PlayerCfg.NetlifeKilled);
		if (swap) {
			PlayerCfg.NetlifeKills = SWAPINT(PlayerCfg.NetlifeKills);
			PlayerCfg.NetlifeKilled = SWAPINT(PlayerCfg.NetlifeKilled);
		}
	}
	else
	{
		PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
	}

	if (player_file_version>=23)
	{
		int i;
		PHYSFS_readSLE32(file, &i);
		if (swap)
			i = SWAPINT(i);
		if (i!=get_lifetime_checksum (PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled))
		{
			PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
			struct error_invalid_player_file_checksum : passive_messagebox
			{
				error_invalid_player_file_checksum() :
					passive_messagebox(menu_title{nullptr}, menu_subtitle{"Lifetime kill error"}, TXT_OK, grd_curscreen->sc_canvas)
				{
				}
			};
			run_blocking_newmenu<error_invalid_player_file_checksum>();
			rewrite_it=1;
		}
	}

	//read guidebot name
	if (player_file_version >= 18 && PHYSFSX_fgets(PlayerCfg.GuidebotName, file))
	{
	}
	else
		PlayerCfg.GuidebotName = "GUIDE-BOT";
	PlayerCfg.GuidebotNameReal = PlayerCfg.GuidebotName;
	{
		if (player_file_version >= 24) 
		{
			PHYSFSX_gets_line_t<128> buf;
			if (PHYSFSX_fgets(buf, file))			// Just read it in fpr DPS.
			{
				/* Nothing to do.  Buffer contents are ignored.  This is only
				 * read for its side effect on the file position.
				 */
			}
		}
	}
#endif

#if defined(DXX_BUILD_DESCENT_II)
	if (rewrite_it)
		write_player_file();
#endif

	filename[strlen(filename) - 4] = 0;
	strcat(filename, ".plx");
	read_player_dxx(filename);
	kc_set_controls();

	return EZERO;

 read_player_file_failed:
	nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "%s\n\n%s", "Error reading PLR file", PHYSFS_getLastError());
	return -1;
}
}

namespace {
/* Given a Mission_path, return a pair of pointers.
 * - If the mission cannot be saved, both pointers are nullptr.
 * - If the mission name was previously used, return a pointer to that
 *   entry and a pointer to end.
 * - If the mission name is not recorded, return a pointer to the first
 *   unused element (which may be end() if all elements are used) and a
 *   pointer to end().  The caller must check that the first unused
 *   element is not end().
 */
static std::array<std::array<hli, MAX_MISSIONS>::pointer, 2> find_hli_entry(const ranges::subrange<hli *> r, const Mission_path &m)
{
	const auto mission_filename = m.filename;
	const auto mission_length = std::distance(mission_filename, m.path.end());
	if (mission_length >= sizeof(hli::Shortname))
	{
		/* Name is too long to store, so there will never be a match
		 * and it is never stored.
		 */
		con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "warning: highest level information cannot be tracked because the mission name is too long to store (should be at most 8 bytes, but is \"%s\")."), &*mission_filename);
		return {{nullptr, nullptr}};
	}
	const auto &&a = [p = &*mission_filename](const hli &h) {
		return !d_stricmp(h.Shortname.data(), p);
	};
	const auto &&i = ranges::find_if(r, a);
	return {{&*i, r.end()}};
}
}

//set a new highest level for player for this mission
void set_highest_level(const uint8_t best_levelnum_this_game)
{
	int ret;

	if ((ret=read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return;

	const auto &&r = partial_range(PlayerCfg.HighestLevels, PlayerCfg.NHighestLevels);
	const auto &&i = find_hli_entry(r, *Current_mission);
	const auto ie = i[1];
	if (!ie)
		return;

	auto ii = i[0];
	const hli *d1_preferred = nullptr;
#if defined(DXX_BUILD_DESCENT_II)
	const hli *d2_preferred = nullptr, *d2x_preferred = nullptr;
#endif
	range_for (auto &m, r)
	{
		hli *preferred;
		/* Swapping instead of rotating will perturb the order a bit,
		 * but this is a one-time fix to get builtin missions to
		 * reserved storage.
		 */
		const auto ms = m.Shortname.data();
		if (!d1_preferred && !strcmp(D1_MISSION_FILENAME, ms))
			d1_preferred = preferred = &PlayerCfg.HighestLevels[0];
#if defined(DXX_BUILD_DESCENT_II)
		else if (!d2_preferred && !strcmp(FULL_MISSION_FILENAME, ms))
			d2_preferred = preferred = &PlayerCfg.HighestLevels[1];
		else if (!d2x_preferred && !strcmp("d2x", ms))
			d2x_preferred = preferred = &PlayerCfg.HighestLevels[2];
#endif
		else
			continue;
		if (preferred == &m)
			continue;
		std::swap(*preferred, m);
	}
	const unsigned reserved_slots = !!d1_preferred
#if defined(DXX_BUILD_DESCENT_II)
		+ !!d2_preferred + !!d2x_preferred
#endif
		;
	auto &hl = PlayerCfg.HighestLevels;
	const auto irs = &hl[reserved_slots];

	uint8_t previous_best_levelnum = 0;
	if (ii == ie)
	{
		/*
		 * If ii == ie, the mission is unknown.  If there is no free
		 * space available, move everything, so that the
		 * least-recently-used element (at *irs) is discarded to make
		 * room to add this mission as most recently used.
		 *
		 * Leave previous_best_levelnum set to 0, so that any progress
		 * at all qualifies for a save.
		 */
		if (ie == PlayerCfg.HighestLevels.end())
		{
			std::move(std::next(irs), ie, irs);
			--ii;
		}
		else
			PlayerCfg.NHighestLevels++;
		ii->Shortname.back() = 0;
		strncpy(ii->Shortname.data(), &*Current_mission->filename, ii->Shortname.size() - 1);
	}
	else if (ii != ie - 1)
	{
		/* If this mission is not the most recently used, reorder the
		 * list so that it becomes the most recently used.
		 *
		 * Leave previous_best_levelnum set to 0, so that a save is
		 * required, even if the player has not set a new record.
		 */
		std::rotate(ii, std::next(ii), ie);
		ii = ie - 1;
	}
	else
		/* Update previous_best_levelnum so that progress is only saved
		 * if this is a new best.
		 */
		previous_best_levelnum = ii->LevelNum;

	if (previous_best_levelnum < best_levelnum_this_game)
	{
		auto &best_levelnum_recorded = ii->LevelNum;
		if (best_levelnum_recorded < best_levelnum_this_game)
			best_levelnum_recorded = best_levelnum_this_game;
		write_player_file();
	}
}

//gets the player's highest level from the file for this mission
int get_highest_level(void)
{
	read_player_file();
	const Mission_path &mission = *Current_mission;
	// Destination Saturn.
	const auto &&r = partial_range(PlayerCfg.HighestLevels, PlayerCfg.NHighestLevels);
#if defined(DXX_BUILD_DESCENT_I)
	unsigned highest_saturn_level = 0;
	if (!*mission.filename)
		range_for (auto &m, r)
			if (!d_stricmp("DESTSAT", m.Shortname.data()))
				highest_saturn_level = m.LevelNum;
#endif
	const auto &&i = find_hli_entry(r, mission);
	const auto ie = i[1];
	const auto ii = i[0];
	if (ii == ie)
		return 0;
	auto LevelNum = ii->LevelNum;
#if defined(DXX_BUILD_DESCENT_I)
	/* Override `LevelNum` of the full campaign with the player's
	 * progress in the OEM mini-campaign, so that users who switch can
	 * keep their progress.
	 *
	 * If the player never played Destination Saturn, or if this is not
	 * the built-in campaign, highest_saturn_level will be 0 and this
	 * will be skipped.
	 */
	if (LevelNum < highest_saturn_level)
		LevelNum = highest_saturn_level;
#endif
	return LevelNum;
}

//write out player's saved games.  returns errno (0 == no error)
namespace dsx {
void write_player_file()
{
	char filename[PATH_MAX];
	int errno_ret;

	if ( Newdemo_state == ND_STATE_PLAYBACK )
		return;

	errno_ret = WriteConfigFile();

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.plx"), static_cast<const char *>(InterfaceUniqueState.PilotName));
	write_player_dxx(filename);
	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.plr"), static_cast<const char *>(InterfaceUniqueState.PilotName));
	auto file = PHYSFSX_openWriteBuffered(filename).first;
	if (!file)
		return;

	//Write out player's info
	PHYSFS_writeULE32(file, SAVE_FILE_ID);
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_writeULE16(file, SAVED_GAME_VERSION);
	PHYSFS_writeULE16(file, PLAYER_STRUCT_VERSION);
	PHYSFS_writeSLE32(file, PlayerCfg.NHighestLevels);
	PHYSFS_writeSLE32(file, underlying_value(PlayerCfg.DefaultDifficulty));
	PHYSFS_writeSLE32(file, PlayerCfg.AutoLeveling);
	errno_ret = EZERO;

	//write higest level info
	if ((PHYSFS_write( file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)) {
		errno_ret = errno;
		return;
	}

	if (PHYSFS_write( file, saved_games,sizeof(saved_games),1) != 1) {
		errno_ret = errno;
		return;
	}

	range_for (auto &i, PlayerCfg.NetworkMessageMacro)
		if (PHYSFS_write(file, i.data(), i.size(), 1) != 1) {
		errno_ret = errno;
		return;
	}

	//write kconfig info
	{
		if (PHYSFS_write(file, PlayerCfg.KeySettings.Keyboard, sizeof(PlayerCfg.KeySettings.Keyboard), 1) != 1)
			errno_ret=errno;
#if DXX_MAX_JOYSTICKS
		auto &KeySettingsJoystick = PlayerCfg.KeySettings.Joystick;
#else
		const std::array<uint8_t, MAX_CONTROLS> KeySettingsJoystick{};
#endif
		if (PHYSFS_write(file, KeySettingsJoystick, sizeof(KeySettingsJoystick), 1) != 1)
			errno_ret=errno;
		for (unsigned i = 0; i < MAX_CONTROLS*3; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
				errno_ret=errno;
		if (PHYSFS_write(file, PlayerCfg.KeySettings.Mouse, sizeof(PlayerCfg.KeySettings.Mouse), 1) != 1)
			errno_ret=errno;
		{
			std::array<uint8_t, MAX_CONTROLS> cyberman{};
			if (PHYSFS_write(file, cyberman.data(), cyberman.size(), 1) != 1)	// Skip obsolete Cyberman map field
				errno_ret=errno;
		}
	
		if(errno_ret == EZERO)
		{
			ubyte old_avg_joy_sensitivity = 8;
			if (PHYSFS_write( file,  &PlayerCfg.ControlType, sizeof(ubyte), 1 )!=1)
				errno_ret=errno;
			else if (PHYSFS_write( file, &old_avg_joy_sensitivity, sizeof(ubyte), 1 )!=1)
				errno_ret=errno;
		}
	}

	if (!file.close())
		errno_ret = errno;

	if (errno_ret != EZERO) {
		PHYSFS_delete(filename);			//delete bogus file
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "%s\n\n%s",TXT_ERROR_WRITING_PLR, strerror(errno_ret));
	}
#elif defined(DXX_BUILD_DESCENT_II)
	(void)errno_ret;
	PHYSFS_writeULE16(file, PLAYER_FILE_VERSION);

	
	PHYSFS_seek(file,PHYSFS_tell(file)+2*(sizeof(PHYSFS_uint16))); // skip Game_window_w, Game_window_h
	PHYSFSX_writeU8(file, underlying_value(PlayerCfg.DefaultDifficulty));
	PHYSFSX_writeU8(file, PlayerCfg.AutoLeveling);
	PHYSFSX_writeU8(file, PlayerCfg.ReticleType==RET_TYPE_NONE?0:1);
	PHYSFSX_writeU8(file, underlying_value(PlayerCfg.CockpitMode[0]));
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(PHYSFS_uint8)); // skip Default_display_mode
	PHYSFSX_writeU8(file, static_cast<uint8_t>(PlayerCfg.MissileViewEnabled));
	PHYSFSX_writeU8(file, PlayerCfg.HeadlightActiveDefault);
	PHYSFSX_writeU8(file, PlayerCfg.GuidedInBigWindow);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(PHYSFS_uint8)); // skip Automap_always_hires

	//write higest level info
	PHYSFS_writeULE16(file, PlayerCfg.NHighestLevels);
	if ((PHYSFS_write(file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels))
		goto write_player_file_failed;

	range_for (auto &i, PlayerCfg.NetworkMessageMacro)
		if (PHYSFS_write(file, i.data(), i.size(), 1) != 1)
		goto write_player_file_failed;

	//write kconfig info
	{

		ubyte old_avg_joy_sensitivity = 8;
		ubyte control_type_dos = PlayerCfg.ControlType;

		if (PHYSFS_write(file, PlayerCfg.KeySettings.Keyboard, sizeof(PlayerCfg.KeySettings.Keyboard), 1) != 1)
			goto write_player_file_failed;
#if DXX_MAX_JOYSTICKS
		auto &KeySettingsJoystick = PlayerCfg.KeySettings.Joystick;
#else
		const std::array<uint8_t, MAX_CONTROLS> KeySettingsJoystick{};
#endif
		if (PHYSFS_write(file, KeySettingsJoystick, sizeof(KeySettingsJoystick), 1) != 1)
			goto write_player_file_failed;
		for (unsigned i = 0; i < MAX_CONTROLS*3; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
				goto write_player_file_failed;
		if (PHYSFS_write(file, PlayerCfg.KeySettings.Mouse, sizeof(PlayerCfg.KeySettings.Mouse), 1) != 1)
			goto write_player_file_failed;
		for (unsigned i = 0; i < MAX_CONTROLS*2; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Cyberman/Winjoy map fields
				goto write_player_file_failed;
		if (PHYSFS_write(file, &control_type_dos, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		ubyte control_type_win = 0;
		if (PHYSFS_write(file, &control_type_win, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		if (PHYSFS_write(file, &old_avg_joy_sensitivity, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;

		range_for (const unsigned i, xrange(11u))
		{
			PHYSFS_write(file, &PlayerCfg.PrimaryOrder[i], sizeof(ubyte), 1);
			PHYSFS_write(file, &PlayerCfg.SecondaryOrder[i], sizeof(ubyte), 1);
		}

		PHYSFS_writeULE32(file, static_cast<unsigned>(PlayerCfg.Cockpit3DView[gauge_inset_window_view::primary]));
		PHYSFS_writeULE32(file, static_cast<unsigned>(PlayerCfg.Cockpit3DView[gauge_inset_window_view::secondary]));

		PHYSFS_writeULE32(file, PlayerCfg.NetlifeKills);
		PHYSFS_writeULE32(file, PlayerCfg.NetlifeKilled);
		int i=get_lifetime_checksum (PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled);
		PHYSFS_writeULE32(file, i);
	}

	//write guidebot name
	PHYSFSX_writeString(file, PlayerCfg.GuidebotNameReal);

	{
		char buf[128];
		strcpy(buf, "DOS joystick");
		PHYSFSX_writeString(file, buf);		// Write out current joystick for player.
	}

	if (!file.close())
		goto write_player_file_failed;

	return;

 write_player_file_failed:
	nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "%s\n\n%s", TXT_ERROR_WRITING_PLR, PHYSFS_getLastError());
	if (file)
	{
		file.reset();
		PHYSFS_delete(filename);        //delete bogus file
	}
#endif
}

namespace {
#if defined(DXX_BUILD_DESCENT_II)
static int get_lifetime_checksum (int a,int b)
{
  int num;

  // confusing enough to beat amateur disassemblers? Lets hope so

  num=(a<<8 ^ b);
  num^=(a | b);
  num*=num>>2;
  return (num);
}
#endif

template <uint_fast32_t shift, uint_fast32_t width>
static void convert_duplicate_powerup_integer(packed_netduplicate_items &d, const char *value)
{
	if (auto r = convert_integer<unsigned>(value); !r)
		return;
	else if (const auto i = *r; !(i & ~((1 << width) - 1)))
		d.set_sub_field<shift, width>(i);
}
}

// read stored values from ngp file to netgame_info
void read_netgame_profile(netgame_info *ng)
{
	char filename[PATH_MAX];
#if DXX_USE_TRACKER
	ng->TrackerNATWarned = TrackerNATHolePunchWarn::Unset;
#endif

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.ngp"), static_cast<const char *>(InterfaceUniqueState.PilotName));
	auto file = PHYSFSX_openReadBuffered(filename).first;
	if (!file)
		return;

	ng->MPGameplayOptions.AutosaveInterval = std::chrono::minutes(10);
	// NOTE that we do not set any defaults here or even initialize netgame_info. For flexibility, leave that to the function calling this.
	for (PHYSFSX_gets_line_t<50> line; const char *const eol = PHYSFSX_fgets(line, file);)
	{
		const auto lb = line.begin();
		if (eol == line.end())
			continue;
		auto eq = std::find(lb, eol, '=');
		if (eq == eol)
			continue;
		auto value = std::next(eq);
		if (cmp(lb, eq, GameNameStr))
			convert_string(ng->game_name, value, eol);
		else if (cmp(lb, eq, GameModeStr))
		{
			if (auto gamemode = convert_integer<uint8_t>(value))
				ng->gamemode = network_game_type{*gamemode};
		}
		else if (cmp(lb, eq, RefusePlayersStr))
			convert_integer(ng->RefusePlayers, value);
		else if (cmp(lb, eq, DifficultyStr))
		{
			if (auto difficulty = convert_integer<uint8_t>(value))
				ng->difficulty = cast_clamp_difficulty(*difficulty);
		}
		else if (cmp(lb, eq, GameFlagsStr))
		{
			if (auto r = convert_integer<uint8_t>(value))
			{
				const packed_game_flags p{*r};
				ng->game_flag = unpack_game_flags(&p);
			}
		}
		else if (cmp(lb, eq, AllowedItemsStr))
			convert_integer(ng->AllowedItems, value);
		else if (cmp(lb, eq, SpawnGrantedItemsStr))
			convert_integer(ng->SpawnGrantedItems.mask, value);
		else if (cmp(lb, eq, DuplicatePrimariesStr))
			convert_duplicate_powerup_integer<packed_netduplicate_items::primary_shift, packed_netduplicate_items::primary_width>(ng->DuplicatePowerups, value);
		else if (cmp(lb, eq, DuplicateSecondariesStr))
			convert_duplicate_powerup_integer<packed_netduplicate_items::secondary_shift, packed_netduplicate_items::secondary_width>(ng->DuplicatePowerups, value);
#if defined(DXX_BUILD_DESCENT_II)
		else if (cmp(lb, eq, DuplicateAccessoriesStr))
			convert_duplicate_powerup_integer<packed_netduplicate_items::accessory_shift, packed_netduplicate_items::accessory_width>(ng->DuplicatePowerups, value);
		else if (cmp(lb, eq, AllowMarkerViewStr))
			convert_integer(ng->Allow_marker_view, value);
		else if (cmp(lb, eq, AlwaysLightingStr))
			convert_integer(ng->AlwaysLighting, value);
		else if (cmp(lb, eq, ThiefAbsenceFlagStr))
		{
			if (strtoul(value, 0, 10))
				ng->ThiefModifierFlags |= ThiefModifier::Absent;
		}
		else if (cmp(lb, eq, ThiefNoEnergyWeaponsFlagStr))
		{
			if (strtoul(value, 0, 10))
				ng->ThiefModifierFlags |= ThiefModifier::NoEnergyWeapons;
		}
		else if (cmp(lb, eq, AllowGuidebotStr))
			convert_integer(ng->AllowGuidebot, value);
#endif
		else if (cmp(lb, eq, ShufflePowerupsStr))
			convert_integer(ng->ShufflePowerupSeed, value);
		else if (cmp(lb, eq, ShowEnemyNamesStr))
			convert_integer(ng->ShowEnemyNames, value);
		else if (cmp(lb, eq, BrightPlayersStr))
			convert_integer(ng->BrightPlayers, value);
		else if (cmp(lb, eq, InvulAppearStr))
			convert_integer(ng->InvulAppear, value);
		else if (cmp(lb, eq, KillGoalStr))
			convert_integer(ng->KillGoal, value);
		else if (cmp(lb, eq, PlayTimeAllowedStr))
		{
			if (const auto r = convert_integer<int>(value))
				ng->PlayTimeAllowed = std::chrono::duration<int, netgame_info::play_time_allowed_abi_ratio>(*r);
		}
		else if (cmp(lb, eq, ControlInvulTimeStr))
			convert_integer(ng->control_invul_time, value);
		else if (cmp(lb, eq, PacketsPerSecStr))
			convert_integer(ng->PacketsPerSec, value);
		else if (cmp(lb, eq, NoFriendlyFireStr))
			convert_integer(ng->NoFriendlyFire, value);
		else if (cmp(lb, eq, MouselookFlagsStr))
			convert_integer(ng->MouselookFlags, value);
		else if (cmp(lb, eq, PitchLockFlagsStr))
			convert_integer(ng->PitchLockFlags, value);
		else if (cmp(lb, eq, AutosaveIntervalStr))
		{
			if (const auto r = convert_integer<uint16_t>(value))
				ng->MPGameplayOptions.AutosaveInterval = std::chrono::seconds(*r);
		}
#if DXX_USE_TRACKER
		else if (cmp(lb, eq, TrackerStr))
			convert_integer(ng->Tracker, value);
		else if (cmp(lb, eq, TrackerNATHPStr))
			ng->TrackerNATWarned = static_cast<TrackerNATHolePunchWarn>(strtoul(value, 0, 10));
#endif
	}
}

// write values from netgame_info to ngp file
void write_netgame_profile(netgame_info *ng)
{
	char filename[PATH_MAX];
	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.8s.ngp"), static_cast<const char *>(InterfaceUniqueState.PilotName));
	auto file = PHYSFSX_openWriteBuffered(filename).first;
	if (!file)
		return;

	PHYSFSX_printf(file, GameNameStr "=%s\n", ng->game_name.data());
	PHYSFSX_printf(file, GameModeStr "=%i\n", underlying_value(ng->gamemode));
	PHYSFSX_printf(file, RefusePlayersStr "=%i\n", ng->RefusePlayers);
	PHYSFSX_printf(file, DifficultyStr "=%i\n", underlying_value(ng->difficulty));
	PHYSFSX_printf(file, GameFlagsStr "=%i\n", pack_game_flags(&ng->game_flag).value);
	PHYSFSX_printf(file, AllowedItemsStr "=%i\n", ng->AllowedItems);
	PHYSFSX_printf(file, SpawnGrantedItemsStr "=%i\n", ng->SpawnGrantedItems.mask);
	PHYSFSX_printf(file, DuplicatePrimariesStr "=%" PRIuFAST32 "\n", ng->DuplicatePowerups.get_primary_count());
	PHYSFSX_printf(file, DuplicateSecondariesStr "=%" PRIuFAST32 "\n", ng->DuplicatePowerups.get_secondary_count());
#if defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_printf(file, DuplicateAccessoriesStr "=%" PRIuFAST32 "\n", ng->DuplicatePowerups.get_accessory_count());
	PHYSFSX_printf(file, AllowMarkerViewStr "=%i\n", ng->Allow_marker_view);
	PHYSFSX_printf(file, AlwaysLightingStr "=%i\n", ng->AlwaysLighting);
	PHYSFSX_printf(file, ThiefAbsenceFlagStr "=%i\n", ng->ThiefModifierFlags & ThiefModifier::Absent);
	PHYSFSX_printf(file, ThiefNoEnergyWeaponsFlagStr "=%i\n", ng->ThiefModifierFlags & ThiefModifier::NoEnergyWeapons);
	PHYSFSX_printf(file, AllowGuidebotStr "=%i\n", ng->AllowGuidebot);
#endif
	PHYSFSX_printf(file, ShufflePowerupsStr "=%i\n", !!ng->ShufflePowerupSeed);
	PHYSFSX_printf(file, ShowEnemyNamesStr "=%i\n", ng->ShowEnemyNames);
	PHYSFSX_printf(file, BrightPlayersStr "=%i\n", ng->BrightPlayers);
	PHYSFSX_printf(file, InvulAppearStr "=%i\n", ng->InvulAppear);
	PHYSFSX_printf(file, KillGoalStr "=%i\n", ng->KillGoal);
	PHYSFSX_printf(file, PlayTimeAllowedStr "=%i\n", std::chrono::duration_cast<std::chrono::duration<int, netgame_info::play_time_allowed_abi_ratio>>(ng->PlayTimeAllowed).count());
	PHYSFSX_printf(file, ControlInvulTimeStr "=%i\n", ng->control_invul_time);
	PHYSFSX_printf(file, PacketsPerSecStr "=%i\n", ng->PacketsPerSec);
	PHYSFSX_printf(file, NoFriendlyFireStr "=%i\n", ng->NoFriendlyFire);
	PHYSFSX_printf(file, MouselookFlagsStr "=%i\n", ng->MouselookFlags);
	PHYSFSX_printf(file, PitchLockFlagsStr "=%i\n", ng->PitchLockFlags);
	PHYSFSX_printf(file, AutosaveIntervalStr "=%i\n", ng->MPGameplayOptions.AutosaveInterval.count());
#if DXX_USE_TRACKER
	PHYSFSX_printf(file, TrackerStr "=%i\n", ng->Tracker);
	PHYSFSX_printf(file, TrackerNATHPStr "=%i\n", ng->TrackerNATWarned);
#else
	PHYSFSX_puts_literal(file, TrackerStr "=0\n" TrackerNATHPStr "=0\n");
#endif
	PHYSFSX_puts_literal(file, NGPVersionStr "=" DXX_VERSION_STR "\n");
}

}
