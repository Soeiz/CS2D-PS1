#include <sys/types.h>
#include <stdio.h>
#include <libgte.h>
#include <libetc.h>
#include <libgpu.h>
#include <libsnd.h>
#include <libspu.h>
#include <libapi.h>
#include <rand.h>

#define VMODE 0                 // Video Mode : 0 : NTSC, 1: PAL
#define SCREENXRES 320          // Screen width
#define SCREENYRES 240          // Screen height
#define CENTERX SCREENXRES/2    // Center of screen on x 
#define CENTERY SCREENYRES/2    // Center of screen on y
#define MARGINX 0                // margins for text display
#define MARGINY 32
#define FONTSIZE 8 * 7           // Text Field Height
#define OTLEN 8              // Ordering Table Length 
DISPENV disp[2];                 // Double buffered DISPENV and DRAWENV
DRAWENV draw[2];
short db = 0;                      // index of which buffer is used, values 0, 1
u_long ot[2][OTLEN];     // double ordering table of length 8 * 32 = 256 bits / 32 bytes
char primbuff[2][32768];     // double primitive buffer of length 32768 * 8 =  262.144 bits / 32,768 Kbytes
char *nextpri = primbuff[0]; // pointer to the next primitive in primbuff. Initially, points to the first bit of primbuff[0]
// Number of VAG files to load
#define VAG_NBR 16
#define VAG_NBRG 1
#define VAG_NBRAK 1
#define MALLOC_MAX VAG_NBR + VAG_NBRG + VAG_NBRAK            // Max number of time we can call SpuMalloc
// convert Little endian to Big endian
#define SWAP_ENDIAN32(x) (((x)>>24) | (((x)>>8) & 0xFF00) | (((x)<<8) & 0x00FF0000) | ((x)<<24))
// Memory management table ; allow MALLOC_MAX calls to SpuMalloc() - libref47.pdf p.1044
char spu_malloc_rec[SPU_MALLOC_RECSIZ * (MALLOC_MAX + 1)]; 
// Custom struct to handle VAG files
typedef struct VAGsound {
    u_char * VAGfile;        // Pointer to VAG data address
    u_long spu_channel;      // SPU voice to playback to
    u_long spu_address;      // SPU address for memory freeing spu mem
    } VAGsound;
// VAG header struct (see fileformat47.pdf, p.209)
typedef struct VAGhdr {                // All the values in this header must be big endian
        char id[4];                    // VAGp         4 bytes -> 1 char * 4
        unsigned int version;          // 4 bytes
        unsigned int reserved;         // 4 bytes
        unsigned int dataSize;         // (in bytes) 4 bytes
        unsigned int samplingFrequency;// 4 bytes
        char  reserved2[12];           // 12 bytes -> 1 char * 12
        char  name[16];                // 16 bytes -> 1 char * 16
        // Waveform data after that
} VAGhdr;
// SPU settings
SpuCommonAttr commonAttributes;          // structure for changing common voice attributes
SpuVoiceAttr  voiceAttributes ;          // structure for changing individual voice attributes                       
// extern VAG files
extern u_char _binary_vag_tick_vag_start;
extern u_char _binary_vag_bombplant_vag_start;
extern u_char _binary_vag_boom_vag_start;
extern u_char _binary_vag_bboom_vag_start;
extern u_char _binary_vag_terroristW_vag_start;
extern u_char _binary_vag_defusing_vag_start;
extern u_char _binary_vag_defusedV_vag_start;
extern u_char _binary_vag_counterW_vag_start;
extern u_char _binary_vag_footstep_vag_start;
extern u_char _binary_vag_planted_vag_start;
extern u_char _binary_vag_AK_vag_start;
extern u_char _binary_vag_reloading1_vag_start;
extern u_char _binary_vag_reloading2_vag_start;
extern u_char _binary_vag_boop_vag_start;
extern u_char _binary_vag_glock_vag_start;
extern u_char _binary_vag_knife_vag_start;

VAGsound soundBank[VAG_NBR] = {
      { &_binary_vag_tick_vag_start,
        SPU_00CH, 0 },
      { &_binary_vag_bombplant_vag_start,
        SPU_01CH, 0 },
      { &_binary_vag_boom_vag_start,
        SPU_02CH, 0 },
      { &_binary_vag_bboom_vag_start,
        SPU_03CH, 0 },
      { &_binary_vag_terroristW_vag_start,
        SPU_04CH, 0 },
      { &_binary_vag_defusing_vag_start,
        SPU_05CH, 0 },
      { &_binary_vag_defusedV_vag_start,
        SPU_06CH, 0 },
      { &_binary_vag_counterW_vag_start,
        SPU_07CH, 0 },
      { &_binary_vag_footstep_vag_start,
        SPU_08CH, 0 },
      { &_binary_vag_planted_vag_start,
        SPU_09CH, 0 },
      { &_binary_vag_AK_vag_start,
        SPU_10CH, 0 },
      { &_binary_vag_reloading1_vag_start,
        SPU_11CH, 0 },
      { &_binary_vag_reloading2_vag_start,
        SPU_12CH, 0 },
      { &_binary_vag_boop_vag_start,
        SPU_13CH, 0 },
      { &_binary_vag_glock_vag_start,
        SPU_14CH, 0 },
      { &_binary_vag_knife_vag_start,
        SPU_15CH, 0 }
};
extern unsigned long _binary_tim_DEFAULT_DUST_tim_start[];
extern unsigned long _binary_tim_DEFAULT_DUST_tim_end[];
extern unsigned long _binary_tim_DEFAULT_DUST_tim_length;

extern unsigned long _binary_tim_DUSTTILE_tim_start[];
extern unsigned long _binary_tim_DUSTTILE_tim_end[];
extern unsigned long _binary_tim_DUSTTILE_tim_length;

extern unsigned long _binary_tim_TESTMAP_tim_start[];
extern unsigned long _binary_tim_TESTMAP_tim_end[];
extern unsigned long _binary_tim_TESTMAP_tim_length;

extern unsigned long _binary_tim_B_tim_start[];
extern unsigned long _binary_tim_B_tim_end[];
extern unsigned long _binary_tim_B_tim_length;

extern unsigned long _binary_tim_wall_tim_start[];
extern unsigned long _binary_tim_wall_tim_end[];
extern unsigned long _binary_tim_wall_tim_length;

extern unsigned long _binary_tim_bomb_tim_start[];
extern unsigned long _binary_tim_bomb_tim_end[];
extern unsigned long _binary_tim_bomb_tim_length;

TIM_IMAGE TESTMAP, DEFAULT_DUST, DUST, BLOGO, wwall, bomb;

int Framecounter = 0;

int menu = 0;

int insite = 1;
int bombplanted = 0;
int timerbomb = 2700;
int delay = 60.0;
int olddelay = 60.0;
int bombplantedsaid = 0;
int bombplantedframes = 0;
int finishedbomb = 0;
int terroristW = 0;
int defusing = 0;
int defusingkit = 1;
int defused = 0;
int CTW = 0;
int bombdefusedvoice = 0;
int planting = 0;
int plantingrand = 4;
int terrorist = 1;

int timer;
int seconds = 0;
int minutes = 5;

int gameendingvar = 0;
int gameendingframes = 0;
int gameended = 0;

int curweapon = 2; //0 knife, 1 pistol, 2 main, 3 C4 (4 grenade, 5 grenade, 6 grenade,)
int cooldownweapon = 7;
int bulletsprimary = 30;
int bulletssub = 20;
int reloadframes;
int reloading;
int weapo;
int basedamage = 0;
int subammo = 0;
int priammo = 0;
int weight = 0;
int cooldownsample = 0;
int reloadsample = 0;
int initW = 1;

int move = 0;
int collision = 0;
int curdir = 0; //1 up, 2 down, 3 right, 4 left, 5 up & right, 6 up & left, 7 down & right, 8 down & left
int curleft = 0;
int curright = 0;
int curup = 0;
int curdown = 0;
int curX = 0;
int curY = 0;
int bombX = 9999;
int bombY = 9999;
int bobing = 0;
int rotat = 0; //0 Right, 90 down, 180 left, 270 up

int cooldownfootstep = 18;

int RAN;
int Ran(int max)
{
    RAN = (rand()%max);
    if (RAN == 0) {RAN++;}
    return 0;
} 


//For the collision system, atm, I use the exact coordinates of the wall and check if the player's box get into these coordinates.

int collision1x1 = -74; //Upper right
int collision1y1 = 15; // Upper
int collision1x2 = -198; // Bottom left
int collision1y2 = -71; //Bottom

//bomb site's coordinates declared but not with any values for now

int collisionbombAx1, collisionbombAx2, collisionbombAy1, collisionbombAy2, collisionbombBx1, collisionbombBx2, collisionbombBy1, collisionbombBy2;

int map = 0; //0 testmap

//Weapon selection

int subW = 2; //1 is USP, 2 Glock, 3 Deagle, 4 P228, 5 Elite, 6 Five-seven
int burstmode = 0;
int shotgunW = 0; //1 is M3, 2 is XM1014;
int subMachineWT = 0; //1 Mac 10, 2 MP5, 3 P90, 4 UMP45
int subMachineWCT = 0; //1 TMP, 2 MP5, 3 P90, 4 UMP45
int rifleWT = 0; //1 Galil, 2 AK, 3 SG, 4 Scout, 5 AWP, 6 G3, 7 SG
int rifleWCT = 0; //1 FAMAS, 2 M4A11, 3 AUG, 4 Scout, 5 AWP, 6 G3, 7 SG
int machineW = 1; // 1 is M249
int bombcarrier = 1;

int dead = 0;

int health = 50;
int protection = 0;

int terroristnum = 5;
int terroristpoints = 0;
int counterterroristnum = 5;
int counterterroristpoints = 0;