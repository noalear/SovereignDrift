/* enumerated types */
enum ObjectType {all_type, game_type, module_type, explosion, generic, planet, designed, missile, ship, colony, last_type};
enum TrackingType {dumb, bomb, smart};
enum TriggerType {on_ship = 1, on_missile = 2, on_colony = 4,
		  on_planet = 8, on_heat = 16,
		  on_proximity = 32, on_time = 64};
enum TargetType {no_target = 0, parent_target = 1, planet_target = 2,
		 ship_target = 4, missile_target = 8, colony_target = 16,
		 free_colony_target = 32, target_target = 64, star_target = 128,
		 all_target = 255};
enum TargetSelector {all, nearest, nearest_enemy,
		     nearest_weaker_enemy, nearest_friendly, 
		     nearest_port, nearest_multi_port,nearest_leader,
		     nearest_thrusting_enemy,
		     specific, sequential, sequential_friendly,
		     sequential_enemy, sequential_onscreen, self};
enum MessageType {personal = 1, broadcast = 2, spawn_with_ship, spawn_with_new_colony, spawn_with_colony,
		  spawn_with_planet};
enum detail {none, escort_detail, defend_detail, protect_detail,
	     patrol_detail, harass_detail, conquer_detail,
	     attack_detail, colonize_detail, plugin, ram_weapon, last_detail};
enum Operations {op_on, op_off, op_toggle};
enum ConditionType {conquer_num, conquer_list, hold_num, hold_list};
enum ConditionTimingType {for_time, within_time, unlimited_time};
enum ConditionResultType {win};
enum ErrorCode {no_error = 0, module_deposit_corrupt, module_fire_corrupt,
		module_withdraw_corrupt, module_requirement_corrupt, module_storage_corrupt};
enum Event {fail_to_conquer_event, supplement_colony_event, conquer_colony_event, colonize_event,
	    capture_enemy_event, captured_by_enemy_event, orbit_event, successful_docking_event, 
	    player_promoted_event, acquire_target_event, distress_call_event, pickup_colony_event, 
	    disengaging_from_dock_event, revolution_event, defection_event, planet_surrenders_event,
	    upgrade_event, free_colony_spawned_event, warping_event, push_button1_event, push_button2_event,
	    push_button3_event, klaxon_event, ping_event, player_logon_event, player_logoff_event, last_event};
enum AILevel {low_AI_level, medium_AI_level, high_AI_level};

