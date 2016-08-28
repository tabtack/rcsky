//-----------------------------------------------------------------------------
// File: RCsimu3.cpp
//
// Copyright (C) 1999-2009 TabTack. All Rights Reserved.
//-----------------------------------------------------------------------------

// for Visual C++ 6.0 and DirectX 9.0 SDK (2009.12 D3DRM3 -> D3DRM2)
 
// ガタガタの改善

// アイコンの変え方
// リソース名とLoadIcon名を一致させる(大文字小文字は区別される)
// アイコンは32x32、16x16両方作る（アイコンエディタの右のアイコンを押す）

// 半透明物体：elfeeniでは最後に作ること

// Link : winmm.lib

#define SAFE_RELEASE(x) if (x != NULL) {x->Release(); x = NULL;}
#define MSG(str)		MessageBox( NULL, str, "Application Message", MB_OK )
#define DISPLAYMSG(x)	MessageBox(NULL, x, "D3DRM Sample", MB_OK);

#define WMAX	3072
#define MMAX	12		// model numbers
#define HELI	5		// heli or plane (not used)
#define GROUND	-10.0

#include <conio.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <windows.h>   // Standard Windows header file 
#include <direct.h>    // DirectDraw definitions 
#include <d3drm.h>     // D3DRM definitions 
#include <mmsystem.h>  // winmm.lib
#include "midiout.h" 

// Global Variables 
extern HWND win;
extern WORD g_wMouseButtons;         // mouse button state
extern WORD g_wMouseX;               // mouse cursor x position
extern WORD g_wMouseY;               // mouse cursor y position
extern int  g_wWheel;                // mouse wheel
extern char cmdline_save[64];

extern LPDIRECT3DRMVIEWPORT g_pViewport;  // D3DRM viewport
extern LPDIRECT3DRMFRAME2    g_pScene;     // Master frame in which others are placed
extern LPDIRECT3DRMFRAME2    g_pCamera;    // Frame describing the users POV

extern LPDIRECT3DRMFRAME2 *cframe;
extern LPDIRECT3DRMFRAME2 childframe;
extern LPDIRECT3DRMFRAME2 hoverframe;
extern LPDIRECT3DRMFRAME2 gliderframe;
extern LPDIRECT3DRMFRAME2 trainframe;
extern LPDIRECT3DRMFRAME2 stuntframe;
extern LPDIRECT3DRMFRAME2 baloonframe;
extern LPDIRECT3DRMFRAME2 cosmo0frame;
extern LPDIRECT3DRMFRAME2 heli1frame;
extern LPDIRECT3DRMFRAME2 heli3dframe;
extern LPDIRECT3DRMFRAME2 hel450frame;
extern LPDIRECT3DRMFRAME2 helbeeframe;
extern LPDIRECT3DRMFRAME2 penginframe;
extern LPDIRECT3DRMFRAME2 ufoframe;
extern LPDIRECT3DRMFRAME2 shadow0frame, shadow1frame, shadow2frame;
extern LPDIRECT3DRMFRAME2 shadow_1frame, shadow_2frame;
extern LPDIRECT3DRMFRAME2 fieldframe;

// original
D3DVECTOR xx, yy, zz;	// Global for debug
JOYCAPS joycap;
int joydata[5], joysex[5], joymax[5], joymin[5], joyon[5];
int thr_type=0, mode_no=0, zoom_no=0;
int JoyM[5]   = {1, 2, 4, 3, 0};
int PropoM[6] = {1, 2, 3, 0, 4, 5};
float JoyMul = 1.0;
//int power=0;
float power=0;	// 2008.2.23 ヘリのホバリングを滑らかにするため
int first=1;
int on_ground=0, crash_on=0;
int JR_propo=0, mode=0, beep_on=0, camera=0, zoom=0;
int model=0, weather=0, kaze=0;
extern int	pertime;
extern int	pcmlevel;
extern int	JRstart;
extern int	ch[6];
extern int	cht, cho;
float fx, fy, fz;
float vx, vy, vz, v, u;
float rx, ry, rz;
float vv=0.0, uu=0.0;
float chf[6];

// 機体パラメータ：param.txtがあればそちらを優先
//          12         glider trainer stunt balloon cosmo0 hover heli1 heli3d ep450 epBee penguin UFO
int   model_t[MMAX] = {    0 ,    0 ,    0 ,    0  ,    0 ,   0 ,   1 ,   1  ,   1 ,   1 ,   1 ,   1 };
int   voice[MMAX]   = {   23 ,   44 ,   52 ,   44  ,   23 ,  44 ,  52 ,  21  ,  21 ,  52 , 102 ,  97 };	// Lo Freq.
int   tone[MMAX]    = {   76 ,   70 ,   52 ,   40  ,  100 ,  90 ,  64 ,  64  ,  80 ,  80 ,  64 ,  64 };
int   shadow[MMAX]  = {    2 ,    1 ,    1 ,    2  ,    2 ,   1 ,   1 ,   1  ,   0 ,   0 ,   2 ,   2 };
float zoom_x[MMAX]  = {  1.0f,  1.0f,  1.0f,  1.0f ,  1.0f, 1.0f, 1.0f, 1.0f , 1.0f, 1.0f, 1.0f, 1.0f};
float height[MMAX]  = {  0.4f,  0.7f,  1.2f,  5.2f ,  2.8f, 1.5f, 2.5f, 1.7f , 1.0f, 0.7f, 5.8f, 1.3f}; 
float weight[MMAX]  = {  0.2f,  0.4f,  0.5f,  0.05f,  1.0f, 0.4f, 0.4f, 0.5f , 0.6f, 0.4f, 0.3f, 0.2f};
float hv_ratio[MMAX]= {  5.0f,  5.0f,  5.0f,  5.0f,   5.0f, 5.0f, 5.0f, 5.0f ,20.0f,15.0f, 5.0f, 5.0f};
float crash[MMAX]	= { 10.0f, 15.0f,  5.0f,  7.0f , 30.0f,15.0f,10.0f, 7.0f , 5.0f, 4.0f,15.0f,20.0f};
float maxpower[MMAX]= {  0.3f,  0.6f,  1.1f,  0.15f,  3.0f, 0.5f, 0.25f,0.5f , 3.0f, 2.0f,0.17f, 0.2f};
float sens[MMAX]    = { 0.25f,  0.5f,  1.0f,  0.25f,  0.3f, 0.5f, 0.5f, 1.0f , 0.5f, 0.3f,0.25f, 1.5f}; 
float rudder_s[MMAX]= { 0.01f,0.015f, 0.02f,  0.01f,0.005f,0.05f, 0.2f, 0.7f , 1.0f, 0.7f, 0.3f, 1.0f}; 
float stable[MMAX]  = {  1.0f,  0.5f,  0.0f,  0.5f ,  0.0f, 0.6f, 1.0f, 0.2f , 0.0f, 1.0f, 2.0f, 1.5f};
float air_res[MMAX] = {  0.5f,  1.5f,  1.0f,  3.0f ,  1.0f, 1.0f, 2.0f, 1.0f , 1.0f, 2.0f, 3.0f, 2.0f}; 
float w_size[MMAX]  = {  1.0f,  1.0f,  1.0f,  1.0f ,  1.0f, 5.0f, 0.0f, 0.0f , 0.0f, 0.0f, 0.0f, 0.0f}; 
float side_r[MMAX]  = {  0.5f,  1.0f,  2.0f,  3.0f ,  3.0f, 0.5f, 0.0f, 0.0f , 0.0f, 0.0f, 0.0f, 0.0f}; 
float lift_p[MMAX]	= {  1.0f,  2.0f,  1.5f,  0.5f ,  2.0f, 1.0f, 0.0f, 0.0f , 0.0f, 0.0f, 0.0f, 0.0f};
float pitchup[MMAX] = { 0.04f, 0.03f, 0.05f,  0.00f, 0.02f,0.04f, 0.0f, 0.0f , 0.0f, 0.0f, 0.0f, 0.0f};
float rotar[MMAX]   = {    0 ,    0 ,    0 ,    0  ,    0 ,   0 ,   1 ,   1  ,   1 ,   1 ,   1 ,  -1 };
float gyro[MMAX]    = {  0.0f,  0.0f,  0.0f,  0.0f,   0.0f, 0.0f,0.99f,0.99f , 0.9f, 0.8f, 1.0f, 0.9f};
char  name[MMAX][256]={ "plane0", "plane1", "plane2", "plane3", "plane4", "plane5", 
						"plane6", "plane7", "plane8", "plane9", "plane10", "plane11" };
// PCM
extern HWAVEIN	hWaveIn;
extern MMTIME	mmTime;

// MIDI
HMIDIOUT         hMidiOut ;
HANDLE           hGmem3 = NULL, hGmem4 = NULL ;
TIMECAPS         tc ;
UINT             nTimerID, nTimerRes ;
int              midi_ok;

// Prototypes 
void play_midi(long);
void stop_midi(void);

// ** original move planes

void reset_para(void)
{
	// フレーム位置・姿勢　設定		@1
	(*cframe)->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(GROUND+height[model]),D3DVAL(30));
	(*cframe)->SetOrientation(g_pScene,
    						    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(-1.0),
						        D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0));
	(*cframe)->SetVelocity(g_pScene, D3DVAL(0),D3DVAL(0),D3DVAL(0),FALSE);

	shadow_2frame->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	shadow_1frame->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	shadow0frame ->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	shadow1frame ->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	shadow2frame ->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));

	first = 1;
	power = 0.0;
	on_ground = crash_on = 0;
	fx = fy = fz = 0.0;
	vz = vy = vz = vv = uu = 0.0;
	g_wWheel = 0;

	sndPlaySound(NULL, SND_ASYNC);			// stop sound
	if(midi_ok)
	{
	    midiOutReset(hMidiOut);

		play_midi(0x0007b0);				// volume=0
		play_midi(0x0007b1);				// volume=0
		play_midi(0x6007b2);				// volume=96
		play_midi(0x6007b3);				// volume=96
		play_midi(0x7f07b4);				// volume=112
		play_midi(0x640ab0);				// pan = middle
		play_midi(0x640ab1);				// pan = middle
		play_midi(0x640ab2);				// pan = middle
		play_midi(0x640ab3);				// pan = middle
		play_midi(0x640ab4);				// pan = middle

		play_midi(0xc0 + 0x100L*voice[model]);	// Low frq.
		play_midi(0x780090L + 0x100L*tone[model]);
		play_midi(0x0065b0L);				// Pitch vend sensitivity
		play_midi(0x0064b0L);
		if(model_t[model]) play_midi(0x1806b0L);	// 2  oct
		else			   play_midi(0x0c06b0L);	// 1/2oct
//		play_midi(0x0026b0L);				// LSB not needs
	}
}

void get_power(unsigned char KeyBuffer[256])
{
	// speed変化
	if(JR_propo==3) power = 100.0*(ch[0] - cht)/36;	// 36:プロポ測定値
	else if(JR_propo==2 && joyon[3])
	{
		if(thr_type){	/* 100 to 0 to -100 type */
			long range= (joymax[3] - joymin[3])/2;
			long half = (joymax[3] + joymin[3])/2;
			if(joysex[3] > 0)
				power = (100.0 * (-(long)joymin[3] + joydata[3] - half)) / range;
			else
				power = (100.0 * ( (long)joymax[3] - joydata[3] - half)) / range;
		}
		else
		{				/* 100 to 0 type */
			long range= joymax[3] - joymin[3];
			if(joysex[3] > 0)
				power = (100.0 * (-(long)joymin[3] + joydata[3])) / range;
			else
				power = (100.0 * ( (long)joymax[3] - joydata[3])) / range;
		}
		power *= JoyMul;
	}
	else if(JR_propo==1)
	{
		if(g_wWheel > 0)
		{
			power += 5.0;if(power > 100.0) power = 100.0;
			g_wWheel = 0;
		}
		if(g_wWheel < 0)
		{
			power -= 5.0;if(power < 0.0) power = 0.0;
			g_wWheel = 0;
		}
	}
	// power-key is always active (for no throttle joystick)
	if(KeyBuffer[VK_UP]&0xf0){				// Upキー
		power = power + 5.0;
		if(power > 100.0) power = 100.0;	// max
	}
	if(KeyBuffer[VK_DOWN]&0xf0){			// Downキー
		power = power - 5.0;
		if(power < 0.0) power = 0.0;		// min
	}

	chf[0] = power;
	if(power>0.0 && first==1) first=0;
}

void play_sound(float power, int crashed, float dist)
{
	static int sound, sound0, crash0=0;
	int power_a = abs((int)power);

	//// sound play ////
	char currentDir[256];
	_getcwd(currentDir, 255);
	_chdir(currentDir);			// 何故かカレントが変わるから

	// beep_on 0:Mix 1:Midi 2:Wave

	if(crashed && !crash0)
	{
		if(beep_on==0 || beep_on==2)
			sndPlaySound("bon1.wav", SND_ASYNC);
		else
		{
			play_midi(0x100L*127+0xc4);	// PPcC : PP = program(voice 47), C=Midi Ch.
			play_midi(0xff3094L);		// VVNN9C : VV=Velocity, NN=Note No, C=Midi Ch. 
		}
	}
	else
	{
 		if(beep_on==2)				// 音を出すとプロポ入力ができないサウンドカードもある
		{
			sound = 0;
			if( 0<power_a && power_a<=33) sound = 1;
			if(33<power_a && power_a<=66) sound = 2;
			if(66<power_a             ) sound = 3;
			if(mode) sound = 3;

			if(sound != sound0)
			{
				struct stat st;
				char file_name[1030];

				sprintf(file_name, "%s.wav", name[model]);
				if(0==stat(file_name, &st)){
					if(sound<=1) sndPlaySound( NULL    , SND_LOOP | SND_ASYNC);	// winmm.lib
					if(sound>=2) sndPlaySound(file_name, SND_LOOP | SND_ASYNC);
				}
				else
				{
					if(sound==0) sndPlaySound( NULL    , SND_LOOP | SND_ASYNC);	// winmm.lib
					if(sound==1) sndPlaySound("ga1.wav", SND_LOOP | SND_ASYNC);
					if(sound==2) sndPlaySound("ga2.wav", SND_LOOP | SND_ASYNC);
					if(sound==3) sndPlaySound("ga3.wav", SND_LOOP | SND_ASYNC);
				}
			}
			sound0 = sound;
		}
		else if(midi_ok)
		{
			static int vol, vol0;
			sound = power_a/2;
			if(dist <1.0) dist  = 1.0;
			vol = 0;
			     if(model_t[model]) vol = 100.0/(dist/200.0);	// ヘリのローター音はパワーに関係なく一定
			else if(power_a>  0 ) vol = (power_a+32.0)/(dist/200.0);
			if(crashed) vol = 0;

			if(vol>100)
			{
				vol = 100;						// 120だとうるさいので
			}
			vol = (vol/5)*5;					// どうせ微妙な音量変化は分からないから
			if(sound!=sound0 || vol!=vol0) //|| wo!=wo0*)	// 距離で音量を（ドップラー効果？）
			{									// キーだとどうしても連続音にならん
				int key0;						// BG入れた方がいい？
				key0 = 0x0000e0L + 0x10000L*(sound+14);	// pitch vend
				play_midi(key0);
				key0 = 0x07b0L + 0x10000L*vol;	// volume change
				play_midi(key0);
			}

			if(rand()%200 == 0)	
			{
				// sea sound (not in Vista)
				play_midi(0x100L*122+0xc2);
				play_midi(0x204092L);
			}
			if(rand()%1000 == 0)
			{
				// bird sound (not in Vista)
				play_midi(0x100L*123+0xc3);
				play_midi(0x204093L);
			}
			sound0 = sound;
			vol0 = vol;
		}
	}

	crash0 = crashed;
}

BOOL move_plane(void)
{
	D3DVECTOR pp;
	JOYINFOEX joy;
	unsigned char KeyBuffer[256];
	char buf[1024];

	if(first) reset_para();

	(*cframe)->GetPosition(g_pScene, &pp);
	float distance = (float)sqrt(pp.x*pp.x + pp.y*pp.y + pp.z*pp.z);

	// キーボードの状態を読み取る
	int joybutton = 0;
	GetKeyboardState(KeyBuffer);
	joy.dwSize = sizeof(joy);
	joy.dwFlags= JOY_RETURNALL;
	if(JR_propo==2) 
	{
		if(joyGetPosEx(JOYSTICKID1, &joy) == JOYERR_NOERROR)	// if err, all will be 0
		{
			joybutton = joy.dwButtons;
			for(int i=0; i<5; i++)
			{
				int j=JoyM[i];

				joysex[i] = 1;
				if(j<0)
				{
					j = -j;
					joysex[i] = -1;
				}

				switch(j)
				{
				case 1: joydata[i]= joy.dwXpos;		// X
						joymax[i] = joycap.wXmax;
						joymin[i] = joycap.wXmin;
						joyon[i]  = 1;
						break;
				case 2: joydata[i]= joy.dwYpos;		// Y
						joymax[i] = joycap.wYmax;
						joymin[i] = joycap.wYmin;
						joyon[i]  = 1;
						break;
				case 3: joydata[i]= joy.dwZpos;		// Throttle
						joymax[i] = joycap.wZmax;
						joymin[i] = joycap.wZmin;
						joyon[i]  = joycap.wCaps & JOYCAPS_HASZ;
						break;
				case 4: joydata[i]= joy.dwRpos;		// Rudder
						joymax[i] = joycap.wRmax;
						joymin[i] = joycap.wRmin;
						joyon[i]  = joycap.wCaps & JOYCAPS_HASR;
						break;
				case 5: joydata[i]= joy.dwUpos;		// 5th
						joymax[i] = joycap.wUmax;
						joymin[i] = joycap.wUmin;
						joyon[i]  = joycap.wCaps & JOYCAPS_HASU;
						break;
				case 6:joydata[i]= joy.dwVpos;		// 6th
						joymax[i] = joycap.wVmax;
						joymin[i] = joycap.wVmin;
						joyon[i]  = joycap.wCaps & JOYCAPS_HASV;
						break;
				default:joydata[i]=  0;				// 0
						joymax[i] =  1;
						joymin[i] = -1;
						joyon[i]  =  0;

				}
			}
		}
	}

	get_power(KeyBuffer);
	if(crash_on) power=0.0;

	/*--------  移動の操作  --------*/

	// 移動ベクトルをリセットする。
	(*cframe)->SetVelocity(g_pScene, D3DVAL(0),D3DVAL(0),D3DVAL(0),FALSE);

	// ch5 （ピッチ：ヘリのみ）
	u = 0.0;
	if(JR_propo==2 && JoyM[4]!=0 && joyon[4]){
		long half = ((long)joymax[4] + joymin[4])/2L;
		long range= (joymax[4] - joymin[4])/2L;
		u = joysex[4]*((float)joydata[4] - half) / range;
		if(JoyM[4]<0) u = -u;
		u *= JoyMul;
	}
	else if(JR_propo==3){
		u = (ch[4] - cho)/18.0F;		// 18:プロポ測定値
		if(PropoM[4]<0) u = -u;			// ピッチはReverseできないプロポがあるため
	}
	chf[4] = u;

	// 地面
	if(pp.y <= GROUND+height[model]+0.01)
	{
		if(on_ground==0 && vy>crash[model]) crash_on = 1;
		on_ground=1;
		pp.y = GROUND+height[model];					// 地面より下には沈まない
		if(vy<0.0) vy = 0.0;
		vx = 0.98*vx; vy = 0.98*vy; vz = 0.98*vz;		// 地面抵抗で減速

		if(power<10.0 && vv<1.0)
		{
			on_ground = 2;
			vx = vy = vz = 0.0;			// 静止摩擦
//			chf[2] =   zz.y/20.0;		// 復元力（エレベーター）
//			chf[1] = - xx.y/20.0;		// 復元力（エルロン）
		}
	}
	else on_ground=0;

	play_sound(power, crash_on, distance);

	(*cframe)->SetPosition(g_pScene, pp.x, pp.y, pp.z);
	if(D3DRM_OK !=  (*cframe)->GetOrientation(g_pScene, &zz, &yy) )
						yy.x = yy.y = yy.z = zz.x = zz.y = zz.z = 0.0;

	// カメラ位置と姿勢の設定
	if(camera)
	{	// ここは機体の長さを考慮した方がいいが
		g_pCamera->SetPosition(g_pScene, pp.x-15*zz.x, pp.y-15*zz.y, pp.z-15*zz.z);
		g_pCamera->SetOrientation(g_pScene, -zz.x, -zz.y, -zz.z, yy.x, yy.y, yy.z);
	}
	else if(zoom == 1)
	{	// Direct3D RMはズームが無いみたいなので、近寄って代用
		float ss = 1.0-0.25/zoom_x[model];
		g_pCamera->SetPosition(g_pScene, ss*pp.x, ss*pp.y, ss*pp.z);
		g_pCamera->SetOrientation(g_pScene, pp.x, pp.y, pp.z, 0, 1.0, 0);
	}
	else if(zoom == 2)	
	{	// Auto Zoom
		float zf = 1.25-(1.25/800.0)*distance;
		if(zf > 1.0 ) zf=1.0;
		if(zf < 0.25) zf=0.25;

		float ss = 1.0-zf/zoom_x[model];
		g_pCamera->SetPosition(g_pScene, ss*pp.x, ss*pp.y, ss*pp.z);
		g_pCamera->SetOrientation(g_pScene, pp.x, pp.y, pp.z, 0, 1.0, 0);
	}
	else
	{
		float ss = 1.0-1.0/zoom_x[model];
		g_pCamera->SetPosition(g_pScene, ss*pp.x, ss*pp.y, ss*pp.z);
//		g_pCamera->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(0.0),D3DVAL(0.0));
		g_pCamera->SetOrientation(g_pScene, pp.x, pp.y, pp.z, 0, 1.0, 0);
	}

	// 力より速度、速度より位置でコントロールしたほうがわかりやすい
	// 旋廻時の機種下げ、失速（低速でも宙返りできるのは...）
	// 機首上げ力：速度に比例（弱いが）
	// 失速：迎角と速度に関係
	// 遠心力
	// ホッピングの問題
	// 飛行機：ナイフエッジ？
	// より慣性を利かす
	// ヘリ：ローター回転（パワー）に比例して梶が利く
	// ヘリ：速度に比例して翼効果ができる
	// ヘリ：ストールターン？
	// ヘリ：ループがいまいち
	u = vx*zz.x + vy*zz.y + vz*zz.z;				// 前方向速度
	if(u>0) vv= sqrt(abs(u)); else vv = -sqrt(abs(-u));
	u = vx*xx.x + vy*xx.y + vz*xx.z;				// 横方向速度
	if(u>0) uu= sqrt(abs(u)); else uu = -sqrt(abs(-u));
	// 進行方向に行く力が働く
	float speed, inout, lift;
	speed = sqrt(vx*vx + vy*vy + vz*vz);
//	if(speed<=0.0 || speed>1000) { speed=10; vx=vy=vz=0; }	// 2005.6.8修正
	if(speed<=0.0) inout = 0.0;								// 2008.2.9
	else inout = (vx*yy.x + vy*yy.y + vz*yy.z)/speed;		// 揚力の上下

	float pow = maxpower[model]*power;
	float wg = 0;
	if(on_ground==0) wg = weight[model]*9.8;		// 空中では重力の影響を受ける

	lift = inout*vv*lift_p[model];					// 2008.1.23 ヘリにも適用 (ループのため)

	if(JR_propo==3) u = pow*chf[4];					// 上昇力＝ピッチ×パワー
	else if(JR_propo==2 && thr_type) u = (float)pow;	// 真ん中が0のJoystick
	else if(JR_propo==2 && JoyM[4]!=0) u = pow*chf[4];	// プロポ-USB
	else if(mode) u = 2*maxpower[model]*(power-50);
	else u = (float)pow;

    if(model_t[model])
	{
//		fx = - 0.2*u*yy.x - lift*vv*yy.x + xx.x*pow/50.0;	// xx:横ドリフトの座標ベクトル
//		fy = - 0.2*u*yy.y - lift*vv*yy.y + xx.y*pow/50.0;
//		fz = - 0.2*u*yy.z - lift*vv*yy.z + xx.z*pow/50.0;
		float sidef = rotar[model]*pow/40.0;
		fx = - 0.2*u*yy.x - lift*vv*yy.x + sidef*xx.x;	// xx:横ドリフトの座標ベクトル
		fy = - 0.2*u*yy.y - lift*vv*yy.y + sidef*xx.y;
		fz = - 0.2*u*yy.z - lift*vv*yy.z + sidef*xx.z;
	}
	else
	{
//		fx = 0.05*pow*zz.x - lift*vv*yy.x - side_r[model]*uu*xx.x;	// side_r:横方向の力（ドリフト対策）
//		fy = 0.05*pow*zz.y - lift*vv*yy.y - side_r[model]*uu*xx.y;	
//		fz = 0.05*pow*zz.z - lift*vv*yy.z - side_r[model]*uu*xx.z;
		fx = 0.05*u*zz.x - lift*vv*yy.x - side_r[model]*uu*xx.x;	// side_r:横方向の力（ドリフト対策）
		fy = 0.05*u*zz.y - lift*vv*yy.y - side_r[model]*uu*xx.y;	
		fz = 0.05*u*zz.z - lift*vv*yy.z - side_r[model]*uu*xx.z;
	}
	float r;
	r = 1.0 - air_res[model]/1000.0;							// air_res:空気抵抗(typ.0.001)
	// 本当にシミュレートすると見えなくなるので縦方向(Y)は特別扱い
	vx = r*(0.99*vx + 0.01*zz.x) + fx/(5.0*hv_ratio[model]);		// 時間積分(正規化の必要なし)
	vy = r*(0.99*vy + 0.01*zz.y) + fy/ 5.0 + wg            ;
	vz = r*(0.99*vz + 0.01*zz.z) + fz/(5.0*hv_ratio[model]);		// 0.01:移動方向を向く

	// 空気抵抗が重力より大きくなると落ちなくなるので注意
	if(crash_on) vx = vy = vz = 0.0;

	static float kx=0.0, ky=0.0, kz=0.0;
	switch(kaze)
	{
	case 5: if(rand()%20 != 0) {rx=ry=rz=0; break;}
			kx = (rand()%200-100)/200.0;
		    ky = (rand()%200-100)/400.0;
			kz = (rand()%200-100)/200.0;
			rx = (rand()%200-100)/200.0;					/* rx, rzは無視されているような */
		    ry = (rand()%200-100)/150.0;
			rz = (rand()%200-100)/150.0;	break;
	case 4: kx=-1.0; ky=0.0; kz=-1.0; break;
	case 3: kx= 1.0; ky=0.0; kz= 1.0; break;
	case 2: kx= 0.0; ky=0.0; kz= 0.5; break;
	case 1: kx= 0.5; ky=0.0; kz= 0.0; break;
	default:kx = ky = kz = rx = ry = rz = 0.0;
	}

	if(first==0)
	{
		(*cframe)->SetVelocity(g_pScene, D3DVAL(0.5*(kx-0.2*vx)), D3DVAL(0.5*(ky-0.2*vy)), D3DVAL(0.5*(kz-0.2*vz)), FALSE);
		// 0.5:fps 100->50のため
		sprintf (buf, "Model %d:%s Kaze:%d In:%d Mode:%d Zoom:%d Beep:%d fPs=%d pow=%3f%% ail=%5.2f ele=%5.2f rud=%5.2f pit=%5.2f speed:%5.0f joy:%d ",
			model, name[model], kaze, JR_propo, mode, zoom, beep_on, 1000/pertime, power, chf[1], chf[2], chf[3], chf[4], 10.0*vv, joybutton);
		SetWindowText(win, buf);
	}

	// shadow
	switch(shadow[model])
	{
	case -2:{shadow_2frame->SetPosition(g_pScene, pp.x, GROUND+0.4, pp.z); break;}
	case -1:{shadow_1frame->SetPosition(g_pScene, pp.x, GROUND+0.4, pp.z); break;}
	case  0:{shadow0frame ->SetPosition(g_pScene, pp.x, GROUND+0.4, pp.z); break;}
	case  1:{shadow1frame ->SetPosition(g_pScene, pp.x, GROUND+0.4, pp.z); break;}
	case  2:{shadow2frame ->SetPosition(g_pScene, pp.x, GROUND+0.4, pp.z); break;}
	}
	switch(shadow[model])
	{
	case -2:{shadow_2frame->SetVelocity(g_pScene, D3DVAL(kx-0.2*vx)/2.0, 0.0, D3DVAL(kz-0.2*vz)/2.0, FALSE); break;}
	case -1:{shadow_1frame->SetVelocity(g_pScene, D3DVAL(kx-0.2*vx)/2.0, 0.0, D3DVAL(kz-0.2*vz)/2.0, FALSE); break;}
	case  0:{shadow0frame ->SetVelocity(g_pScene, D3DVAL(kx-0.2*vx)/2.0, 0.0, D3DVAL(kz-0.2*vz)/2.0, FALSE); break;}
	case  1:{shadow1frame ->SetVelocity(g_pScene, D3DVAL(kx-0.2*vx)/2.0, 0.0, D3DVAL(kz-0.2*vz)/2.0, FALSE); break;}
	case  2:{shadow2frame ->SetVelocity(g_pScene, D3DVAL(kx-0.2*vx)/2.0, 0.0, D3DVAL(kz-0.2*vz)/2.0, FALSE); break;}
	}

	// 機首下げ力：一定
	// X軸方向の計算
	float a=1.0;
//	aa =  (yy.y*zz.x-yy.x*zz.y)*(yy.y*zz.x-yy.x*zz.y)
//		+ (yy.z*zz.y-yy.y*zz.z)*(yy.z*zz.y-yy.y*zz.z)i
//		+ (yy.x*zz.z-yy.z*zz.x)*(yy.x*zz.z-yy.z*zz.x); //これでやると正規化されない...
//	a = sqrt(a);
	xx.x = (yy.z*zz.y - yy.y*zz.z)/a;
	xx.y = (yy.x*zz.z - yy.z*zz.x)/a;
	xx.z = (yy.y*zz.x - yy.x*zz.y)/a;

	chf[1] = chf[2] = 0.0;
	if(!crash_on)
	{
		RECT rc;
	    GetClientRect( win, &rc );
		float r;

		// Ｘ軸まわりの回転（エレベーター）
		u = 0.0;
		if(model_t[model])	  u = zz.y*stable[model]/20.0;		// 復元力
		else if(on_ground==0) u = vv*pitchup[model]-0.1*weight[model]/((vv+0.01)/10.0);	// 高速機首上げ/低速機首下げ

		if(JR_propo==3) u += -1.0*(ch[2] - cho)/18.0;			// 18:プロポ測定値
		else if(JR_propo==2 && joyon[1])	
		{
			long half = ((long)joymax[1] + joymin[1])/2L;
			long range= (joymax[1] - joymin[1])/2L;
			u += 0.5*joysex[1]*((float)joydata[1] - half) / range;
			u *= JoyMul;
		}
		else if(JR_propo==1)
		{
			long half = (rc.bottom + rc.top)/2L;
			long range= abs(rc.bottom - rc.top)/2L;
			u += 0.5*((float)g_wMouseY - half) / range;
		}
		if(KeyBuffer['C']&0xf0) u +=  0.5;
		if(KeyBuffer['E']&0xf0) u += -0.5;
		r = u*sens[model];
	    if(model_t[model]) chf[2] = 0.4*r; else chf[2] = 0.1*vv*r;

		// 操作により働く力(ラダーもあるが弱い)
		vx -= w_size[model]*vv*fabs(vv)*yy.x*sin(chf[2])/hv_ratio[model];	// 係数w_sizeは適当
		vy -= w_size[model]*vv*fabs(vv)*yy.y*sin(chf[2]);					// 空気抵抗は二乗（正負あり）
		vz -= w_size[model]*vv*fabs(vv)*yy.z*sin(chf[2])/hv_ratio[model];

		// Ｙ軸まわりの回転（ラダー）
		u = 0.0;

		/* torque effect */
		if(model_t[model]){
			static float ry0, wy, pow0;
			u += rotar[model]*(pow - pow0)/10.0;
			pow0 = pow;
			/* gyro */
			float gforce = gyro[model] * (ry+u - ry0);
			ry0 = ry+u;
			wy -= gforce;		/* verocity */
			u  += wy;			/* angle */
		}

		/* key */
		if(JR_propo==3) u += (ch[3] - cho)/18.0;			// 18:プロポ測定値
		else if(JR_propo==2 && joyon[2])
		{
			long half = ((long)joymax[2] + joymin[2])/2L;
			long range= (joymax[2] - joymin[2])/2L;
			u += 0.5*joysex[2]*((float)joydata[2] - half) / range;
			u *= JoyMul;
		}
		else if(JR_propo==1)
		{
			if(g_wMouseButtons & MK_LBUTTON) u +=  0.5;
			if(g_wMouseButtons & MK_RBUTTON) u += -0.5;
		}
		if(KeyBuffer['S']&0xf0) u +=  0.5;					//　効かせすぎるとドリフトしてしまう
		if(KeyBuffer['F']&0xf0) u += -0.5;
		chf[3] = u*rudder_s[model];
		     if(on_ground==2) u = 0;
		else if(model_t[model]) u = -chf[3]    -0.05*xx.y;	//ヘリの場合、xx.yは機首を進行方向に向ける
		else					u = -chf[3]*vv -0.05*xx.y;	//ラダー＋機首曲げ力(Xy)

		(*cframe)->SetRotation((*cframe), D3DVAL(0),D3DVAL(1),D3DVAL(0),D3DVAL(ry+u));

		// Ｚ軸まわりの回転（エルロン）
		u = 0.0;
		if(JR_propo==3) u += 0.7*(ch[1] - cho)/18.0;		// 18:プロポ測定値
		else if(JR_propo==2 && joyon[0])
		{
			long half = ((long)joymax[0] + joymin[0])/2L;
			long range= (joymax[0] - joymin[0])/2L;
			u -= 0.5*joysex[0]*((float)joydata[0] - half) / range;
			u *= JoyMul;
		}
		else if(JR_propo==1)
		{
			long half = (rc.right + rc.left)/2L;
			long range= abs(rc.right - rc.left)/2L;
			u -= 0.5*((float)g_wMouseX - half) / range;
		}
		if(KeyBuffer[VK_LEFT] &0xf0) u +=  0.5;
		if(KeyBuffer[VK_RIGHT]&0xf0) u += -0.5;
		if(!model_t[model]) u += sens[model]*(power/100.0)/(1.0+speed);		// プロペラトルク
		r = u*sens[model] - xx.y*stable[model]/20.0;						// stable:復元力
	    if(model_t[model]) chf[1] = -0.4*r; else chf[1] = -0.3*vv*r;

		// 操作により働く力(サイドループができるかと思ったが、エルロンは要らない？) 
		vx -= w_size[model]*uu*fabs(uu)*yy.x*sin(chf[1])/hv_ratio[model];
		vy -= w_size[model]*uu*fabs(uu)*yy.y*sin(chf[1]);
		vz -= w_size[model]*uu*fabs(uu)*yy.z*sin(chf[1])/hv_ratio[model];
	}

	//// key input and mode change ////
	static int key0, key1, key2, Pcount=0;
	int key = 0;
	if(key0==key1 && key1==key2 && Pcount>500)
	{
		// corrective pitch Mode key (Heli and keyboard only)
		if((KeyBuffer['M']&0xf0) || (joybutton & mode_no)) { key = 1; mode = !mode; }

		// Input device key / JR-Propo
		if(KeyBuffer['I']&0xf0)
		{
			key = 2; 
			if(++JR_propo>3) JR_propo=0;
			if(JR_propo ==3) JRstart =1;	// Calibrate JR propo
		}

		// Beep key
		if(KeyBuffer['B']&0xf0)
		{
			key = 3;
			if(beep_on==0 || beep_on==2) sndPlaySound( NULL , SND_LOOP | SND_ASYNC);
			if(beep_on<=1) midiOutReset(hMidiOut);
//			else stop_midi();
			beep_on++;
			if(beep_on>2) beep_on=0;
			reset_para();
		}

		// camera View & Zoom key
		if(KeyBuffer['V']&0xf0) { key = 4; camera = !camera; }

		if((KeyBuffer['Z']&0xf0) || (joybutton & zoom_no))
		{
			key = 5;
			zoom++;
			if(zoom > 2) zoom=0;
		}

		if(JR_propo==3){
			static int chz;
			if(!zoom && ch[5]-cho>0 && chz<0) { key = 5; zoom = 1; }
			if( zoom && ch[5]-cho<0 && chz>0) { key = 5; zoom = 0; }
			chz = ch[5]-cho;
		}

		// model Type key
		if(KeyBuffer['T']&0xf0)
		{
			key = 6;
			(*cframe)->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0)); // goto unvisual place
			if(beep_on<=1)  play_midi(0x000090L + 0x100L*tone[model]);	

			if(++model>=MMAX) model=0;
			switch(model)
			{
				case  0 :cframe = &gliderframe;break;
				case  1 :cframe = &trainframe; break;
				case  2 :cframe = &stuntframe; break;
				case  3 :cframe = &baloonframe;break;
				case  4 :cframe = &cosmo0frame;break;
				case  5 :cframe = &hoverframe; break;
				case  6 :cframe = &heli1frame; break;
				case  7 :cframe = &heli3dframe;break;
				case  8 :cframe = &hel450frame;break;
				case  9 :cframe = &helbeeframe;break;
				case 10 :cframe = &penginframe;break;
				default :cframe = &ufoframe;
			}
			reset_para();
		}

		if(KeyBuffer['Y']&0xf0)
		{
			key = 7;
			(*cframe)->SetPosition(g_pScene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0)); // goto unvisual place
			if(beep_on<=1)  play_midi(0x000090L + 0x100L*tone[model]);	

			if(--model<0) model=MMAX-1;
			switch(model)
			{
				case  0 :cframe = &gliderframe;break;
				case  1 :cframe = &trainframe; break;
				case  2 :cframe = &stuntframe; break;
				case  3 :cframe = &baloonframe;break;
				case  4 :cframe = &cosmo0frame;break;
				case  5 :cframe = &hoverframe; break;
				case  6 :cframe = &heli1frame; break;
				case  7 :cframe = &heli3dframe;break;
				case  8 :cframe = &hel450frame;break;
				case  9 :cframe = &helbeeframe;break;
				case 10 :cframe = &penginframe;break;
				default :cframe = &ufoframe;
			}
			reset_para();
		}

		// sky color
		if(KeyBuffer['W']&0xf0)
		{
			key = 8;
			if(++weather>2) weather=0;
			     if(weather==2) g_pScene->SetSceneBackgroundRGB(0, 0, 0);
			else if(weather==1) g_pScene->SetSceneBackgroundRGB(0.5, 0.5, 0.5);
			else                g_pScene->SetSceneBackgroundRGB(0, 0, 1);
		}	

		// kaze (wind)
		if(KeyBuffer['K']&0xf0) { key=9; if(++kaze>5) kaze=0; }

		// TimePerCycle
		if(KeyBuffer['P']&0xf0)
		{
			key = 10;
			if(pertime==20) pertime = 50;
			else			pertime *= 2;
			if(pertime>200) pertime = 10;

			KillTimer(win, 1);			// 自分のタイマーを書き換える荒技
			SetTimer(win, 1, pertime, NULL);
		}

		// Reset Plane
		if(KeyBuffer['R']&0xf0) { reset_para(); key=11; }

		if(key > 0) { Pcount = 0; first = 0; }
	}
	Pcount += pertime;
	// チャタリング防止用
	key2 = key1; key1 = key0; key0 = key;

	// ESCキーが押されていたら・・・
	if(KeyBuffer[VK_ESCAPE]&0xf0) return FALSE;			// 終了する
	
	return TRUE;
}

////////// MIDI functions ///////////////////////////////////////////

    // outline of a MIDI output callback function
void CALLBACK _export MidiOutCall (HMIDIOUT hMidiOut, UINT wMsg, 
    DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
    switch (wMsg)
    {
        case MOM_CLOSE: // midiOutClose() called
            break ;
        case MOM_DONE:  // midiOutLongMsg() completed
            break ;
        case MOM_OPEN:  // midiOutOpen() called
            break ;
    }
    return ;
}

void play_midi(long d)
{
    LPMIDISONG lpMidiSong ;
    LPMIDIDATA lpMidiData, lpmd ;

	if(!midi_ok) return;
    // allocate global blocks to hold data
    hGmem3 = GlobalAlloc (GMEM_FIXED | GMEM_SHARE, sizeof (MIDISONG)) ;
    hGmem4 = GlobalAlloc (GMEM_FIXED | GMEM_SHARE, 6 * sizeof (MIDIDATA)) ;
    if (hGmem3 == NULL || hGmem4 == NULL)
    {
		SetWindowText(win, "Could not allocate memory");
        return;
    }
    lpMidiSong = (MIDISONG FAR *)GlobalLock(hGmem3);
    lpMidiData = (MIDIDATA FAR *)GlobalLock(hGmem4);
    lpmd = lpMidiData ;
    lpmd->dwTimeMs   = 0;
    lpmd->dwMidiData = d;
    lpMidiSong->dwEvents = 1;  // init MIDISONG
    lpMidiSong->dwLastEvent = 0 ;
    lpMidiSong->dwTime = 0 ;
    lpMidiSong->lpMidiData = lpMidiData ;
    lpMidiSong->hMidiOut = hMidiOut ;

    midiOutShortMsg (lpMidiSong->hMidiOut, lpMidiSong->lpMidiData[0].dwMidiData) ;
}

void stop_midi(void)
{
	if(!midi_ok) return;
//	timeKillEvent (nTimerID) ;		// shutting down
//  timeEndPeriod (nTimerRes) ;		// timer stops MIDI
    midiOutReset (hMidiOut) ;
//  PostMessage (win, WM_USER, 0, 0L) ; // free mem ? need?
}

//// D3D functions ////
bool addframe(LPDIRECT3DRM2 pD3DRM, LPDIRECT3DRMFRAME2 scene, 
			  LPDIRECT3DRMFRAME2 *frame, char *name, float px, float py, float pz)
{  
	// メッシュ生成
    LPDIRECT3DRMMESHBUILDER2 meshbuilder = NULL;
    HRESULT rval;

    if (FAILED(pD3DRM->CreateMeshBuilder(&meshbuilder)))
        goto generic_error;
    rval = meshbuilder->Load(name, NULL, D3DRMLOAD_FROMFILE, NULL, NULL);
    if (FAILED(rval)) 
    {
		char s[1024];
		sprintf(s, "Failed to load %s file.", name);
		MSG(s);
        goto ret_with_error;
    }
	// フレーム位置・姿勢　設定
    if (FAILED(pD3DRM->CreateFrame(scene, frame)))
        goto generic_error;
	(*frame)->SetPosition(scene, D3DVAL(px), D3DVAL(py), D3DVAL(pz));
	(*frame)->SetOrientation(scene,
   						    D3DVAL(0.0), D3DVAL(0.0), D3DVAL(-1.0),
					        D3DVAL(0.0), D3DVAL(1.0), D3DVAL(0.0));
    if (FAILED((*frame)->AddVisual((LPDIRECT3DRMVISUAL)meshbuilder)))
        goto generic_error;
    SAFE_RELEASE(meshbuilder);
    return TRUE;

generic_error:
    MSG("A failure occured in addframe.");
    SAFE_RELEASE(meshbuilder);
ret_with_error:
    return FALSE;
}

//-----------------------------------------------------------------------------

int skip_sharp(FILE *fp, char *s)
{
	do{
		if(fgets(s, 1023, fp)==NULL) return -1;
	}while(*s == '#');

	return 0;
}

// パラメータ読み込み
int get10int(FILE *fp, char *s, int d[MMAX])
{
	if( skip_sharp(fp, s) ) return -1;

	return sscanf(s, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
			&d[0],&d[1],&d[2],&d[3],&d[4],&d[5],&d[6],&d[7],&d[8],&d[9],&d[10],&d[11]);
}

int get10float(FILE *fp, char *s, float d[MMAX])
{
	if( skip_sharp(fp, s) ) return -1;

	return sscanf(s, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
			&d[0],&d[1],&d[2],&d[3],&d[4],&d[5],&d[6],&d[7],&d[8],&d[9],&d[10],&d[11]);
}

void get_x(LPDIRECT3DRM2 pD3DRM, LPDIRECT3DRMFRAME2 scene, LPDIRECT3DRMFRAME2 *frame, char *s)
{
	if(*s>' '){
		char buf[1030];
		sprintf(buf, "%s.x", s);
		addframe(pD3DRM, scene, frame, buf, 0, 0, 0);
	}
}

//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
BOOL BuildScene( LPDIRECT3DRM2 pD3DRM, 
				 LPDIRECT3DRMDEVICE2 dev, LPDIRECT3DRMVIEWPORT,
	             LPDIRECT3DRMFRAME2 scene, LPDIRECT3DRMFRAME2 camera)
{
    D3DRMRENDERQUALITY quality = D3DRMRENDER_FLAT;
    LPDIRECT3DRMFRAME2 lights = NULL;
    LPDIRECT3DRMLIGHT l1=NULL, l2 = NULL;

	// read plane parameters from file
	FILE	*fp;
	char	str[1024];
	int		i;

	if(cmdline_save[0] < 'A') strcpy(cmdline_save, "param.txt");
	fp = fopen(cmdline_save, "r");
	if(fp != NULL)	// if error, use initial parameters
	{
		if( skip_sharp(fp, str) ) goto error_skip;
		i = sscanf(str, "%d,%d,%d,%d,%d,%d,%f,%d,%d",
				&JoyM[0],&JoyM[1],&JoyM[2],&JoyM[3],&JoyM[4], &thr_type, &JoyMul, &mode_no, &zoom_no);
		if(i != 9) { SetWindowText(win, "param err Joystick"  ); goto error_skip; }

		if( skip_sharp(fp, str) ) goto error_skip;
		i = sscanf(str, "%d,%d,%d,%d,%d,%d,%d",
				&pcmlevel, &PropoM[0],&PropoM[1],&PropoM[2],&PropoM[3],&PropoM[4],&PropoM[5]);
		if(i != 7) { SetWindowText(win, "param err Propo"  ); goto error_skip; }

		if(get10int(  fp, str, model_t )<MMAX) { SetWindowText(win, "param err type"  ); goto error_skip; }
		if(get10int(  fp, str, voice   )<MMAX) { SetWindowText(win, "param err voice" ); goto error_skip; }
		if(get10int(  fp, str, tone    )<MMAX) { SetWindowText(win, "param err tone"  ); goto error_skip; }
		if(get10int(  fp, str, shadow  )<MMAX) { SetWindowText(win, "param err shadow"); goto error_skip; }
		if(get10float(fp, str, zoom_x  )<MMAX) { SetWindowText(win, "param err zoom"  ); goto error_skip; }
		if(get10float(fp, str, height  )<MMAX) { SetWindowText(win, "param err height"); goto error_skip; }
		if(get10float(fp, str, weight  )<MMAX) { SetWindowText(win, "param err weight"); goto error_skip; }
		if(get10float(fp, str, hv_ratio)<MMAX) { SetWindowText(win, "param err hv_rat"); goto error_skip; }
		if(get10float(fp, str, crash   )<MMAX) { SetWindowText(win, "param err crash" ); goto error_skip; }
		if(get10float(fp, str, maxpower)<MMAX) { SetWindowText(win, "param err power" ); goto error_skip; }
		if(get10float(fp, str, sens    )<MMAX) { SetWindowText(win, "param err sense" ); goto error_skip; }
		if(get10float(fp, str, rudder_s)<MMAX) { SetWindowText(win, "param err rudder"); goto error_skip; }
		if(get10float(fp, str, stable  )<MMAX) { SetWindowText(win, "param err stable"); goto error_skip; }
		if(get10float(fp, str, air_res )<MMAX) { SetWindowText(win, "param err res."  ); goto error_skip; }
		if(get10float(fp, str, w_size  )<MMAX) { SetWindowText(win, "param err w_size"); goto error_skip; }
		if(get10float(fp, str, lift_p  )<MMAX) { SetWindowText(win, "param err lift"  ); goto error_skip; }
		if(get10float(fp, str, side_r  )<MMAX) { SetWindowText(win, "param err side-r"); goto error_skip; }
		if(get10float(fp, str, pitchup )<MMAX) { SetWindowText(win, "param err pitch" ); goto error_skip; }
		if(get10float(fp, str, rotar   )<MMAX) { SetWindowText(win, "param err rotar" ); goto error_skip; }
		if(get10float(fp, str, gyro    )<MMAX) { SetWindowText(win, "param err gyro"  ); goto error_skip; }
		// x files
		for(i=0; i<MMAX; i++){
			if( skip_sharp(fp, str) ) goto error_skip;
			sscanf(str, "%s", name[i]);		// fgetsは改行が付く
		}
		fclose(fp);
		sprintf(str, "RCsky! ver.0.82, %s reading completed (Joystick=%d,%d,%d,%d,%d,%d,%f,%d,%d PCM level=%d Propo=%d,%d,%d,%d,%d,%d)",
						cmdline_save, JoyM[0], JoyM[1], JoyM[2], JoyM[3], JoyM[4], thr_type, JoyMul, mode_no, zoom_no,
						pcmlevel, PropoM[0], PropoM[1], PropoM[2], PropoM[3], PropoM[4], PropoM[5]);
		SetWindowText(win, str);
	}

error_skip:

    if (FAILED( dev->SetQuality( quality)))
		goto generic_error;
    if (FAILED( scene->SetSceneBackgroundRGB( 
			D3DVAL(0), D3DVAL(0), D3DVAL(1))))
		goto generic_error;

    // initialize the lights in the scene
    if (FAILED( pD3DRM->CreateFrame( scene, &lights)))
		goto generic_error;
    if (FAILED( lights->SetPosition( scene, 
				D3DVAL(0), D3DVAL(0), -D3DVAL(10))))
		goto generic_error;
//  if (FAILED( lights->SetRotation( scene, 
//		D3DVAL(0), D3DVAL(1),-D3DVAL(1), D3DVAL(3.14/4))))
//		goto generic_error;
    if (FAILED( pD3DRM->CreateLightRGB(D3DRMLIGHT_DIRECTIONAL,
				D3DVAL(0.5), D3DVAL(0.5), D3DVAL(0.5), &l1)))
		goto generic_error;
    if (FAILED( lights->AddLight(l1)))
		goto generic_error;

    if (FAILED( pD3DRM->CreateLightRGB( D3DRMLIGHT_AMBIENT,
				D3DVAL(0.5), D3DVAL(0.5), D3DVAL(0.5), &l2)))
		goto generic_error;
    if (FAILED( scene->AddLight(l2)))	// l2??
		goto generic_error;

	// mesh of stunt plane
	get_x(pD3DRM, scene, &gliderframe, name[0]);
	get_x(pD3DRM, scene,  &trainframe, name[1]);
	get_x(pD3DRM, scene,  &stuntframe, name[2]);
	get_x(pD3DRM, scene, &baloonframe, name[3]);
	get_x(pD3DRM, scene, &cosmo0frame, name[4]);
	get_x(pD3DRM, scene,  &hoverframe, name[5]);
	get_x(pD3DRM, scene,  &heli1frame, name[6]);
	get_x(pD3DRM, scene, &heli3dframe, name[7]);
	get_x(pD3DRM, scene, &hel450frame, name[8]);
	get_x(pD3DRM, scene, &helbeeframe, name[9]);
	get_x(pD3DRM, scene, &penginframe, name[10]);
	get_x(pD3DRM, scene,    &ufoframe, name[11]);
	addframe(pD3DRM, scene, &shadow_2frame,"shadow_2.x",0, 0, 0);
	addframe(pD3DRM, scene, &shadow_1frame,"shadow_1.x",0, 0, 0);
	addframe(pD3DRM, scene, &shadow0frame, "shadow0.x", 0, 0, 0);
	addframe(pD3DRM, scene, &shadow1frame, "shadow1.x", 0, 0, 0);
	addframe(pD3DRM, scene, &shadow2frame, "shadow2.x", 0, 0, 0);
	 hoverframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));		// goto unvisible place
	gliderframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	 trainframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	 stuntframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	baloonframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	cosmo0frame->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	 heli1frame->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	heli3dframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	hel450frame->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	helbeeframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	penginframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	   ufoframe->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	cframe = &gliderframe;	// default
	shadow_2frame->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));	// goto unvisible place
	shadow_1frame->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	shadow0frame ->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));	// goto unvisible place
	shadow1frame ->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	shadow2frame ->SetPosition(scene,D3DVAL(0.0),D3DVAL(-100.0),D3DVAL(0.0));
	//
	addframe(pD3DRM, scene, &fieldframe, "field.x", 0,-10, 0);
	addframe(pD3DRM, scene, &fieldframe, "cloud.x", 0,  0, 0);

    // set up the frames position, orientation and rotation
    if (FAILED( camera->SetPosition( scene, D3DVAL(0), D3DVAL(0), D3DVAL(0))))
		goto generic_error;
    if (FAILED( camera->SetOrientation( scene, D3DVAL(0), D3DVAL(0), D3DVAL(1),
										D3DVAL(0), D3DVAL(1), D3DVAL(0))))
		goto generic_error;

    SAFE_RELEASE(lights);
    SAFE_RELEASE(l1);
    SAFE_RELEASE(l2);
    return TRUE;
generic_error:
    MSG("A failure occurred while building the scene.\n");
    SAFE_RELEASE(lights);
    SAFE_RELEASE(l1);
    SAFE_RELEASE(l2);
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
VOID OverrideDefaults( BOOL* pbNoTextures, BOOL* pbResizingDisabled, 
					   BOOL* pbConstRenderQuality, CHAR** pstrName )
{
    (*pbNoTextures) = TRUE;
    (*pbConstRenderQuality) = TRUE;
    (*pstrName) = "Shadow Direct3DRM Example";
}
