/*
    TO FIX :

    FIXED ? (it is not doing that anymore but I'm not sure if it's fixed) : 

    TO FIX ON THE PS1 :

    TO MAKE (doing ideas/partially implemented things) : 
    Change sounds depending on the weapon (SPU related)
    Creating bullets
    Be able to knife the walls
    Create layers for the levels (Can go down or up)

    TO DO (only ideas) :
    Menu
    Bots
    Grenades
    Buy menu

    OTHER (info) :
    Check if we can go a lil bit faster on 3 weight weapons
*/

#include "constants.h"

void collisions(void) {
    if (curX <= collision1x1 && curX >= collision1x2 && curY <= collision1y1 && curY >= collision1y2) {
        collision = 1;
    } else {
        collision = 0;
    }
}

void LoadTexture(u_long * tim, TIM_IMAGE * tparam){     // This part is from Lameguy64's tutorial series : lameguy64.net/svn/pstutorials/chapter1/3-textures.html login/pw: annoyingmous
        OpenTIM(tim);                                   // Open the tim binary data, feed it the address of the data in memory
        ReadTIM(tparam);                                // This read the header of the TIM data and sets the corresponding members of the TIM_IMAGE structure
        LoadImage(tparam->prect, tparam->paddr);        // Transfer the data from memory to VRAM at position prect.x, prect.y
        DrawSync(0);                                    // Wait for the drawing to end
        if (tparam->mode & 0x8){ // check 4th bit       // If 4th bit == 1, TIM has a CLUT
            LoadImage(tparam->crect, tparam->caddr);    // Load it to VRAM at position crect.x, crect.y
            DrawSync(0);                                // Wait for drawing to end
    }
}
void initSnd(void){
    SpuInitMalloc(MALLOC_MAX, spu_malloc_rec);                      // Maximum number of blocks, mem. management table address.
    commonAttributes.mask = (SPU_COMMON_MVOLL | SPU_COMMON_MVOLR);  // Mask which attributes to set
    commonAttributes.mvol.left  = 0x3fff;                           // Master volume left
    commonAttributes.mvol.right = 0x3fff;                           // see libref47.pdf, p.1058
    SpuSetCommonAttr(&commonAttributes);                            // set attributes
    SpuSetIRQ(SPU_OFF);
    SpuSetKey(SpuOff, SPU_ALLCH);
}
u_long sendVAGtoSPU(unsigned int VAG_data_size, u_char *VAG_data){
    u_long transferred;
    SpuSetTransferMode(SpuTransByDMA);                              // DMA transfer; can do other processing during transfer
    transferred = SpuWrite (VAG_data + sizeof(VAGhdr), VAG_data_size);     // transfer VAG_data_size bytes from VAG_data  address to sound buffer
    SpuIsTransferCompleted (SPU_TRANSFER_WAIT);                     // Checks whether transfer is completed and waits for completion
    return transferred;
}
void setVoiceAttr(unsigned int pitch, long channel, unsigned long soundAddr ){
    voiceAttributes.mask=                                   //~ Attributes (bit string, 1 bit per attribute)
    (
      SPU_VOICE_VOLL |
      SPU_VOICE_VOLR |
      SPU_VOICE_PITCH |
      SPU_VOICE_WDSA |
      SPU_VOICE_ADSR_AMODE |
      SPU_VOICE_ADSR_SMODE |
      SPU_VOICE_ADSR_RMODE |
      SPU_VOICE_ADSR_AR |
      SPU_VOICE_ADSR_DR |
      SPU_VOICE_ADSR_SR |
      SPU_VOICE_ADSR_RR |
      SPU_VOICE_ADSR_SL
    );
    voiceAttributes.voice        = channel;                 //~ Voice (low 24 bits are a bit string, 1 bit per voice )
    voiceAttributes.volume.left  = 0x0;                  //~ Volume 
    voiceAttributes.volume.right = 0x0;                  //~ Volume
    voiceAttributes.pitch        = pitch;                   //~ Interval (set pitch)
    voiceAttributes.addr         = soundAddr;               //~ Waveform data start address
    voiceAttributes.a_mode       = SPU_VOICE_LINEARIncN;    //~ Attack rate mode  = Linear Increase - see libref47.pdf p.1091
    voiceAttributes.s_mode       = SPU_VOICE_LINEARIncN;    //~ Sustain rate mode = Linear Increase
    voiceAttributes.r_mode       = SPU_VOICE_LINEARDecN;    //~ Release rate mode = Linear Decrease
    voiceAttributes.ar           = 0x0;                     //~ Attack rate
    voiceAttributes.dr           = 0x0;                     //~ Decay rate
    voiceAttributes.rr           = 0x0;                     //~ Release rate
    voiceAttributes.sr           = 0x0;                     //~ Sustain rate
    voiceAttributes.sl           = 0xf;                     //~ Sustain level
    SpuSetVoiceAttr(&voiceAttributes);                      // set attributes
    SpuSetVoiceVolume(8, 0x2500, 0x2500); //Footstep
}
u_long setSPUtransfer(VAGsound * sound){
    // Return spu_address
    u_long transferred, spu_address;
    u_int pitch;
    const VAGhdr * VAGheader = (VAGhdr *) sound->VAGfile;
    pitch = (SWAP_ENDIAN32(VAGheader->samplingFrequency) << 12) / 44100L; 
    spu_address = SpuMalloc(SWAP_ENDIAN32(VAGheader->dataSize));                
    SpuSetTransferStartAddr(spu_address);                                       
    transferred = sendVAGtoSPU(SWAP_ENDIAN32(VAGheader->dataSize), sound->VAGfile);
    setVoiceAttr(pitch, sound->spu_channel, spu_address); 
    return spu_address;
}
void playSFX(VAGsound *  sound){
    // Set voice volume to max
    voiceAttributes.mask= ( SPU_VOICE_VOLL | SPU_VOICE_VOLR );
    voiceAttributes.voice        = sound->spu_channel;
    voiceAttributes.volume.left  = 0x1000;
    voiceAttributes.volume.right = 0x1000;
    SpuSetVoiceAttr(&voiceAttributes);
    // Play voice
    SpuSetKey(SpuOn, sound->spu_channel);
}
void init(void){
    ResetGraph(0);                 // Initialize drawing engine with a complete reset (0)
    InitGeom();
    SetGeomOffset(CENTERX,CENTERY);
    SetGeomScreen(CENTERX);
    SetDefDispEnv(&disp[0], 0, 0         , SCREENXRES, SCREENYRES);     // Set display area for both &disp[0] and &disp[1]
    SetDefDispEnv(&disp[1], 0, SCREENYRES, SCREENXRES, SCREENYRES);     // &disp[0] is on top  of &disp[1]
    SetDefDrawEnv(&draw[0], 0, SCREENYRES, SCREENXRES, SCREENYRES);     // Set draw for both &draw[0] and &draw[1]
    SetDefDrawEnv(&draw[1], 0, 0         , SCREENXRES, SCREENYRES);     // &draw[0] is below &draw[1]
    if (VMODE)                  // PAL
    {
        SetVideoMode(MODE_PAL);
        disp[0].disp.y = disp[1].disp.y = 8;
        }
    SetDispMask(1);                 // Display on screen    
    setRGB0(&draw[0], 50, 50, 50); // set color for first draw area
    setRGB0(&draw[1], 50, 50, 50); // set color for second draw area
    draw[0].isbg = 1;               // set mask for draw areas. 1 means repainting the area with the RGB color each frame 
    draw[1].isbg = 1;
    PutDispEnv(&disp[db]);          // set the disp and draw environnments
    PutDrawEnv(&draw[db]);
    FntLoad(960, 0);                // Load font to vram at 960,0(+128)
    FntOpen(MARGINX, SCREENYRES - MARGINY - FONTSIZE, SCREENXRES - MARGINX * 2, FONTSIZE, 0, 280 ); // FntOpen(x, y, width, height,  black_bg, max. nbr. chars
}
void display(void){
    DrawSync(0);                    // Wait for all drawing to terminate
    VSync(0);                       // Wait for the next vertical blank
    PutDispEnv(&disp[db]);          // set alternate disp and draw environnments
    PutDrawEnv(&draw[db]);  
    DrawOTag(&ot[db][OTLEN - 1]);
    db = !db;                       // flip db value (0 or 1)
    nextpri = primbuff[db];
}
void createbullet(void) {
}
void shoot(void) {
    if (planting == 0) {
        if (curweapon == 2) {
            playSFX(&soundBank[10]);
            SpuSetVoiceVolume(10, 0x2200, 0x2200);
            createbullet();
            bulletsprimary--;
        }
        if (curweapon == 1) {
            if (subW == 2 && burstmode == 1) {
                for (int i = 0; i < 3; i++) {
                    if (bulletssub > 0) {
                        playSFX(&soundBank[14]);
                        SpuSetVoiceVolume(14, 0x0800, 0x0800);
                        createbullet();
                        bulletssub--;
                    }
                } 
            }else {
                playSFX(&soundBank[14]);
                SpuSetVoiceVolume(14, 0x0800, 0x0800);
                createbullet();
                bulletssub--;
            }
        }
        if (curweapon == 0) { //Knife
            playSFX(&soundBank[15]);
        }
    }
}
void reload(void) {
    if (reloadframes == 0 && bulletsprimary != priammo || reloadframes == 0 && bulletssub != subammo) {
        reloadframes = reloadsample;
        weapo = curweapon;
    }
    reloadframes--;
    if (weapo != curweapon) { //PUNISH HIM
        reloadframes = 0; //no ammo for u
        reloading = 0;
        if (SpuGetKeyStatus(SPU_12CH) == SPU_ON) {SpuSetKey(SPU_12CH, SPU_OFF);}
        if (SpuGetKeyStatus(SPU_11CH) == SPU_ON) {SpuSetKey(SPU_11CH, SPU_OFF);}
    }
    if (reloadframes == 10) {
        playSFX(&soundBank[12]);
        SpuSetVoiceVolume(12, 0x2200, 0x2200);
    }
    if (reloadframes == reloadsample - 1) {
        playSFX(&soundBank[11]);
        SpuSetVoiceVolume(11, 0x2200, 0x2200);
    }
    if (reloadframes == 1) {
        if (weapo == 1) {bulletssub = subammo;}
        if (weapo == 2) {bulletsprimary = priammo;}
        reloadframes--;
        reloading--;
    }
}
void gameending(int situation) {
    if (gameended == 0) {
        gameendingframes++;
        if (situation == 1 || situation == 2) { // T Win Via bomb OR Kills
            if (SpuGetKeyStatus(SPU_00CH) == SPU_ON) { //situ 1 
                SpuSetKey(SPU_00CH, SPU_OFF);
                playSFX(&soundBank[2]);
            }
            if (gameendingframes == 5 && situation == 1) {
                playSFX(&soundBank[3]);
            }
            if (gameendingframes == 25) {
                playSFX(&soundBank[13]);
            }
            if (gameendingframes == 30) {
                playSFX(&soundBank[4]);
                gameended = 1;
            }
        }
        if (situation == 3) {// CT Win Via Defuse
            if (gameendingframes == 4) {
                playSFX(&soundBank[13]);
            }
            if (gameendingframes == 10) {
                playSFX(&soundBank[6]);
            }
            if (gameendingframes == 115) {
                playSFX(&soundBank[13]);
            }
            if (gameendingframes == 144) {
                playSFX(&soundBank[13]);
            }
            if (gameendingframes == 150) {
                playSFX(&soundBank[7]);
            }
            if (gameendingframes == 180) {
                playSFX(&soundBank[4]);
            }
            if (gameendingframes == 270) {
                playSFX(&soundBank[13]);
                gameended = 1;
            }
        }
        if (situation == 4 || situation == 5) {// CT Win Via Kills OR Time
            if (gameendingframes == 4) {
                playSFX(&soundBank[13]);
            }
            if (gameendingframes == 10) {
                playSFX(&soundBank[7]);
            }
            if (gameendingframes == 40) {
                playSFX(&soundBank[4]);
            }
            if (gameendingframes == 130) {
                playSFX(&soundBank[13]);
                gameended = 1;
            }
        }
    }
}
void timering(void) {
    if (cooldownfootstep > 0) {cooldownfootstep--;}
    timer++;
    if (timer == 60) {
        timer = 0;
        seconds--;
        srand(seconds);
    }
    if (seconds == -1) {
        seconds = 59;
        minutes--;
        srand(minutes * 55);
    }
    if (minutes == -1) {
        gameendingvar = 5; 
        minutes = 0;
        seconds = 0;
    }
}
void changeWeapons(int mainOc4) {
    if (mainOc4 == 1) {
        if (curweapon == 1) {
            curweapon = 2;
            if (shotgunW == 1) {
                basedamage = 26;
                priammo = 8;
                weight = 2;
                cooldownsample = 60;
                reloadsample = 25;
                if (initW == 1) {
                    bulletsprimary = 8;
                }
            }
            if (shotgunW == 2) {
                basedamage = 14;
                priammo = 7;
                weight = 2;
                cooldownsample = 15;
                reloadsample = 25;
                if (initW == 1) {
                    bulletsprimary = 7;
                }
            }
            if (subMachineWT == 1) {
                basedamage = 9;
                priammo = 30;
                weight = 1;
                cooldownsample = 5;
                reloadsample = 90;
                if (initW == 1) {
                    bulletsprimary = 30;
                }
            }
            if (subMachineWCT == 1) {
                basedamage = 9;
                priammo = 30;
                weight = 1;
                cooldownsample = 5;
                reloadsample = 60;
                if (initW == 1) {
                    bulletsprimary = 30;
                }
            }
            if (subMachineWT == 2 || subMachineWCT == 2) {
                basedamage = 13;
                priammo = 30;
                weight = 2;
                cooldownsample = 8;
                reloadsample = 90;
                if (initW == 1) {
                    bulletsprimary = 30;
                }
            }
            if (subMachineWT == 3 || subMachineWCT == 3) {
                basedamage = 11;
                priammo = 50;
                weight = 2;
                cooldownsample = 5;
                reloadsample = 90;
                if (initW == 1) {
                    bulletsprimary = 50;
                }
            }
            if (subMachineWT == 4 || subMachineWCT == 4) {
                basedamage = 15;
                priammo = 25;
                weight = 2;
                cooldownsample = 9;
                reloadsample = 90;
                if (initW == 1) {
                    bulletsprimary = 25;
                }
            }
            if (rifleWT == 1) {
                basedamage = 13;
                priammo = 35;
                weight = 2;
                cooldownsample = 8;
                reloadsample = 60;
                if (initW == 1) {
                    bulletsprimary = 35;
                }
            }
            if (rifleWT == 2) {
                basedamage = 22;
                priammo = 30;
                weight = 2;
                cooldownsample = 8;
                reloadsample = 90;
                if (initW == 1) {
                    bulletsprimary = 30;
                }
            }
            if (rifleWT == 3) {
                basedamage = 24;
                priammo = 30;
                weight = 2; //Zoom
                cooldownsample = 9;
                reloadsample = 90;
                if (initW == 1) {
                    bulletsprimary = 30;
                }
            }
            if (rifleWCT == 1) {
                basedamage = 14;
                priammo = 25;
                weight = 2;
                cooldownsample = 8;
                reloadsample = 60;
                if (initW == 1) {
                    bulletsprimary = 35;
                }
            }
            if (rifleWCT == 2) {
                basedamage = 22;
                priammo = 30;
                weight = 2;
                cooldownsample = 8;
                reloadsample = 60;
                if (initW == 1) {
                    bulletsprimary = 30;
                }
            }
            if (rifleWCT == 3) {
                basedamage = 24;
                priammo = 30;
                weight = 2; //Zoom
                cooldownsample = 9;
                reloadsample = 120;
                if (initW == 1) {
                    bulletsprimary = 30;
                }
            }
            if (rifleWT == 4 || rifleWCT == 4) {
                basedamage = 45;
                priammo = 10;
                weight = 1; //Zoom
                cooldownsample = 60;
                reloadsample = 60;
                if (initW == 1) {
                    bulletsprimary = 10;
                }
            }
            if (rifleWT == 5 || rifleWCT == 5) {
                basedamage = 50;
                priammo = 10;
                weight = 3; //Zoom
                cooldownsample = 126;
                reloadsample = 120;
                if (initW == 1) {
                    bulletsprimary = 10;
                }
            }
            if (rifleWT == 6 || rifleWCT == 6) {
                basedamage = 32;
                priammo = 20;
                weight = 3; //Zoom
                cooldownsample = 14;
                reloadsample = 120;
                if (initW == 1) {
                    bulletsprimary = 20;
                }
            }
            if (rifleWT == 7 || rifleWCT == 7) {
                basedamage = 30;
                priammo = 30;
                weight = 3; //Zoom
                cooldownsample = 14;
                reloadsample = 120;
                if (initW == 1) {
                    bulletsprimary = 30;
                }
            }
            if (machineW == 1) {
                basedamage = 15;
                priammo = 100;
                weight = 3;
                cooldownsample = 5;
                reloadsample = 120;
                if (initW == 1) {
                    bulletsprimary = 100;
                }
            }
        } else {
            curweapon = 1;
            if (subW == 1) {
                basedamage = 24;
                subammo = 12;
                weight = 1;
                cooldownsample = 20;
                reloadsample = 60;
                if (initW == 1) {
                    bulletssub = 12;
                }
            }
            if (subW == 2) {
                basedamage = 21;
                subammo = 20;
                weight = 1;
                cooldownsample = 20;
                reloadsample = 60;
                if (initW == 1) {
                    bulletssub = 20;
                }
            }
            if (subW == 3) {
                basedamage = 34;
                subammo = 7;
                weight = 1;
                cooldownsample = 30;
                reloadsample = 60;
                if (initW == 1) {
                    bulletssub = 7;
                }
            }
            if (subW == 4) {
                basedamage = 22;
                subammo = 13;
                weight = 1;
                cooldownsample = 20;
                reloadsample = 90;
                if (initW == 1) {
                    bulletssub = 13;
                }
            }
            if (subW == 5) {
                basedamage = 22;
                subammo = 15;
                weight = 1;
                cooldownsample = 15;
                reloadsample = 90;
                if (initW == 1) {
                    bulletssub = 15;
                }
            }
            if (subW == 6) {
                basedamage = 18;
                subammo = 20;
                weight = 1;
                cooldownsample = 10;
                reloadsample = 120;
                if (initW == 1) {
                    bulletssub = 20;
                }
            }
        }
        if (curweapon != 1 && curweapon != 2) {
            curweapon = 2;
        }
    } else {
        if (bombplanted == 0 && curweapon != 3) {
            curweapon = 3; //C4
            weight = 1;
            
        } else {
            curweapon = 0; //Knife
            weight = 1;
        }
    }
}
void moving(void) {
    if (collision == 0) {
        if (Framecounter%weight == 0) { //Need to depend on the weapon's weight (1's for knife )
            if (curup == 1 && curleft == 1) {
            }
            if (curup == 1) {
                move = 1;
                curY++;
            } 
            if (curdown == 1) {
                move = 1;
                curY--;
            } 
            if (curright == 1) {
                move = 1;
                curX++;
            }
            if (curleft == 1) {
                move = 1;
                curX--;
            }
            if (curleft == 0 && curup == 0 && curdown == 0 && curright == 0) {move = 0;}
        }

        if (curup == 1) {if (curleft == 1) {curdir = 6;} else {if (curright == 1) {curdir = 5;} else {curdir = 1;}}}
        if (curdown == 1) {if (curleft == 1) {curdir = 8;} else {if (curright == 1) {curdir = 7;} else {curdir = 2;}}}
        if (curleft == 1) {if (curdown == 0 && curup == 0) {curdir = 4;}}
        if (curright == 1) {if (curdown == 0 && curup == 0) {curdir = 3;}}
    } else {
        //only that for the moment
        if (curdir == 4 || curdir == 8 || curdir == 6) {curX++;}
        else if (curdir == 3 || curdir == 7 || curdir == 5) {curX--;}

        if (curdir == 1 || curdir == 6 || curdir == 5) {curY--;}
        else if (curdir == 2 || curdir == 7 || curdir == 8) {curY++;}
        //move = 0;
    }
   
    if (curX > collisionbombAx1 && curX < collisionbombAx2 && curY > collisionbombAy1 && curY < collisionbombAy2 || curX > collisionbombBx1 && curX < collisionbombBx2 && curY > collisionbombBy1 && curY < collisionbombBy2) {insite = 1;} else {insite = 0;}
    if (move == 1) {
        if (cooldownfootstep == 0) {
            SpuSetKey(SPU_ON, SPU_08CH);
            playSFX(&soundBank[8]);
            cooldownfootstep = 19;
        }
    }
}
void healthandshit(void) {
    if (health == 0 && dead == 0) {
        dead = 1;
    }
    if (dead == 1) {
        health = 0;
        protection = 0;//In case you want to test things out
    }
}
int main(void)
{
    //player ?
    POLY_F4 *polyplayer = {0};                        

    SVECTOR RotVectorplayer = {0, 0, 0};               
    VECTOR  MovVectorplayer = {0, 0, CENTERX, 0};
    VECTOR  ScaleVectorplayer = {ONE, ONE, ONE};       

    SVECTOR VertPosplayer[4] = {     
            {-12, -12, 1 },                       
            {-12,  12, 1 },                       
            { 12, -12, 1 },                        
            { 12,  12, 1  }                                  
        };        

    MATRIX PolyMatrixplayer = {0};  

    SPRT * DEFAULT_DUSTT;       
    DR_TPAGE * DUSTT;           

    SPRT * TESTMAPP;            
    DR_TPAGE * TESTMAPPING;     

    SPRT * B;
    DR_TPAGE * BB;

    SPRT * DAWALL;
    DR_TPAGE * DAWAALL;

    SPRT * TEST;
    DR_TPAGE * TESTTT;

    SPRT * bombb;
    DR_TPAGE * bbomb;

    long polydepth;
    long polyflag;
    long OTz;

    u_long spu_address;
    // Init Spu
    SpuInit();
    // Init sound settings
    initSnd();

    // Beware that the SPU only has 512KB of RAM so a 'soundbank' should stay within that limit.
    // If more is needed, a second soundbank could be loaded on demand.
    init();                         // execute init()

    int pad, oldpad;

    PadInit(0);   

    if (map == 0) {
        LoadTexture(_binary_tim_DEFAULT_DUST_tim_start, &DEFAULT_DUST);
        LoadTexture(_binary_tim_TESTMAP_tim_start, &TESTMAP);
        LoadTexture(_binary_tim_DUSTTILE_tim_start, &DUST);  
        LoadTexture(_binary_tim_wall_tim_start, &wwall);  
        //curX > 0 && curX < 40 && curY > 10 && curY < 50
        collisionbombAx1 = 0;
        collisionbombAx2 = 40;
        collisionbombAy1 = 10;
        collisionbombAy2 = 50;
    }
    LoadTexture(_binary_tim_B_tim_start, &BLOGO);
    LoadTexture(_binary_tim_bomb_tim_start, &bomb);  

    for (u_short vag = 0; vag < VAG_NBR; vag++ ){
        soundBank[vag].spu_address = setSPUtransfer(&soundBank[vag]);
    }
    changeWeapons(1); //BUG if you don't have that with ammos and shit
    changeWeapons(1); 
    initW = 0;

    while (1)                       // infinite loop
    {   
        /*
        if (move == 1) {
            if (RotVectorplayer.vz < rotat + 60) {
                if (bobing != 15) {
                    bobing++;
                }
                RotVectorplayer.vz = RotVectorplayer.vz + bobing;
            } else {
                if (bobing != -15) {
                    bobing--;
                }
                RotVectorplayer.vz = RotVectorplayer.vz + bobing;
            }
            if (RotVectorplayer.vz == 0 || RotVectorplayer.vz == 90 || RotVectorplayer.vz == 180 || RotVectorplayer.vz == 270) {
                RotVectorplayer.vz = rotat;
            }

            RotVectorplayer.vz = rotat;

        } //else {RotVectorplayer.vz = rotat;} Bruh
        */
        Framecounter++;
        pad = PadRead(0);
        ClearOTagR(ot[db], OTLEN);

        timering();
        healthandshit();

        if (dead == 0) {
            moving();
            collisions();

            polyplayer = (POLY_F4 *)nextpri;                    
            // Set transform matrices for this polygon
            RotMatrix(&RotVectorplayer, &PolyMatrixplayer);     
            TransMatrix(&PolyMatrixplayer, &MovVectorplayer);
            ScaleMatrix(&PolyMatrixplayer, &ScaleVectorplayer); 
            SetRotMatrix(&PolyMatrixplayer);                    
            SetTransMatrix(&PolyMatrixplayer);                  
            setPolyF4(polyplayer);                              
            setRGB0(polyplayer, 0, 0, 0);                
            OTz = RotTransPers4(
                        &VertPosplayer[0],      &VertPosplayer[1],      &VertPosplayer[2],      &VertPosplayer[3],
                        (long*)&polyplayer->x0, (long*)&polyplayer->x1, (long*)&polyplayer->x2, (long*)&polyplayer->x3,
                        &polydepth,
                        &polyflag
                        );                               
            //RotVector.vy += 4;
            //RotVector.vz += 4;                         
            addPrim(ot[db], polyplayer);                 
            nextpri += sizeof(POLY_F4);                  
        }
    
        if (bombplanted == 1 && timerbomb != 0) {
            if (bombX == 9999) {
                bombX = CENTERX + curX * 2 - 10;
                bombY = CENTERY - curY * 2;
            }
            bombb = (SPRT *)nextpri; 
            setSprt(bombb);
            setRGB0(bombb, 128, 128, 128);        
            setXY0(bombb, bombX - curX * 2, bombY + curY * 2);
            setWH(bombb, 16, 8 );
            addPrim(ot[db], bombb);
            nextpri += sizeof(SPRT);
            bbomb = (DR_TPAGE*)nextpri;
            setDrawTPage(bbomb, 0, 1,getTPage(bomb.mode&0x3, 0,bomb.prect->x, bomb.prect->y ));
            addPrim(ot[db], bbomb);                  
            nextpri += sizeof(DR_TPAGE);    
        }

        //Only a square for the moment

        if (map == 0) {
            TEST = (SPRT *)nextpri; 
            setSprt(TEST);
            setRGB0(TEST, 128, 128, 128);        
            setXY0(TEST, -100 - curX * 2, 0 + curY * 2);
            setWH(TEST, 32, 32 );
            setClut(TEST, DUST.crect->x, DUST.crect->y);
            addPrim(ot[db], TEST);
            nextpri += sizeof(SPRT);
            TESTTT = (DR_TPAGE*)nextpri;
            setDrawTPage(TESTTT, 0, 1,getTPage(DUST.mode&0x3, 0,DUST.prect->x, DUST.prect->y )+32);
            addPrim(ot[db], TESTTT);                  
            nextpri += sizeof(DR_TPAGE);    

            B = (SPRT *)nextpri; //FUNNI THING, we can't place 2 sprites on the same layer without the older one getting "deleted" :(
            setSprt(B);
            setRGB0(B, 128, 128, 128);        
            setXY0(B, 160 - curX * 2, 20 + curY * 2);
            setWH(B, 80, 80 );
            setClut(B, BLOGO.crect->x, BLOGO.crect->y);
            addPrim(ot[db], B);
            nextpri += sizeof(SPRT);
            BB = (DR_TPAGE*)nextpri;
            setDrawTPage(BB, 0, 1,getTPage(BLOGO.mode&0x3, 0,BLOGO.prect->x, BLOGO.prect->y));
            addPrim(ot[db], BB);                  
            nextpri += sizeof(DR_TPAGE);            
        

            DAWALL = (SPRT *)nextpri; 
            setSprt(DAWALL);
            setRGB0(DAWALL, 128, 128, 128);        
            setXY0(DAWALL, -225 - curX * 2, 100 + curY * 2);
            setWH(DAWALL, 225, 150 );
            setClut(DAWALL, wwall.crect->x, wwall.crect->y);
            addPrim(ot[db], DAWALL);
            nextpri += sizeof(SPRT);
            DAWAALL = (DR_TPAGE*)nextpri;
            setDrawTPage(DAWAALL, 0, 1,getTPage(wwall.mode&0x3, 0,wwall.prect->x, wwall.prect->y));
            addPrim(ot[db], DAWAALL);                  
            nextpri += sizeof(DR_TPAGE);    

            TESTMAPP = (SPRT *)nextpri;
            setSprt(TESTMAPP);
            setRGB0(TESTMAPP, 128, 128, 128);        
            setXY0(TESTMAPP, -50 - curX * 2, -50 + curY * 2);
            setWH(TESTMAPP, 500, 500 );
            setClut(TESTMAPP, TESTMAP.crect->x, TESTMAP.crect->y);
            addPrim(ot[db], TESTMAPP);
            nextpri += sizeof(SPRT);
            TESTMAPPING = (DR_TPAGE*)nextpri;
            setDrawTPage(TESTMAPPING, 0, 1,getTPage(TESTMAP.mode&0x3, 0,TESTMAP.prect->x, TESTMAP.prect->y));
            addPrim(ot[db], TESTMAPPING);                  
            nextpri += sizeof(DR_TPAGE);       
        }
        FntPrint("Hello CS ! %d %d %d pos %d %d %d %d %d %d\n", reloadframes, delay, curweapon, curX, curY, curup, curdown, curleft, curright);  // Send string to print stream
        if (seconds < 10) {
            FntPrint("Time left %d:0%d, T : %d, CT : %d\n", minutes, seconds, terroristnum, counterterroristnum);
        } else {
            FntPrint("Time left %d:%d, T : %d, CT : %d\n", minutes, seconds, terroristnum, counterterroristnum);
        }
        FntPrint("Ammo Primary %d, sub %d\n", bulletsprimary, bulletssub);
        FntPrint("Cur Dir %d\n", curdir);
        FntPrint("Health %d, Protection %d\n", health, protection);

        if (cooldownweapon > 0) {cooldownweapon--;}
        if (reloading == 1) {
            if (curweapon == 1) {reload();}
            if (curweapon == 2) {reload();}
        }
        if (counterterroristnum == 0) {gameendingvar = 2;}
        if (terroristnum == 0) {gameendingvar = 4;}
        if (gameendingvar != 0) {gameending(gameendingvar);}



        if (pad & PADLleft) {
            curleft = 1;
        } else {curleft = 0;}
        if (pad & PADLright) {
            curright = 1;
        } else {curright = 0;}
        if (pad & PADLup) {
            curup = 1;
        } else {curup = 0;}
        if (pad & PADLdown) {
            curdown = 1;
        } else {curdown = 0;}


        if (pad & PADR2) { 
            if (curweapon == 2) { 
                //Main weapon
                //AK
                if (cooldownweapon == 0 && reloading == 0) {
                    if (bulletsprimary > 0) {
                        if (defusing == 0) {
                            shoot();
                            cooldownweapon = cooldownsample;
                        }
                    } else {
                        reloading = 1;
                        weapo = 2;
                    }
                }
            }
            if (curweapon == 1) {
                //Sub weapon
                //Glock
                if (cooldownweapon == 0 && reloading == 0) {
                    if (bulletssub > 0) {
                        if (defusing == 0) {
                            shoot();
                            cooldownweapon = cooldownsample;
                        }
                    } else {
                        reloading = 1;
                        weapo = 1;
                    }
                }
            }
            if (curweapon == 0) {
                if (cooldownweapon == 0) { //Knife big attack
                    if (defusing == 0) {
                        shoot(); //'shoot'
                        cooldownweapon = 20;
                    }
                }
            }
        }

        if (pad & PADL2) { //Special things from weapons

            if (!(oldpad & PADL2)) {
                if (curweapon == 1) {//Switch to burst mode 
                    if (subW == 2) {
                        if (burstmode == 0) {burstmode = 1;} else {burstmode = 0;}
                    }
                }  
            }
            if (curweapon == 0) {
                if (cooldownweapon == 0) {
                    if (defusing == 0) {
                        shoot(); //'shoot'
                        cooldownweapon = 70;
                    }
                }
            }
        }

        if (pad & PADRright) {
            if (curweapon == 2) {
                if (bulletsprimary != 30) {
                    reloading = 1;
                }
            } 
            if (curweapon == 1) {
                if (bulletssub != 20) {
                    reloading = 1;
                }
            }
        }

        if (pad & PADRup && !(oldpad & PADRup)) {
            changeWeapons(1);
        }

        if (pad & PADL1 && !(oldpad & PADL1)) {
            changeWeapons(2);
        }

        if (bombplanted == 0) {
            if (pad & PADR2 && terrorist == 1 && insite == 1 && curweapon == 3 && move == 0) {
                planting++;
                Ran(25);
                if (RAN == 1 && plantingrand > 0) {
                    playSFX(&soundBank[0]);
                    plantingrand--;
                }
                if (planting == 240) {
                    bombplanted = 1;
                    planting = 0;
                    curweapon = 0;
                }
            } else {planting = 0; plantingrand = 4;}
        }
        if (bombplanted == 1) {
            if (timerbomb == 2700) {
                playSFX(&soundBank[9]);
                minutes = 0;
                seconds = 45;
            }
            if (timerbomb != 0) {
                if (defused == 0) {
                    if (bombplantedsaid == 0) {
                        bombplantedframes++;
                        if (bombplantedframes == 1) {
                            playSFX(&soundBank[13]);
                        }
                        if (bombplantedframes == 7) {
                            playSFX(&soundBank[1]);
                        }
                        if (bombplantedframes == 100) {
                            bombplantedsaid = 1;
                        }
                    }
                    timerbomb--;
                    if (timerbomb%2 == 0 || timerbomb%3 == 0 || timerbomb%7 == 0 || timerbomb%19 == 0) {
                        delay--;
                    }
                    if (delay < 0) {
                        playSFX(&soundBank[0]);
                        delay = olddelay;
                        olddelay = olddelay - 1;
                    }      
                    if (pad & PADstart && terrorist == 1 && insite == 1) {
                        defusing++;
                        if (defusing == 1) {
                            playSFX(&soundBank[5]);
                            SpuSetVoiceVolume(5, 0x2200, 0x2200);
                        }
                        if (defusingkit == 1) {
                            if (defusing == 360) {defused = 1;}
                        } else {
                            if (defusing == 720) {defused = 1;}
                        }
                    } else {defusing = 0;}
                } else {
                    gameendingvar = 3;
                    seconds = 0;
                    defusing = 0;
                }
            } else {
                gameendingvar = 1;
            }
            if (timerbomb > 2460) {FntPrint("Bomb has been planted.");}
        }

        // Reset oldpad    
        if (!(pad & PADRdown) ||
            !(pad & PADRup)   ||
            !(pad & PADRleft) ||
            !(pad & PADRright) ||
            !(pad & PADLdown) ||
            !(pad & PADLup)   ||
            !(pad & PADLleft) ||
            !(pad & PADLright)
            ){ oldpad = pad; }
        FntFlush(-1);               // Draw printe stream
        display();                  // Execute display()
    }
    return 0;
}
