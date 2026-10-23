/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
/* Global Constants */
const char  max_players = 128;
const char  max_races = 0;
const char  max_planets = 0;
const short max_missiles = 255;
const short max_stored_players = 160;
const short max_power_level = 1000;
const char  max_name_char = 32;
const char  max_module_name_char = 12;
const short max_ship_designs = 256;
const short max_missile_designs = 512;
const short max_modules = 2048;
const char  max_module_par = 6;
const char  max_module_spar = 5;
const char  max_module_wpar = 2;
const char  max_module_list = 12;
const char  max_tubes_per_ship = 8;
const char  max_angles = 36;
const char  max_slide = 40;
const short max_bitmaps = 128;
const slong max_colony_millions = 1000;
const short planet_ship_g_factor = 100;
const short planet_planet_g_factor = 100;
const slong  power_overhead_per_slot = 1;
const slong  sensor_threshold = 1000;
const slong  min_sensor_value = 2;
const slong  forget_interval = 1000;
const slong  typical_ship_mass = 5000;
const slong  NoObjectBitmapIndex = -1;
const slong  long_billion = 1000000000;

#define MAXANGLES  360

/* Buttons */
const char straight = 102;
#if defined(WIN32)
const char ctl_button_left = 29;
const char ctl_button_right = 29;
#endif
#if defined(OS2) || defined(UNIX)
const char ctl_button_left = 29;
const char ctl_button_right = 91;
#endif
const char space_button = 57;
const char fire_tube_a = space_button;
const char left_shift_button = 42;
const char right_shift_button = 54;
const char left_arrow_down = 75;
const char alt_left_arrow_down = 99;
const char right_arrow_down = 77;
const char alt_right_arrow_down = 100;
const char all_stop_button_down = 82;
const char thrust_button = 76;
const char alt_thrust_button = 97;
const char thrust_reverse_button = 80;
const char alt_thrust_reverse_button = 102;
const char thrust_left_button = 79;
const char thrust_right_button = 81;
const char exchange_button = 23;
const char racelist_button = 19;
const char establish_orbit_button = 24;
const char playerlist_button = 25;
const char query_mark_button = 26;
const char mark_button = 27;
const char system_button = 31;
const char save_button = 37;
const char go_dock_button_down = 32;
const char go_planet_button_down = 34;
const char distress_button_down = 35;
const char target_button = 20;
const char query_button = 16;
const char acquire_button = 30;
const char page_up_button = 52;
const char page_down_button = 51;
const char bind_button = 48;
const char f9_button_down = 67;
const char message_button = 50;
const char c_button_down = 46;
const char z_button_down = 44;
const char auto_button_down = 18;
const char tractor_button_down = 21;
const char go_button_down = 34;
const char attack_button_down = 33; /*f*/
const char join_button_down = 36; /*j*/
const char respond_button_down = 38; /* l */
const char scram_button_down = 40;
const char select_button = 90;
const char alt_select_button = 28;
const char boost_button_down = 72;
const char release_button_down = 53;
/* 0 -> 245 */
const char go_home_planet = 96;
const char view_button_down = 22;
const char destruct_button_down = 45;
const char detonate_button = 83;
const char alt_detonate_button = 105;
const char navigate_button = 49;
const char stealth_button_down = 46;
const char refit_button = 14;
const char alt_drop_button = 86;
const char drop_button = 43;
const char one_button = 2;
const char two_button = 3;
const char three_button = 4;
const char four_button = 5;
const char five_button = 6;
const char six_button = 7;
const char seven_button = 8;
const char eight_button = 9;
const char nine_button = 10;

/* AI parameters */
const slong fire_rate_AI = 2;
const slong low_priority_distress_rate = 20;

/* racial parameters */
const slong racial_loyalty = 200;
const slong defection_rate = 100000;

/* default rates */
const char default_rotation_rate = 8;
const char bounty_pct = 50;
const char offensive_factor = 200;
const char salvage_right_pct = 10;
const char bounty_right_pct = 2;
const slong land_right_factor = 30000;

/* limits */
const slong pc_long_max = 2147483647;
const ulong pc_ulong_max = (ulong) pc_long_max;
const short pc_short_max = 32767;
const slong pc_long_sqrt_max = 45000;
const slong lower_production_limit = 15000;
const short max_stealth_level = 7;
const char max_rotation_interval = 4;
const short num_extra_colonies = 4;
const short num_reward_plugins = 20;
const slong  plugin_production_req = 1500000;
const slong  forced_plugin_production_limit = 2 * plugin_production_req;
const slong  max_acc = 120; /* pixels / s**2 */
const slong  min_acc = 10;
const slong max_credit = 1000000000;
const slong max_bonus = 1000;
const short star_prox_check_frames = 3;
const short max_supply = 100;
const short self_sustain_limit = 30;

/* scales */
const char  dt = 1;
const slong  sub_scale = 1000;
const slong  usub_scale = (ulong) 1000;
const slong  sub_scale2 = sub_scale*sub_scale;
const slong  moon_orbit_scale = 10;
const slong  unit_ship_mass = 200; /* metric tons */
const slong  unit_missile_mass = 1; /* metric tons */
const slong  hull_mass_per_slot = 10;
const slong  hull_mass_per_unit_volume = 2;
const slong  unit_thrust = 200; /* standard thrust */
const slong  g_scale = 10;
const slong  terraform_scale = 1000000;
const slong  terraform_base  = terraform_scale / 100;
const slong  loyalty_cycle = 10; /* seconds */
const short mouse_scale = 5;
const short module_function_scale = 100;
const short ship_distress_func_level = module_function_scale / 2;
const short module_break_limit = module_function_scale / 3;
const short module_warning_limit = 2 * module_function_scale / 3;
const double grav_constant_1 = 1e6;
const slong grav_constant_2 = 1000;

/* important distances */
const slong dist_crit = sub_scale * 200;
const slong min_arm_frame = 30;
const slong max_grav_dist = 1000 * sub_scale;
const slong max_patrol_dist = 1000 * sub_scale;
const slong sensor_dist = 10 * sub_scale;
const slong light_speed = 500 * sub_scale; /* pixels/s */
const slong attack_dist = 500 * sub_scale;
const slong avoid_dist = 50 * sub_scale;
const slong alert_dist = 200 * sub_scale;
const slong min_warp_dist = 800 * sub_scale;
const slong min_attack_dist = 400 * sub_scale;
const slong min_approach_dist = 100 * sub_scale;
const slong plugin_spawn_dist = 150 * sub_scale;
const ulong max_orbit_dist = 500 * sub_scale;
const ulong max_blockade_dist = 300 * sub_scale;
const ulong ship_dock_range = 3 * sub_scale;
const slong colony_dock_range = 10 * sub_scale;
const slong free_colony_pickup_range = 2000 * sub_scale;
const slong max_missile_correction = 100 * sub_scale; /* pixels * sub_scale */

/* conversions */
const slong au_to_pixel = pc_long_max / 200;
const slong mo_to_pixel = au_to_pixel / 150000 * moon_orbit_scale;
const slong dam_to_mass = 100; /* mass to damage conversion */
const slong vel_to_damage = 1; /* energy to warhead conversion/o */
const slong energy_to_damage = 1000; /* energy to warhead conversion/o */
const slong energy_to_prox_damage = 1000; /* energy to warhead conversion/oo */
const slong score_to_crew_experience_level = 5;
const char thrust_to_rotation = 2;

#ifdef SF_MAIN
const char* PlanetClassName[4] = {"Star", "Planet", "Moon", "Asteroid"};
const char* detailName[last_detail-1] = {
  "none", "escort", "defend", "protect",
  "patrol", "harass", "conquer",
  "attack", "colonize"
};
const char* objectTypeName[last_type] = {
  "all", "game", "module",
  "explosion", "generic", "planet", "designed",
  "missile", "ship", "colony"
};
const char* eventName[last_event] = {
  "fail_to_conquer", "supplement_colony", "conquer_colony", "colonize",
  "capture_enemy", "captured_by_enemy", "orbit", "successful_docking", 
  "player_promoted", "acquire_target", "distress_call", "pickup_colony", 
  "disengaging_from_dock", "revolution", "defection", "planet_surrenders",
  "upgrade", "free_colony_spawned", "warping", "push_button1", "push_button2",
  "push_button3", "klaxon", "ping", "player_logon", "player_logoff"
};
#else
extern const char* PlanetClassName[];
extern const char* detailName[];
extern const char* objectTypeName[];
extern const char* eventName[];
#endif
