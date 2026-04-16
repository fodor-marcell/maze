#ifndef CONFIG_H
#define CONFIG_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef GL_LIGHT_MODEL_AMBIENT
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#define WIDTH 22
#define HEIGHT 14

#define TILE 1.0f
#define WALL_HEIGHT 4.0f

#define PLAYER_HEIGHT 0.90f
#define PLAYER_COLLISION_RADIUS 0.18f

#define MOVE_SPEED 0.0175f
#define MOUSE_SENSITIVITY 0.0007f
#define MAX_PITCH 1.2f

#define WALK_BOB_FREQ 7.0f
#define WALK_BOB_AMPLITUDE 0.045f

#define SCENE_BRIGHTNESS 0.68f
#define SKYBOX_SIZE 30.0f

#define END_TILE_X 14
#define END_TILE_Y 10

extern const char* WALL_TEXTURE_PATH;
extern const char* FLOOR_TEXTURE_PATH;
extern const char* SKY_TEXTURE_PATH;

extern int endTileX;
extern int endTileY;
extern int startTileX;
extern int startTileY;

extern int maze[HEIGHT][WIDTH];

#endif
