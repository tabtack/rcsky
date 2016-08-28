//-----------------------------------------------------------------------------
// File: rmmain.cpp
//
// Desc: Main file for retained-mode samples. Contains Windows UI and D3DRM
//       initialization/cleanup code.
//
//
// Copyright (C) 1998-1999 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

// 起動時だけ画質が悪い：現在の所謎

#define INITGUID
#include <conio.h>
#include <windows.h>
#include <zmouse.h>		// wheel mouse
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <d3drm.h>
#include <d3drmwin.h>
#include <mmsystem.h>	// winmm.lib
#include "resource.h"
#include "midiout.h" 

#define MAX_DRIVERS 5           // maximum D3D drivers we ever expect to find
#define SAFE_RELEASE(x) if (x != NULL) {x->Release(); x = NULL;}
#define MSG(msg) MessageBox( NULL, msg, "Application Message", MB_OK )

#define WMAX	3072
#define MMAX	10

#define MYAPPNAME "RCsky simulator"

// Functions to build the customize the app for each sample
BOOL BuildScene( LPDIRECT3DRM2 pD3DRM,
				 LPDIRECT3DRMDEVICE2 dev, LPDIRECT3DRMVIEWPORT view,
				 LPDIRECT3DRMFRAME2 scene, LPDIRECT3DRMFRAME2 camera );
VOID OverrideDefaults( BOOL* bNoTextures, BOOL* pbResizingDisabled, 
					   BOOL* pbConstRenderQuality, CHAR** pstrName );

//-----------------------------------------------------------------------------
// GLOBAL VARIABLES
//-----------------------------------------------------------------------------
extern float chf[6], vv;
extern int PropoM[6];
extern int JR_propo, beep_on, camera, mode, model;
extern int model_t[MMAX];

char g_DriverName[MAX_DRIVERS][50]; // names of the available D3D drivers
int  g_NumDrivers;                  // number of available D3D drivers
int  g_CurrDriver;                  // D3D driver currently being used
int  JRstart = 1;
int  pertime = 50;
char cmdline_save[64];

// PCM
int pcmlock, pcmlevel=80;
int	ch[6];
int	cht = 31, cho = 49;		// 2005.6.4 for difference of Sound cards
static WAVEFORMATEX     pcmWaveFormat ;
static char				waveData[WMAX+10];
static WAVEHDR          WaveHdr;
static HWAVEIN			hWaveIn=NULL;

// MIDI
extern HMIDIOUT         hMidiOut;
extern HANDLE           hGmem3, hGmem4;
extern TIMECAPS         tc;
extern UINT             nTimerID, nTimerRes;
extern int              midi_ok;

// Global Variables 
HWND win;
extern JOYCAPS	joycap;

LPDIRECT3DRM2         g_pD3DRM;     // D3DRM object
LPDIRECTDRAWCLIPPER   g_pDDClipper; // DDrawClipper object
LPDIRECT3DRMDEVICE2   g_pDevice;    // D3DRM device 
LPDIRECT3DRMVIEWPORT  g_pViewport;  // D3DRM viewport
LPDIRECT3DRMFRAME2    g_pScene;     // Master frame in which others are placed
LPDIRECT3DRMFRAME2    g_pCamera;    // Frame describing the users POV

LPDIRECT3DRMFRAME2 *cframe;
LPDIRECT3DRMFRAME2 childframe   = NULL;
LPDIRECT3DRMFRAME2 hoverframe   = NULL;
LPDIRECT3DRMFRAME2 gliderframe  = NULL;
LPDIRECT3DRMFRAME2 trainframe   = NULL;
LPDIRECT3DRMFRAME2 stuntframe   = NULL;
LPDIRECT3DRMFRAME2 baloonframe  = NULL;
LPDIRECT3DRMFRAME2 cosmo0frame  = NULL;
LPDIRECT3DRMFRAME2 heli1frame   = NULL;
LPDIRECT3DRMFRAME2 heli3dframe  = NULL;
LPDIRECT3DRMFRAME2 hel450frame  = NULL;
LPDIRECT3DRMFRAME2 helbeeframe  = NULL;
LPDIRECT3DRMFRAME2 penginframe  = NULL;
LPDIRECT3DRMFRAME2 ufoframe     = NULL;
LPDIRECT3DRMFRAME2 shadow_2frame= NULL;
LPDIRECT3DRMFRAME2 shadow_1frame= NULL;
LPDIRECT3DRMFRAME2 shadow0frame = NULL;
LPDIRECT3DRMFRAME2 shadow1frame = NULL;
LPDIRECT3DRMFRAME2 shadow2frame = NULL;
LPDIRECT3DRMFRAME2 fieldframe   = NULL;

GUID g_DriverGUID[MAX_DRIVERS];     // GUIDs of the available D3D drivers

D3DRMRENDERQUALITY  g_RenderQuality;  // Current shade, fill and light state
D3DRMTEXTUREQUALITY g_TextureQuality; // Current texture interpolation
BOOL                g_bDithering;     // Dithering enable flag
BOOL                g_bAntialiasing;  // Antialiasing enable flag

BOOL g_bQuit;                 // Program is about to terminate
BOOL g_bInitialized;          // All D3DRM objects have been initialized
BOOL g_bMinimized;            // Window is minimized
BOOL g_bSingleStepMode;       // Render one frame at a time
BOOL g_bDrawAFrame;           // Render on this pass of the main loop
BOOL g_bNoTextures;           // This sample doesn't use any textures
BOOL g_bConstRenderQuality;   // Whether sample is not constructed with
                              // MeshBuilders and so the RenderQuality
                              // cannot be changed

DWORD g_wBPP;                 // bit depth of the current display mode

UINT uMSH_MOUSEWHEEL = 0;   // Value returned from RegisterWindowMessage()
WORD g_wMouseButtons;         // mouse button state
WORD g_wMouseX;               // mouse cursor x position
WORD g_wMouseY;               // mouse cursor y position
int  g_wWheel = 0;            // mouse wheel

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
LONG FAR PASCAL WindowProc( HWND, UINT, WPARAM, LPARAM );

HWND     InitApp( HINSTANCE, int );
VOID     InitGlobals();
// HEL sounds used if HAL is not on
HRESULT  CreateDevAndView( LPDIRECTDRAWCLIPPER lpDDClipper, int driver,
						   DWORD dwWidth, DWORD dwHeight );
HRESULT  RenderScene();
VOID     CleanUpAndPostQuit();
VOID     SetRenderState();
BOOL     BuildDeviceMenu( HMENU hmenu );

extern int pcm_start(HWND hWnd);
extern int get_ch(void);
extern BOOL move_plane(void);
////////// PCM functions ////////////////////////////////////////////

int FAR pcm_start(HWND hWnd)
{
	int	i;

    pcmWaveFormat.wFormatTag = WAVE_FORMAT_PCM ;
    pcmWaveFormat.nChannels = 1 ;
    pcmWaveFormat.nSamplesPerSec = 44100L ;
    pcmWaveFormat.nAvgBytesPerSec = 44100L ;
    pcmWaveFormat.nBlockAlign = 1 ;
    pcmWaveFormat.wBitsPerSample = 8 ;  // note!!
    pcmWaveFormat.cbSize = 0;
    if (waveInOpen (&hWaveIn, 0, &pcmWaveFormat, 
	         (DWORD)hWnd, NULL, CALLBACK_WINDOW)) return 1 ;
    // pass buffer to Windows
    WaveHdr.lpData = waveData ;
    WaveHdr.dwBufferLength = (DWORD)WMAX;
//  lpWaveHdr->dwBytesRecorded = (DWORD)WMAX;	
//  lpWaveHdr->dwFlags = (DWORD)WHDR_PREPARED;
    i = waveInPrepareHeader(hWaveIn, &WaveHdr, sizeof(WAVEHDR)) ;
	if(i!=MMSYSERR_NOERROR) { MSG("Err Prepare"); return 1; }
    i = waveInAddBuffer(hWaveIn, &WaveHdr, sizeof(WAVEHDR)) ;
	if(i!=MMSYSERR_NOERROR) { MSG("Err Add"); return 1; }
    i = waveInStart(hWaveIn);
	if(i!=MMSYSERR_NOERROR) { MSG("Err WaveIn"); return 1; }
	return 0;
}

/* 暴走対策のため範囲チェックを行う */
int abs_limit(int i, int max)
{
	if(i<0) i = -i;
	if(i >= max) i = max;

	return i;
}

int get_ch(void)
{
	long i, k, data, max, startp;
	int  h[6];

	// dataが250時間以上0以下の点が、スタート点
	max = startp = 0;
	for(i=0; i<2048; i++)
	{
		data = (unsigned char)waveData[i];
			 if((int)data> pcmlevel && pcmlevel>=0) max++;
		else if((int)data<-pcmlevel && pcmlevel< 0) max++;	// down
		else
		{
			if(max > 250)
			{
				startp = i;
				break;
			}
			max = 0;
		}
	}
	if(startp == 0) return 1;

	// 変化率が31～67(cnt=49)で少ない...
	// しかし44.1kHz以上のサンプルレートはサポートされない
	i = startp;
	for(k=0; k<=5; k++)
	{
		h[k] = 0;
		for(; i<WMAX; i++) 
		{
			data = (unsigned char)waveData[i];
			if((int)data> pcmlevel && pcmlevel>=0) break;
			if((int)data<-pcmlevel && pcmlevel< 0) break;
		}
		if(i>=WMAX) break;
		for(; i<WMAX; i++) 
		{
			h[k]++;
			data = (unsigned char)waveData[i];
			if((int)data< pcmlevel && pcmlevel>=0) break;
			if((int)data>-pcmlevel && pcmlevel< 0) break;
		}
	}

	// Transfer
	ch[0] = h[abs_limit(PropoM[3]-1, 5)];	// Throttle
	ch[1] = h[abs_limit(PropoM[0]-1, 5)];	// Aileron
	ch[2] = h[abs_limit(PropoM[1]-1, 5)];	// Elevator
	ch[3] = h[abs_limit(PropoM[2]-1, 5)];	// Rudder
	ch[4] = h[abs_limit(PropoM[4]-1, 5)];	// Pitch (Heli)
	if(PropoM[5]!=0)
		ch[5] = h[abs_limit(PropoM[5]-1, 5)];	// Zoom
	else
		ch[5] = 0;

	// スティック状態初期化
	if(JRstart)
	{
		if(JR_propo==3){	// JR
			cht = ch[0];						// throttle
			cho = (ch[1] + ch[2] + ch[3])/3;	// other sticks
		}
		JRstart = 0;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Initializes the application then enters a message loop which renders
//       the scene until a quit message is received.
//-----------------------------------------------------------------------------
int PASCAL WinMain( HINSTANCE hInst, HINSTANCE, LPSTR cmdline, int cmdshow)
{
    HWND    hWnd;
    MSG     msg;

	// Save cmdline
	strncpy(cmdline_save, cmdline, 64);
    
    // Create the window and initialize all objects needed to begin rendering
    if( !( hWnd = InitApp( hInst, cmdshow ) ) )
        return 1;

    HACCEL accel = LoadAccelerators( hInst, "AppAccel" );

    uMSH_MOUSEWHEEL = RegisterWindowMessage(MSH_MOUSEWHEEL); 
    if ( !uMSH_MOUSEWHEEL ) MessageBox(NULL, "RegisterWindowMessage Failed!", "Warning", MB_OK);

    while( !g_bQuit )
	{
        // Monitor the message queue until there are no pressing messages
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
            if( msg.message == WM_QUIT )
			{
                CleanUpAndPostQuit();
                break;
            }
            if( !TranslateAccelerator( msg.hwnd, accel, &msg ) )
			{
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
		if( g_bQuit )
			break;
	}
/*
		// If the app is not minimized, not about to quit and D3DRM has
        // been initialized, we can render
        if( !g_bMinimized && !g_bQuit && g_bInitialized )
		{
            // If were are not in single step mode or if we are and the
            // bDrawAFrame flag is set, render one frame
            if( !( g_bSingleStepMode && !g_bDrawAFrame ) ) 
			{
                // Attempt to render a frame, if it fails, take a note.  If
                // rendering fails more than twice, abort execution.
                if( FAILED( RenderScene() ) )
                    ++failcount;
                if (failcount > 2)
				{
                    MSG( "Rendering has failed too many times.  Aborting execution.\n" );
                    CleanUpAndPostQuit();
                    break;
                }
            }

            // Reset the bDrawAFrame flag if we are in single step mode
            if( g_bSingleStepMode )
                g_bDrawAFrame = FALSE;
        }
		else
			WaitMessage();
    }
*/
    DestroyWindow( hWnd );
    return msg.wParam;
}

//-----------------------------------------------------------------------------
// Name: AddMediaPath()
// Desc: Looks in the system registry to determine the media path for the
//       sample. Then, it adds that path to the string passed in, checks if the
//       file exists, and returns a path to the file.
//-----------------------------------------------------------------------------
VOID AddMediaPath( LPDIRECT3DRM2 pD3DRM )
{
	HKEY   key;
	LONG   result;
	TCHAR  strPath[512];
	DWORD  type, size = 512;

	// Open the registry
	result = RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectX",
						   0, KEY_READ, &key );
	if( ERROR_SUCCESS != result )
		return;

	// Search for the desired registry value, and close the registry
        result = RegQueryValueEx( key, "DXSDK Samples Path", NULL, &type, 
		                      (BYTE*)strPath, &size );
	RegCloseKey( key );

	if( ERROR_SUCCESS != result )
		return;

	strcat( strPath, "\\D3DRM\\Media" );

	pD3DRM->AddSearchPath( strPath );

	return;
}

//-----------------------------------------------------------------------------
// Name: InitApp()
// Desc: Creates window and initializes all objects neccessary to begin
//       rendering.
//-----------------------------------------------------------------------------
HWND InitApp( HINSTANCE this_inst, int cmdshow )
{
    DWORD flags;
    WNDCLASS wc;
    HRESULT hr;
    RECT rc;

    // set up and registers the window class
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(DWORD);
    wc.hInstance = this_inst;
    wc.hIcon = LoadIcon(this_inst, "AppIcon");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
//  wc.lpszMenuName = "AppMenu";
    wc.lpszClassName = MYAPPNAME;
    if (!RegisterClass(&wc))
        return FALSE;

	// Direct3D Initialize the global variables and allow the sample code
    // to override some of these default settings.
    g_pD3DRM              = NULL;
	g_pDDClipper          = NULL;
	g_pDevice             = NULL;
	g_pViewport           = NULL;
    g_pScene              = NULL;
	g_pCamera             = NULL;
	g_NumDrivers          = 0;
	g_CurrDriver          = 0;
	g_RenderQuality       = 0;
    g_bDithering          = 0;
	g_bAntialiasing       = 0;
	g_bQuit               = 0;
	g_bInitialized        = 0;
	g_bMinimized          = 0;
	g_bSingleStepMode     = 0;
	g_bDrawAFrame         = 0;
	g_bNoTextures         = 0;
	g_bConstRenderQuality = 0;
    g_RenderQuality  = D3DRMLIGHT_ON | D3DRMFILL_SOLID | D3DRMSHADE_GOURAUD;
	g_TextureQuality = D3DRMTEXTURE_NEAREST;
//  g_TextureQuality = D3DRMTEXTURE_LINEARMIPLINEAR;  not effects

    BOOL bResizingDisabled = FALSE;
    CHAR* strName = MYAPPNAME;
    OverrideDefaults( &g_bNoTextures, &bResizingDisabled, &g_bConstRenderQuality, &strName );

    // Create the window
    if( bResizingDisabled )
        flags =  WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                 WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    else
        flags = WS_OVERLAPPEDWINDOW;
    
	HWND hWnd = win = CreateWindow( MYAPPNAME, strName, flags,
		                            CW_USEDEFAULT, CW_USEDEFAULT, 600, 450,
						            NULL, NULL, this_inst, NULL );
    if( !hWnd )
        return FALSE;

    // Record the current display BPP
    HDC hdc = GetDC( hWnd);
    g_wBPP = GetDeviceCaps( hdc, BITSPIXEL );
    ReleaseDC( hWnd, hdc );

	// Enumerate the D3D drivers into a menu and select one
    if( !BuildDeviceMenu( GetSubMenu( GetMenu(hWnd), 0 ) ) )
        return FALSE;

	// Create the D3DRM object and the D3DRM window object
    LPDIRECT3DRM pD3DRM;
    if( FAILED( Direct3DRMCreate( &pD3DRM ) ) )
	{
        MSG("Failed to create Direct3DRM.\n" );
        return FALSE;
    }

    // Now query for the D3DRM3 object
    if (FAILED( pD3DRM->QueryInterface( IID_IDirect3DRM2, (void **)&g_pD3DRM) ))
    {
        SAFE_RELEASE( pD3DRM );
        MSG("Failed query for  Direct3DRM2.\n" );
        return FALSE;
    }
    SAFE_RELEASE( pD3DRM );

	// Create the master scene frame and camera frame
    if( FAILED( g_pD3DRM->CreateFrame( NULL, &g_pScene ) ) )
	{
        MSG( "Failed to create the master scene frame.\n" );
        return FALSE;
    }
    if( FAILED( g_pD3DRM->CreateFrame(g_pScene, &g_pCamera) ) )
	{
        MSG("Failed to create the camera's frame.\n" );
        return FALSE;
    }
    if( FAILED( g_pCamera->SetPosition( g_pScene, 0.0f, 0.0f, 0.0f ) ) )
	{
        MSG("Failed to position the camera in the frame.\n" );
        return FALSE;
    }

	// Create a clipper and associate the window with it
    if( FAILED( DirectDrawCreateClipper(0, &g_pDDClipper, NULL) ) )
	{
        MSG("Failed to create DirectDrawClipper");
        return FALSE;
    }
    if( FAILED( g_pDDClipper->SetHWnd( 0, hWnd ) ) )
	{
        MSG("Failed to set hwnd on the clipper");
        return FALSE;
    }

	// Created the D3DRM device with the selected D3D driver
    GetClientRect( hWnd, &rc );
    if( hr = CreateDevAndView( g_pDDClipper, g_CurrDriver, rc.right, rc.bottom ) )
	{
		g_CurrDriver = 0;
		if( hr = CreateDevAndView(g_pDDClipper, g_CurrDriver, rc.right, rc.bottom ) )
		{
			MSG("Failed to create the D3DRM device.\n" );
			return FALSE;
		}
    }

    AddMediaPath( g_pD3DRM );

	// Create the scene to be rendered by calling this sample's BuildScene
    if( !BuildScene( g_pD3DRM, g_pDevice, g_pViewport, g_pScene, g_pCamera ) )
        return FALSE;

	// Display the window
    ShowWindow( hWnd, cmdshow );
    UpdateWindow( hWnd );

	// Now we are ready to render
    g_bInitialized = TRUE;

	// MIDI ////
    HINSTANCE   hInstance ;
    UINT		nStatus ;
    char		cBuf [128] ;

	midi_ok = 1;
	beep_on = 0;
    if (midiOutGetNumDevs() == 0)
	{
		midi_ok = 0;
		beep_on = 1;
	}
	else
	{
	    hInstance = (HINSTANCE)
	    GetWindowLong (win, GWL_HINSTANCE) ;
		nStatus = midiOutOpen (&hMidiOut, (UINT) MIDI_MAPPER, 
			(DWORD) MidiOutCall, (DWORD) hInstance, CALLBACK_FUNCTION) ;
	    if (nStatus != 0)
	    {
	        midiOutGetErrorText (nStatus, cBuf, 256) ;
	        MessageBox (win, cBuf, "MIDI Error", MB_ICONHAND | MB_OK) ;
			midi_ok = 0;
			beep_on = 1;
	    }
		else
		{
/*		    nStatus = timeGetDevCaps (&tc, sizeof (TIMECAPS)) ;
		    if (nStatus == TIMERR_NOCANDO)
		    {
		         MessageBox (win, 
		            "MM timer not found - terminating.",
		            "Error", MB_OK | MB_ICONHAND) ;
		        DestroyWindow (win) ;
		    }
		    // find resolution closest to 1 millisecond
		    nTimerRes = min (max (tc.wPeriodMin, 1), tc.wPeriodMax) ;
*/		}
	}

	//// Joystick ////
	joyGetDevCaps(JOYSTICKID1, &joycap, sizeof(joycap));
	joySetThreshold(JOYSTICKID1, 1024);

	// Timer Start
	SetTimer(win, 1, pertime, NULL);

    return hWnd;
}

//-----------------------------------------------------------------------------
// Name: CreateDevAndView()
// Desc: Create the D3DRM device and viewport with the given D3D driver and of
//       the specified size.
//-----------------------------------------------------------------------------
HRESULT CreateDevAndView( LPDIRECTDRAWCLIPPER pDDClipper, int driver,
						  DWORD dwWidth, DWORD dwHeight )
{
    HRESULT hr;

    if( !dwWidth || !dwHeight)
        return D3DRMERR_BADVALUE;

	// Create the D3DRM device from this window and using the specified D3D
    // driver.
    if( FAILED( hr = g_pD3DRM->CreateDeviceFromClipper( pDDClipper,
		                                &g_DriverGUID[driver],
                                        dwWidth, dwHeight, &g_pDevice ) ) )
        return hr;

	// Create the D3DRM viewport using the camera frame.  Set the background
    // depth to a large number.  The width and height may be slightly
    // adjusted, so get them from the device to be sure.
    dwWidth  = g_pDevice->GetWidth();
    dwHeight = g_pDevice->GetHeight();
    
	if( FAILED( hr = g_pD3DRM->CreateViewport( g_pDevice, g_pCamera, 0, 0,
		                                       dwWidth, dwHeight,
											   &g_pViewport ) ) )
	{
		g_bInitialized = FALSE;
        SAFE_RELEASE(g_pDevice);
        return hr;
    }

    g_pViewport->SetBack( 100000.0f );

	// Set the render quality, fill mode, lighting state and color shade info
    SetRenderState();

    return D3DRM_OK;
}

//-----------------------------------------------------------------------------
// Name: BPPToDDBD()
// Desc: Converts bits per pixel to a DirectDraw bit depth flag
//-----------------------------------------------------------------------------
DWORD BPPToDDBD( int bpp )
{
    switch( bpp )
	{
        case 1:
            return DDBD_1;
        case 2:
            return DDBD_2;
        case 4:
            return DDBD_4;
        case 8:
            return DDBD_8;
        case 16:
            return DDBD_16;
        case 24:
            return DDBD_24;
        case 32:
            return DDBD_32;
        default:
            return 0;
    }
}

//-----------------------------------------------------------------------------
// Name: SetRenderState()
// Desc: Set the render quality, dither toggle and shade info if any of them
//       has changed
//-----------------------------------------------------------------------------
VOID SetRenderState()
{
	g_pDevice->SetRenderMode(D3DRMRENDERMODE_BLENDEDTRANSPARENCY
							| D3DRMRENDERMODE_SORTEDTRANSPARENCY);	// 2005.6.4

	// Set the render quality (light toggle, fill mode, shade mode)
    if( g_pDevice->GetQuality() != g_RenderQuality )
        g_pDevice->SetQuality( g_RenderQuality );

	// Set dithering toggle
    if( g_pDevice->GetDither() != g_bDithering )
        g_pDevice->SetDither( g_bDithering );

	// Set the texture quality (point or linear filtering)
    if( g_pDevice->GetTextureQuality() != g_TextureQuality )
        g_pDevice->SetTextureQuality( g_TextureQuality );

	// Set shade info based on current bits per pixel
    switch( g_wBPP )
	{
		case 1:
			g_pDevice->SetShades(4);
			g_pD3DRM->SetDefaultTextureShades(4);
			break;
		case 16:
			g_pDevice->SetShades(32);
			g_pD3DRM->SetDefaultTextureColors(64);
			g_pD3DRM->SetDefaultTextureShades(32);
			break;
		case 24:
		case 32:
			g_pDevice->SetShades(256);
			g_pD3DRM->SetDefaultTextureColors(64);
			g_pD3DRM->SetDefaultTextureShades(256);
			break;
    }
}

//-----------------------------------------------------------------------------
// Name: enumDeviceFunc()
// Desc: Callback function which records each usable D3D driver's name and GUID
//       Chooses a driver to begin with and sets *lpContext to this starting
//       driver
//-----------------------------------------------------------------------------
HRESULT WINAPI enumDeviceFunc( LPGUID lpGuid, LPSTR lpDeviceDescription,
							   LPSTR lpDeviceName, LPD3DDEVICEDESC lpHWDesc, 
							   LPD3DDEVICEDESC lpHELDesc, LPVOID lpContext )
{
    static BOOL hardware = FALSE; // current start driver is hardware
    static BOOL mono = FALSE;     // current start driver is mono light
    LPD3DDEVICEDESC lpDesc;
    int *lpStartDriver = (int *)lpContext;

	// Decide which device description we should consult
    lpDesc = lpHWDesc->dcmColorModel ? lpHWDesc : lpHELDesc;

	// If this driver cannot render in the current display bit depth skip
    // it and continue with the enumeration.
    if (!(lpDesc->dwDeviceRenderBitDepth & BPPToDDBD(g_wBPP)))
        return D3DENUMRET_OK;

	// Record this driver's info
    memcpy(&g_DriverGUID[g_NumDrivers], lpGuid, sizeof(GUID));
    lstrcpy(&g_DriverName[g_NumDrivers][0], lpDeviceName);

	// Choose hardware over software, RGB lights over mono lights
    if (*lpStartDriver == -1)
	{
        // this is the first valid driver
        *lpStartDriver = g_NumDrivers;
        hardware = lpDesc == lpHWDesc ? TRUE : FALSE;
        mono = lpDesc->dcmColorModel & D3DCOLOR_MONO ? TRUE : FALSE;
    }
	else if (lpDesc == lpHWDesc && !hardware)
	{
        // this driver is hardware and start driver is not
        *lpStartDriver = g_NumDrivers;
        hardware = lpDesc == lpHWDesc ? TRUE : FALSE;
        mono = lpDesc->dcmColorModel & D3DCOLOR_MONO ? TRUE : FALSE;
    }
	else if ((lpDesc == lpHWDesc && hardware ) || (lpDesc == lpHELDesc
                                                     && !hardware))
	{
        if (lpDesc->dcmColorModel == D3DCOLOR_MONO && !mono)
		{
            // this driver and start driver are the same type and this
            // driver is mono while start driver is not
            *lpStartDriver = g_NumDrivers;
            hardware = lpDesc == lpHWDesc ? TRUE : FALSE;
            mono = lpDesc->dcmColorModel & D3DCOLOR_MONO ? TRUE : FALSE;
        }
    }
    g_NumDrivers++;
    if (g_NumDrivers == MAX_DRIVERS)
        return D3DENUMRET_CANCEL;
    return D3DENUMRET_OK;
}

//-----------------------------------------------------------------------------
// Name: BuildDeviceMenu()
// Desc: Enumerate the available D3D drivers, add them to the file menu, and
//       choose one to use.
//-----------------------------------------------------------------------------
BOOL BuildDeviceMenu( HMENU hmenu )
{
    LPDIRECTDRAW pDD;
    LPDIRECT3D   pD3D;

    // Create a DirectDraw object and query for the Direct3D interface to use
    // to enumerate the drivers.
    if( FAILED( DirectDrawCreate( NULL, &pDD, NULL ) ) )
	{
        MSG( "Creation of DirectDraw HEL failed.\n" );
        return FALSE;
    }

    if( FAILED( pDD->QueryInterface( IID_IDirect3D, (VOID**)&pD3D ) ) )
	{
        MSG( "Creation of Direct3D interface failed.\n" );
        pDD->Release();
        return FALSE;
    }

	// Enumerate the drivers, setting CurrDriver to -1 to initialize the
    // driver selection code in enumDeviceFunc
    g_CurrDriver = -1;
    if( FAILED( pD3D->EnumDevices( enumDeviceFunc, &g_CurrDriver ) ) )
	{
        MSG("Enumeration of drivers failed.\n" );
        return FALSE;
    }

	// Make sure we found at least one valid driver
    if( g_NumDrivers == 0 )
	{
        MSG("Could not find a D3D driver which is compatible with this display depth");
        return FALSE;
    }

    pD3D->Release();
    pDD->Release();

	// Add the driver names to the File menu
    for( int i=0; i<g_NumDrivers; i++ )
	{
        InsertMenu( hmenu, 2+i, MF_BYPOSITION | MF_STRING, MENU_FIRST_DRIVER+i,
                    g_DriverName[i] );
	}

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RenderScene()
// Desc: Clear the viewport, render the next frame and update the window
//-----------------------------------------------------------------------------
HRESULT RenderScene()
{
    HRESULT rval;

    // Move the scene.
    rval = g_pScene->Move(D3DVAL(1.0));
    if (FAILED(rval)) 
    {
        MSG("Moving scene failed.");
        return E_FAIL;
    }
	if(fabs(chf[2])>=0.0)	// エレベーター
	{
		(*cframe)->SetVelocity(g_pScene, D3DVAL(0),D3DVAL(0),D3DVAL(0),FALSE);
		(*cframe)->SetRotation((*cframe), D3DVAL(1),D3DVAL(0),D3DVAL(0),D3DVAL(chf[2]));

		g_pScene->Move(D3DVAL(1.0));
	}
	if(fabs(chf[1])>=0.0)	// エルロン
	{
		(*cframe)->SetVelocity(g_pScene, D3DVAL(0),D3DVAL(0),D3DVAL(0),FALSE);
		(*cframe)->SetRotation((*cframe), D3DVAL(0),D3DVAL(0),D3DVAL(1),D3DVAL(chf[1]));

		g_pScene->Move(D3DVAL(1.0));
	}

    // Clear the viewport.
    //rval = g_pViewport->Clear(D3DRMCLEAR_ALL);
    rval = g_pViewport->Clear();
    if (FAILED(rval)) 
    {
        MSG("Clearing viewport failed.");
        return E_FAIL;
    }

    // Render the scene to the viewport.
    rval = g_pViewport->Render(g_pScene);
    if (FAILED(rval)) 
    {
        MSG("Rendering scene failed.");
        return E_FAIL;
    }

    // Update the window.
    rval = g_pDevice->Update();
    if (FAILED(rval)) 
    {
        MSG("Updating device failed.");
        return E_FAIL;
    }
    return S_OK;

	/*	// Tick the scene
    if( SUCCEEDED( g_pScene->Move( 1.0f ) ) )
	{
		// Clear the viewport
		if( SUCCEEDED( g_pViewport->Clear( D3DRMCLEAR_ALL ) ) )
		{
			// Render the scene to the viewport
			if( SUCCEEDED( g_pViewport->Render( g_pScene ) ) )
			{
				// Update the window
				if( SUCCEEDED( g_pDevice->Update() ) )
					return S_OK;
			}
		}
	}

	MSG("Could not render the scene.\n" );
    return E_FAIL;
	*/
}

void CreateDandV(int width, int height)
{
	int rval;

	g_bInitialized = FALSE;
	SAFE_RELEASE(g_pViewport);
	SAFE_RELEASE(g_pDevice);
	if (rval = CreateDevAndView(g_pDDClipper, g_CurrDriver, width, height))
	{
		g_CurrDriver = 0;
		if (rval = CreateDevAndView(g_pDDClipper, g_CurrDriver, width, height))
		{
			MSG("Failed to create the D3DRM device.\n" );
			return;
//			CleanUpAndPostQuit();
		}
		else
		{
			MSG("Not enough vidmem to use the 3D hardware device.\nUsing software rendering.");
			g_bInitialized = TRUE;
		}
	}
	else
	{
		g_bInitialized = TRUE;
	}
	g_bInitialized = TRUE;
}

//-----------------------------------------------------------------------------
// Name: WindowProc()
// Desc: Main window message handler
//-----------------------------------------------------------------------------
LONG FAR PASCAL WindowProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
    HRESULT		rval;
    RECT		rc;
    int		i, failcount = 0;  // number of times RenderScene() has failed

	if( !g_bInitialized )
		return DefWindowProc(win, msg, wparam, lparam);

    switch( msg )
	{
//		case WM_CREATE:	//it not works...

		case WM_TIMER:
			if(!move_plane())
			{	// ESC key pushed
				CleanUpAndPostQuit();
				PostMessage(NULL, WM_DESTROY, 0, 0);
                break;
			}

			if(JR_propo==2) JRstart = 1;	/* Reset each input change */
			if(JR_propo>=3 && pcmlock==0) 
			{
				if(pcm_start(win)==0) pcmlock = 1;
			}
			// If the app is not minimized, not about to quit and D3DRM has
	        // been initialized, we can render
	        if( !g_bMinimized && !g_bQuit && g_bInitialized )
			{
	            // If were are not in single step mode or if we are and the
	            // bDrawAFrame flag is set, render one frame
	            if( !( g_bSingleStepMode && !g_bDrawAFrame ) ) 
				{
					// Attempt to render a frame, if it fails, take a note.  If
					// rendering fails more than twice, abort execution.
					if( FAILED( RenderScene() ) )
						++failcount;
					if (failcount > 2)
					{
						MSG( "Rendering has failed too many times.  Aborting execution.\n" );
						CleanUpAndPostQuit();
						PostMessage(NULL, WM_DESTROY, 0, 0);
						break;
					}
				}
			}
			break;

        case MM_WIM_DATA:   //it not works... // notification recording has stopped
//			MSG("MM_WIM");
            if(MMSYSERR_NOERROR != waveInClose(hWaveIn))
				MSG("Err Close");
			else
			{
				// この関数なぜか必ずエラーを返す
				waveInUnprepareHeader(hWaveIn, &WaveHdr, sizeof(WAVEHDR));
				get_ch();
			}
			pcmlock = 0;
			break;

        case WM_MOUSEWHEEL:
            ((short) HIWORD(wparam)< 0) ? g_wWheel=-1 : g_wWheel=1;
			break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
            g_wMouseButtons = wparam;		// Record the mouse state
            g_wMouseX       = LOWORD(lparam);
            g_wMouseY       = HIWORD(lparam);
            break;

		case WM_INITMENUPOPUP:
            // Check and enable the appropriate menu items
			for (i = 0; i < g_NumDrivers; i++)
                CheckMenuItem((HMENU)wparam, MENU_FIRST_DRIVER + i,
                       (i == g_CurrDriver) ? MF_CHECKED : MF_UNCHECKED);
            break;

        case WM_COMMAND:
            switch( LOWORD(wparam) )
			{
                case MENU_EXIT:
                    CleanUpAndPostQuit();
                    break;
            }
            
            // Changing the D3D Driver
			if( LOWORD(wparam) >= MENU_FIRST_DRIVER &&
                    LOWORD(wparam) < MENU_FIRST_DRIVER + MAX_DRIVERS &&
                    g_CurrDriver != LOWORD(wparam) - MENU_FIRST_DRIVER )
			{
				// Release the current viewport and device and create the new one
			    int LastDriver = g_CurrDriver;
			    g_bInitialized = FALSE;
                SAFE_RELEASE(g_pViewport);
                SAFE_RELEASE(g_pDevice);
                g_CurrDriver = LOWORD(wparam)-MENU_FIRST_DRIVER;
                GetClientRect(win, &rc);
                if (rval = CreateDevAndView(g_pDDClipper, g_CurrDriver, rc.right, rc.bottom) )
				{
					g_CurrDriver = LastDriver;
					if (rval = CreateDevAndView(g_pDDClipper, g_CurrDriver, rc.right, rc.bottom))
					{
						MSG("Failed to create the D3DRM device.\n" );
						CleanUpAndPostQuit();
					}
					else
					{
						MSG("Not enough vidmem to use the 3D hardware device.\nRestoring software device.");
						g_bInitialized = TRUE;
					}
                }
				else
				{
					g_bInitialized = TRUE;
				}
            }
            break;

		case WM_DESTROY:
			CleanUpAndPostQuit();
			KillTimer(win, 1);
            if(midi_ok) midiOutClose (hMidiOut);
			SAFE_RELEASE(childframe);
			SAFE_RELEASE(gliderframe);
			SAFE_RELEASE(trainframe);
			SAFE_RELEASE(stuntframe);
			SAFE_RELEASE(baloonframe);
			SAFE_RELEASE(cosmo0frame);
			SAFE_RELEASE(hoverframe);
			SAFE_RELEASE(heli1frame);
			SAFE_RELEASE(heli3dframe);
			SAFE_RELEASE(hel450frame);
			SAFE_RELEASE(helbeeframe);
			SAFE_RELEASE(penginframe);
			SAFE_RELEASE(ufoframe);
			SAFE_RELEASE(shadow_2frame);
			SAFE_RELEASE(shadow_1frame);
			SAFE_RELEASE(shadow0frame);
			SAFE_RELEASE(shadow1frame);
			SAFE_RELEASE(shadow2frame);
			SAFE_RELEASE(fieldframe);
//          PostQuitMessage(0);
 			break;
			
		case WM_SIZE:
			// Handle resizing of the window
			{
				int width = LOWORD(lparam);
				int height = HIWORD(lparam);
				if (width && height && g_pViewport && g_pDevice)
				{
					int view_width  = g_pViewport->GetWidth();
					int view_height = g_pViewport->GetHeight();
					int dev_width   = g_pDevice->GetWidth();
					int dev_height  = g_pDevice->GetHeight();
					// If the window hasn't changed size and we aren't returning from
					// a minimize, there is nothing to do
					if (view_width == width && view_height == height &&
						!g_bMinimized)
						break;
					if (width <= dev_width && height <= dev_height)
					{
						// If the window has shrunk, we can use the same device with a new viewport
						g_bInitialized = FALSE;
						SAFE_RELEASE(g_pViewport);
						rval = g_pD3DRM->CreateViewport(g_pDevice, g_pCamera,
													   0, 0, width, height,
													   &g_pViewport);
						if (rval != D3DRM_OK)
						{
							MSG("Failed to resize the viewport.\n" );
							CleanUpAndPostQuit();
							break;
						}
						rval = g_pViewport->SetBack(D3DVAL(5000.0));
						if (rval != D3DRM_OK)
						{
							MSG("Failed to set background depth after viewport resize.\n" );
							CleanUpAndPostQuit();
							break;
						}
						g_bInitialized = TRUE;
					} 
					else
					{
					 	// If the window got larger than the current device, create a new device.
						CreateDandV(width, height);
						if(g_bInitialized == FALSE)	CleanUpAndPostQuit();
					}
					// We must not longer be minimized
					g_bMinimized = FALSE;
				} 
				else
				{
					// This is a minimize message
					g_bMinimized = TRUE;
				}
			}
			g_bDrawAFrame = TRUE;
			break;
		case WM_ACTIVATE:
			{
				 // Create a Windows specific D3DRM window device to handle this message
				LPDIRECT3DRMWINDEVICE windev;
				if (!g_pDevice)
					break;
				if (SUCCEEDED(g_pDevice->QueryInterface(IID_IDirect3DRMWinDevice,
													(void **) &windev)))
				{
					if (FAILED(windev->HandleActivate(wparam)))
						MSG("Failed to handle WM_ACTIVATE.\n");
					windev->Release();
				}
				else
				{
					MSG("Failed to create Windows device to handle WM_ACTIVATE.\n");
				}
			}
			break;
		case WM_PAINT:
			if (!g_bInitialized || !g_pDevice)
				return DefWindowProc(win, msg, wparam, lparam);

			// Create a Windows specific D3DRM window device to handle this message
			RECT r;
			PAINTSTRUCT ps;
			LPDIRECT3DRMWINDEVICE windev;

			if (GetUpdateRect(win, &r, FALSE))
			{
				BeginPaint(win, &ps);
				if (SUCCEEDED(g_pDevice->QueryInterface(IID_IDirect3DRMWinDevice,
					(void **) &windev)))
				{
					if (FAILED(windev->HandlePaint(ps.hdc)))
						MSG("Failed to handle WM_PAINT.\n");
					windev->Release();
				}
				else
				{
					MSG("Failed to create Windows device to handle WM_PAINT.\n");
				}
				EndPaint(win, &ps);
			}
			break;
		case WM_DISPLAYCHANGE:
			{
				// If this display change message was generated because another application
				// has gone exclusive, ignore it.
				LPDIRECTDRAW lpDD;
				LPDIRECTDRAWSURFACE lpDDS;
				DDSURFACEDESC ddsd;
				HRESULT err;

				if (DirectDrawCreate(NULL, &lpDD, NULL))
					break;
				err = lpDD->SetCooperativeLevel(win, DDSCL_NORMAL);
				if (err)
				{
					lpDD->Release();
					break;
				}
				memset(&ddsd, 0, sizeof(ddsd));
				ddsd.dwSize = sizeof(ddsd);
				ddsd.dwFlags = DDSD_CAPS;
				ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
				err = lpDD->CreateSurface(&ddsd, &lpDDS, NULL);
				if (err == DDERR_NOEXCLUSIVEMODE)
				{
					// This exclusive mode generated WM_DISPLAYCHANGE, ignoring
					lpDD->Release();
					break;
				}
				if (!err)
					lpDDS->Release();
				lpDD->Release();	    
			}
			GetClientRect(win, &rc);
			CreateDandV(rc.right, rc.bottom);
			if(g_bInitialized == FALSE)	CleanUpAndPostQuit();
/*
			g_bInitialized = FALSE;
			if (rval = CreateDevAndView(g_pDDClipper, g_CurrDriver,
										rc.right, rc.bottom))
			{
				g_CurrDriver = 0;
				if (rval = CreateDevAndView(g_pDDClipper, g_CurrDriver,
							rc.right, rc.bottom))
				{
					MSG("Failed to create the D3DRM device.\n" );
					CleanUpAndPostQuit();
				}
				else
				{
					// Don't bother the user with an error message here
					// Msg("There was not enough video memory available to use the 3D accelerated hardware device.\nUsing software rendering instead.");
					g_bInitialized = TRUE;
				}
			}
			else
			{
				g_bInitialized = TRUE;
			}
*/
			break;
		default:
			return DefWindowProc(win, msg, wparam, lparam);
    }

    return 0L;
}

//-----------------------------------------------------------------------------
// Name: CleanUpAndPostQuit()
// Desc: Release all D3DRM objects, post a quit message and set the bQuit flag
//-----------------------------------------------------------------------------
VOID CleanUpAndPostQuit()
{
    SAFE_RELEASE( g_pScene );
    SAFE_RELEASE( g_pCamera );
    SAFE_RELEASE( g_pViewport );
    SAFE_RELEASE( g_pDevice );
    SAFE_RELEASE( g_pD3DRM );
    SAFE_RELEASE( g_pDDClipper );

    g_bInitialized = FALSE;
    g_bQuit        = TRUE;
}




