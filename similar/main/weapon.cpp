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
 * Functions for weapons...
 *
 */

#include <stdexcept>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <type_traits>

#include "hudmsg.h"
#include "game.h"
#include "laser.h"
#include "weapon.h"
#include "player.h"
#include "dxxerror.h"
#include "sounds.h"
#include "text.h"
#include "powerup.h"
#include "newdemo.h"
#include "multi.h"
#include "object.h"
#include "segment.h"
#include "newmenu.h"
#include "playsave.h"
#include "physfs-serial.h"
#include "vclip.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "partial_range.h"

//	Note, only Vulcan cannon requires ammo.
// NOTE: Now Vulcan and Gauss require ammo. -5/3/95 Yuan
//ubyte	Default_primary_ammo_level[MAX_PRIMARY_WEAPONS] = {255, 0, 255, 255, 255};
//ubyte	Default_secondary_ammo_level[MAX_SECONDARY_WEAPONS] = {3, 0, 0, 0, 0};

//	Convert primary weapons to indices in Weapon_info array.
#if defined(DXX_BUILD_DESCENT_I)
namespace dsx {
const enumerated_array<weapon_id_type, MAX_PRIMARY_WEAPONS, primary_weapon_index_t> Primary_weapon_to_weapon_info{{
	{
		weapon_id_type::LASER_ID,
		weapon_id_type::VULCAN_ID,
		weapon_id_type::CHEAP_SPREADFIRE_ID,
		weapon_id_type::PLASMA_ID,
		weapon_id_type::FUSION_ID
	}
}};
const enumerated_array<weapon_id_type, MAX_SECONDARY_WEAPONS, secondary_weapon_index_t> Secondary_weapon_to_weapon_info{{
	{
		weapon_id_type::CONCUSSION_ID,
		weapon_id_type::HOMING_ID,
		weapon_id_type::PROXIMITY_ID,
		weapon_id_type::SMART_ID,
		weapon_id_type::MEGA_ID
	}
}};

}
#elif defined(DXX_BUILD_DESCENT_II)
#include "fvi.h"

namespace dsx {
const enumerated_array<weapon_id_type, MAX_PRIMARY_WEAPONS, primary_weapon_index_t> Primary_weapon_to_weapon_info{{
	{
		weapon_id_type::LASER_ID,
		weapon_id_type::VULCAN_ID,
		weapon_id_type::SPREADFIRE_ID,
		weapon_id_type::PLASMA_ID,
		weapon_id_type::FUSION_ID,
		weapon_id_type::SUPER_LASER_ID,
		weapon_id_type::GAUSS_ID,
		weapon_id_type::HELIX_ID,
		weapon_id_type::PHOENIX_ID,
		weapon_id_type::OMEGA_ID
	}
}};
const enumerated_array<weapon_id_type, MAX_SECONDARY_WEAPONS, secondary_weapon_index_t> Secondary_weapon_to_weapon_info{{
	{
		weapon_id_type::CONCUSSION_ID,
		weapon_id_type::HOMING_ID,
		weapon_id_type::PROXIMITY_ID,
		weapon_id_type::SMART_ID,
		weapon_id_type::MEGA_ID,
		weapon_id_type::FLASH_ID,
		weapon_id_type::GUIDEDMISS_ID,
		weapon_id_type::SUPERPROX_ID,
		weapon_id_type::MERCURY_ID,
		weapon_id_type::EARTHSHAKER_ID
	}
}};

namespace {

/*
 * On entry:
 * - base_weapon must be the non-super version of a weapon.
 */
template <typename T>
static T get_super_weapon_from_base_weapon(const T base_weapon)
{
	return static_cast<T>(static_cast<unsigned>(base_weapon) + SUPER_WEAPON);
}

/*
 * On entry:
 * - current_weapon may be any valid weapon index, whether regular or
 *   super.
 * - base_weapon must be the non-super version of current_weapon.  If
 *   current_weapon is the regular version, then base_weapon ==
 *   current_weapon.  If current_weapon is the super version, then
 *   base_weapon + SUPER_WEAPON == current_weapon.
 */
template <typename T>
static T get_alternate_weapon(const T current_weapon, const T base_weapon)
{
	const auto b = static_cast<unsigned>(base_weapon);
	const auto c = static_cast<unsigned>(current_weapon);
	const auto s = static_cast<unsigned>(get_super_weapon_from_base_weapon(base_weapon));
	/* If current_weapon == base_weapon, then this expression simplifies
	 * to (base_weapon + SUPER_WEAPON) and produces the super form of
	 * base_weapon.
	 *
	 * If current_weapon == base_weapon+SUPER_WEAPON, then this
	 * expression simplifies to (base_weapon), and produces the
	 * non-super form of base_weapon.
	 */
	return static_cast<T>(b + s - c);
}

}

}
#endif

namespace dsx {

//for each Secondary weapon, which gun it fires out of
const std::array<gun_num_t, MAX_SECONDARY_WEAPONS> Secondary_weapon_to_gun_num{{
	gun_num_t::_4,
	gun_num_t::_4,
	gun_num_t::_7,
	gun_num_t::_7,
	gun_num_t::_7,
#if defined(DXX_BUILD_DESCENT_II)
	gun_num_t::_4,
	gun_num_t::_4,
	gun_num_t::_7,
	gun_num_t::_4,
	gun_num_t::_7
#endif
}};

const enumerated_array<uint8_t, MAX_SECONDARY_WEAPONS, secondary_weapon_index_t> Secondary_ammo_max{{
	{
		20, 10, 10, 5, 5,
#if defined(DXX_BUILD_DESCENT_II)
		20, 20, 15, 10, 10
#endif
	}
}};

//for each primary weapon, what kind of powerup gives weapon
const enumerated_array<powerup_type_t, MAX_PRIMARY_WEAPONS, primary_weapon_index_t> Primary_weapon_to_powerup{{
	{
		powerup_type_t::POW_LASER,
		powerup_type_t::POW_VULCAN_WEAPON,
		powerup_type_t::POW_SPREADFIRE_WEAPON,
		powerup_type_t::POW_PLASMA_WEAPON,
		powerup_type_t::POW_FUSION_WEAPON,
#if defined(DXX_BUILD_DESCENT_II)
		powerup_type_t::POW_LASER,
		powerup_type_t::POW_GAUSS_WEAPON,
		powerup_type_t::POW_HELIX_WEAPON,
		powerup_type_t::POW_PHOENIX_WEAPON,
		powerup_type_t::POW_OMEGA_WEAPON,
#endif
	}
}};

//for each Secondary weapon, what kind of powerup gives weapon
const enumerated_array<powerup_type_t, MAX_SECONDARY_WEAPONS, secondary_weapon_index_t> Secondary_weapon_to_powerup{{
	{
		powerup_type_t::POW_MISSILE_1,
		powerup_type_t::POW_HOMING_AMMO_1,
		powerup_type_t::POW_PROXIMITY_WEAPON,
		powerup_type_t::POW_SMARTBOMB_WEAPON,
		powerup_type_t::POW_MEGA_WEAPON,
#if defined(DXX_BUILD_DESCENT_II)
		powerup_type_t::POW_SMISSILE1_1,
		powerup_type_t::POW_GUIDED_MISSILE_1,
		powerup_type_t::POW_SMART_MINE,
		powerup_type_t::POW_MERCURY_MISSILE_1,
		powerup_type_t::POW_EARTHSHAKER_MISSILE,
#endif
	}
}};

weapon_info_array Weapon_info;
}
namespace dcx {
unsigned N_weapon_types;

namespace {

template <typename cycle_weapon_state>
struct weapon_reorder_menu_items
{
	std::array<newmenu_item, cycle_weapon_state::max_weapons + 1> menu_items;
	weapon_reorder_menu_items();
};

template <typename cycle_weapon_state>
struct weapon_reorder_menu : weapon_reorder_menu_items<cycle_weapon_state>, reorder_newmenu
{
	using weapon_reorder_menu_items<cycle_weapon_state>::menu_items;
	weapon_reorder_menu(grs_canvas &src) :
		reorder_newmenu(menu_title{cycle_weapon_state::reorder_title}, menu_subtitle{"Shift+Up/Down arrow to move item"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(menu_items, 0), src)
	{
	}
	virtual window_event_result event_handler(const d_event &event) override;
};

template <typename cycle_weapon_state>
weapon_reorder_menu_items<cycle_weapon_state>::weapon_reorder_menu_items()
{
	for (auto &&[i, mi] : enumerate(menu_items))
	{
		const auto o = cycle_weapon_state::get_weapon_by_order_slot(i);
		mi.value = o;
		nm_set_item_menu(mi, cycle_weapon_state::get_weapon_name(o));
	}
}

template <typename cycle_weapon_state>
window_event_result weapon_reorder_menu<cycle_weapon_state>::event_handler(const d_event &event)
{
	switch(event.type)
	{
		case EVENT_KEY_COMMAND:
			event_key_command(event);
			break;
		case EVENT_WINDOW_CLOSE:
			for (auto &&[i, mi] : enumerate(menu_items))
				cycle_weapon_state::get_weapon_by_order_slot(i) = mi.value;
			break;
		default:
			break;
	}
	return newmenu::event_handler(event);
}

}

template <typename Accessor>
static void check_enum(Accessor &, polygon_model_index &pmi)
{
	pmi = build_polygon_model_index_from_untrusted(underlying_value(pmi));
}

}

// autoselect ordering

namespace dsx {
namespace {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::array<uint8_t, MAX_PRIMARY_WEAPONS + 1> DefaultPrimaryOrder{{ 4, 3, 2, 1, 0, 255 }};
constexpr std::array<uint8_t, MAX_SECONDARY_WEAPONS + 1> DefaultSecondaryOrder{{ 4, 3, 1, 0, 255, 2 }};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::array<uint8_t, MAX_PRIMARY_WEAPONS + 1> DefaultPrimaryOrder={{9,8,7,6,5,4,3,2,1,0,255}};
constexpr std::array<uint8_t, MAX_SECONDARY_WEAPONS + 1> DefaultSecondaryOrder={{9,8,4,3,1,5,0,255,7,6,2}};

//flags whether the last time we use this weapon, it was the 'super' version
#endif

static primary_weapon_index_t get_mapped_weapon_index(const player_info &player_info, const primary_weapon_index_t weapon_index)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)player_info;
#elif defined(DXX_BUILD_DESCENT_II)
	if (weapon_index == primary_weapon_index_t::LASER_INDEX && player_info.laser_level > MAX_LASER_LEVEL)
		return primary_weapon_index_t::SUPER_LASER_INDEX;
#endif
	return weapon_index;
}
}
}

// ; (0) Laser Level 1
// ; (1) Laser Level 2
// ; (2) Laser Level 3
// ; (3) Laser Level 4
// ; (4) Unknown Use
// ; (5) Josh Blobs
// ; (6) Unknown Use
// ; (7) Unknown Use
// ; (8) ---------- Concussion Missile ----------
// ; (9) ---------- Flare ----------
// ; (10) ---------- Blue laser that blue guy shoots -----------
// ; (11) ---------- Vulcan Cannon ----------
// ; (12) ---------- Spreadfire Cannon ----------
// ; (13) ---------- Plasma Cannon ----------
// ; (14) ---------- Fusion Cannon ----------
// ; (15) ---------- Homing Missile ----------
// ; (16) ---------- Proximity Bomb ----------
// ; (17) ---------- Smart Missile ----------
// ; (18) ---------- Mega Missile ----------
// ; (19) ---------- Children of the PLAYER'S Smart Missile ----------
// ; (20) ---------- Bad Guy Spreadfire Laser ----------
// ; (21) ---------- SuperMech Homing Missile ----------
// ; (22) ---------- Regular Mech's missile -----------
// ; (23) ---------- Silent Spreadfire Laser ----------
// ; (24) ---------- Red laser that baby spiders shoot -----------
// ; (25) ---------- Green laser that rifleman shoots -----------
// ; (26) ---------- Plasma gun that 'plasguy' fires ------------
// ; (27) ---------- Blobs fired by Red Spiders -----------
// ; (28) ---------- Final Boss's Mega Missile ----------
// ; (29) ---------- Children of the ROBOT'S Smart Missile ----------
// ; (30) Laser Level 5
// ; (31) Laser Level 6
// ; (32) ---------- Super Vulcan Cannon ----------
// ; (33) ---------- Super Spreadfire Cannon ----------
// ; (34) ---------- Super Plasma Cannon ----------
// ; (35) ---------- Super Fusion Cannon ----------

//	------------------------------------------------------------------------------------
//	Return:
// Bits set:
//		HAS_WEAPON_FLAG
//		HAS_ENERGY_FLAG
//		HAS_AMMO_FLAG
// See weapon.h for bit values
namespace dsx {
has_weapon_result player_has_primary_weapon(const player_info &player_info, primary_weapon_index_t weapon_num)
{
	int	return_value = 0;

	//	Hack! If energy goes negative, you can't fire a weapon that doesn't require energy.
	//	But energy should not go negative (but it does), so find out why it does!
	auto &energy = player_info.energy;

	const auto weapon_index = Primary_weapon_to_weapon_info[weapon_num];

	if (player_info.primary_weapon_flags & HAS_PRIMARY_FLAG(weapon_num))
			return_value |= has_weapon_result::has_weapon_flag;

		// Special case: Gauss cannon uses vulcan ammo.
		if (weapon_index_uses_vulcan_ammo(weapon_num)) {
			if (Weapon_info[weapon_index].ammo_usage <= player_info.vulcan_ammo)
				return_value |= has_weapon_result::has_ammo_flag;
		}
		/* Hack to work around check in do_primary_weapon_select */
		else
			return_value |= has_weapon_result::has_ammo_flag;

#if defined(DXX_BUILD_DESCENT_I)
		//added on 1/21/99 by Victor Rachels... yet another hack
		//fusion has 0 energy usage, HAS_ENERGY_FLAG was always true
		if(weapon_num == primary_weapon_index_t::FUSION_INDEX)
		{
			if (energy >= F1_0*2)
				return_value |= has_weapon_result::has_energy_flag;
		}
#elif defined(DXX_BUILD_DESCENT_II)
		if (weapon_num == primary_weapon_index_t::OMEGA_INDEX) {	// Hack: Make sure player has energy to omega
			if (energy > 0 || player_info.Omega_charge)
				return_value |= has_weapon_result::has_energy_flag;
		}
#endif
		else
		{
			const auto energy_usage = Weapon_info[weapon_index].energy_usage;
			/* The test for `energy_usage <= 0` should not be needed.
			 * However, a Parallax comment suggests that players
			 * sometimes get negative energy.  Use this test in
			 * preference to coercing negative player energy to zero.
			 */
			if (energy_usage <= 0 || energy_usage <= energy)
				return_value |= has_weapon_result::has_energy_flag;
		}
	return return_value;
}

has_weapon_result player_has_secondary_weapon(const player_info &player_info, const secondary_weapon_index_t weapon_num)
{
	int	return_value = 0;
	const auto secondary_ammo = player_info.secondary_ammo[weapon_num];
	const auto weapon_index = Secondary_weapon_to_weapon_info[weapon_num];
	if (secondary_ammo && Weapon_info[weapon_index].ammo_usage <= secondary_ammo)
		return_value = has_weapon_result::has_weapon_flag | has_weapon_result::has_energy_flag | has_weapon_result::has_ammo_flag;
	return return_value;
}

void InitWeaponOrdering ()
 {
  // short routine to setup default weapon priorities for new pilots
	PlayerCfg.PrimaryOrder = DefaultPrimaryOrder;
	PlayerCfg.SecondaryOrder = DefaultSecondaryOrder;
 }

namespace {

static uint_fast32_t POrderList(primary_weapon_index_t num);
static uint_fast32_t SOrderList(secondary_weapon_index_t num);

class cycle_weapon_state
{
public:
	static constexpr char DXX_WEAPON_TEXT_NEVER_AUTOSELECT[] = "--- Never autoselect below ---";
	[[noreturn]]
	__attribute_cold
	static void report_runtime_error(const char *);
};

class cycle_primary_state : public cycle_weapon_state
{
	player_info &pl_info;
public:
	using weapon_index_type = primary_weapon_index_t;
	static constexpr std::integral_constant<weapon_index_type, weapon_index_type{255}> cycle_never_autoselect_below{};
	cycle_primary_state(player_info &p) :
		pl_info(p)
	{
	}
	static constexpr std::integral_constant<uint_fast32_t, MAX_PRIMARY_WEAPONS> max_weapons{};
	static constexpr char reorder_title[] = "Reorder Primary";
	static constexpr char error_weapon_list_corrupt[] = "primary weapon list corrupt";
	static uint_fast32_t get_cycle_position(primary_weapon_index_t i)
	{
		return POrderList(i);
	}
	static player_config::primary_weapon_order::reference get_weapon_by_order_slot(player_config::primary_weapon_order::size_type cur_order_slot)
	{
		return PlayerCfg.PrimaryOrder[cur_order_slot];
	}
	bool maybe_select_weapon_by_order_slot(uint_fast32_t cur_order_slot) const
	{
		return maybe_select_weapon_by_type(get_weapon_by_order_slot(cur_order_slot));
	}
	bool maybe_select_weapon_by_type(const uint_fast32_t desired_weapon_idx) const
	{
		weapon_index_type desired_weapon = static_cast<weapon_index_type>(desired_weapon_idx);
#if defined(DXX_BUILD_DESCENT_II)
		// some remapping for SUPER LASER which is not an actual weapon type at all
		if (desired_weapon == primary_weapon_index_t::LASER_INDEX)
		{
			if (pl_info.laser_level > MAX_LASER_LEVEL)
				return false;
		}
		else if (desired_weapon == primary_weapon_index_t::SUPER_LASER_INDEX)
		{
			if (pl_info.laser_level <= MAX_LASER_LEVEL)
				return false;
			else
				desired_weapon = primary_weapon_index_t::LASER_INDEX;
		}
#endif
		if (!player_has_primary_weapon(pl_info, desired_weapon).has_all())
			return false;
		select_primary_weapon(pl_info, PRIMARY_WEAPON_NAMES(desired_weapon), desired_weapon, 1);
		return true;
	}
	void abandon_auto_select()
	{
		HUD_init_message_literal(HM_DEFAULT, TXT_NO_PRIMARY);
		if (pl_info.Primary_weapon == primary_weapon_index_t::LASER_INDEX)
			return;
		select_primary_weapon(pl_info, nullptr, primary_weapon_index_t::LASER_INDEX, 1);
	}
	static const char *get_weapon_name(uint8_t i)
	{
		return i == cycle_never_autoselect_below ? DXX_WEAPON_TEXT_NEVER_AUTOSELECT : PRIMARY_WEAPON_NAMES(i);
	}
};

class cycle_secondary_state : public cycle_weapon_state
{
	player_info &pl_info;
public:
	using weapon_index_type = secondary_weapon_index_t;
	static constexpr std::integral_constant<weapon_index_type, weapon_index_type{255}> cycle_never_autoselect_below{};
	cycle_secondary_state(player_info &p) :
		pl_info(p)
	{
	}
	static constexpr std::integral_constant<uint_fast32_t, MAX_SECONDARY_WEAPONS> max_weapons{};
	static constexpr char reorder_title[] = "Reorder Secondary";
	static constexpr char error_weapon_list_corrupt[] = "secondary weapon list corrupt";
	static uint_fast32_t get_cycle_position(secondary_weapon_index_t i)
	{
		return SOrderList(i);
	}
	static player_config::secondary_weapon_order::reference get_weapon_by_order_slot(player_config::secondary_weapon_order::size_type cur_order_slot)
	{
		return PlayerCfg.SecondaryOrder[cur_order_slot];
	}
	bool maybe_select_weapon_by_order_slot(uint_fast32_t cur_order_slot)
	{
		return maybe_select_weapon_by_type(get_weapon_by_order_slot(cur_order_slot));
	}
	bool maybe_select_weapon_by_type(const uint_fast32_t desired_weapon_idx)
	{
		const weapon_index_type desired_weapon = static_cast<weapon_index_type>(desired_weapon_idx);
		if (!player_has_secondary_weapon(pl_info, desired_weapon).has_all())
			return false;
		select_secondary_weapon(pl_info, SECONDARY_WEAPON_NAMES(desired_weapon), desired_weapon, 1);
		return true;
	}
	static void abandon_auto_select()
	{
		HUD_init_message_literal(HM_DEFAULT, "No secondary weapons available!");
	}
	static const char *get_weapon_name(uint8_t i)
	{
		return i == cycle_never_autoselect_below ? DXX_WEAPON_TEXT_NEVER_AUTOSELECT : SECONDARY_WEAPON_NAMES(i);
	}
};

void cycle_weapon_state::report_runtime_error(const char *const p)
{
	throw std::runtime_error(p);
}

template <typename T>
void CycleWeapon(T t, const typename T::weapon_index_type effective_weapon)
{
	auto cur_order_slot = t.get_cycle_position(effective_weapon);
	const auto autoselect_order_slot = t.get_cycle_position(t.cycle_never_autoselect_below);
	const auto use_restricted_autoselect =
		(cur_order_slot < autoselect_order_slot) &&
		(1 < autoselect_order_slot) &&
		PlayerCfg.CycleAutoselectOnly;
	for (uint_fast32_t loop = t.max_weapons + 1; loop--;)
	{
		cur_order_slot++; // next slot
		if (cur_order_slot >= t.max_weapons + 1) // loop if necessary
			cur_order_slot = 0;
		if (cur_order_slot == autoselect_order_slot) // what to to with non-autoselect weapons?
		{
			if (use_restricted_autoselect)
			{
				cur_order_slot = 0; // loop over or ...
			}
			else
			{
				continue; // continue?
			}
		}
		if (t.maybe_select_weapon_by_order_slot(cur_order_slot)) // now that is the weapon next to our current one
		// select the weapon if we have it
			return;
	}
}

}

void CyclePrimary(player_info &player_info)
{
	CycleWeapon(cycle_primary_state(player_info), get_mapped_weapon_index(player_info, player_info.Primary_weapon));
}

void CycleSecondary(player_info &player_info)
{
	CycleWeapon(cycle_secondary_state(player_info), player_info.Secondary_weapon);
}

#if defined(DXX_BUILD_DESCENT_II)
namespace {
static inline void set_weapon_last_was_super(uint8_t &last, const uint8_t mask, const bool is_super)
{
	if (is_super)
		last |= mask;
	else
		last &= ~mask;
}

static inline void set_weapon_last_was_super(uint8_t &last, const uint_fast32_t weapon_num)
{
	const bool is_super = weapon_num >= SUPER_WEAPON;
	set_weapon_last_was_super(last, 1 << (is_super ? weapon_num - SUPER_WEAPON : weapon_num), is_super);
}
}
#endif

void set_primary_weapon(player_info &player_info, const uint_fast32_t weapon_num)
{
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_weapon(0, weapon_num);
	player_info.Fusion_charge=0;
	player_info.Next_laser_fire_time = 0;
	player_info.Primary_weapon = static_cast<primary_weapon_index_t>(weapon_num);
#if defined(DXX_BUILD_DESCENT_II)
	//save flag for whether was super version
	auto &Primary_last_was_super = player_info.Primary_last_was_super;
	set_weapon_last_was_super(Primary_last_was_super, weapon_num);
#endif
}

//	------------------------------------------------------------------------------------
//if message flag set, print message saying selected
void select_primary_weapon(player_info &player_info, const char *const weapon_name, const uint_fast32_t weapon_num, const int wait_for_rearm)
{
	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(0, weapon_num);

	{
		auto &Primary_weapon = player_info.Primary_weapon;
		if (Primary_weapon != weapon_num) {
			Primary_weapon = static_cast<primary_weapon_index_t>(weapon_num);
#ifndef FUSION_KEEPS_CHARGE
			//added 8/6/98 by Victor Rachels to fix fusion charge bug
			player_info.Fusion_charge=0;
			//end edit - Victor Rachels
#endif
			auto &Next_laser_fire_time = player_info.Next_laser_fire_time;
			if (wait_for_rearm)
			{
				multi_digi_play_sample_once(SOUND_GOOD_SELECTION_PRIMARY, F0_5);
				Next_laser_fire_time = GameTime64 + REARM_TIME;
			}
			else
				Next_laser_fire_time = 0;
		}
		else
		{
#if defined(DXX_BUILD_DESCENT_I)
			if (wait_for_rearm)
				/*
				 * In Descent 1, requesting a weapon that is already
				 * armed is pointless, so play a warning sound.
				 *
				 * In Descent 2, the player may be trying to toggle
				 * between the base and super forms of a weapon, so do
				 * not generate a warning sound in that case.
				 */
				digi_play_sample(SOUND_ALREADY_SELECTED, F1_0);
#endif
		}
#if defined(DXX_BUILD_DESCENT_II)
		//save flag for whether was super version
		set_weapon_last_was_super(player_info.Primary_last_was_super, weapon_num);
#endif
	}
	if (weapon_name)
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (weapon_num == primary_weapon_index_t::LASER_INDEX)
			HUD_init_message(HM_DEFAULT, "%s Level %u %s", weapon_name, static_cast<unsigned>(player_info.laser_level) + 1, TXT_SELECTED);
		else
#endif
			HUD_init_message(HM_DEFAULT, "%s %s", weapon_name, TXT_SELECTED);
	}

}

void set_secondary_weapon_to_concussion(player_info &player_info)
{
	const uint_fast32_t weapon_num = 0;
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_player_weapon(1, weapon_num);

	player_info.Next_missile_fire_time = 0;
	Global_missile_firing_count = 0;
	player_info.Secondary_weapon = static_cast<secondary_weapon_index_t>(weapon_num);
#if defined(DXX_BUILD_DESCENT_II)
	//save flag for whether was super version
	set_weapon_last_was_super(player_info.Secondary_last_was_super, weapon_num);
#endif
}

void select_secondary_weapon(player_info &player_info, const char *const weapon_name, const uint_fast32_t weapon_num, const int wait_for_rearm)
{
	if (Newdemo_state==ND_STATE_RECORDING )
		newdemo_record_player_weapon(1, weapon_num);

	{
		auto &Secondary_weapon = player_info.Secondary_weapon;
		if (Secondary_weapon != weapon_num) {
			auto &Next_missile_fire_time = player_info.Next_missile_fire_time;
			if (wait_for_rearm)
			{
				multi_digi_play_sample_once(SOUND_GOOD_SELECTION_SECONDARY, F0_5);
				Next_missile_fire_time = GameTime64 + REARM_TIME;
			}
			else
				Next_missile_fire_time = 0;
			Global_missile_firing_count = 0;
		} else	{
			if (wait_for_rearm)
			{
				digi_play_sample_once( SOUND_ALREADY_SELECTED, F1_0 );
			}

		}
		Secondary_weapon = static_cast<secondary_weapon_index_t>(weapon_num);
#if defined(DXX_BUILD_DESCENT_II)
		//save flag for whether was super version
		set_weapon_last_was_super(player_info.Secondary_last_was_super, weapon_num);
#endif
	}
	if (weapon_name)
	{
		HUD_init_message(HM_DEFAULT, "%s %s", weapon_name, TXT_SELECTED);
	}
}

#if defined(DXX_BUILD_DESCENT_I)
namespace {
static bool reject_shareware_weapon_select(const uint_fast32_t weapon_num, const char *const weapon_name)
{
	// do special hud msg. for picking registered weapon in shareware version.
	if (PCSharePig)
		if (weapon_num >= NUM_SHAREWARE_WEAPONS) {
			HUD_init_message(HM_DEFAULT, "%s %s!", weapon_name,TXT_NOT_IN_SHAREWARE);
			return true;
		}
	return false;
}

static bool reject_unusable_primary_weapon_select(const player_info &player_info, const primary_weapon_index_t weapon_num, const char *const weapon_name)
{
	const auto weapon_status = player_has_primary_weapon(player_info, weapon_num);
	const char *prefix;
	if (!weapon_status.has_weapon())
		prefix = TXT_DONT_HAVE;
	else if (!weapon_status.has_ammo())
		prefix = TXT_DONT_HAVE_AMMO;
	else
		return false;
	HUD_init_message(HM_DEFAULT, "%s %s!", prefix, weapon_name);
	return true;
}

static bool reject_unusable_secondary_weapon_select(const player_info &player_info, const secondary_weapon_index_t weapon_num, const char *const weapon_name)
{
	const auto weapon_status = player_has_secondary_weapon(player_info, weapon_num);
	if (weapon_status.has_all())
		return false;
	HUD_init_message(HM_DEFAULT, "%s %s%s", TXT_HAVE_NO, weapon_name, TXT_SX);
	return true;
}
}
#endif

//	------------------------------------------------------------------------------------
//	Select a weapon, primary or secondary.
void do_primary_weapon_select(player_info &player_info, primary_weapon_index_t weapon_num)
{
#if defined(DXX_BUILD_DESCENT_I)
        //added on 10/9/98 by Victor Rachels to add laser cycle
        //end this section addition - Victor Rachels
	const auto weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
	if (reject_shareware_weapon_select(weapon_num, weapon_name) || reject_unusable_primary_weapon_select(player_info, weapon_num, weapon_name))
	{
		digi_play_sample(SOUND_BAD_SELECTION, F1_0);
		return;
	}
#elif defined(DXX_BUILD_DESCENT_II)
	has_weapon_result weapon_status;

	auto &Primary_weapon = player_info.Primary_weapon;
	const auto current = Primary_weapon.get_active();
	const auto last_was_super = player_info.Primary_last_was_super & (1 << weapon_num);
	const auto has_flag = weapon_status.has_weapon_flag;

	if (current == weapon_num || current == weapon_num+SUPER_WEAPON) {
		//already have this selected, so toggle to other of normal/super version
		weapon_num = get_alternate_weapon(current, weapon_num);
		weapon_status = player_has_primary_weapon(player_info, weapon_num);
	}
	else {
		const auto weapon_num_save = weapon_num;

		//go to last-select version of requested missile

		if (last_was_super)
			weapon_num = get_super_weapon_from_base_weapon(weapon_num);

		weapon_status = player_has_primary_weapon(player_info, weapon_num);

		//if don't have last-selected, try other version

		if ((weapon_status.flags() & has_flag) != has_flag) {
			weapon_num = get_alternate_weapon(weapon_num, weapon_num_save);
			weapon_status = player_has_primary_weapon(player_info, weapon_num);
			if ((weapon_status.flags() & has_flag) != has_flag)
				weapon_num = get_alternate_weapon(weapon_num, weapon_num_save);
		}
	}

	//if we don't have the weapon we're switching to, give error & bail
	const auto weapon_name = PRIMARY_WEAPON_NAMES(weapon_num);
	if ((weapon_status.flags() & has_flag) != has_flag) {
		{
			if (weapon_num == primary_weapon_index_t::SUPER_LASER_INDEX)
				return; 		//no such thing as super laser, so no error
			HUD_init_message(HM_DEFAULT, "%s %s!", TXT_DONT_HAVE, weapon_name);
		}
		digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
		return;
	}

	//now actually select the weapon
#endif
	select_primary_weapon(player_info, weapon_name, weapon_num, 1);
}

void do_secondary_weapon_select(player_info &player_info, secondary_weapon_index_t weapon_num)
{
#if defined(DXX_BUILD_DESCENT_I)
        //added on 10/9/98 by Victor Rachels to add laser cycle
        //end this section addition - Victor Rachels
	// do special hud msg. for picking registered weapon in shareware version.
	const auto weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
	if (reject_shareware_weapon_select(weapon_num, weapon_name) || reject_unusable_secondary_weapon_select(player_info, weapon_num, weapon_name))
	{
		digi_play_sample(SOUND_BAD_SELECTION, F1_0);
		return;
	}
#elif defined(DXX_BUILD_DESCENT_II)
	has_weapon_result weapon_status;

	const auto current = player_info.Secondary_weapon.get_active();
	const auto last_was_super = player_info.Secondary_last_was_super & (1 << weapon_num);
	const auto has_flag = weapon_status.has_weapon_flag | weapon_status.has_ammo_flag;

	if (current == weapon_num || current == weapon_num+SUPER_WEAPON) {
		//already have this selected, so toggle to other of normal/super version
		weapon_num = get_alternate_weapon(current, weapon_num);
		weapon_status = player_has_secondary_weapon(player_info, weapon_num);
	}
	else {
		const auto weapon_num_save = weapon_num;

		//go to last-select version of requested missile

		if (last_was_super)
			weapon_num = get_super_weapon_from_base_weapon(weapon_num);

		weapon_status = player_has_secondary_weapon(player_info, weapon_num);

		//if don't have last-selected, try other version

		if ((weapon_status.flags() & has_flag) != has_flag) {
			weapon_num = get_alternate_weapon(weapon_num, weapon_num_save);
			weapon_status = player_has_secondary_weapon(player_info, weapon_num);
			if ((weapon_status.flags() & has_flag) != has_flag)
				weapon_num = get_alternate_weapon(weapon_num, weapon_num_save);
		}
	}

	//if we don't have the weapon we're switching to, give error & bail
	const auto weapon_name = SECONDARY_WEAPON_NAMES(weapon_num);
	if ((weapon_status.flags() & has_flag) != has_flag) {
		HUD_init_message(HM_DEFAULT, "%s %s%s", TXT_HAVE_NO, weapon_name, TXT_SX);
		digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
		return;
	}

	//now actually select the weapon
#endif
	select_secondary_weapon(player_info, weapon_name, weapon_num, 1);
}
}

namespace {

template <typename T>
void auto_select_weapon(T t)
{
	for (uint_fast32_t cur_order_slot = 0; cur_order_slot != t.max_weapons + 1; ++cur_order_slot)
	{
		const auto weapon_type = t.get_weapon_by_order_slot(cur_order_slot);
		if (weapon_type >= t.max_weapons)
		{
			t.abandon_auto_select();
			return;
		}
		if (t.maybe_select_weapon_by_type(weapon_type))
			return;
	}
}

}

namespace dsx {

//	----------------------------------------------------------------------------------------
//	Automatically select next best weapon if unable to fire current weapon.
// Weapon type: 0==primary, 1==secondary
void auto_select_primary_weapon(player_info &player_info)
{
	if (!player_has_primary_weapon(player_info, player_info.Primary_weapon).has_all())
		auto_select_weapon(cycle_primary_state(player_info));
}

void auto_select_secondary_weapon(player_info &player_info)
{
	if (!player_has_secondary_weapon(player_info, player_info.Secondary_weapon).has_all())
		auto_select_weapon(cycle_secondary_state(player_info));
}

void delayed_autoselect(player_info &player_info, const control_info &Controls)
{
	if (!Controls.state.fire_primary)
	{
		auto &Primary_weapon = player_info.Primary_weapon;
		const auto primary_weapon = Primary_weapon.get_active();
		const auto delayed_primary = Primary_weapon.get_delayed();
		if (delayed_primary != primary_weapon)
		{
			if (player_has_primary_weapon(player_info, delayed_primary).has_all())
				select_primary_weapon(player_info, nullptr, delayed_primary, 1);
			else
				Primary_weapon.set_delayed(primary_weapon);
		}
	}
	if (!Controls.state.fire_secondary)
	{
		auto &Secondary_weapon = player_info.Secondary_weapon;
		const auto secondary_weapon = Secondary_weapon.get_active();
		const auto delayed_secondary = Secondary_weapon.get_delayed();
		if (delayed_secondary != secondary_weapon)
		{
			if (player_has_secondary_weapon(player_info, delayed_secondary).has_all())
				select_secondary_weapon(player_info, nullptr, delayed_secondary, 1);
			else
				Secondary_weapon.set_delayed(secondary_weapon);
		}
	}
}

namespace {

static void maybe_autoselect_primary_weapon(player_info &player_info, primary_weapon_index_t weapon_index, const control_info &Controls)
{
	const auto want_switch = [weapon_index, &player_info]{
		const auto cutpoint = POrderList(cycle_primary_state::cycle_never_autoselect_below);
		const auto weapon_order = POrderList(weapon_index);
		return weapon_order < cutpoint && weapon_order < POrderList(get_mapped_weapon_index(player_info, player_info.Primary_weapon.get_delayed()));
	};
	if (Controls.state.fire_primary && PlayerCfg.NoFireAutoselect != FiringAutoselectMode::Immediate)
	{
		if (PlayerCfg.NoFireAutoselect == FiringAutoselectMode::Delayed)
		{
			if (want_switch())
				player_info.Primary_weapon.set_delayed(weapon_index);
		}
	}
	else if (want_switch())
		select_primary_weapon(player_info, nullptr, weapon_index, 1);
}

}

//	---------------------------------------------------------------------
//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
//	Returns true if powerup picked up, else returns false.
int pick_up_secondary(player_info &player_info, secondary_weapon_index_t weapon_index, int count, const control_info &Controls)
{
	int	num_picked_up;
	const auto max = PLAYER_MAX_AMMO(player_info.powerup_flags, Secondary_ammo_max[weapon_index]);
	auto &secondary_ammo = player_info.secondary_ammo;
	if (secondary_ammo[weapon_index] >= max)
	{
		HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %i %ss!", TXT_ALREADY_HAVE, secondary_ammo[weapon_index], SECONDARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	secondary_ammo[weapon_index] += count;

	num_picked_up = count;
	if (secondary_ammo[weapon_index] > max)
	{
		num_picked_up = count - (secondary_ammo[weapon_index] - max);
		secondary_ammo[weapon_index] = max;
	}

	if (secondary_ammo[weapon_index] == count)	// only autoselect if player didn't have any
	{
		const auto weapon_order = SOrderList(weapon_index);
		auto &Secondary_weapon = player_info.Secondary_weapon;
		const auto want_switch = [weapon_order, &secondary_ammo, &Secondary_weapon]{
			return weapon_order < SOrderList(cycle_secondary_state::cycle_never_autoselect_below) && (
				secondary_ammo[Secondary_weapon.get_delayed()] == 0 ||
				weapon_order < SOrderList(Secondary_weapon.get_delayed())
				);
		};
		if (Controls.state.fire_secondary && PlayerCfg.NoFireAutoselect != FiringAutoselectMode::Immediate)
		{
			if (PlayerCfg.NoFireAutoselect == FiringAutoselectMode::Delayed)
			{
				if (want_switch())
					Secondary_weapon.set_delayed(weapon_index);
			}
		}
		else if (want_switch())
			select_secondary_weapon(player_info, nullptr, weapon_index, 1);
#if defined(DXX_BUILD_DESCENT_II)
			//if it's a proxbomb or smart mine,
			//we want to do a mini-auto-selection that applies to the drop bomb key

			if (weapon_index_is_player_bomb(weapon_index) &&
				!weapon_index_is_player_bomb(player_info.Secondary_weapon))
			{
				const auto mask = 1 << PROXIMITY_INDEX;
				if (weapon_order < SOrderList((player_info.Secondary_last_was_super & mask) ? SMART_MINE_INDEX : PROXIMITY_INDEX))
					set_weapon_last_was_super(player_info.Secondary_last_was_super, mask, weapon_index == SMART_MINE_INDEX);
			}
#endif
	}

	//note: flash for all but concussion was 7,14,21
	if (num_picked_up>1) {
		PALETTE_FLASH_ADD(15,15,15);
		HUD_init_message(HM_DEFAULT, "%d %s%s",num_picked_up,SECONDARY_WEAPON_NAMES(weapon_index), TXT_SX);
	}
	else {
		PALETTE_FLASH_ADD(10,10,10);
		HUD_init_message(HM_DEFAULT, "%s!",SECONDARY_WEAPON_NAMES(weapon_index));
	}

	return 1;
}
}

namespace {

template <typename cycle_weapon_state>
static void ReorderWeapon()
{
	auto menu = window_create<weapon_reorder_menu<cycle_weapon_state>>(grd_curscreen->sc_canvas);
	(void)menu;
}

}

namespace dsx {

void ReorderPrimary()
{
	ReorderWeapon<cycle_primary_state>();
}

void ReorderSecondary()
{
	ReorderWeapon<cycle_secondary_state>();
}

}

namespace {

template <typename T>
static uint_fast32_t search_weapon_order_list(uint8_t goal)
{
	for (uint_fast32_t i = 0; i != T::max_weapons + 1; ++i)
		if (T::get_weapon_by_order_slot(i) == goal)
			return i;
	T::report_runtime_error(T::error_weapon_list_corrupt);
}

}

namespace dsx {

namespace {

uint_fast32_t POrderList (primary_weapon_index_t num)
{
	return search_weapon_order_list<cycle_primary_state>(num);
}

uint_fast32_t SOrderList (secondary_weapon_index_t num)
{
	return search_weapon_order_list<cycle_secondary_state>(num);
}

}

//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(player_info &player_info, const primary_weapon_index_t weapon_index)
{
	ushort flag = HAS_PRIMARY_FLAG(weapon_index);

	if (weapon_index != primary_weapon_index_t::LASER_INDEX &&
		(player_info.primary_weapon_flags & flag))
	{		//already have
		HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %s!", TXT_ALREADY_HAVE_THE, PRIMARY_WEAPON_NAMES(weapon_index));
		return 0;
	}

	player_info.primary_weapon_flags |= flag;

	maybe_autoselect_primary_weapon(player_info, weapon_index, Controls);

	PALETTE_FLASH_ADD(7,14,21);

	if (weapon_index != primary_weapon_index_t::LASER_INDEX)
   	HUD_init_message(HM_DEFAULT, "%s!",PRIMARY_WEAPON_NAMES(weapon_index));

	return 1;
}

#if defined(DXX_BUILD_DESCENT_II)
void check_to_use_primary_super_laser(player_info &player_info)
{
	if (!(player_info.primary_weapon_flags & HAS_SUPER_LASER_FLAG))
	{
		const auto weapon_index = primary_weapon_index_t::SUPER_LASER_INDEX;
		const auto pwi = POrderList(weapon_index);
		if (pwi < POrderList(cycle_primary_state::cycle_never_autoselect_below) &&
			pwi < POrderList(player_info.Primary_weapon))
		{
			select_primary_weapon(player_info, nullptr, primary_weapon_index_t::LASER_INDEX, 1);
		}
	}
	PALETTE_FLASH_ADD(7,14,21);
}
#endif

namespace {

static void maybe_autoselect_vulcan_weapon(player_info &player_info)
{
#if defined(DXX_BUILD_DESCENT_I)
	const auto weapon_flag_mask = HAS_VULCAN_FLAG;
#elif defined(DXX_BUILD_DESCENT_II)
	const auto weapon_flag_mask = HAS_VULCAN_FLAG | HAS_GAUSS_FLAG;
#endif
	const auto primary_weapon_flags = player_info.primary_weapon_flags;
	if (!(primary_weapon_flags & weapon_flag_mask))
		return;
	const auto cutpoint = POrderList(cycle_primary_state::cycle_never_autoselect_below);
	auto weapon_index = primary_weapon_index_t::VULCAN_INDEX;
#if defined(DXX_BUILD_DESCENT_I)
	const auto weapon_order_vulcan = POrderList(primary_weapon_index_t::VULCAN_INDEX);
	const auto better = weapon_order_vulcan;
#elif defined(DXX_BUILD_DESCENT_II)
	/* If a weapon is missing, pretend its auto-select priority is equal
	 * to cutpoint.  Priority at or worse than cutpoint is never
	 * auto-selected.
	 */
	const auto weapon_order_vulcan = (primary_weapon_flags & HAS_VULCAN_FLAG)
		? POrderList(primary_weapon_index_t::VULCAN_INDEX)
		: cutpoint;
	const auto weapon_order_gauss = (primary_weapon_flags & HAS_GAUSS_FLAG)
		? POrderList(primary_weapon_index_t::GAUSS_INDEX)
		: cutpoint;
	/* Set better to whichever vulcan-based weapon is higher priority.
	 * The chosen weapon might still be worse than cutpoint.
	 */
	const auto better = (weapon_order_vulcan < weapon_order_gauss)
		? weapon_order_vulcan
		: (weapon_index = primary_weapon_index_t::GAUSS_INDEX, weapon_order_gauss);
#endif
	if (better >= cutpoint)
		/* Preferred weapon is not auto-selectable */
		return;
	if (better >= POrderList(get_mapped_weapon_index(player_info, player_info.Primary_weapon)))
		/* Preferred weapon is not as desirable as the current weapon */
		return;
	maybe_autoselect_primary_weapon(player_info, weapon_index, Controls);
}

}

//called when ammo (for the vulcan cannon) is picked up
//	Returns the amount picked up
int pick_up_vulcan_ammo(player_info &player_info, uint_fast32_t ammo_count, const bool change_weapon)
{
	const auto max = PLAYER_MAX_AMMO(player_info.powerup_flags, VULCAN_AMMO_MAX);
	auto &plr_vulcan_ammo = player_info.vulcan_ammo;
	const auto old_ammo = plr_vulcan_ammo;
	if (old_ammo >= max)
		return 0;

	const auto amount_can_add = max - old_ammo;
	/* If the amount available will not exceed maximum, then add
	 * everything and report the entire amount as used.
	 * If the amount available would exceed maximum, set player's count
	 * to maximum and report as used the delta between maximum and the
	 * player's previous count (`old_ammo`).
	 */
	const auto used = (ammo_count < amount_can_add)
		? (plr_vulcan_ammo += ammo_count, ammo_count)
		: (plr_vulcan_ammo = max, amount_can_add);

	if (change_weapon &&
		plr_vulcan_ammo &&
		!old_ammo)
		maybe_autoselect_vulcan_weapon(player_info);
	return used;	//return amount used
}

#if defined(DXX_BUILD_DESCENT_II)
// Homing weapons cheat

void weapons_homing_all()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	range_for(auto &&objp, vmobjptr)
		if (objp->type == OBJ_WEAPON)
			objp->ctype.laser_info.track_goal = object_none;
	range_for (auto &w, Weapon_info)
		w.homing_flag |= 2;
}

void weapons_homing_all_reset()
{
	range_for (auto &w, Weapon_info)
		w.homing_flag &= ~2;
}

#define	SMEGA_SHAKE_TIME		(F1_0*2)

//	Call this to initialize for a new level.
//	Sets all super mega missile detonation times to 0 which means there aren't any.
void init_smega_detonates()
{
	LevelUniqueSeismicState.Earthshaker_detonate_times = {};
}

constexpr std::integral_constant<int, SOUND_SEISMIC_DISTURBANCE_START> Seismic_sound{};

namespace {

static void start_seismic_sound()
{
	if (LevelUniqueSeismicState.Next_seismic_sound_time)
		return;
	LevelUniqueSeismicState.Next_seismic_sound_time = GameTime64 + d_rand()/2;
	digi_play_sample_looping(Seismic_sound, F1_0, -1, -1);
}

static void apply_seismic_effect(const int entry_fc)
{
	const auto fc = std::clamp(entry_fc, 1, 16);
	LevelUniqueSeismicState.Seismic_tremor_volume += fc;

	if (!d_tick_step)
		return;
	const auto get_base_disturbance = [fc]() {
		return fixmul(d_rand() - 16384, 3 * F1_0 / 16 + (F1_0 * (16 - fc)) / 32);
	};
	const fix disturb_x = get_base_disturbance();
	const fix disturb_z = get_base_disturbance();

	{
		auto &rotvel = ConsoleObject->mtype.phys_info.rotvel;
		rotvel.x += disturb_x;
		rotvel.z += disturb_z;
	}

	//	Shake the buddy!
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	const auto Buddy_objnum = BuddyState.Buddy_objnum;
	if (Buddy_objnum != object_none) {
		auto &objp = *LevelUniqueObjectState.Objects.vmptr(Buddy_objnum);
		auto &rotvel = objp.mtype.phys_info.rotvel;
		rotvel.x += disturb_x * 4;
		rotvel.z += disturb_z * 4;
	}
	//	Shake a guided missile!
	LevelUniqueSeismicState.Seismic_tremor_magnitude += disturb_x;
}

}

//	If a smega missile been detonated, rock the mine!
//	This should be called every frame.
//	Maybe this should affect all robots, being called when they get their physics done.
void rock_the_mine_frame(void)
{
	range_for (auto &i, LevelUniqueSeismicState.Earthshaker_detonate_times)
	{
		if (i != 0) {
			fix	delta_time = GameTime64 - i;
			start_seismic_sound();
			if (delta_time < SMEGA_SHAKE_TIME) {

				//	Control center destroyed, rock the player's ship.
				// -- fc = abs(delta_time - SMEGA_SHAKE_TIME/2);
				//	Changed 10/23/95 to make decreasing for super mega missile.
				const int fc = (SMEGA_SHAKE_TIME - delta_time) / 2;
				apply_seismic_effect(fc / (SMEGA_SHAKE_TIME / 32));
			} else
				i = 0;
		}
	}

	//	Hook in the rumble sound effect here.
}

void init_seismic_disturbances()
{
	LevelUniqueSeismicState.Seismic_disturbance_end_time = 0;
}

namespace {

//	Return true if time to start a seismic disturbance.
static bool seismic_disturbance_active()
{
	const auto level_shake_duration = LevelSharedSeismicState.Level_shake_duration;
	if (level_shake_duration < 1)
		return false;

	if (LevelUniqueSeismicState.Seismic_disturbance_end_time && LevelUniqueSeismicState.Seismic_disturbance_end_time < GameTime64)
		return true;

	bool rval;
	rval =  (2 * fixmul(d_rand(), LevelSharedSeismicState.Level_shake_frequency)) < FrameTime;

	if (rval) {
		LevelUniqueSeismicState.Seismic_disturbance_end_time = GameTime64 + level_shake_duration;
		start_seismic_sound();
		if (Game_mode & GM_MULTI)
			multi_send_seismic(level_shake_duration);
	}
	return rval;
}

static void seismic_disturbance_frame(void)
{
	if (LevelSharedSeismicState.Level_shake_frequency) {
		if (seismic_disturbance_active()) {
			fix delta_time = static_cast<fix>(GameTime64 - LevelUniqueSeismicState.Seismic_disturbance_end_time);
			const int fc = abs(delta_time - LevelSharedSeismicState.Level_shake_duration / 2);
			apply_seismic_effect(fc / (F1_0 / 16));
		}
	}
}

}

//	Call this when a smega detonates to start the process of rocking the mine.
void smega_rock_stuff(void)
{
	auto least = &LevelUniqueSeismicState.Earthshaker_detonate_times[0];
	range_for (auto &i, LevelUniqueSeismicState.Earthshaker_detonate_times)
	{
		if (i + SMEGA_SHAKE_TIME < GameTime64)
			i = 0;
		if (*least > i)
			least = &i;
	}
	*least = GameTime64;
}

namespace {

static uint8_t Super_mines_yes = 1;

static bool immediate_detonate_smart_mine(const vcobjptridx_t smart_mine, const vcobjptridx_t target)
{
	if (smart_mine->segnum == target->segnum)
		return true;
	//	Object which is close enough to detonate smart mine is not in same segment as smart mine.
	//	Need to do a more expensive check to make sure there isn't an obstruction.
	if (likely((d_tick_count ^ (static_cast<vcobjptridx_t::integral_type>(smart_mine) + static_cast<vcobjptridx_t::integral_type>(target))) % 4))
		// Maybe next frame
		return false;
	fvi_info		hit_data;
	auto fate = find_vector_intersection(fvi_query{
		smart_mine->pos,
		target->pos,
		fvi_query::unused_ignore_obj_list,
		fvi_query::unused_LevelUniqueObjectState,
		fvi_query::unused_Robot_info,
		0,
		smart_mine,
	}, smart_mine->segnum, 0, hit_data);
	return fate != fvi_hit_type::Wall;
}

}

//	Call this once/frame to process all super mines in the level.
void process_super_mines_frame(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	int	start, add;

	//	If we don't know of there being any super mines in the level, just
	//	check every 8th object each frame.
	if (Super_mines_yes == 0) {
		start = d_tick_count & 7;
		add = 8;
	} else {
		start = 0;
		add = 1;
	}

	Super_mines_yes = 0;

	for (objnum_t i=start; i<=Highest_object_index; i+=add) {
		const auto io = vmobjptridx(i);
		if (likely(io->type != OBJ_WEAPON || get_weapon_id(io) != weapon_id_type::SUPERPROX_ID))
			continue;
		Super_mines_yes = 1;
		if (unlikely(io->lifeleft + F1_0*2 >= Weapon_info[weapon_id_type::SUPERPROX_ID].lifetime))
			continue;
		const auto parent_num = io->ctype.laser_info.parent_num;
		const auto &bombpos = io->pos;
		range_for (const auto &&jo, vmobjptridx)
		{
			if (unlikely(jo == parent_num))
				continue;
			if (jo->type != OBJ_PLAYER && jo->type != OBJ_ROBOT)
				continue;
			const auto dist_squared = vm_vec_dist2(bombpos, jo->pos);
			const vm_distance distance_threshold{F1_0 * 20};
			const auto distance_threshold_squared = distance_threshold * distance_threshold;
			if (likely(distance_threshold_squared < dist_squared))
				/* Cheap check, some false negatives */
				continue;
			const fix64 j_size = jo->size;
			const fix64 j_size_squared = j_size * j_size;
			if (dist_squared - j_size_squared >= distance_threshold_squared)
				/* Accurate check */
				continue;
			if (immediate_detonate_smart_mine(io, jo))
				io->lifeleft = 1;
		}
	}
}
#endif

#define SPIT_SPEED 20

//this function is for when the player intentionally drops a powerup
//this function is based on drop_powerup()
imobjptridx_t spit_powerup(d_level_unique_object_state &LevelUniqueObjectState, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const d_vclip_array &Vclip, const object_base &spitter, const unsigned id, const unsigned seed)
{
	d_srand(seed);

	auto new_velocity = vm_vec_scale_add(spitter.mtype.phys_info.velocity, spitter.orient.fvec, i2f(SPIT_SPEED));

	new_velocity.x += (d_rand() - 16384) * SPIT_SPEED * 2;
	new_velocity.y += (d_rand() - 16384) * SPIT_SPEED * 2;
	new_velocity.z += (d_rand() - 16384) * SPIT_SPEED * 2;

	// Give keys zero velocity so they can be tracked better in multi

	if ((Game_mode & GM_MULTI) && (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD))
		vm_vec_zero(new_velocity);

	//there's a piece of code which lets the player pick up a powerup if
	//the distance between him and the powerup is less than 2 time their
	//combined radii.  So we need to create powerups pretty far out from
	//the player.

	const auto new_pos = vm_vec_scale_add(spitter.pos, spitter.orient.fvec, spitter.size);

	if (Game_mode & GM_MULTI)
	{
		if (Net_create_loc >= MAX_NET_CREATE_OBJECTS)
		{
			return object_none;
		}
	}

	const auto &&objp = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_POWERUP, id, vmsegptridx(spitter.segnum), new_pos, &vmd_identity_matrix, Powerup_info[id].size, object::control_type::powerup, object::movement_type::physics, RT_POWERUP);

	if (objp == object_none)
		return objp;
	auto &obj = *objp;
	obj.mtype.phys_info.velocity = new_velocity;
	obj.mtype.phys_info.drag = 512;	//1024;
	obj.mtype.phys_info.mass = F1_0;

	obj.mtype.phys_info.flags = PF_BOUNCE;

	obj.rtype.vclip_info.vclip_num = Powerup_info[get_powerup_id(obj)].vclip_num;
	obj.rtype.vclip_info.frametime = Vclip[obj.rtype.vclip_info.vclip_num].frame_time;
	obj.rtype.vclip_info.framenum = 0;

	if (&spitter == ConsoleObject)
		obj.ctype.powerup_info.flags |= PF_SPAT_BY_PLAYER;

	switch (id)
	{
		case POW_MISSILE_1:
		case POW_MISSILE_4:
		case POW_SHIELD_BOOST:
		case POW_ENERGY:
			obj.lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
			if (Game_mode & GM_MULTI)
				obj.lifeleft /= 2;
			break;
		default:
			break;
	}
	return objp;
}

void DropCurrentWeapon (player_info &player_info)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	if (LevelUniqueObjectState.num_objects >= Objects.size())
		return;

	powerup_type_t drop_type;
	const auto &Primary_weapon = player_info.Primary_weapon;
	const auto GrantedItems = (Game_mode & GM_MULTI) ? Netgame.SpawnGrantedItems : 0;
	auto weapon_name = PRIMARY_WEAPON_NAMES(Primary_weapon);
	if (Primary_weapon == primary_weapon_index_t::LASER_INDEX)
	{
		if ((player_info.powerup_flags & PLAYER_FLAGS_QUAD_LASERS) && !GrantedItems.has_quad_laser())
		{
			/* Sorry, no message.  Need to fall through in case player
			 * wanted to drop a laser powerup.
			 */
			drop_type = POW_QUAD_FIRE;
			weapon_name = TXT_QUAD_LASERS;
		}
		else if (player_info.laser_level == laser_level::_1)
		{
			HUD_init_message_literal(HM_DEFAULT, "You cannot drop your base weapon!");
			return;
		}
#if defined(DXX_BUILD_DESCENT_II)
		else if (player_info.laser_level > MAX_LASER_LEVEL)
		{
			/* Disallow dropping any super lasers until someone requests
			 * it.
			 */
			HUD_init_message_literal(HM_DEFAULT, "You cannot drop super lasers!");
			return;
		}
#endif
		else if (player_info.laser_level <= map_granted_flags_to_laser_level(GrantedItems))
		{
			HUD_init_message_literal(HM_DEFAULT, "You cannot drop granted lasers!");
			return;
		}
		else
			drop_type = POW_LASER;
	}
	else
	{
		if (HAS_PRIMARY_FLAG(Primary_weapon) & map_granted_flags_to_primary_weapon_flags(GrantedItems))
		{
			HUD_init_message(HM_DEFAULT, "You cannot drop granted %s!", weapon_name);
			return;
		}
		drop_type = Primary_weapon_to_powerup[Primary_weapon];
	}

	const auto seed = d_rand();
	const auto objnum = spit_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, *ConsoleObject, drop_type, seed);
	if (objnum == object_none)
	{
		HUD_init_message(HM_DEFAULT, "Failed to drop %s!", weapon_name);
		return;
	}

	HUD_init_message(HM_DEFAULT, "%s dropped!", weapon_name);
#if defined(DXX_BUILD_DESCENT_II)
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);
#endif

	if (weapon_index_uses_vulcan_ammo(Primary_weapon)) {

		//if it's one of these, drop some ammo with the weapon
		auto &plr_vulcan_ammo = player_info.vulcan_ammo;
		auto ammo = plr_vulcan_ammo;
#if defined(DXX_BUILD_DESCENT_II)
		const auto HAS_VULCAN_AND_GAUSS_FLAGS = HAS_VULCAN_FLAG | HAS_GAUSS_FLAG;
		if ((player_info.primary_weapon_flags & HAS_VULCAN_AND_GAUSS_FLAGS) == HAS_VULCAN_AND_GAUSS_FLAGS)
			ammo /= 2;		//if both vulcan & gauss, drop half
#endif

		plr_vulcan_ammo -= ammo;

			objnum->ctype.powerup_info.count = ammo;
	}
#if defined(DXX_BUILD_DESCENT_II)
	if (Primary_weapon == primary_weapon_index_t::OMEGA_INDEX) {

		//dropped weapon has current energy

			objnum->ctype.powerup_info.count = player_info.Omega_charge;
	}
#endif

	if (Game_mode & GM_MULTI)
		multi_send_drop_weapon(objnum,seed);

	if (Primary_weapon == primary_weapon_index_t::LASER_INDEX)
	{
		if (drop_type == POW_QUAD_FIRE)
			player_info.powerup_flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		else
			-- player_info.laser_level;
	}
	else
		player_info.primary_weapon_flags &= ~HAS_PRIMARY_FLAG(Primary_weapon);
	auto_select_primary_weapon(player_info);
}

void DropSecondaryWeapon (player_info &player_info)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	int seed;
	ushort sub_ammo=0;

	if (LevelUniqueObjectState.num_objects >= Objects.size())
		return;

	auto &Secondary_weapon = player_info.Secondary_weapon;
	auto &secondary_ammo = player_info.secondary_ammo[Secondary_weapon];
	if (secondary_ammo == 0)
	{
		HUD_init_message(HM_DEFAULT, "Cannot drop %s: you have none to drop!", SECONDARY_WEAPON_NAMES(Secondary_weapon));
		return;
	}

	auto weapon_drop_id = Secondary_weapon_to_powerup[Secondary_weapon];

	// see if we drop single or 4-pack
	switch (weapon_drop_id)
	{
		case POW_MISSILE_1:
		case POW_HOMING_AMMO_1:
#if defined(DXX_BUILD_DESCENT_II)
		case POW_SMISSILE1_1:
		case POW_GUIDED_MISSILE_1:
		case POW_MERCURY_MISSILE_1:
#endif
			if (secondary_ammo % 4)
			{
				sub_ammo = 1;
			}
			else
			{
				sub_ammo = 4;
				//4-pack always is next index
				weapon_drop_id = static_cast<powerup_type_t>(1 + static_cast<uint_fast32_t>(weapon_drop_id));
			}
			break;
		case POW_PROXIMITY_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
		case POW_SMART_MINE:
#endif
			if (secondary_ammo < 4)
			{
				HUD_init_message(HM_DEFAULT, "Cannot drop %s: You need at least 4 to drop, but have only %u!", SECONDARY_WEAPON_NAMES(Secondary_weapon), secondary_ammo);
				return;
			}
			else
			{
				sub_ammo = 4;
			}
			break;
		case POW_SMARTBOMB_WEAPON:
		case POW_MEGA_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
		case POW_EARTHSHAKER_MISSILE:
#endif
			sub_ammo = 1;
			break;
		case POW_EXTRA_LIFE:
		case POW_ENERGY:
		case POW_SHIELD_BOOST:
		case POW_LASER:
		case POW_KEY_BLUE:
		case POW_KEY_RED:
		case POW_KEY_GOLD:
		case POW_MISSILE_4:
		case POW_QUAD_FIRE:
		case POW_VULCAN_WEAPON:
		case POW_SPREADFIRE_WEAPON:
		case POW_PLASMA_WEAPON:
		case POW_FUSION_WEAPON:
		case POW_HOMING_AMMO_4:
		case POW_VULCAN_AMMO:
		case POW_CLOAK:
		case POW_TURBO:
		case POW_INVULNERABILITY:
		case POW_MEGAWOW:
#if defined(DXX_BUILD_DESCENT_II)
		case POW_GAUSS_WEAPON:
		case POW_HELIX_WEAPON:
		case POW_PHOENIX_WEAPON:
		case POW_OMEGA_WEAPON:
		case POW_SUPER_LASER:
		case POW_FULL_MAP:
		case POW_CONVERTER:
		case POW_AMMO_RACK:
		case POW_AFTERBURNER:
		case POW_HEADLIGHT:
		case POW_SMISSILE1_4:
		case POW_GUIDED_MISSILE_4:
		case POW_MERCURY_MISSILE_4:
		case POW_FLAG_BLUE:
		case POW_FLAG_RED:
		case POW_HOARD_ORB:
#endif
			break;
	}

	seed = d_rand();

	auto objnum = spit_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, *ConsoleObject, weapon_drop_id, seed);

	const auto weapon_name = SECONDARY_WEAPON_NAMES(Secondary_weapon);
	if (objnum == object_none)
	{
		HUD_init_message(HM_DEFAULT, "Failed to drop %s%s!", weapon_name, sub_ammo > 1 ? "s" : "");
		return;
	}
#if defined(DXX_BUILD_DESCENT_II)
	digi_play_sample(SOUND_DROP_WEAPON, F1_0);
#endif

	if (Game_mode & GM_MULTI)
		multi_send_drop_weapon(objnum,seed);

	secondary_ammo -= sub_ammo;
	HUD_init_message(HM_DEFAULT, "Dropped %s%s, leaving %u on board!", weapon_name, sub_ammo > 1 ? "s" : "", secondary_ammo);

	if (secondary_ammo == 0)
	{
		auto_select_secondary_weapon(player_info);
	}
}

#if defined(DXX_BUILD_DESCENT_II)
//	---------------------------------------------------------------------------------------
//	Do seismic disturbance stuff including the looping sounds with changing volume.
void do_seismic_stuff(void)
{
	const auto stv_save = std::exchange(LevelUniqueSeismicState.Seismic_tremor_volume, 0);
	LevelUniqueSeismicState.Seismic_tremor_magnitude = 0;

	rock_the_mine_frame();
	seismic_disturbance_frame();

	if (stv_save != 0) {
		const auto Seismic_tremor_volume = LevelUniqueSeismicState.Seismic_tremor_volume;
		if (Seismic_tremor_volume == 0)
		{
			digi_stop_looping_sound();
			LevelUniqueSeismicState.Next_seismic_sound_time = 0;
		}
		else if (GameTime64 > LevelUniqueSeismicState.Next_seismic_sound_time)
		{
			int	volume;

			volume = Seismic_tremor_volume * 2048;
			if (volume > F1_0)
				volume = F1_0;
			digi_change_looping_volume(volume);
			LevelUniqueSeismicState.Next_seismic_sound_time = GameTime64 + d_rand()/4 + 8192;
		}
	}

}
#endif

}

namespace serial {

template <typename T>
class is_cxx_array<enumerated_array<T, NDL, Difficulty_level_type>> : public is_cxx_array<std::array<T, NDL>>
{
};

}

#if defined(DXX_BUILD_DESCENT_I)
DEFINE_SERIAL_UDT_TO_MESSAGE(dsx::weapon_info, w, (w.render, w.model_num, w.model_num_inner, w.persistent, w.flash_vclip, w.flash_sound, w.robot_hit_vclip, w.robot_hit_sound, w.wall_hit_vclip, w.wall_hit_sound, w.fire_count, w.ammo_usage, w.weapon_vclip, w.destroyable, w.matter, w.bounce, w.homing_flag, w.dum1, w.dum2, w.dum3, w.energy_usage, w.fire_wait, w.bitmap, w.blob_size, w.flash_size, w.impact_size, w.strength, w.speed, w.mass, w.drag, w.thrust, w.po_len_to_width_ratio, w.light, w.lifetime, w.damage_radius, w.picture));
#elif defined(DXX_BUILD_DESCENT_II)
namespace {

struct v2_weapon_info : weapon_info
{
};

}

template <typename Accessor>
void postprocess_udt(Accessor &, v2_weapon_info &w)
{
	w.children = weapon_id_type::unspecified;
	w.multi_damage_scale = F1_0;
	w.hires_picture = w.picture;
}

DEFINE_SERIAL_UDT_TO_MESSAGE(v2_weapon_info, w, (w.render, w.persistent, w.model_num, serial::pad<1>(), w.model_num_inner, serial::pad<1>(), w.flash_vclip, w.robot_hit_vclip, w.flash_sound, w.wall_hit_vclip, w.fire_count, w.robot_hit_sound, w.ammo_usage, w.weapon_vclip, w.wall_hit_sound, w.destroyable, w.matter, w.bounce, w.homing_flag, w.speedvar, w.flags, w.flash, w.afterburner_size, w.energy_usage, w.fire_wait, w.bitmap, w.blob_size, w.flash_size, w.impact_size, w.strength, w.speed, w.mass, w.drag, w.thrust, w.po_len_to_width_ratio, w.light, w.lifetime, w.damage_radius, w.picture));
DEFINE_SERIAL_UDT_TO_MESSAGE(weapon_info, w, (w.render, w.persistent, w.model_num, serial::pad<1>(), w.model_num_inner, serial::pad<1>(), w.flash_vclip, w.robot_hit_vclip, w.flash_sound, w.wall_hit_vclip, w.fire_count, w.robot_hit_sound, w.ammo_usage, w.weapon_vclip, w.wall_hit_sound, w.destroyable, w.matter, w.bounce, w.homing_flag, w.speedvar, w.flags, w.flash, w.afterburner_size, w.children, w.energy_usage, w.fire_wait, w.multi_damage_scale, w.bitmap, w.blob_size, w.flash_size, w.impact_size, w.strength, w.speed, w.mass, w.drag, w.thrust, w.po_len_to_width_ratio, w.light, w.lifetime, w.damage_radius, w.picture, w.hires_picture));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(weapon_info, 125);
ASSERT_SERIAL_UDT_MESSAGE_SIZE(v2_weapon_info, 118);
#endif

#if 0
void weapon_info_write(PHYSFS_File *fp, const weapon_info &w)
{
	PHYSFSX_serialize_write(fp, w);
}
#endif

/*
 * reads n weapon_info structs from a PHYSFS_File
 */
namespace dsx {

void weapon_info_read_n(weapon_info_array &wi, std::size_t count, PHYSFS_File *fp,
#if defined(DXX_BUILD_DESCENT_II)
						const pig_hamfile_version file_version,
#endif
						std::size_t offset)
{
	auto r = partial_range(wi, offset, count);
#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
	if (file_version < pig_hamfile_version::_3)
	{
		range_for (auto &w, r)
			PHYSFSX_serialize_read(fp, static_cast<v2_weapon_info &>(w));
		/* Set the type of children correctly when using old
		 * datafiles.  In earlier descent versions this was simply
		 * hard-coded in create_smart_children().
		 */
		wi[weapon_id_type::SMART_ID].children = weapon_id_type::PLAYER_SMART_HOMING_ID;
		wi[weapon_id_type::SUPERPROX_ID].children = weapon_id_type::SMART_MINE_HOMING_ID;
		return;
	}
#endif
	range_for (auto &w, r)
	{
		PHYSFSX_serialize_read(fp, w);
	}
}
}
