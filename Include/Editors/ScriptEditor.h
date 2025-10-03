#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <regex>
#include <TextEditor.h>
#include <Game/Actor.h>

static const std::set<std::string> g_MafiaCmds = {
    // commandKeywords (first block)
    "autosavegame",
    "autosavegamefull",
    "get_pm_crashtime",
    "get_pm_firetime",
    "get_pm_humanstate",
    "get_pm_state",
    "get_remote_actor",
    "get_remote_float",
    "getactivecamera",
    "getactiveplayer",
    "getactorframe",
    "getactorsdist",
    "getangleactortoactor",
    "getcardamage",
    "getcarlinenumfromtable",
    "getenemyaistate",
    "getfilmmusic",
    "getframefromactor",
    "getgametime",
    "getlastsavenum",
    "getmissionnumber",
    "getsoundtime",
    "getticktime",
    // statementKeywords
    "goto",
    "let",
    "wait",
    "if",
    "end",
    "call",
    "gosub",
    "return",
    "rnd",
    "up",
    "down",
    // commandKeywords (second block)
    "act_setstate",
    "actor_adddorici",
    "actor_delete",
    "actor_duplicate",
    "actor_setdir",
    "actor_setplacement",
    "actor_setpos",
    "actorlightnesson",
    "actorupdateplacement",
    "airplane_isdestroyed",
    "airplane_start",
    "airplane_start2",
    "airplaneshowdamage",
    "bridge_shutdown",
    "callsubroutine",
    "camera_getfov",
    "camera_lock",
    "camera_setfov",
    "camera_setrange",
    "camera_setswing",
    "camera_unlock",
    "car_breakmotor",
    "car_calm",
    "car_disable_uo",
    "car_enableus",
    "car_explosion",
    "car_forcestop",
    "car_getactlevel",
    "car_getmaxlevels",
    "car_getseatcount",
    "car_getspeccoll",
    "car_getspeed",
    "car_invisible",
    "car_inwater",
    "car_lock",
    "car_lock_all",
    "car_muststeal",
    "car_remove_driver",
    "car_repair",
    "car_setactlevel",
    "car_setaxis",
    "car_setdestroymotor",
    "car_setdooropen",
    "car_setprojector",
    "car_setspeccoll",
    "car_setspeed",
    "car_switchshowenergy",
    "car_unbreakable",
    "cardamagevisible",
    "carlight_indic_l",
    "carlight_indic_off",
    "carlight_indic_r",
    "carlight_light",
    "carlight_main",
    "cartridge_invalidate",
    "change_mission",
    "character_pop",
    "character_push",
    "citycaching_tick",
    "citymusic_off",
    "citymusic_on",
    "cleardifferences",
    "coll_testline",
    "commandblock",
    "compareactors",
    "compareframes",
    "compareownerwith",
    "compareownerwithex",
    "create_physicalobject",
    "createweaponfromframe",
    "ctrl_read",
    "dan_internal_1",
    "dan_internal_2",
    "destroy_physicalobject",
    "detector_createdyncoll",
    "detector_erasedyncoll",
    "detector_inrange",
    "detector_issignal",
    "detector_setsignal",
    "detector_waitforhit",
    "detector_waitforuse",
    "dialog_begin",
    "dialog_camswitch",
    "dialog_end",
    "disablecolls",
    "door_enableus",
    "door_getstate",
    "door_lock",
    "door_open",
    "door_setopenpose",
    "emitparticle",
    "enablemap",
    "endofmission",
    "event_use_cb",
    "explosion",
    "findactor",
    "findframe",
    "findnearactor",
    "floatreg_pop",
    "floatreg_push",
    "frameistelephone",
    "freeride_enablecar",
    "freeride_scoreadd",
    "freeride_scoreget",
    "freeride_scoreon",
    "freeride_scoreset",
    "freeride_traffsetup",
    "frm_getchild",
    "frm_getdir",
    "frm_getlocalmatrix",
    "frm_getnumchildren",
    "frm_getparent",
    "frm_getpos",
    "frm_getrot",
    "frm_getscale",
    "frm_getworlddir",
    "frm_getworldmatrix",
    "frm_getworldpos",
    "frm_getworldrot",
    "frm_getworldscale",
    "frm_ison",
    "frm_linkto",
    "frm_setalpha",
    "frm_setdir",
    "frm_seton",
    "frm_setpos",
    "frm_setrot",
    "frm_setscale",
    "fuckingbox_add",
    "fuckingbox_add_dest",
    "fuckingbox_getnumbox",
    "fuckingbox_getnumdest",
    "fuckingbox_move",
    "fuckingbox_recompile",
    "game_nightmission",
    "garage_addcar",
    "garage_addcaridx",
    "garage_addlaststolen",
    "garage_cansteal",
    "garage_carmanager",
    "garage_delcar",
    "garage_enablesteal",
    "garage_generatecars",
    "garage_nezahazuj",
    "garage_releasecars",
    "garage_show",
    "group_disband",
    "gunshop_menu",
    // commandKeywords (third block)
    "beep",
    "console_addtext",
    "debug_getframeinfo",
    "debug_text",
    // commandKeywords (fourth block)
    "enemy_action_backlook",
    "enemy_action_fire",
    "enemy_action_follow",
    "enemy_action_jump",
    "enemy_action_phonecall",
    "enemy_action_runaway",
    "enemy_action_runcloserto",
    "enemy_action_sidelook",
    "enemy_action_stepaway",
    "enemy_action_strafe",
    "enemy_actionsclear",
    "enemy_arrest_player",
    "enemy_az_po_panice_hovno",
    "enemy_blastfire",
    "enemy_block",
    "enemy_block_checkpoint",
    "enemy_box_goto",
    "enemy_box_put",
    "enemy_box_take",
    "enemy_brainwash",
    "enemy_car_escape",
    "enemy_car_hunt",
    "enemy_changeanim",
    "enemy_enter_door",
    "enemy_exit_railway",
    "enemy_followplayer",
    "enemy_forcescript",
    "enemy_gethostilestate",
    "enemy_getstate",
    "enemy_group_add",
    "enemy_group_addcar",
    "enemy_group_chcipni_hajzel",
    "enemy_group_del",
    "enemy_group_new",
    "enemy_ignore_corpse",
    "enemy_kickdriver",
    "enemy_lockstate",
    "enemy_look",
    "enemy_lookto",
    "enemy_move",
    "enemy_move_to_car",
    "enemy_move_to_frame",
    "enemy_naprdelivaute",
    "enemy_narohysevyser",
    "enemy_naserse",
    "enemy_nevyhazuj",
    "enemy_playanim",
    "enemy_podvadim_jak",
    "enemy_setstate",
    "enemy_shutup",
    "enemy_stop",
    "enemy_stopanim",
    "enemy_talk",
    "enemy_turnback",
    "enemy_unblock",
    "enemy_use_detector",
    "enemy_use_railway",
    "enemy_usecar",
    "enemy_vidim",
    "enemy_wait",
    "enemy_watch",
    // commandKeywords (fifth block)
    "human_activateweapon",
    "human_addweapon",
    "human_anyweaponinhand",
    "human_anyweaponininventory",
    "human_canaddweapon",
    "human_candie",
    "human_changeanim",
    "human_changemodel",
    "human_createab",
    "human_death",
    "human_delweapon",
    "human_entertotruck",
    "human_eraseab",
    "human_force_settocar",
    "human_forcefall",
    "human_fromcar",
    "human_getactanimid",
    "human_getiteminrhand",
    "human_getowner",
    "human_getproperty",
    "human_getseatidx",
    "human_havebody",
    "human_havebox",
    "human_holster",
    "human_isweapon",
    "human_linktohand",
    "human_looktoactor",
    "human_looktoframe",
    "human_reset",
    "human_returnfrompanic",
    "human_returntotraff",
    "human_set8slot",
    "human_setfiretarget",
    "human_setproperty",
    "human_shutup",
    "human_stoptalk",
    "human_talk",
    "human_throwgrenade",
    "human_throwitem",
    "human_unlinkfromhand",
    "human_waittoready",
    "iffltinrange",
    "ifplayerstealcar",
    "intro_subtitle_add",
    "inventory_clear",
    "inventory_pop",
    "inventory_push",
    "iscarusable",
    "ispointinarea",
    "ispointinsector",
    "isvalidtaxipassenger",
    "loadcolltree",
    "loaddifferences",
    "m7_zastavzkurvenepolise",
    "math_abs",
    "math_cos",
    "math_sin",
    "enemy_car_moveto",
    "entity_debug_graphs",
    "get_pm_numpredators",
    "frm_getlocalmatrix",
    "mission_objectives",
    "mission_objectivesclear",
    "mission_objectivesremove",
    "model_create",
    "model_destroy",
    "model_playanim",
    "model_stopanim",
    "noanimpreload",
    "npc_shutup",
    "person_playanim",
    "person_stopanim",
    "phobj_impuls",
    "play_avi_intro",
    "player_lockcontrols",
    "playsound",
    "playsoundex",
    "playsoundstop",
    "pm_setprogress",
    "pm_showprogress",
    "pm_showsymbol",
    "pockurvenychbedencar",
    "police_speed_factor",
    "police_support",
    "policeitchforplayer",
    "policemanager_add",
    "policemanager_del",
    "policemanager_forcearrest",
    "policemanager_on",
    "policemanager_setspeed",
    "preloadmodel",
    "program_storage",
    "pumper_canwork",
    "racing_autoinvisible",
    "racing_change_model",
    "racing_mission6_init",
    "racing_mission6_start",
    "recaddactor",
    "recclear",
    "recload",
    "recloadfull",
    "recunload",
    "recwaitforend",
    "set_remote_actor",
    "set_remote_float",
    "set_remote_frame",
    "setaipriority",
    "setcitytrafficvisible",
    "setcompass",
    "setevent",
    "setfilmmusic",
    "setfreeride",
    "setlmlevel",
    "setmissionnameid",
    "setmissionnumber",
    "setmodeltocar",
    "setnoanimhit",
    "setnpckillevent",
    "setnullactor",
    "setnullframe",
    "setplayerfireevent",
    "setplayerhornevent",
    "setplayerfallevent",
    "settankhitcount",
    "settimeoutevent",
    "settraffsectorsnd",
    "showcardammage",
    "sound_getvolume",
    "sound_setvolume",
    "soundfade",
    "stopparticle",
    "stopsound",
    "stream_connect",
    "stream_create",
    "stream_destroy",
    "stream_fadevol",
    "stream_getpos",
    "stream_pause",
    "stream_play",
    "stream_setloop",
    "stream_setpos",
    "stream_stop",
    "subtitle",
    "taxidriver_enable",
    "timer_getinterval",
    "timer_setinterval",
    "timeroff",
    "timeron",
    "use_lightcache",
    "version_is_editor",
    "version_is_germany",
    "vlvp",
    "wagon_getlastnode",
    "wagon_setevent",
    "weather_preparebuffer",
    "weather_reset",
    "weather_setparam",
    "wingman_delindicator",
    "wingman_setindicator",
    "zatmyse",
    // commandKeywords (sixth block)
    "matrix_copy",
    "matrix_identity",
    "matrix_inverse",
    "matrix_mul",
    "matrix_zero",
    "quat_add",
    "quat_copy",
    "quat_dot",
    "quat_extract",
    "quat_getrotmatrix",
    "quat_inverse",
    "quat_make",
    "quat_mul_quat",
    "quat_mul_scl",
    "quat_normalize",
    "quat_rotbymatrix",
    "quat_setdir",
    "quat_slerp",
    "vect_add_vect",
    "vect_angleto",
    "vect_copy",
    "vect_inverse",
    "vect_magnitude",
    "vect_mul_matrix",
    "vect_mul_quat",
    "vect_mul_scl",
    "vect_mul_vect",
    "vect_normalize",
    "vect_set",
    "vect_sub_vect",
    // labelKeywords
    "label",
    "event",
    // dimKeywords
    "dim_flt",
    "dim_act",
    "dim_frm",
    "flt"};

// List of constants (unchanged, complete from MSDR.CZ.pdf)
static const std::set<std::string> g_MafiaConstants = {"active",
                                                       "inactive",
                                                       "tretera",
                                                       "off",
                                                       "close",
                                                       "target",
                                                       "nonext",
                                                       "nolink",
                                                       "return",
                                                       "crouch",
                                                       "run",
                                                       "walk",
                                                       "natvrdo",
                                                       "stopvzad",
                                                       "vzad",
                                                       "stop",
                                                       "sledovani",
                                                       "bokovka",
                                                       "robertek",
                                                       "zpet_ni_krok",
                                                       "pasivni",
                                                       "hostile_search",
                                                       "hostile_hostile",
                                                       "panic_runaway",
                                                       "panic_freeze",
                                                       "fight_guard_nohostile",
                                                       "fight_guard_stand",
                                                       "fight_guard",
                                                       "fight_dogfight",
                                                       "search",
                                                       "hostile",
                                                       "runaway",
                                                       "freeze",
                                                       "sleep",
                                                       "on",
                                                       "max_cnt",
                                                       "energy",
                                                       "energyhandl",
                                                       "energyhandr",
                                                       "energylegl",
                                                       "energylegr",
                                                       "reactions",
                                                       "speed",
                                                       "aggresivity",
                                                       "intelligence",
                                                       "shooting",
                                                       "sight",
                                                       "hearing",
                                                       "driving",
                                                       "mass",
                                                       "colorh",
                                                       "len",
                                                       "width",
                                                       "max_dist",
                                                       "max_height",
                                                       "dir_x",
                                                       "dir_y",
                                                       "dir_z",
                                                       "mode",
                                                       "sector",
                                                       "dummies",
                                                       "action1",
                                                       "action",
                                                       "clutch1",
                                                       "clutch",
                                                       "crouch1",
                                                       "crouch",
                                                       "down1",
                                                       "down",
                                                       "drvup1",
                                                       "drvup",
                                                       "drvdown1",
                                                       "drvdown",
                                                       "drvleft1",
                                                       "drvleft",
                                                       "drvright1",
                                                       "drvright",
                                                       "fire1",
                                                       "fire",
                                                       "gearup1",
                                                       "gearup",
                                                       "geardown1",
                                                       "geardown",
                                                       "handbrake1",
                                                       "handbrake",
                                                       "holster",
                                                       "horn1",
                                                       "horn",
                                                       "inventory",
                                                       "jump1",
                                                       "jump",
                                                       "left1",
                                                       "left",
                                                       "map",
                                                       "motorswitch",
                                                       "objectives",
                                                       "right1",
                                                       "right",
                                                       "run1",
                                                       "run",
                                                       "runswitch",
                                                       "reload",
                                                       "snipermode",
                                                       "speedlimit1",
                                                       "speedlimit",
                                                       "up1",
                                                       "up",
                                                       "weapondrop"};

// Parameter map for meaningful names (from MSDR.CZ.pdf, with placeholders for commands not in PDF)
static const std::map<std::string, std::vector<std::string>> g_ParamMap = {
    {"act_setstate", {"<actorID>", "<state>"}},
    {"actor_adddorici", {"<actorID>"}},
    {"actor_delete", {"<actorID>"}},
    {"actor_duplicate", {"<sourceActorID>", "<newActorID>", "<state>"}},
    {"actor_setdir", {"<actorID>", "<frameID>"}},
    {"actor_setplacement", {"<actorID>", "<frameID>"}},
    {"actor_setpos", {"<actorID>", "<frameID>"}},
    {"actorlightnesson", {"<actorID>", "<state>"}},
    {"actorupdateplacement", {"<actorID>"}},
    {"airplane_isdestroyed", {"<airplaneActorID>", "<variableID>"}},
    {"airplane_start", {"<airplaneActorID>", "<navID>", "<speed>"}},
    {"airplane_start2", {"<airplaneActorID>"}},
    {"airplaneshowdamage", {"<airplaneActorID>", "<show>"}},
    {"autosavegame", {"<saveID>"}},
    {"autosavegamefull", {"<saveID>"}},
    {"beep", {}},
    {"bridge_shutdown", {"<bridgeID>", "<state>"}},
    {"call", {"<label>"}},
    {"callsubroutine", {"<scriptName>"}},
    {"camera_getfov", {"<variableID>"}},
    {"camera_lock", {"<frameID>"}},
    {"camera_setfov", {"<fov>"}},
    {"camera_setrange", {"<minDistance>", "<maxDistance>"}},
    {"camera_setswing", {"<unknown>", "<swingAngle>", "<excludedFrameID>"}},
    {"camera_unlock", {}},
    {"car_breakmotor", {"<carActorID>", "<state>"}},
    {"car_calm", {"<carActorID>"}},
    {"car_disable_uo", {"<carActorID>", "<seatID>", "<state>"}},
    {"car_enableus", {"<carActorID>", "<state>"}},
    {"car_explosion", {"<carActorID>"}},
    {"car_forcestop", {"<carActorID>"}},
    {"car_getactlevel", {"<carActorID>", "<variableID>"}},
    {"car_getmaxlevels", {"<carActorID>", "<variableID>"}},
    {"car_getseatcount", {"<carActorID>", "<variableID>"}},
    {"car_getspeccoll", {"<carActorID>", "<variableID>"}},
    {"car_getspeed", {"<carActorID>", "<variableID>"}},
    {"car_invisible", {"<carActorID>", "<state>"}},
    {"car_inwater", {"<carActorID>", "<variableID>"}},
    {"car_lock", {"<carActorID>", "<state>"}},
    {"car_lock_all", {"<carActorID>", "<state>"}},
    {"car_muststeal", {"<carActorID>", "<state>"}},
    {"car_remove_driver", {"<carActorID>"}},
    {"car_repair", {"<carActorID>"}},
    {"car_setactlevel", {"<carActorID>", "<fuelLevel>"}},
    {"car_setaxis", {"<carActorID>"}},
    {"car_setdestroymotor", {"<carActorID>", "<state>"}},
    {"car_setdooropen", {"<carActorID>", "<doorID>", "<openPercent>"}},
    {"car_setprojector", {"<carActorID>", "<param1>", "<param2>", "<param3>"}},
    {"car_setspeccoll", {"<carActorID>", "<state>"}},
    {"car_setspeed", {"<carActorID>", "<speed>"}},
    {"car_switchshowenergy", {"<carActorID>", "<damageType>"}},
    {"car_unbreakable", {"<carActorID>", "<state>"}},
    {"cardamagevisible", {"<state>"}},
    {"carlight_indic_l", {"<carActorID>", "<state>"}},
    {"carlight_indic_off", {"<carActorID>", "<state>"}},
    {"carlight_indic_r", {"<carActorID>", "<state>"}},
    {"carlight_light", {"<carActorID>", "<state>"}},
    {"carlight_main", {"<carActorID>", "<state>"}},
    {"cartridge_invalidate", {"<weaponID>"}},
    {"change_mission", {"<missionFolder>", "<frameName>", "<carSpeed>"}},
    {"character_pop", {"<actorID>"}},
    {"character_push", {"<actorID>"}},
    {"citycaching_tick", {}},
    {"citymusic_off", {}},
    {"citymusic_on", {}},
    {"cleardifferences", {}},
    {"coll_testline", {"<variableID>", "<vector1ID>", "<vector2ID>", "<outputVector1ID>", "<outputVector2ID>"}},
    {"commandblock", {"<state>"}},
    {"compareactors", {"<actor1ID>", "<actor2ID>", "<variableID>"}},
    {"compareframes", {"<frame1ID>", "<frame2ID>", "<variableID>"}},
    {"compareownerwith", {"<carActorID>", "<labelTrue>", "<labelFalse>"}},
    {"compareownerwithex", {"<characterActorID>", "<carActorID>", "<labelTrue>", "<labelFalse>"}},
    {"console_addtext", {"<textID>"}},
    {"create_physicalobject", {"<frameID>", "<impulse>", "<immediateFall>", "<weight>"}},
    {"createweaponfromframe", {"<frameID>", "<weaponID>", "<magazineAmmo>", "<reserveAmmo>"}},
    {"ctrl_read", {"<variableID>", "<keyID>"}},
    {"destroy_physicalobject", {"<objectID>"}},
    {"detector_createdyncoll", {"<actorID>", "<param1>"}},
    {"detector_erasedyncoll", {"<actorID>"}}, // Note: duplicated in mafiasyntax.cpp
    {"detector_inrange", {"<actorID>", "<range>", "<variableID>"}},
    {"detector_issignal", {"<actorID>", "<variableID>"}},
    {"detector_setsignal", {"<actorID>", "<signal>"}},
    {"detector_waitforhit", {"<actorID>", "<variableID>"}},
    {"detector_waitforuse", {"<textID>"}},
    {"dialog_begin", {"<unknown>", "<shoulder>"}},
    {"dialog_camswitch", {"<actorID>", "<targetActorID>"}},
    {"dialog_end", {}},
    {"dim_act", {"<count>"}},
    {"dim_flt", {"<count>"}},
    {"dim_frm", {"<count>"}},
    {"disablecolls", {"<frameID>"}},
    {"door_enableus", {"<doorActorID>", "<state>"}},
    {"door_getstate", {"<doorActorID>", "<variableID>"}},
    {"door_lock", {"<doorActorID>", "<state>"}},
    {"door_open", {"<doorActorID>", "<state>"}},
    {"door_setopenpose", {"<doorActorID>", "<openPercent>"}},
    {"emitparticle", {"<frameID>", "<effectID>", "<variableID>"}},
    {"enablemap", {"<state>"}},
    {"end", {}},
    {"endofmission", {"<state>", "<textID>"}},
    {"enemy_action_backlook", {"<duration>"}},
    {"enemy_action_fire", {"<targetActorID>", "<distance>"}},
    {"enemy_action_follow", {"<targetActorID>", "<distance>", "<param1>", "<param2>"}},
    {"enemy_action_jump", {"<targetFrameID>"}},
    {"enemy_action_phonecall", {"<duration>"}},
    {"enemy_action_runaway", {"<targetActorID>", "<distance>", "<label>"}},
    {"enemy_action_runcloserto", {"<targetActorID>", "<distance>"}},
    {"enemy_action_sidelook", {"<targetActorID>", "<duration>"}},
    {"enemy_action_stepaway", {"<targetActorID>", "<distance>"}},
    {"enemy_action_strafe", {"<targetActorID>", "<distance>"}},
    {"enemy_actionsclear", {"<actorID>"}},
    {"enemy_arrest_player", {"<actorID>"}},
    {"enemy_az_po_panice_hovno", {"<actorID>"}},
    {"enemy_blastfire", {"<actorID>", "<targetFrameID>"}},
    {"enemy_block", {"<actorID>", "<state>"}},
    {"enemy_block_checkpoint", {"<actorID>", "<checkpointID>"}},
    {"enemy_box_goto", {"<boxGroupID>", "<distance>"}},
    {"enemy_box_put", {"<boxGroupID>"}},
    {"enemy_box_take", {"<boxGroupID>"}},
    {"enemy_brainwash", {"<actorID>", "<state>"}},
    {"enemy_car_escape", {"<carActorID>", "<targetActorID>", "<frameID>", "<minSpeed>", "<maxSpeed>", "<speedMultiplier>"}},
    {"enemy_car_hunt", {"<carActorID>", "<targetActorID>", "<huntType>", "<param1>", "<param2>"}},
    {"enemy_car_moveto", {"<carActorID>", "<frameID>", "<driveType>"}},
    {"enemy_changeanim", {"<originalAnim>", "<newAnim>"}},
    {"enemy_enter_door", {"<doorActorID>", "<state>"}},
    {"enemy_exit_railway", {"<actorID>"}},
    {"enemy_followplayer", {"<actorID>", "<distance>"}},
    {"enemy_forcescript", {"<actorID>", "<scriptID>"}},
    {"enemy_gethostilestate", {"<actorID>", "<variableID>"}},
    {"enemy_getstate", {"<actorID>", "<variableID>"}},
    {"enemy_group_add", {"<groupID>", "<actorID>"}},
    {"enemy_group_addcar", {"<groupID>", "<carID>"}},
    {"enemy_group_chcipni_hajzel", {"<groupID>"}},
    {"enemy_group_del", {"<groupID>"}},
    {"enemy_group_new", {"<groupID>"}},
    {"enemy_ignore_corpse", {"<actorID>", "<state>"}},
    {"enemy_kickdriver", {"<actorID>", "<carID>"}},
    {"enemy_lockstate", {"<actorID>", "<state>"}},
    {"enemy_look", {"<actorID>", "<frameID>"}},
    {"enemy_lookto", {"<actorID>", "<targetActorID>"}},
    {"enemy_move", {"<navID>", "<moveType>"}},
    {"enemy_move_to_car", {"<carActorID>", "<seatID>", "<unknown>"}},
    {"enemy_move_to_frame", {"<actorID>", "<frameID>"}},
    {"enemy_naprdelivaute", {"<actorID>"}},
    {"enemy_narohysevyser", {"<actorID>"}},
    {"enemy_naserse", {"<actorID>"}},
    {"enemy_nevyhazuj", {"<actorID>"}},
    {"enemy_playanim", {"<actorID>", "<animID>"}},
    {"enemy_podvadim_jak", {"<actorID>", "<state>"}},
    {"enemy_setstate", {"<actorID>", "<state>"}},
    {"enemy_shutup", {"<actorID>"}},
    {"enemy_stop", {"<actorID>"}},
    {"enemy_stopanim", {"<actorID>"}},
    {"enemy_talk", {"<actorID>", "<speechID>"}},
    {"enemy_turnback", {"<actorID>"}},
    {"enemy_unblock", {"<actorID>"}},
    {"enemy_use_detector", {"<actorID>", "<detectorID>"}},
    {"enemy_use_railway", {"<actorID>", "<railwayID>"}},
    {"enemy_usecar", {"<actorID>", "<carID>"}},
    {"enemy_vidim", {"<actorID>", "<targetActorID>"}},
    {"enemy_wait", {"<actorID>", "<duration>"}},
    {"enemy_watch", {"<actorID>", "<targetActorID>"}},
    {"event", {"<eventName>"}},
    {"event_use_cb", {"<callbackID>"}},
    {"explosion", {"<frameID>", "<type>"}},
    {"findactor", {"<actorID>", "<variableID>"}},
    {"findframe", {"<frameID>", "<variableID>"}},
    {"findnearactor", {"<actorID>", "<range>", "<variableID>"}},
    {"floatreg_pop", {"<variableID>"}},
    {"floatreg_push", {"<value>"}},
    {"frameistelephone", {"<frameID>", "<variableID>"}},
    {"freeride_enablecar", {"<carID>", "<state>"}},
    {"freeride_scoreadd", {"<score>"}},
    {"freeride_scoreget", {"<variableID>"}},
    {"freeride_scoreon", {"<state>"}},
    {"freeride_scoreset", {"<score>"}},
    {"freeride_traffsetup", {"<param1>"}},
    {"frm_getchild", {"<frameID>", "<index>", "<childFrameID>"}},
    {"frm_getdir", {"<frameID>", "<vectorID>"}},
    {"frm_getlocalmatrix", {"<frameID>", "<matrixID>"}},
    {"frm_getnumchildren", {"<frameID>", "<variableID>"}},
    {"frm_getparent", {"<frameID>", "<parentFrameID>"}},
    {"frm_getpos", {"<frameID>", "<vectorID>"}},
    {"frm_getrot", {"<frameID>", "<quaternionID>"}},
    {"frm_getscale", {"<frameID>", "<vectorID>"}},
    {"frm_getworlddir", {"<frameID>", "<vectorID>"}},
    {"frm_getworldmatrix", {"<frameID>", "<matrixID>"}},
    {"frm_getworldpos", {"<frameID>", "<vectorID>"}},
    {"frm_getworldrot", {"<frameID>", "<quaternionID>"}},
    {"frm_getworldscale", {"<frameID>", "<vectorID>"}},
    {"frm_ison", {"<frameID>", "<variableID>"}},
    {"frm_linkto", {"<frameID>", "<parentFrameID>"}},
    {"frm_setalpha", {"<frameID>", "<alpha>"}},
    {"frm_setdir", {"<frameID>", "<vectorID>"}},
    {"frm_seton", {"<frameID>", "<state>"}},
    {"frm_setpos", {"<frameID>", "<vectorID>"}},
    {"frm_setrot", {"<frameID>", "<quaternionID>"}},
    {"frm_setscale", {"<frameID>", "<vectorID>"}},
    {"fuckingbox_add", {"<boxID>", "<frameID>"}},
    {"fuckingbox_add_dest", {"<boxID>", "<destFrameID>"}},
    {"fuckingbox_getnumbox", {"<boxID>", "<variableID>"}},
    {"fuckingbox_getnumdest", {"<boxID>", "<variableID>"}},
    {"fuckingbox_move", {"<boxID>", "<destFrameID>"}},
    {"fuckingbox_recompile", {"<boxID>"}},
    {"game_nightmission", {"<state>"}},
    {"garage_addcar", {"<carID>"}},
    {"garage_addcaridx", {"<carID>", "<index>"}},
    {"garage_addlaststolen", {"<carID>"}},
    {"garage_cansteal", {"<carID>", "<variableID>"}},
    {"garage_carmanager", {"<state>"}},
    {"garage_delcar", {"<carID>"}},
    {"garage_enablesteal", {"<state>"}},
    {"garage_generatecars", {"<count>"}},
    {"garage_nezahazuj", {"<state>"}},
    {"garage_releasecars", {}},
    {"garage_show", {"<state>"}},
    {"gosub", {"<label>"}},
    {"group_disband", {"<groupID>"}},
    {"gunshop_menu", {}},
    {"human_activateweapon", {"<actorID>", "<weaponID>"}},
    {"human_addweapon", {"<actorID>", "<weaponID>"}},
    {"human_anyweaponinhand", {"<actorID>", "<variableID>"}},
    {"human_anyweaponininventory", {"<actorID>", "<variableID>"}},
    {"human_canaddweapon", {"<actorID>", "<weaponID>", "<variableID>"}},
    {"human_candie", {"<actorID>", "<variableID>"}},
    {"human_changeanim", {"<actorID>", "<animID>"}},
    {"human_changemodel", {"<actorID>", "<modelID>"}},
    {"human_createab", {"<actorID>", "<param1>"}},
    {"human_death", {"<actorID>"}},
    {"human_delweapon", {"<actorID>", "<weaponID>"}},
    {"human_entertotruck", {"<actorID>", "<truckID>"}},
    {"human_eraseab", {"<actorID>"}},
    {"human_force_settocar", {"<actorID>", "<carID>", "<seatID>"}},
    {"human_forcefall", {"<actorID>"}},
    {"human_fromcar", {"<actorID>"}},
    {"human_getactanimid", {"<actorID>", "<variableID>"}},
    {"human_getiteminrhand", {"<actorID>", "<variableID>"}},
    {"human_getowner", {"<actorID>", "<ownerID>"}},
    {"human_getproperty", {"<actorID>", "<property>", "<variableID>"}},
    {"human_getseatidx", {"<actorID>", "<variableID>"}},
    {"human_havebody", {"<actorID>", "<variableID>"}},
    {"human_havebox", {"<actorID>", "<variableID>"}},
    {"human_holster", {"<actorID>"}},
    {"human_isweapon", {"<actorID>", "<weaponID>", "<variableID>"}},
    {"human_linktohand", {"<actorID>", "<itemID>"}},
    {"human_looktoactor", {"<actorID>", "<targetActorID>"}},
    {"human_looktoframe", {"<actorID>", "<frameID>"}},
    {"human_reset", {"<actorID>"}},
    {"human_returnfrompanic", {"<actorID>"}},
    {"human_returntotraff", {"<actorID>"}},
    {"human_set8slot", {"<actorID>", "<slotID>"}},
    {"human_setfiretarget", {"<actorID>", "<targetFrameID>"}},
    {"human_setproperty", {"<actorID>", "<property>", "<value>"}},
    {"human_shutup", {"<actorID>"}},
    {"human_stoptalk", {"<actorID>"}},
    {"human_talk", {"<actorID>", "<speechID>"}},
    {"human_throwgrenade", {"<actorID>", "<targetFrameID>"}},
    {"human_throwitem", {"<actorID>", "<itemID>"}},
    {"human_unlinkfromhand", {"<actorID>"}},
    {"human_waittoready", {"<actorID>"}},
    {"if", {"<condition>", "<labelTrue>", "<labelFalse>"}},
    {"iffltinrange", {"<actorID>", "<range>", "<labelTrue>", "<labelFalse>"}},
    {"ifplayerstealcar", {"<carID>", "<labelTrue>", "<labelFalse>"}},
    {"intro_subtitle_add", {"<textID>", "<duration>"}},
    {"inventory_clear", {"<actorID>"}},
    {"inventory_pop", {"<actorID>", "<itemID>"}},
    {"inventory_push", {"<actorID>", "<itemID>"}},
    {"iscarusable", {"<carID>", "<variableID>"}},
    {"ispointinarea", {"<pointID>", "<areaID>", "<variableID>"}},
    {"ispointinsector", {"<pointID>", "<sectorID>", "<variableID>"}},
    {"isvalidtaxipassenger", {"<actorID>", "<variableID>"}},
    {"label", {"<labelName>"}},
    {"let", {"<variableID>", "<value>"}},
    {"loadcolltree", {"<treeID>"}},
    {"loaddifferences", {"<diffID>"}},
    {"m7_zastavzkurvenepolise", {"<state>"}},
    {"math_abs", {"<value>", "<variableID>"}},
    {"math_cos", {"<angle>", "<variableID>"}},
    {"math_sin", {"<angle>", "<variableID>"}},
    {"matrix_copy", {"<destMatrixID>", "<sourceMatrixID>"}},
    {"matrix_identity", {"<matrixID>"}},
    {"matrix_inverse", {"<matrixID>"}},
    {"matrix_mul", {"<matrix1ID>", "<matrix2ID>", "<resultMatrixID>"}},
    {"matrix_zero", {"<matrixID>"}},
    {"mission_objectives", {"<objectiveID>", "<textID>"}},
    {"mission_objectivesclear", {}},
    {"mission_objectivesremove", {"<objectiveID>"}},
    {"model_create", {"<modelID>", "<fileID>"}},
    {"model_destroy", {"<modelID>"}},
    {"model_playanim", {"<modelID>", "<animID>"}},
    {"model_stopanim", {"<modelID>"}},
    {"noanimpreload", {"<state>"}},
    {"npc_shutup", {"<actorID>"}},
    {"person_playanim", {"<actorID>", "<animID>"}},
    {"person_stopanim", {"<actorID>"}},
    {"phobj_impuls", {"<objectID>", "<impulseVectorID>"}},
    {"play_avi_intro", {"<aviID>"}},
    {"player_lockcontrols", {"<state>"}},
    {"playsound", {"<soundID>", "<frameID>"}},
    {"playsoundex", {"<soundID>", "<frameID>", "<volume>"}},
    {"playsoundstop", {"<soundID>"}},
    {"pm_setprogress", {"<progress>"}},
    {"pm_showprogress", {"<state>"}},
    {"pm_showsymbol", {"<symbolID>"}},
    {"pockurvenychbedencar", {"<carID>"}},
    {"police_speed_factor", {"<factor>"}},
    {"police_support", {"<state>"}},
    {"policeitchforplayer", {"<state>"}},
    {"policemanager_add", {"<managerID>"}},
    {"policemanager_del", {"<managerID>"}},
    {"policemanager_forcearrest", {"<state>"}},
    {"policemanager_on", {"<state>"}},
    {"policemanager_setspeed", {"<speed>"}},
    {"preloadmodel", {"<modelID>"}},
    {"program_storage", {"<param1>"}},
    {"pumper_canwork", {"<pumperID>", "<variableID>"}},
    {"quat_add", {"<quat1ID>", "<quat2ID>", "<resultQuatID>"}},
    {"quat_copy", {"<destQuatID>", "<sourceQuatID>"}},
    {"quat_dot", {"<quat1ID>", "<quat2ID>", "<variableID>"}},
    {"quat_extract", {"<matrixID>", "<quatID>"}},
    {"quat_getrotmatrix", {"<quatID>", "<matrixID>"}},
    {"quat_inverse", {"<quatID>"}},
    {"quat_make", {"<axisVectorID>", "<angle>", "<quatID>"}},
    {"quat_mul_quat", {"<quat1ID>", "<quat2ID>", "<resultQuatID>"}},
    {"quat_mul_scl", {"<quatID>", "<scalar>"}},
    {"quat_normalize", {"<quatID>"}},
    {"quat_rotbymatrix", {"<matrixID>", "<quatID>"}},
    {"quat_setdir", {"<vectorID>", "<quatID>"}},
    {"quat_slerp", {"<quat1ID>", "<quat2ID>", "<t>", "<resultQuatID>"}},
    {"racing_autoinvisible", {"<state>"}},
    {"racing_change_model", {"<carID>", "<modelID>"}},
    {"racing_mission6_init", {"<param1>"}},
    {"racing_mission6_start", {}},
    {"recaddactor", {"<actorID>"}},
    {"recclear", {}},
    {"recload", {"<recordID>"}},
    {"recloadfull", {"<recordID>"}},
    {"recunload", {}},
    {"recwaitforend", {"<variableID>"}},
    {"return", {}},
    {"rnd", {"<variableID>", "<maxValue>"}},
    {"set_remote_actor", {"<actorID>", "<remoteActorID>"}},
    {"set_remote_float", {"<floatID>", "<value>"}},
    {"set_remote_frame", {"<frameID>", "<remoteFrameID>"}},
    {"setaipriority", {"<actorID>", "<priority>"}},
    {"setcitytrafficvisible", {"<state>"}},
    {"setcompass", {"<frameID>"}},
    {"setevent", {"<eventID>"}},
    {"setfilmmusic", {"<trackID>"}},
    {"setfreeride", {"<state>"}},
    {"setlmlevel", {"<level>"}},
    {"setmissionnameid", {"<nameID>"}},
    {"setmissionnumber", {"<number>"}},
    {"setmodeltocar", {"<carID>", "<modelID>"}},
    {"setnoanimhit", {"<state>"}},
    {"setnpckillevent", {"<eventID>"}},
    {"setnullactor", {"<actorID>"}},
    {"setnullframe", {"<frameID>"}},
    {"setplayerfireevent", {"<eventID>"}},
    {"setplayerhornevent", {"<eventID>"}},
    {"setplayerfallevent", {"<eventID>"}},
    {"settankhitcount", {"<count>"}},
    {"settimeoutevent", {"<eventID>", "<timeout>"}},
    {"settraffsectorsnd", {"<sectorID>", "<soundID>"}},
    {"showcardammage", {"<state>"}},
    {"sound_getvolume", {"<soundID>", "<variableID>"}},
    {"sound_setvolume", {"<soundID>", "<volume>"}},
    {"soundfade", {"<soundID>", "<duration>"}},
    {"stopparticle", {"<particleID>"}},
    {"stopsound", {"<soundID>"}},
    {"stream_connect", {"<streamID>", "<sourceID>"}},
    {"stream_create", {"<streamID>"}},
    {"stream_destroy", {"<streamID>"}},
    {"stream_fadevol", {"<streamID>", "<volume>", "<duration>"}},
    {"stream_getpos", {"<streamID>", "<variableID>"}},
    {"stream_pause", {"<streamID>"}},
    {"stream_play", {"<streamID>"}},
    {"stream_setloop", {"<streamID>", "<state>"}},
    {"stream_setpos", {"<streamID>", "<position>"}},
    {"stream_stop", {"<streamID>"}},
    {"subtitle", {"<textID>", "<duration>"}},
    {"taxidriver_enable", {"<state>"}},
    {"timer_getinterval", {"<timerID>", "<variableID>"}},
    {"timer_setinterval", {"<timerID>", "<interval>"}},
    {"timeroff", {"<timerID>"}},
    {"timeron", {"<timerID>", "<hour>", "<minute>", "<second>"}},
    {"use_lightcache", {"<state>"}},
    {"vect_add_vect", {"<vector1ID>", "<vector2ID>"}},
    {"vect_angleto", {"<vector1ID>", "<vector2ID>", "<variableID>"}},
    {"vect_copy", {"<destVectorID>", "<sourceVectorID>"}},
    {"vect_inverse", {"<vectorID>"}},
    {"vect_magnitude", {"<vectorID>", "<variableID>"}},
    {"vect_mul_matrix", {"<vectorID>", "<matrixID>"}},
    {"vect_mul_quat", {"<vectorID>", "<quaternionID>"}},
    {"vect_mul_scl", {"<vectorID>", "<scalar>"}},
    {"vect_mul_vect", {"<vector1ID>", "<vector2ID>", "<resultVectorID>"}},
    {"vect_normalize", {"<vectorID>"}},
    {"vect_set", {"<vectorID>", "<x>", "<y>", "<z>"}},
    {"vect_sub_vect", {"<vector1ID>", "<vector2ID>"}},
    {"version_is_editor", {"<variableID>"}},
    {"version_is_germany", {"<variableID>"}},
    {"vlvp", {"<variableID>"}},
    {"wagon_getlastnode", {"<wagonActorID>", "<variableID>"}},
    {"wagon_setevent", {"<wagonActorID>", "<event>"}},
    {"wait", {"<duration>"}},
    {"weather_preparebuffer", {}},
    {"weather_reset", {}},
    {"weather_setparam", {"<param>", "<value>"}},
    {"wingman_delindicator", {"<actorID>", "<positionID>"}},
    {"wingman_setindicator", {"<actorID>", "<imageID>", "<positionID>"}},
    {"zatmyse", {"<state>", "<immediate>"}},
    // Commands not in MSDR.CZ.pdf (generic placeholders)
    {"get_pm_crashtime", {"<variableID>"}},
    {"get_pm_firetime", {"<variableID>"}},
    {"get_pm_humanstate", {"<actorID>", "<variableID>"}},
    {"get_pm_state", {"<variableID>"}},
    {"get_remote_actor", {"<remoteActorID>", "<variableID>"}},
    {"get_remote_float", {"<floatID>", "<variableID>"}},
    {"getactivecamera", {"<variableID>"}},
    {"getactiveplayer", {"<variableID>"}},
    {"getactorframe", {"<actorID>", "<variableID>"}},
    {"getactorsdist", {"<actor1ID>", "<actor2ID>", "<variableID>"}},
    {"getangleactortoactor", {"<actor1ID>", "<actor2ID>", "<variableID>"}},
    {"getcardamage", {"<carID>", "<variableID>"}},
    {"getcarlinenumfromtable", {"<tableID>", "<variableID>"}},
    {"getenemyaistate", {"<actorID>", "<variableID>"}},
    {"getfilmmusic", {"<variableID>"}},
    {"getframefromactor", {"<actorID>", "<variableID>"}},
    {"getgametime", {"<variableID>"}},
    {"getlastsavenum", {"<variableID>"}},
    {"getmissionnumber", {"<variableID>"}},
    {"getsoundtime", {"<soundID>", "<variableID>"}},
    {"getticktime", {"<variableID>"}},
    {"entity_debug_graphs", {"<state>"}},
    {"get_pm_numpredators", {"<variableID>"}},
};

namespace MafiaScript {
    enum class ArgType { Number, String, Operator, Identifier };

    struct CommandRule {
        std::vector<ArgType> args;
        bool opensBlock = false;
        bool isEvent = false;
    };

    static const std::map<std::string, CommandRule> commandRules = {
        {"act_setstate", {{ArgType::Number, ArgType::Number}}},
        {"actor_adddorici", {{ArgType::Number}}},
        {"actor_delete", {{ArgType::Number}}},
        {"actor_duplicate", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"actor_setdir", {{ArgType::Number, ArgType::Number}}},
        {"actor_setplacement", {{ArgType::Number, ArgType::Number}}},
        {"actor_setpos", {{ArgType::Number, ArgType::Number}}},
        {"actorlightnesson", {}},
        {"actorupdateplacement", {}},
        {"airplane_isdestroyed", {{ArgType::Number, ArgType::Number}}},
        {"airplane_start", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"airplane_start2", {{ArgType::Number}}},
        {"airplaneshowdamage", {{ArgType::Number, ArgType::Number}}},
        {"autosavegame", {{ArgType::Number}}},
        {"autosavegamefull", {{ArgType::Number}}},
        {"beep", {}},
        {"bridge_shutdown", {{ArgType::Number, ArgType::Number}}},
        {"call", {{ArgType::Identifier}}},
        {"callsubroutine", {{ArgType::String}}},
        {"camera_getfov", {{ArgType::Number}}},
        {"camera_lock", {{ArgType::Number}}},
        {"camera_setfov", {{ArgType::Number}}},
        {"camera_setrange", {{ArgType::Number, ArgType::Number}}},
        {"camera_setswing", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"camera_unlock", {}},
        {"car_breakmotor", {{ArgType::Number, ArgType::Number}}},
        {"car_calm", {{ArgType::Number}}},
        {"car_disable_uo", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"car_enableus", {{ArgType::Number, ArgType::Number}}},
        {"car_explosion", {{ArgType::Number}}},
        {"car_forcestop", {{ArgType::Number}}},
        {"car_getactlevel", {{ArgType::Number, ArgType::Number}}},
        {"car_getmaxlevels", {{ArgType::Number, ArgType::Number}}},
        {"car_getseatcount", {{ArgType::Number, ArgType::Number}}},
        {"car_getspeccoll", {{ArgType::Number, ArgType::Number}}},
        {"car_getspeed", {{ArgType::Number, ArgType::Number}}},
        {"car_invisible", {{ArgType::Number, ArgType::Number}}},
        {"car_inwater", {{ArgType::Number, ArgType::Number}}},
        {"car_lock", {{ArgType::Number, ArgType::Number}}},
        {"car_lock_all", {{ArgType::Number, ArgType::Number}}},
        {"car_muststeal", {{ArgType::Number, ArgType::Number}}},
        {"car_remove_driver", {{ArgType::Number}}},
        {"car_repair", {{ArgType::Number}}},
        {"car_setactlevel", {{ArgType::Number, ArgType::Number}}},
        {"car_setaxis", {{ArgType::Number}}},
        {"car_setdestroymotor", {{ArgType::Number, ArgType::Number}}},
        {"car_setdooropen", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"car_setprojector", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"car_setspeccoll", {{ArgType::Number, ArgType::Number}}},
        {"car_setspeed", {{ArgType::Number, ArgType::Number}}},
        {"car_switchshowenergy", {{ArgType::Number, ArgType::Number}}},
        {"car_unbreakable", {{ArgType::Number, ArgType::Number}}},
        {"cardamagevisible", {{ArgType::Number}}},
        {"carlight_indic_l", {{ArgType::Number, ArgType::Number}}},
        {"carlight_indic_off", {{ArgType::Number, ArgType::Number}}},
        {"carlight_indic_r", {{ArgType::Number, ArgType::Number}}},
        {"carlight_light", {{ArgType::Number, ArgType::Number}}},
        {"carlight_main", {{ArgType::Number, ArgType::Number}}},
        {"cartridge_invalidate", {{ArgType::Number}}},
        {"change_mission", {{ArgType::String, ArgType::String, ArgType::Number}}},
        {"character_pop", {{ArgType::Number}}},
        {"character_push", {{ArgType::Number}}},
        {"citycaching_tick", {}},
        {"citymusic_off", {}},
        {"citymusic_on", {}},
        {"cleardifferences", {}},
        {"coll_testline", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"commandblock", {{ArgType::Number}}},
        {"compareactors", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"compareframes", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"compareownerwith", {{ArgType::Number, ArgType::Identifier, ArgType::Identifier}}},
        {"compareownerwithex", {{ArgType::Number, ArgType::Number, ArgType::Identifier, ArgType::Identifier}}},
        {"console_print", {{ArgType::String}}},
        {"create_physicalobject", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"createweaponfromframe", {{ArgType::String, ArgType::Number, ArgType::Number}}},
        {"ctrl_read", {{ArgType::Number, ArgType::Number}}},
        {"dan_internal_1", {}},
        {"dan_internal_2", {}},
        {"destroy_physicalobject", {{ArgType::Number}}},
        {"detector_createdyncoll", {{ArgType::Number, ArgType::Number}}},
        {"detector_destroyedyncoll", {{ArgType::Number}}},
        {"detector_inrange", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"detector_issignal", {{ArgType::Number, ArgType::Number}}},
        {"detector_setsignal", {{ArgType::Number, ArgType::Number}}},
        {"detector_waitforhit", {{ArgType::Number, ArgType::Number}}},
        {"detector_waitforuse", {{ArgType::String}}},
        {"dialog_begin", {{ArgType::Number, ArgType::Number}}},
        {"dialog_camswitch", {{ArgType::Number, ArgType::Number}}},
        {"dialog_end", {}},
        {"dim_act", {{ArgType::Number}}},
        {"dim_flt", {{ArgType::Number}}},
        {"dim_frm", {{ArgType::Number}}},
        {"disablecolls", {{ArgType::Number}}},
        {"door_enableus", {{ArgType::Number, ArgType::Number}}},
        {"door_getstate", {{ArgType::Number, ArgType::Number}}},
        {"door_lock", {{ArgType::Number, ArgType::Number}}},
        {"door_open", {{ArgType::Number, ArgType::Number}}},
        {"door_setopenpose", {{ArgType::Number, ArgType::Number}}},
        {"down", {}},
        {"emitparticle", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"enablemap", {{ArgType::Number}}},
        {"end", {}},
        {"endofmission", {{ArgType::Number, ArgType::Number}}},
        {"enemy_action_backlook", {{ArgType::Number}}},
        {"enemy_action_fire", {{ArgType::Number, ArgType::Number}}},
        {"enemy_action_follow", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"enemy_action_jump", {{ArgType::Number}}},
        {"enemy_action_phonecall", {{ArgType::Number}}},
        {"enemy_action_runaway", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"enemy_action_runcloserto", {{ArgType::Number, ArgType::Number}}},
        {"enemy_action_sidelook", {{ArgType::Number, ArgType::Number}}},
        {"enemy_action_stepaway", {{ArgType::Number, ArgType::Number}}},
        {"enemy_action_strafe", {{ArgType::Number, ArgType::Number}}},
        {"enemy_actionsclear", {{ArgType::Number}}},
        {"enemy_arrest_player", {{ArgType::Number}}},
        {"enemy_az_po_panice_hovno", {{ArgType::Number}}},
        {"enemy_blastfire", {{ArgType::Number, ArgType::Number}}},
        {"enemy_block", {{ArgType::Number, ArgType::Number}}},
        {"enemy_block_checkpoint", {{ArgType::Number, ArgType::Number}}},
        {"enemy_box_goto", {{ArgType::Number, ArgType::Number}}},
        {"enemy_box_put", {{ArgType::Number}}},
        {"enemy_box_take", {{ArgType::Number}}},
        {"enemy_brainwash", {{ArgType::Number, ArgType::Number}}},
        {"enemy_car_escape", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"enemy_car_hunt", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"enemy_car_moveto", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"enemy_changeanim", {{ArgType::String, ArgType::String}}},
        {"enemy_enter_door", {{ArgType::Number, ArgType::Number}}},
        {"enemy_exit_railway", {{ArgType::Number}}},
        {"enemy_followplayer", {{ArgType::Number, ArgType::Number}}},
        {"enemy_forcescript", {{ArgType::Number, ArgType::Number}}},
        {"enemy_gethostilestate", {{ArgType::Number, ArgType::Number}}},
        {"enemy_getstate", {{ArgType::Number, ArgType::Number}}},
        {"enemy_group_add", {{ArgType::Number, ArgType::Number}}},
        {"enemy_group_addcar", {{ArgType::Number, ArgType::Number}}},
        {"enemy_group_chcipni_hajzel", {{ArgType::Number}}},
        {"enemy_group_del", {{ArgType::Number}}},
        {"enemy_group_new", {{ArgType::Number}}},
        {"enemy_ignore_corpse", {{ArgType::Number, ArgType::Number}}},
        {"enemy_kickdriver", {{ArgType::Number, ArgType::Number}}},
        {"enemy_lockstate", {{ArgType::Number, ArgType::Number}}},
        {"enemy_look", {{ArgType::Number, ArgType::Number}}},
        {"enemy_lookto", {{ArgType::Number, ArgType::Number}}},
        {"enemy_move", {{ArgType::Number, ArgType::Number}}},
        {"enemy_move_to_car", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"enemy_move_to_frame", {{ArgType::Number, ArgType::Number}}},
        {"enemy_naprdelivaute", {{ArgType::Number}}},
        {"enemy_narohysevyser", {{ArgType::Number}}},
        {"enemy_naserse", {{ArgType::Number}}},
        {"enemy_nevyhazuj", {{ArgType::Number}}},
        {"enemy_playanim", {{ArgType::Number, ArgType::Number}}},
        {"enemy_podvadim_jak", {{ArgType::Number, ArgType::Number}}},
        {"enemy_setstate", {{ArgType::Number, ArgType::Number}}},
        {"enemy_shutup", {{ArgType::Number}}},
        {"enemy_stop", {{ArgType::Number}}},
        {"enemy_stopanim", {{ArgType::Number}}},
        {"enemy_talk", {{ArgType::Number, ArgType::Number}}},
        {"enemy_turnback", {{ArgType::Number}}},
        {"enemy_unblock", {{ArgType::Number}}},
        {"enemy_use_detector", {{ArgType::Number, ArgType::Number}}},
        {"enemy_use_railway", {{ArgType::Number, ArgType::Number}}},
        {"enemy_usecar", {{ArgType::Number, ArgType::Number}}},
        {"enemy_vidim", {{ArgType::Number, ArgType::Number}}},
        {"enemy_wait", {{ArgType::Number, ArgType::Number}}},
        {"enemy_watch", {{ArgType::Number, ArgType::Number}}},
        {"entity_debug_graphs", {{ArgType::Number}}},
        {"event", {{ArgType::Identifier}, false, true}},
        {"event_use_cb", {{ArgType::Number}}},
        {"explosion", {{ArgType::Number, ArgType::Number}}},
        {"findactor", {{ArgType::Number, ArgType::String}}},
        {"findframe", {{ArgType::Number, ArgType::String}}},
        {"findnearactor", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"floatreg_pop", {{ArgType::Number}}},
        {"floatreg_push", {{ArgType::Number}}},
        {"frameistelephone", {{ArgType::Number, ArgType::Number}}},
        {"freeride_enablecar", {{ArgType::Number, ArgType::Number}}},
        {"freeride_scoreadd", {{ArgType::Number}}},
        {"freeride_scoreget", {{ArgType::Number}}},
        {"freeride_scoreon", {{ArgType::Number}}},
        {"freeride_scoreset", {{ArgType::Number}}},
        {"freeride_traffsetup", {{ArgType::Number}}},
        {"frm_getchild", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"frm_getdir", {{ArgType::Number, ArgType::Number}}},
        {"frm_getlocalmatrix", {{ArgType::Number, ArgType::Number}}},
        {"frm_getnumchildren", {{ArgType::Number, ArgType::Number}}},
        {"frm_getparent", {{ArgType::Number, ArgType::Number}}},
        {"frm_getpos", {{ArgType::Number, ArgType::Number}}},
        {"frm_getrot", {{ArgType::Number, ArgType::Number}}},
        {"frm_getscale", {{ArgType::Number, ArgType::Number}}},
        {"frm_getworlddir", {{ArgType::Number, ArgType::Number}}},
        {"frm_getworldmatrix", {{ArgType::Number, ArgType::Number}}},
        {"frm_getworldpos", {{ArgType::Number, ArgType::Number}}},
        {"frm_getworldrot", {{ArgType::Number, ArgType::Number}}},
        {"frm_getworldscale", {{ArgType::Number, ArgType::Number}}},
        {"frm_ison", {{ArgType::Number, ArgType::Number}}},
        {"frm_linkto", {{ArgType::Number, ArgType::Number}}},
        {"frm_setalpha", {{ArgType::Number, ArgType::Number}}},
        {"frm_setdir", {{ArgType::Number, ArgType::Number}}},
        {"frm_seton", {{ArgType::Number, ArgType::Number}, true}},
        {"frm_setpos", {{ArgType::Number, ArgType::Number}}},
        {"frm_setrot", {{ArgType::Number, ArgType::Number}}},
        {"frm_setscale", {{ArgType::Number, ArgType::Number}}},
        {"fuckingbox_add", {{ArgType::Number, ArgType::Number}}},
        {"fuckingbox_add_dest", {{ArgType::Number, ArgType::Number}}},
        {"fuckingbox_getnumbox", {{ArgType::Number, ArgType::Number}}},
        {"fuckingbox_getnumdest", {{ArgType::Number, ArgType::Number}}},
        {"fuckingbox_move", {{ArgType::Number, ArgType::Number}}},
        {"fuckingbox_recompile", {{ArgType::Number}}},
        {"game_nightmission", {{ArgType::Number}}},
        {"garage_addcar", {{ArgType::Number}}},
        {"garage_addcaridx", {{ArgType::Number, ArgType::Number}}},
        {"garage_addlaststolen", {{ArgType::Number}}},
        {"garage_cansteal", {{ArgType::Number, ArgType::Number}}},
        {"garage_carmanager", {{ArgType::Number}}},
        {"garage_delcar", {{ArgType::Number}}},
        {"garage_enablesteal", {{ArgType::Number}}},
        {"garage_generatecars", {{ArgType::Number}}},
        {"garage_nezahazuj", {{ArgType::Number}}},
        {"garage_releasecars", {}},
        {"garage_show", {{ArgType::Number}}},
        {"get_pm_crashtime", {{ArgType::Number}}},
        {"get_pm_firetime", {{ArgType::Number}}},
        {"get_pm_humanstate", {{ArgType::Number, ArgType::Number}}},
        {"get_pm_numpredators", {{ArgType::Number}}},
        {"get_pm_state", {{ArgType::Number}}},
        {"get_remote_actor", {{ArgType::Number, ArgType::Number}}},
        {"get_remote_float", {{ArgType::Number, ArgType::Number}}},
        {"getactivecamera", {{ArgType::Number}}},
        {"getactiveplayer", {{ArgType::Number}}},
        {"getactorframe", {{ArgType::Number, ArgType::Number}}},
        {"getactorsdist", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"getangleactortoactor", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"getcardamage", {{ArgType::Number, ArgType::Number}}},
        {"getcarlinenumfromtable", {{ArgType::Number, ArgType::Number}}},
        {"getenemyaistate", {{ArgType::Number, ArgType::Number}}},
        {"getfilmmusic", {{ArgType::Number}}},
        {"getframefromactor", {{ArgType::Number, ArgType::Number}}},
        {"getgametime", {{ArgType::Number}}},
        {"getlastsavenum", {{ArgType::Number}}},
        {"getmissionnumber", {{ArgType::Number}}},
        {"getsoundtime", {{ArgType::Number, ArgType::Number}}},
        {"getticktime", {{ArgType::Number}}},
        {"gosub", {{ArgType::Identifier}}},
        {"goto", {{ArgType::Identifier}}},
        {"group_disband", {{ArgType::Number}}},
        {"gunshop_menu", {}},
        {"human_activateweapon", {{ArgType::Number, ArgType::Number}}},
        {"human_addweapon", {{ArgType::Number, ArgType::Number}}},
        {"human_anyweaponinhand", {{ArgType::Number, ArgType::Number}}},
        {"human_anyweaponininventory", {{ArgType::Number, ArgType::Number}}},
        {"human_canaddweapon", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"human_candie", {{ArgType::Number, ArgType::Number}}},
        {"human_changeanim", {{ArgType::Number, ArgType::Number}}},
        {"human_changemodel", {{ArgType::Number, ArgType::Number}}},
        {"human_createab", {{ArgType::Number, ArgType::Number}}},
        {"human_death", {{ArgType::Number}}},
        {"human_delweapon", {{ArgType::Number, ArgType::Number}}},
        {"human_entertotruck", {{ArgType::Number, ArgType::Number}}},
        {"human_eraseab", {{ArgType::Number}}},
        {"human_force_settocar", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"human_forcefall", {{ArgType::Number}}},
        {"human_fromcar", {{ArgType::Number}}},
        {"human_getactanimid", {{ArgType::Number, ArgType::Number}}},
        {"human_getiteminrhand", {{ArgType::Number, ArgType::Number}}},
        {"human_getowner", {{ArgType::Number, ArgType::Number}}},
        {"human_getproperty", {{ArgType::Number, ArgType::String, ArgType::Number}}},
        {"human_getseatidx", {{ArgType::Number, ArgType::Number}}},
        {"human_havebody", {{ArgType::Number, ArgType::Number}}},
        {"human_havebox", {{ArgType::Number, ArgType::Number}}},
        {"human_holster", {{ArgType::Number}}},
        {"human_isweapon", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"human_linktohand", {{ArgType::Number, ArgType::Number}}},
        {"human_looktoactor", {{ArgType::Number, ArgType::Number}}},
        {"human_looktoframe", {{ArgType::Number, ArgType::Number}}},
        {"human_reset", {{ArgType::Number}}},
        {"human_returnfrompanic", {{ArgType::Number}}},
        {"human_returntotraff", {{ArgType::Number}}},
        {"human_set8slot", {{ArgType::Number, ArgType::Number}}},
        {"human_setfiretarget", {{ArgType::Number, ArgType::Number}}},
        {"human_setproperty", {{ArgType::Number, ArgType::String, ArgType::Number}}},
        {"human_shutup", {{ArgType::Number}}},
        {"human_stoptalk", {{ArgType::Number}}},
        {"human_talk", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"human_throwgrenade", {{ArgType::Number, ArgType::Number}}},
        {"human_throwitem", {{ArgType::Number, ArgType::Number}}},
        {"human_unlinkfromhand", {{ArgType::Number}}},
        {"human_waittoready", {{ArgType::Number}}},
        {"if", {{ArgType::Number, ArgType::Identifier, ArgType::Identifier}}},
        {"iffltinrange", {{ArgType::Number, ArgType::Number, ArgType::Identifier}}},
        {"ifplayerstealcar", {{ArgType::Number, ArgType::Identifier}}},
        {"intro_subtitle_add", {{ArgType::String, ArgType::Number}}},
        {"inventory_clear", {{ArgType::Number}}},
        {"inventory_pop", {{ArgType::Number, ArgType::Number}}},
        {"inventory_push", {{ArgType::Number, ArgType::Number}}},
        {"iscarusable", {{ArgType::Number, ArgType::Number}}},
        {"ispointinarea", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"ispointinsector", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"isvalidtaxipassenger", {{ArgType::Number, ArgType::Number}}},
        {"label", {{ArgType::Identifier}}},
        {"let", {{ArgType::Number, ArgType::Number}}},
        {"loadcolltree", {{ArgType::Number}}},
        {"loaddifferences", {{ArgType::Number}}},
        {"m7_zastavzkurvenepolise", {{ArgType::Number}}},
        {"math_abs", {{ArgType::Number, ArgType::Number}}},
        {"math_cos", {{ArgType::Number, ArgType::Number}}},
        {"math_sin", {{ArgType::Number, ArgType::Number}}},
        {"matrix_copy", {{ArgType::Number, ArgType::Number}}},
        {"matrix_identity", {{ArgType::Number}}},
        {"matrix_inverse", {{ArgType::Number}}},
        {"matrix_mul", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"matrix_zero", {{ArgType::Number}}},
        {"mission_objectives", {{ArgType::Number, ArgType::String}}},
        {"mission_objectivesclear", {}},
        {"mission_objectivesremove", {{ArgType::Number}}},
        {"model_create", {{ArgType::Number, ArgType::String}}},
        {"model_destroy", {{ArgType::Number}}},
        {"model_playanim", {{ArgType::Number, ArgType::Number}}},
        {"model_stopanim", {{ArgType::Number}}},
        {"noanimpreload", {{ArgType::Number}}},
        {"npc_shutup", {{ArgType::Number}}},
        {"person_playanim", {{ArgType::Number, ArgType::Number}}},
        {"person_stopanim", {{ArgType::Number}}},
        {"phobj_impuls", {{ArgType::Number, ArgType::Number}}},
        {"play_avi_intro", {{ArgType::String}}},
        {"player_lockcontrols", {{ArgType::Number}}},
        {"playsound", {{ArgType::Number, ArgType::Number}}},
        {"playsoundex", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"playsoundstop", {{ArgType::Number}}},
        {"pm_setprogress", {{ArgType::Number}}},
        {"pm_showprogress", {{ArgType::Number}}},
        {"pm_showsymbol", {{ArgType::Number}}},
        {"pockurvenychbedencar", {{ArgType::Number}}},
        {"police_speed_factor", {{ArgType::Number}}},
        {"police_support", {{ArgType::Number}}},
        {"policeitchforplayer", {{ArgType::Number}}},
        {"policemanager_add", {{ArgType::Number}}},
        {"policemanager_del", {{ArgType::Number}}},
        {"policemanager_forcearrest", {{ArgType::Number}}},
        {"policemanager_on", {{ArgType::Number}}},
        {"policemanager_setspeed", {{ArgType::Number}}},
        {"preloadmodel", {{ArgType::Number}}},
        {"program_storage", {{ArgType::Number}}},
        {"pumper_canwork", {{ArgType::Number, ArgType::Number}}},
        {"quat_add", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"quat_copy", {{ArgType::Number, ArgType::Number}}},
        {"quat_dot", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"quat_extract", {{ArgType::Number, ArgType::Number}}},
        {"quat_getrotmatrix", {{ArgType::Number, ArgType::Number}}},
        {"quat_inverse", {{ArgType::Number}}},
        {"quat_make", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"quat_mul_quat", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"quat_mul_scl", {{ArgType::Number, ArgType::Number}}},
        {"quat_normalize", {{ArgType::Number}}},
        {"quat_rotbymatrix", {{ArgType::Number, ArgType::Number}}},
        {"quat_setdir", {{ArgType::Number, ArgType::Number}}},
        {"quat_slerp", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"racing_autoinvisible", {{ArgType::Number}}},
        {"racing_change_model", {{ArgType::Number, ArgType::Number}}},
        {"racing_mission6_init", {{ArgType::Number}}},
        {"racing_mission6_start", {}},
        {"recaddactor", {{ArgType::Number}}},
        {"recclear", {}},
        {"recload", {{ArgType::String}}},
        {"recloadfull", {{ArgType::String}}},
        {"recunload", {}},
        {"recwaitforend", {}},
        {"return", {}},
        {"rnd", {{ArgType::Number, ArgType::Number}}},
        {"set_remote_actor", {{ArgType::Number, ArgType::Number}}},
        {"set_remote_float", {{ArgType::Number, ArgType::Number}}},
        {"set_remote_frame", {{ArgType::Number, ArgType::Number}}},
        {"setaipriority", {{ArgType::Number, ArgType::Number}}},
        {"setcitytrafficvisible", {{ArgType::Number}}},
        {"setcompass", {{ArgType::Number}}},
        {"setevent", {{ArgType::Number}}},
        {"setfilmmusic", {{ArgType::Number}}},
        {"setfreeride", {{ArgType::Number}}},
        {"setlmlevel", {{ArgType::Number}}},
        {"setmissionnameid", {}},
        {"setmissionnumber", {{ArgType::Number}}},
        {"setmodeltocar", {{ArgType::Number}}},
        {"setnoanimhit", {{ArgType::Number, ArgType::Number}}},
        {"setnpckillevent", {{ArgType::Number, ArgType::Number, ArgType::String}}},
        {"setnullactor", {{ArgType::Number}}},
        {"setnullframe", {{ArgType::Number}}},
        {"setplayerfireevent", {{ArgType::Number, ArgType::Number}}},
        {"setplayerhornevent", {{ArgType::Number, ArgType::Number}}},
        {"setplayerfallevent", {{ArgType::Number, ArgType::Number}}},
        {"settankhitcount", {{ArgType::Number, ArgType::Number}}},
        {"settimeoutevent", {{ArgType::Number, ArgType::Number}}},
        {"settraffsectorsnd", {{ArgType::String}}},
        {"showcardammage", {{ArgType::Number, ArgType::Number}}},
        {"sound_getvolume", {{ArgType::Number, ArgType::Number}}},
        {"sound_setvolume", {{ArgType::Number, ArgType::Number}}},
        {"soundfade", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"stopparticle", {{ArgType::Number}}},
        {"stopsound", {{ArgType::Number}}},
        {"stream_connect", {}},
        {"stream_create", {{ArgType::Number, ArgType::String}}},
        {"stream_destroy", {{ArgType::Number}}},
        {"stream_fadevol", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"stream_getpos", {{ArgType::Number, ArgType::Number}}},
        {"stream_pause", {{ArgType::Number}}},
        {"stream_play", {{ArgType::Number}}},
        {"stream_setloop", {{ArgType::Number, ArgType::Number}}},
        {"stream_setpos", {{ArgType::Number, ArgType::Number}}},
        {"stream_stop", {{ArgType::Number}}},
        {"subtitle_add", {{ArgType::Number}}},
        {"taxidriver_enable", {{ArgType::Number}}},
        {"timer_getinterval", {{ArgType::Number, ArgType::Number}}},
        {"timer_setinterval", {{ArgType::Number, ArgType::Number}}},
        {"timeroff", {{ArgType::Number}}},
        {"timeron", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"up", {}},
        {"use_lightcache", {{ArgType::Number}}},
        {"vect_add_vect", {{ArgType::Number, ArgType::Number}}},
        {"vect_angleto", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"vect_copy", {{ArgType::Number, ArgType::Number}}},
        {"vect_inverse", {{ArgType::Number}}},
        {"vect_magnitude", {{ArgType::Number, ArgType::Number}}},
        {"vect_mul_matrix", {{ArgType::Number, ArgType::Number}}},
        {"vect_mul_quat", {{ArgType::Number, ArgType::Number}}},
        {"vect_mul_scl", {{ArgType::Number, ArgType::Number}}},
        {"vect_mul_vect", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"vect_normalize", {{ArgType::Number}}},
        {"vect_set", {{ArgType::Number, ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"vect_sub_vect", {{ArgType::Number, ArgType::Number}}},
        {"version_is_editor", {{ArgType::Number}}},
        {"version_is_germany", {{ArgType::Number}}},
        {"vlvp", {{ArgType::Number}}},
        {"wagon_getlastnode", {{ArgType::Number, ArgType::Number}}},
        {"wagon_setevent", {{ArgType::Number, ArgType::Number}}},
        {"wait", {{ArgType::Number}}},
        {"weather_preparebuffer", {}},
        {"weather_reset", {}},
        {"weather_setparam", {{ArgType::Number, ArgType::Number}}},
        {"wingman_delindicator", {{ArgType::Number, ArgType::Number}}},
        {"wingman_setindicator", {{ArgType::Number, ArgType::Number, ArgType::Number}}},
        {"zatmyse", {{ArgType::Number, ArgType::Number}}}};

    // AST Node Types
    enum class NodeType {
        Program, // Root node containing all statements
        Command, // General command (e.g., "beep", "setcitytrafficvisible")
        LetAssignment, // "let" statement for variable assignment
        IfStatement, // "if" statement for conditional branching
        Block, // "{}" block (e.g., "up", "down")
        Label, // "label" definition
        Event, // "event" definition
        Goto, // "goto" statement
        Gosub, // "gosub" statement
        Call, // "call" statement
        Return, // "return" statement
    };

    // Argument Type for AST nodes
    struct Argument {
        enum class Type { Number, String, Identifier, Operator, FloatArray };
        Type type;
        std::string value;
        int line;
    };

    // Base AST Node
    struct Node {
        NodeType type;
        int line;
        virtual ~Node() = default;
        explicit Node(NodeType t, int l) : type(t), line(l) {}
    };

    // Program Node (root, contains all statements)
    struct ProgramNode : Node {
        std::vector<std::unique_ptr<Node>> statements;
        ProgramNode(int l) : Node(NodeType::Program, l) {}
    };

    // Command Node (generic commands)
    struct CommandNode : Node {
        std::string command;
        std::vector<Argument> arguments;
        CommandNode(int l, std::string cmd) : Node(NodeType::Command, l), command(std::move(cmd)) {}
    };

    // Let Assignment Node (e.g., "let flt[0] = flt[1] + 1")
    struct LetAssignmentNode : Node {
        std::string target; // e.g., "flt[0]"
        std::vector<Argument> expression; // e.g., ["flt[1]", "+", "1"]
        LetAssignmentNode(int l, std::string tgt) : Node(NodeType::LetAssignment, l), target(std::move(tgt)) {}
    };

    // If Statement Node (e.g., "if flt[0] = 1, label1, -1")
    struct IfStatementNode : Node {
        Argument conditionLeft; // e.g., "flt[0]"
        std::string conditionOp; // e.g., "="
        Argument conditionRight; // e.g., "1"
        std::string trueLabel;
        std::string falseLabel;
        IfStatementNode(int l) : Node(NodeType::IfStatement, l) {}
    };

    // Block Node (e.g., "{ up ... }")
    struct BlockNode : Node {
        std::string blockName; // e.g., "up", "down"
        std::vector<std::unique_ptr<Node>> statements;
        BlockNode(int l, std::string name) : Node(NodeType::Block, l), blockName(std::move(name)) {}
    };

    // Label Node (e.g., "label DETECT")
    struct LabelNode : Node {
        std::string labelName;
        LabelNode(int l, std::string name) : Node(NodeType::Label, l), labelName(std::move(name)) {}
    };

    // Event Node (e.g., "event 1 ... end")
    struct EventNode : Node {
        std::string eventName;
        std::vector<std::unique_ptr<Node>> statements;
        EventNode(int l, std::string name) : Node(NodeType::Event, l), eventName(std::move(name)) {}
    };

    // Control Flow Nodes
    struct GotoNode : Node {
        std::string label;
        GotoNode(int l, std::string lbl) : Node(NodeType::Goto, l), label(std::move(lbl)) {}
    };

    struct GosubNode : Node {
        std::string label;
        GosubNode(int l, std::string lbl) : Node(NodeType::Gosub, l), label(std::move(lbl)) {}
    };

    struct CallNode : Node {
        std::string label;
        CallNode(int l, std::string lbl) : Node(NodeType::Call, l), label(std::move(lbl)) {}
    };

    struct ReturnNode : Node {
        ReturnNode(int l) : Node(NodeType::Return, l) {}
    };

    // AST Node Types, Argument, Node, ProgramNode, CommandNode, LetAssignmentNode, IfStatementNode,
    // BlockNode, LabelNode, EventNode, GotoNode, GosubNode, CallNode, ReturnNode remain unchanged
    // (Omitted for brevity, assumed identical to previous code)

    // Validation Error (unchanged, included for reference)
    struct ValidationError {
        int line;
        std::string message;
    };

    // Parser Class (modified to use named ValidationError variables)
    class Parser {
      public:
        explicit Parser(const std::string& script) : script_(script), pos_(0), line_(1) {}

        std::unique_ptr<ProgramNode> Parse() {
            auto program = std::make_unique<ProgramNode>(1);
            SkipWhitespace();
            while(pos_ < script_.size()) {
                if(auto node = ParseStatement()) {
                    program->statements.push_back(std::move(node));
                } else {
                    break;
                }
                SkipWhitespace();
            }
            return program;
        }

      private:
        void SkipWhitespace() {
            while(pos_ < script_.size()) {
                if(script_[pos_] == '\n') {
                    ++line_;
                    ++pos_;
                } else if(isspace(script_[pos_])) {
                    ++pos_;
                } else if(pos_ + 1 < script_.size() && script_[pos_] == '/' && script_[pos_ + 1] == '/') {
                    while(pos_ < script_.size() && script_[pos_] != '\n')
                        ++pos_;
                } else {
                    break;
                }
            }
        }

        std::string ReadToken() {
            std::string token;
            bool inString = false;
            while(pos_ < script_.size()) {
                char c = script_[pos_];
                if(inString) {
                    token += c;
                    if(c == '"') inString = false;
                    ++pos_;
                    continue;
                }
                if(c == '"') {
                    inString = true;
                    token += c;
                    ++pos_;
                    continue;
                }
                if(isspace(c) || strchr("[]{},><=+-*/", c)) {
                    if(!token.empty()) break;
                    if(strchr("[]{},", c)) {
                        token = c;
                        ++pos_;
                        break;
                    }
                    if(strchr("><=+-*/", c)) {
                        token = c;
                        ++pos_;
                        break;
                    }
                    if(c == '\n') ++line_;
                    ++pos_;
                    continue;
                }
                token += c;
                ++pos_;
            }
            return token;
        }

        bool PeekToken(std::string& token) {
            size_t saved_pos = pos_;
            int saved_line = line_;
            token = ReadToken();
            pos_ = saved_pos;
            line_ = saved_line;
            return !token.empty();
        }

        std::unique_ptr<Node> ParseStatement() {
            std::string token;
            if(!PeekToken(token)) return nullptr;

            std::string lower_token = token;
            std::transform(lower_token.begin(), lower_token.end(), lower_token.begin(), ::tolower);

            if(lower_token == "let") {
                return ParseLetAssignment();
            } else if(lower_token == "if") {
                return ParseIfStatement();
            } else if(lower_token == "label") {
                return ParseLabel();
            } else if(lower_token == "event") {
                return ParseEvent();
            } else if(lower_token == "goto") {
                return ParseGoto();
            } else if(lower_token == "gosub") {
                return ParseGosub();
            } else if(lower_token == "call") {
                return ParseCall();
            } else if(lower_token == "return") {
                return ParseReturn();
            } else if(lower_token == "up" || lower_token == "down") {
                return ParseBlock();
            } else if(g_MafiaCmds.find(lower_token) != g_MafiaCmds.end()) {
                return ParseCommand();
            } else if(token == "{") {
                return ParseBlock();
            } else {
                ValidationError error = {line_, "Unexpected token: " + token};
                errors_.push_back(error);
                ReadToken(); // Skip invalid token
                return nullptr;
            }
        }

        std::unique_ptr<LetAssignmentNode> ParseLetAssignment() {
            int line = line_;
            ReadToken(); // Consume "let"
            std::string target = ReadToken();
            if(target.find("flt[") != 0 || target.back() != ']') {
                ValidationError error = {line, "Expected flt[xx] in let"};
                errors_.push_back(error);
                return nullptr;
            }
            std::string eq = ReadToken();
            if(eq != "=") {
                ValidationError error = {line, "Expected = in let"};
                errors_.push_back(error);
                return nullptr;
            }
            auto node = std::make_unique<LetAssignmentNode>(line, target);
            std::string token;
            while(PeekToken(token) && token != "\n" && token != "}") {
                Argument arg;
                arg.line = line_;
                token = ReadToken();
                if(token.find("flt[") == 0 && token.back() == ']') {
                    arg.type = Argument::Type::FloatArray;
                    arg.value = token;
                } else if((token[0] == '-' && std::all_of(token.begin() + 1, token.end(), ::isdigit)) || std::all_of(token.begin(), token.end(), ::isdigit)) {
                    arg.type = Argument::Type::Number;
                    arg.value = token;
                } else if(strchr("+-*/", token[0])) {
                    arg.type = Argument::Type::Operator;
                    arg.value = token;
                } else {
                    ValidationError error = {line, "Invalid expression in let: " + token};
                    errors_.push_back(error);
                    return nullptr;
                }
                node->expression.push_back(arg);
            }
            if(node->expression.empty()) {
                ValidationError error = {line, "Expected expression in let"};
                errors_.push_back(error);
                return nullptr;
            }
            return node;
        }

        std::unique_ptr<IfStatementNode> ParseIfStatement() {
            int line = line_;
            ReadToken(); // Consume "if"
            auto node = std::make_unique<IfStatementNode>(line);
            std::string left = ReadToken();
            if(left.find("flt[") != 0 || left.back() != ']') {
                ValidationError error = {line, "Expected flt[xx] in if"};
                errors_.push_back(error);
                return nullptr;
            }
            node->conditionLeft = {Argument::Type::FloatArray, left, line};
            node->conditionOp = ReadToken();
            if(node->conditionOp != "=" && node->conditionOp != "<" && node->conditionOp != ">") {
                ValidationError error = {line, "Expected =, <, or > in if"};
                errors_.push_back(error);
                return nullptr;
            }
            std::string right = ReadToken();
            if((right[0] == '-' && std::all_of(right.begin() + 1, right.end(), ::isdigit)) || std::all_of(right.begin(), right.end(), ::isdigit)) {
                node->conditionRight = {Argument::Type::Number, right, line};
            } else if(right.find("flt[") == 0 && right.back() == ']') {
                node->conditionRight = {Argument::Type::FloatArray, right, line};
            } else {
                ValidationError error = {line, "Expected number or flt[yy] in if"};
                errors_.push_back(error);
                return nullptr;
            }
            std::string comma = ReadToken();
            if(comma != ",") {
                ValidationError error = {line, "Expected , in if"};
                errors_.push_back(error);
                return nullptr;
            }
            node->trueLabel = ReadToken();
            comma = ReadToken();
            if(comma != ",") {
                ValidationError error = {line, "Expected , after true label in if"};
                errors_.push_back(error);
                return nullptr;
            }
            node->falseLabel = ReadToken();
            return node;
        }

        std::unique_ptr<LabelNode> ParseLabel() {
            int line = line_;
            ReadToken(); // Consume "label"
            std::string name = ReadToken();
            if(name.empty()) {
                ValidationError error = {line, "Expected identifier for label"};
                errors_.push_back(error);
                return nullptr;
            }
            return std::make_unique<LabelNode>(line, name);
        }

        std::unique_ptr<EventNode> ParseEvent() {
            int line = line_;
            ReadToken(); // Consume "event"
            std::string name = ReadToken();
            if(name.empty()) {
                ValidationError error = {line, "Expected identifier for event"};
                errors_.push_back(error);
                return nullptr;
            }
            auto node = std::make_unique<EventNode>(line, name);
            SkipWhitespace();
            while(pos_ < script_.size()) {
                std::string token;
                if(!PeekToken(token) || token == "end") { break; }
                if(auto stmt = ParseStatement()) {
                    node->statements.push_back(std::move(stmt));
                } else {
                    ValidationError error = {line_, "Invalid statement in event"};
                    errors_.push_back(error);
                    break;
                }
                SkipWhitespace();
            }
            if(pos_ < script_.size()) {
                ReadToken(); // Consume "end"
            } else {
                ValidationError error = {line, "Unclosed event block"};
                errors_.push_back(error);
            }
            return node;
        }

        std::unique_ptr<BlockNode> ParseBlock() {
            int line = line_;
            std::string blockName;
            std::string token = ReadToken();
            if(token == "{") {
                blockName = "";
            } else {
                blockName = token;
                std::string openBrace = ReadToken();
                if(openBrace != "{") {
                    ValidationError error = {line, "Expected { after block name"};
                    errors_.push_back(error);
                    return nullptr;
                }
            }
            auto node = std::make_unique<BlockNode>(line, blockName);
            SkipWhitespace();
            while(pos_ < script_.size()) {
                std::string nextToken;
                if(!PeekToken(nextToken) || nextToken == "}") { break; }
                if(auto stmt = ParseStatement()) {
                    node->statements.push_back(std::move(stmt));
                } else {
                    ValidationError error = {line_, "Invalid statement in block"};
                    errors_.push_back(error);
                    break;
                }
                SkipWhitespace();
            }
            if(pos_ < script_.size()) {
                ReadToken(); // Consume "}"
            } else {
                ValidationError error = {line, "Unclosed block"};
                errors_.push_back(error);
            }
            return node;
        }

        std::unique_ptr<GotoNode> ParseGoto() {
            int line = line_;
            ReadToken(); // Consume "goto"
            std::string label = ReadToken();
            if(label.empty()) {
                ValidationError error = {line, "Expected identifier for goto"};
                errors_.push_back(error);
                return nullptr;
            }
            return std::make_unique<GotoNode>(line, label);
        }

        std::unique_ptr<GosubNode> ParseGosub() {
            int line = line_;
            ReadToken(); // Consume "gosub"
            std::string label = ReadToken();
            if(label.empty()) {
                ValidationError error = {line, "Expected identifier for gosub"};
                errors_.push_back(error);
                return nullptr;
            }
            return std::make_unique<GosubNode>(line, label);
        }

        std::unique_ptr<CallNode> ParseCall() {
            int line = line_;
            ReadToken(); // Consume "call"
            std::string label = ReadToken();
            if(label.empty()) {
                ValidationError error = {line, "Expected identifier for call"};
                errors_.push_back(error);
                return nullptr;
            }
            return std::make_unique<CallNode>(line, label);
        }

        std::unique_ptr<ReturnNode> ParseReturn() {
            int line = line_;
            ReadToken(); // Consume "return"
            return std::make_unique<ReturnNode>(line);
        }

        std::unique_ptr<CommandNode> ParseCommand() {
            int line = line_;
            std::string cmd = ReadToken();
            std::string lower_cmd = cmd;
            std::transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
            auto node = std::make_unique<CommandNode>(line, lower_cmd);
            std::string token;
            while(PeekToken(token) && token != "\n" && token != "}" && token != "{") {
                Argument arg;
                arg.line = line_;
                token = ReadToken();
                if(token[0] == '"' && token.back() == '"') {
                    arg.type = Argument::Type::String;
                    arg.value = token;
                } else if(token.find("flt[") == 0 && token.back() == ']') {
                    arg.type = Argument::Type::FloatArray;
                    arg.value = token;
                } else if((token[0] == '-' && std::all_of(token.begin() + 1, token.end(), ::isdigit)) || std::all_of(token.begin(), token.end(), ::isdigit)) {
                    arg.type = Argument::Type::Number;
                    arg.value = token;
                } else if(strchr("><=+-*/", token[0])) {
                    arg.type = Argument::Type::Operator;
                    arg.value = token;
                } else {
                    arg.type = Argument::Type::Identifier;
                    arg.value = token;
                }
                node->arguments.push_back(arg);
            }
            return node;
        }

        std::string script_;
        size_t pos_;
        int line_;
        std::vector<ValidationError> errors_;
    };

    // Validator Class
    class Validator {
      public:
        explicit Validator(const ProgramNode* program) : program_(program) {}

        std::vector<ValidationError> Validate() {
            definedLabels_.clear();
            CollectLabels(program_);
            ValidateNode(program_);
            return errors_;
        }

      private:
        void CollectLabels(const Node* node) {
            if(node->type == NodeType::Label) {
                const auto* labelNode = static_cast<const LabelNode*>(node);
                definedLabels_.insert(labelNode->labelName);
            } else if(node->type == NodeType::Event) {
                const auto* eventNode = static_cast<const EventNode*>(node);
                definedLabels_.insert(eventNode->eventName);
                for(const auto& stmt: eventNode->statements) {
                    CollectLabels(stmt.get());
                }
            } else if(node->type == NodeType::Block) {
                const auto* blockNode = static_cast<const BlockNode*>(node);
                for(const auto& stmt: blockNode->statements) {
                    CollectLabels(stmt.get());
                }
            } else if(node->type == NodeType::Program) {
                const auto* programNode = static_cast<const ProgramNode*>(node);
                for(const auto& stmt: programNode->statements) {
                    CollectLabels(stmt.get());
                }
            }
        }

        void ValidateNode(const Node* node) {
            switch(node->type) {
            case NodeType::Program: ValidateProgram(static_cast<const ProgramNode*>(node)); break;
            case NodeType::Command: ValidateCommand(static_cast<const CommandNode*>(node)); break;
            case NodeType::LetAssignment: ValidateLetAssignment(static_cast<const LetAssignmentNode*>(node)); break;
            case NodeType::IfStatement: ValidateIfStatement(static_cast<const IfStatementNode*>(node)); break;
            case NodeType::Block: ValidateBlock(static_cast<const BlockNode*>(node)); break;
            case NodeType::Label:
                // No additional validation needed (labels collected earlier)
                break;
            case NodeType::Event: ValidateEvent(static_cast<const EventNode*>(node)); break;
            case NodeType::Goto: ValidateGoto(static_cast<const GotoNode*>(node)); break;
            case NodeType::Gosub: ValidateGosub(static_cast<const GosubNode*>(node)); break;
            case NodeType::Call: ValidateCall(static_cast<const CallNode*>(node)); break;
            case NodeType::Return:
                // No validation needed
                break;
            }
        }

        void ValidateProgram(const ProgramNode* node) {
            for(const auto& stmt: node->statements) {
                ValidateNode(stmt.get());
            }
        }

        void ValidateCommand(const CommandNode* node) {
            auto ruleIt = commandRules.find(node->command);
            if(ruleIt == commandRules.end()) {
                ValidationError error = {node->line, "Unknown command: " + node->command};
                errors_.push_back(error);
                return;
            }
            const CommandRule& rule = ruleIt->second;
            if(node->arguments.size() != rule.args.size()) {
                ValidationError error = {node->line,
                                         "Incorrect argument count for " + node->command + ": expected " + std::to_string(rule.args.size()) + ", got " +
                                             std::to_string(node->arguments.size())};
                errors_.push_back(error);
                return;
            }
            for(size_t i = 0; i < node->arguments.size(); ++i) {
                const Argument& arg = node->arguments[i];
                ArgType expected = rule.args[i];
                if(expected == ArgType::Number && arg.type != Argument::Type::Number && arg.type != Argument::Type::FloatArray) {
                    ValidationError error = {arg.line, "Expected number for argument " + std::to_string(i + 1) + " of " + node->command};
                    errors_.push_back(error);
                } else if(expected == ArgType::String && arg.type != Argument::Type::String) {
                    ValidationError error = {arg.line, "Expected string for argument " + std::to_string(i + 1) + " of " + node->command};
                    errors_.push_back(error);
                } else if(expected == ArgType::Operator && arg.type != Argument::Type::Operator) {
                    ValidationError error = {arg.line, "Expected operator for argument " + std::to_string(i + 1) + " of " + node->command};
                    errors_.push_back(error);
                } else if(expected == ArgType::Identifier && arg.type != Argument::Type::Identifier) {
                    ValidationError error = {arg.line, "Expected identifier for argument " + std::to_string(i + 1) + " of " + node->command};
                    errors_.push_back(error);
                }
            }
        }

        void ValidateLetAssignment(const LetAssignmentNode* node) {
            if(node->expression.empty()) {
                ValidationError error = {node->line, "Empty expression in let"};
                errors_.push_back(error);
                return;
            }
            for(const auto& arg: node->expression) {
                if(arg.type != Argument::Type::Number && arg.type != Argument::Type::FloatArray && arg.type != Argument::Type::Operator) {
                    ValidationError error = {arg.line, "Invalid argument in let expression: " + arg.value};
                    errors_.push_back(error);
                }
            }
            // Check operator sequence (e.g., number op number)
            for(size_t i = 0; i < node->expression.size(); ++i) {
                if(node->expression[i].type == Argument::Type::Operator) {
                    if(i == 0 || i == node->expression.size() - 1) {
                        ValidationError error = {node->expression[i].line, "Operator at invalid position in let"};
                        errors_.push_back(error);
                    } else if(node->expression[i - 1].type == Argument::Type::Operator || node->expression[i + 1].type == Argument::Type::Operator) {
                        ValidationError error = {node->expression[i].line, "Consecutive operators in let"};
                        errors_.push_back(error);
                    }
                }
            }
        }

        void ValidateIfStatement(const IfStatementNode* node) {
            if(node->conditionLeft.type != Argument::Type::FloatArray) {
                ValidationError error = {node->conditionLeft.line, "Expected flt[xx] as left operand in if"};
                errors_.push_back(error);
            }
            if(node->conditionRight.type != Argument::Type::Number && node->conditionRight.type != Argument::Type::FloatArray) {
                ValidationError error = {node->conditionRight.line, "Expected number or flt[yy] as right operand in if"};
                errors_.push_back(error);
            }
            if(node->trueLabel != "-1" && definedLabels_.find(node->trueLabel) == definedLabels_.end()) {
                ValidationError error = {node->line, "Undefined true label in if: " + node->trueLabel};
                errors_.push_back(error);
            }
            if(node->falseLabel != "-1" && definedLabels_.find(node->falseLabel) == definedLabels_.end()) {
                ValidationError error = {node->line, "Undefined false label in if: " + node->falseLabel};
                errors_.push_back(error);
            }
        }

        void ValidateBlock(const BlockNode* node) {
            for(const auto& stmt: node->statements) {
                ValidateNode(stmt.get());
            }
        }

        void ValidateEvent(const EventNode* node) {
            bool hasReturn = false;
            for(const auto& stmt: node->statements) {
                if(stmt->type == NodeType::Return) { hasReturn = true; }
                ValidateNode(stmt.get());
            }
            if(!hasReturn && !node->statements.empty()) {
                ValidationError error = {node->line, "Event block must end with return"};
                errors_.push_back(error);
            }
        }

        void ValidateGoto(const GotoNode* node) {
            if(node->label != "-1" && definedLabels_.find(node->label) == definedLabels_.end()) {
                ValidationError error = {node->line, "Undefined label in goto: " + node->label};
                errors_.push_back(error);
            }
        }

        void ValidateGosub(const GosubNode* node) {
            if(node->label != "-1" && definedLabels_.find(node->label) == definedLabels_.end()) {
                ValidationError error = {node->line, "Undefined label in gosub: " + node->label};
                errors_.push_back(error);
            }
        }

        void ValidateCall(const CallNode* node) {
            if(node->label != "-1" && definedLabels_.find(node->label) == definedLabels_.end()) {
                ValidationError error = {node->line, "Undefined label in call: " + node->label};
                errors_.push_back(error);
            }
        }

        const ProgramNode* program_;
        std::vector<ValidationError> errors_;
        std::set<std::string> definedLabels_;
    };

    // Updated ValidateScript Function (unchanged)
    static std::vector<ValidationError> ValidateScript(const std::string& script) {
        Parser parser(script);
        auto program = parser.Parse();
        Validator validator(program.get());
        return validator.Validate();
    }
}; // namespace MafiaScript

const static TextEditor::Palette& GetEditorPalette() {
    const static TextEditor::Palette p = {{
        0xffffffff, // Default
        0xffdfa0d8, // Keyword
        0xffb5cea8, // Number
        0xff859dd6, // String
        0xff859dd6, // Char literal
        0xffb4b4b4, // Punctuation
        0xff408080, // Preprocessor
        0xffaadcdc, // Identifier
        0xffffb7be, // Known identifier
        0xffc040a0, // Preproc identifier
        0xff4aa657, // Comment (single line)
        0xff4aa657, // Comment (multi line)
        0xff1e1e1e, // Background
        0xffe0e0e0, // Cursor
        0x80ff9933, // Selection
        0x803e3eff, // ErrorMarker
        0x40f08000, // Breakpoint
        0xff8a8a8a, // Line number
        0x40000000, // Current line fill
        0x40808080, // Current line fill (inactive)
        0x40a0a0a0, // Current line edge
    }};
    return p;
}

class ScriptEditor {
  public:
    void Open(GameScript* script) {
        for(auto& tab: m_Tabs) {
            if(tab.script == script) return;
        }

        auto& tab = m_Tabs.emplace_back();
        tab.script = script;
        tab.editor.SetLanguageDefinition(LanguageDefinition_Mafia());
        tab.editor.SetText(script->script);
        tab.editor.SetPalette(GetEditorPalette());
        m_ActiveTab = 0;
    }

    void Render() {
        if(m_Tabs.size() > 0) {
            if(ImGui::Begin("Script Editor")) {
                if(ImGui::BeginTabBar("ScriptTabs")) {
                    for(uint32_t i = 0; i < m_Tabs.size(); ++i) {
                        bool open = true;
                        if(ImGui::BeginTabItem(m_Tabs[i].script->name.c_str(), &open)) {
                            m_ActiveTab = i;
                            m_Tabs[m_ActiveTab].editor.Render("ScriptEditor");

                            std::string scriptContent = m_Tabs[m_ActiveTab].editor.GetText();
                            // NOTE: Validation will be left disabled until further notice
                            /*auto errors = MafiaScript::ValidateScript(scriptContent);
                            TextEditor::ErrorMarkers markers;
                            for(const auto& error: errors) {
                                markers[error.line] = error.message;
                                debugPrintf("Validation error at line %d: %s\n", error.line, error.message.c_str());
                            }
                            m_Tabs[m_ActiveTab].editor.SetErrorMarkers(markers);*/

                            ImGui::EndTabItem();
                        }
                        if(!open) {
                            Save(m_Tabs[i]);
                            m_Tabs.erase(m_Tabs.begin() + i);
                            if(m_Tabs.empty()) {
                                m_ActiveTab = -1;
                            } else {
                                if(m_ActiveTab >= i) m_ActiveTab--;
                                if(m_ActiveTab >= static_cast<int>(m_Tabs.size())) m_ActiveTab = m_Tabs.size() - 1;
                            }
                            --i;
                        }
                    }

                    ImGui::EndTabBar();
                }
                ImGui::End();
            }
        }
    }

    void SaveAll() {
        for(auto& tab : m_Tabs) {
            Save(tab);
        }
    }

  private:
    struct ScriptTab {
        GameScript* script;
        TextEditor editor;
    };

    void Save(const ScriptTab& tab) { tab.script->script = tab.editor.GetText(); }

    std::string ReplaceParameters(const std::string& command, const std::string& line) {
        std::string result = line;
        if(g_ParamMap.find(command) != g_ParamMap.end()) {
            const auto& params = g_ParamMap.at(command);
            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string token;
            while(ss >> token) {
                tokens.push_back(token);
            }

            if(tokens[0] == command && tokens.size() >= params.size() + 1) {
                result = command;
                for(size_t i = 0; i < params.size(); ++i) {
                    result += " " + params[i];
                }
            }
        }
        return result;
    }

    TextEditor::LanguageDefinition LanguageDefinition_Mafia() {
        TextEditor::LanguageDefinition lang;
        lang.mName = "MafiaScript";
        lang.mCaseSensitive = false;
        lang.mAutoIndentation = false;
        lang.mCommentStart = "/*";
        lang.mCommentEnd = "*/";
        lang.mSingleLineComment = "//";

        for(auto& constant: g_MafiaConstants) {
            TextEditor::Identifier id;
            id.mDeclaration = "Macro";
            lang.mIdentifiers.insert(std::make_pair(std::string(constant), id));
        }

        for(auto& keyword: g_MafiaCmds) {
            lang.mKeywords.insert(keyword);
        }

        lang.mTokenize =
            [](const char* inBegin, const char* inEnd, const char*& outBegin, const char*& outEnd, TextEditor::PaletteIndex& paletteIndex) -> bool {
            static std::set<std::string> definedLabels;
            if(outBegin == inBegin) definedLabels.clear();

            static std::string lastCommand;
            static size_t argIndex = 0;
            static bool afterLabel = false;

            outBegin = inBegin;
            outEnd = inBegin;

            // Skip whitespace
            while(outBegin < inEnd && isspace(*outBegin)) {
                ++outBegin;
            }
            outEnd = outBegin;

            if(outBegin >= inEnd) {
                paletteIndex = TextEditor::PaletteIndex::Default;
                return false; // No more tokens
            }

            // Comments (// to end of line)
            if(outEnd + 1 < inEnd && *outEnd == '/' && *(outEnd + 1) == '/') {
                outEnd = inEnd; // Consume to end of line
                paletteIndex = TextEditor::PaletteIndex::Comment;
                //debugPrintf("Token: [%s] Comment", std::string(outBegin, outEnd).c_str());
                return true;
            }

            // Strings ("...")
            if(*outEnd == '"') {
                ++outEnd; // Skip opening quote
                while(outEnd < inEnd) {
                    if(*outEnd == '\\' && outEnd + 1 < inEnd) {
                        outEnd += 2; // Skip escaped character
                    } else if(*outEnd == '"') {
                        ++outEnd; // Include closing quote
                        paletteIndex = TextEditor::PaletteIndex::String;
                        //debugPrintf("Token: [%s] String", std::string(outBegin, outEnd).c_str());
                        return true;
                    } else {
                        ++outEnd;
                    }
                }
                // Unclosed string
                paletteIndex = TextEditor::PaletteIndex::String;
                //debugPrintf("Token: [%s] String (unclosed)", std::string(outBegin, outEnd).c_str());
                return true;
            }

            // Operators (,[],-+={<})
            if(strchr(",[]-+={<>}", *outEnd)) {
                ++outEnd; // Single-character operator
                paletteIndex = TextEditor::PaletteIndex::Punctuation;
                //debugPrintf("Token: [%s] Punctuation", std::string(outBegin, outEnd).c_str());
                return true;
            }

            // Keywords or Numbers (build token until delimiter)
            while(outEnd < inEnd && !isspace(*outEnd) && !strchr(",[]-+={<>}\"", *outEnd)) {
                ++outEnd;
            }

            if(outEnd > outBegin) {
                std::string token(outBegin, outEnd);
                std::string lowerToken = token;
                std::transform(lowerToken.begin(), lowerToken.end(), lowerToken.begin(), ::tolower);

                // Keywords
                if(g_MafiaCmds.find(lowerToken) != g_MafiaCmds.end()) {
                    lastCommand = lowerToken;
                    argIndex = 0;
                    afterLabel = (lastCommand == "label");
                    paletteIndex = TextEditor::PaletteIndex::Keyword;
                    return true;
                }

                // Label parameter
                if(afterLabel) {
                    definedLabels.insert(token);
                    paletteIndex = TextEditor::PaletteIndex::Identifier;
                    afterLabel = false;
                    return true;
                }

                // Label reference
                if(!lastCommand.empty()) {
                    auto paramIt = g_ParamMap.find(lastCommand);
                    if(paramIt != g_ParamMap.end() && argIndex < paramIt->second.size()) {
                        bool isLabelArg = false;
                        if(lastCommand == "goto" || lastCommand == "gosub" || lastCommand == "call") {
                            isLabelArg = (argIndex == 0);
                        } else if(lastCommand == "if") {
                            isLabelArg = (argIndex == 2 || argIndex == 3);
                        } else if(lastCommand == "iffltinrange") {
                            isLabelArg = (argIndex == 2);
                        } else if(lastCommand == "ifplayerstealcar") {
                            isLabelArg = (argIndex == 1);
                        } else if(lastCommand == "compareownerwith") {
                            isLabelArg = (argIndex == 1 || argIndex == 2);
                        } else if(lastCommand == "compareownerwithex") {
                            isLabelArg = (argIndex == 2 || argIndex == 3);
                        } else if(lastCommand == "enemy_action_runaway") {
                            isLabelArg = (argIndex == 2);
                        }

                        if(isLabelArg && definedLabels.find(token) != definedLabels.end()) {
                            paletteIndex = TextEditor::PaletteIndex::Identifier;
                            argIndex++;
                            return true;
                        }
                    }
                    argIndex++;
                }

                // Numbers (start and end with digit, optional decimal)
                bool isNumber = !token.empty() && isdigit(token[0]) && isdigit(token.back());
                if(isNumber) {
                    size_t dotCount = std::count(token.begin(), token.end(), '.');
                    if(dotCount <= 1) { // Integer or simple float
                        paletteIndex = TextEditor::PaletteIndex::Number;
                        //debugPrintf("Token: [%s] Number", token.c_str());
                        return true;
                    }
                }

                // Identifiers
                if(g_MafiaConstants.find(lowerToken) != g_MafiaConstants.end()) {
                    paletteIndex = TextEditor::PaletteIndex::KnownIdentifier;
                    //debugPrintf("Token: [%s] Keyword", token.c_str());
                    return true;
                }
                paletteIndex = TextEditor::PaletteIndex::Default;
                return false;
            }

            // Fallback for single characters
            ++outEnd;
            paletteIndex = TextEditor::PaletteIndex::Default;
            //debugPrintf("Token: [%s] Default", std::string(outBegin, outEnd).c_str());
            return true;
        };

        return lang;
    }

    std::vector<ScriptTab> m_Tabs;
    uint32_t m_ActiveTab = -1;
    uint32_t m_NextTabID = 1;
};