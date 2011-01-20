#include "ofxPd.h"

#include "ofUtils.h"

#define USEAPI_DUMMY

// pointer to this class for static callback member functions
ofxPd* thisPd = NULL;

//--------------------------------------------------------------------
ofxPd::ofxPd()
{
	// common
	bVerbose 			= false;
	bPdInited 			= false;
	
	thisPd = this;
}


//--------------------------------------------------------------------
ofxPd::~ofxPd()
{
    pdClear();
}

//--------------------------------------------------------------------
bool ofxPd::pdInit()
{
	pdClear();
	
	// attach callbacks
	libpd_printhook = (t_libpd_printhook) _print;
	libpd_noteonhook = (t_libpd_noteonhook) _noteon;
	libpd_controlchangehook = (t_libpd_controlchangehook) _controlchange;
	libpd_programchangehook = (t_libpd_programchangehook) _programchange;
	libpd_pitchbendhook = (t_libpd_pitchbendhook) _pitchbend;
	libpd_aftertouchhook = (t_libpd_aftertouchhook) _aftertouch;
	libpd_polyaftertouchhook = (t_libpd_polyaftertouchhook) _polyaftertouch;
	
	// init pd
	srate = 44100;
	libpd_init();
	if(libpd_init_audio(2, 2, srate, 1) != 0)
	{
		ofLog(OF_LOG_ERROR, "ofxPd: could not init");
		OF_EXIT_APP(0);
	}
	
    bPdInited = true;
    //if(bVerbose)
	//	cout << "ofxPd: Inited" << endl;

    return bPdInited;
}

void ofxPd::pdClear()
{
    bPdInited = false;
}

void ofxPd::pdUpdate()
{
	libpd_process_float(inbuf, outbuf);
}

void ofxPd::pdAddToSearchPath(string path)
{
	libpd_add_to_search_path(path.c_str());
}
		
void ofxPd::pdClearSearchPath()
{
	libpd_clear_search_path();
}

void ofxPd::pdOpenPatch(string file, string folder)
{
	// [; pd open file folder(
	libpd_start_message();
	libpd_add_symbol(file.c_str());
	libpd_add_symbol(folder.c_str());
	if(libpd_finish_message("pd", "open") != 0)
	{
		ofLog(OF_LOG_ERROR, "ofxPd: couldn't open file");
	}
}

void ofxPd::pdClosePatch(string name)
{
	// [; pd-name menuclose 1(
	string patchname = (string) "pd-"+name;
	libpd_start_message();
	libpd_add_float(1.0f);
	libpd_finish_message(patchname.c_str(), "menuclose");
}

void ofxPd::pdDspOn()
{
	// [; pd dsp 1(
	libpd_start_message();
	libpd_add_float(1.0f);
	libpd_finish_message("pd", "dsp");
}

void ofxPd::pdDspOff()
{
	// [; pd dsp 0(
	libpd_start_message();
	libpd_add_float(0.0f);
	libpd_finish_message("pd", "dsp");
}

/* ***** PRIVATE ***** */

//----------------------------------------------------------
void ofxPd::_print(const char *s)
{
	cout << "print: " << s << endl;
	thisPd->pdPrintReceived((string) s);
}

void ofxPd::_noteon(int channel, int pitch, int velocity)
{
	cout << "noteon" << endl;
	thisPd->pdNoteonReceived(channel, pitch, velocity);
}

void ofxPd::_controlchange(int channel, int controller, int val)
{
	thisPd->pdControlChangeReceived(channel, controller, val);
}

void ofxPd::_programchange(int channel, int program)
{
	thisPd->pdProgramChangeReceived(channel, program);
}

void ofxPd::_pitchbend(int channel, int val)
{
	thisPd->pdPitchbendReceived(channel, val);
}

void ofxPd::_aftertouch(int channel, int val)
{
	thisPd->pdAftertouchReceived(channel, val);
}

void ofxPd::_polyaftertouch(int channel, int pitch, int val)
{
	thisPd->pdPolyAftertouchReceived(channel, pitch, val);
}
