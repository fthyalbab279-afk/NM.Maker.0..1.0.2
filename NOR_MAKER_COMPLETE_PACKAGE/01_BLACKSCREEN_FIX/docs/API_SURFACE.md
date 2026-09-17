# NOR Maker Core – API Surface (ما هو منفّذ فعلاً)

## Load / Decode
- `gm82_decode_sprites_from_gmk` / `_file`
- `gm82_decode_backgrounds_from_gmk`
- `gm82_decode_objects_from_gmk`
- `gm82_decode_rooms_from_gmk` (instances + tiles)
- `gm82_decode_sounds_from_gmk` (headers only)
- `gm82_actions_scan_gmk`

## Runtime
- `gm82_runtime_init` / `bind_assets` / `goto_room` / `step` / `draw`
- `gm82_runtime_instance_create` / `destroy`
- Events: Create/Step behaviors + action subset
- Collision: AABB instances + tiles
- Input: keyboard/mouse state
- View: follow + clamp

## GML builtins (C)
instance_create/destroy/number/exists  
motion_set/add, move_towards_point, point_distance/direction  
place_meeting, position_meeting, instance_place  
x/y/hspeed/vspeed/direction/speed/sprite_index/...  
room_width/height/speed, room_goto/next/previous/restart, game_end  
score/lives/health  
keyboard_check/_pressed/_released  
abs/sign/clamp/lerp/irandom/random  

## Android (scaffold)
- Gm82Native.java + jni bridge + CMakeLists
- native_draw = STUB (no GLES yet)

## NOT implemented
GML script interpreter, full DnD, audio playback, precise masks, gm82core, APK integration
- instance_nearest / instance_find / distance_to_object
