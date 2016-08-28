// midiout.h 

#define IDM_PLAY        1       // menu item ID numbers
#define IDM_STOP        2
#define IDM_QUIT        3

#define APPNAME         "midiout"

typedef struct tagMIDIDATA  // structure to hold MIDI short data
{
    DWORD   dwTimeMs ;      // time of event, ms from start
    DWORD   dwMidiData ;    // midi short message
} MIDIDATA ;
typedef MIDIDATA FAR * LPMIDIDATA ;

                        // short demo song data
/*MIDIDATA MidiData [6] = {0, 0x403c90,       // middle C on
                        200, 0x003c90,      // middle C off
                        300, 0x16c0,      // G off
                        400, 0x404090,      // G on
                        9000, 0x004090,      // G off
//                      800, 0x404090,      // E on
                        10000, 0x004090 } ;  // E off
*/

typedef struct tagMIDISONG  // structure to hold a bunch of MIDI data
{
    HMIDIOUT        hMidiOut ;      // midi output port handle
    DWORD           dwEvents ;      // number of events
    DWORD           dwLastEvent ;   // ID number of last event played
    DWORD           dwTime ;        // current ms time for playback
    MIDIDATA far    *lpMidiData ;   // pointer to array of MIDIDATA
} MIDISONG ;
typedef MIDISONG FAR * LPMIDISONG ;

// function prototypes 

LRESULT FAR PASCAL WndProc (HWND, UINT, WPARAM, LPARAM) ;

// midi_cb.h  header file for midi_cb.dll

void CALLBACK _export MidiOutCall (HMIDIOUT hMidiOut, UINT wMsg,
    DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) ;
