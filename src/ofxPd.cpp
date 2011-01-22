#include "ofxPd.h"

#include "ofUtils.h"

#include <Poco/Path.h>

// needed for libpd audio passing
#define USEAPI_DUMMY

// pointer to this class for static callback member functions
ofxPd* thisPd = NULL;

//--------------------------------------------------------------------
ofxPd::ofxPd()
{
	// common
	bPdInited 			= false;
	
	thisPd = this;
	
	ofLogAddTopic("ofxPd");
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
	
	libpd_banghook = (t_libpd_banghook) _bang;
	libpd_floathook = (t_libpd_floathook) _float;
	libpd_symbolhook = (t_libpd_symbolhook) _symbol;
	libpd_listhook = (t_libpd_listhook) _symbol;
	libpd_messagehook = (t_libpd_messagehook) _message;
	
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
		ofLogFatalError("ofxPd") << "could not init";
		return false;
	}
	
    bPdInited = true;
	ofLogVerbose("ofxPd") << "inited";

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

void ofxPd::pdAddToSearchPath(const string& path)
{
	libpd_add_to_search_path(path.c_str());
}
		
void ofxPd::pdClearSearchPath()
{
	libpd_clear_search_path();
}

//
//	references http://pocoproject.org/docs/Poco.Path.html
//
void ofxPd::pdOpenPatch(const string& patch)
{
	Poco::Path path(ofToDataPath(patch));
	string folder = path.parent().toString();
	
	// trim the trailing slash Poco::Path always adds ... blarg
	if(folder.size() > 0 && folder.at(folder.size()-1) == '/')
	{
		folder.erase(folder.end()-1);
	}
	
	ofLogVerbose("ofxPd") << "opening path: " << folder
				 << " filename: " << path.getFileName();

	// [; pd open file folder(
	libpd_start_message();
	libpd_add_symbol(path.getFileName().c_str());
	libpd_add_symbol(folder.c_str());
	libpd_finish_message("pd", "open");
}

void ofxPd::pdClosePatch(const string& name)
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

//----------------------------------------------------------
void ofxPd::pdSendFloat(const string& receiverName, float value)
{
	libpd_float(receiverName.c_str(), value);
}

void ofxPd::pdSendBang(const string& receiverName)
{

	libpd_bang(receiverName.c_str());
}

void ofxPd::pdSendMidiNote(int channel, int note, int velocity)
{
	libpd_start_message();
	libpd_add_float(note);
	libpd_add_float(velocity);
	libpd_add_float(channel);
	libpd_finish_list("#notein");
}

void ofxPd::pdSendMidiControlChange(int channel, int control, int value)
{
	libpd_controlchange(channel, control, value);
}

void ofxPd::pdSendMidiBend(int channel, int value)
{
	libpd_pitchbend(channel, value);
}

void ofxPd::pdSendMidiAfterTouch(int channel, int value)
{
	libpd_aftertouch(channel, value);
}

void ofxPd::pdSendMidiPolyTouch(int channel, int note, int value)
{
	libpd_polyaftertouch(channel, note, value);
}

void ofxPd::pdSendMidiProgramChange(int channel, int program)
{
	libpd_programchange(channel, program);
}

//----------------------------------------------------------
void ofxPd::pdBind(const string& source)
{

	libpd_bind(source.c_str());
}

void ofxPd::pdUnbind(const string& source)
{
	//libpd_unbind(source.c_str());
}

/* ***** PROTECTED ***** */

//----------------------------------------------------------
void ofxPd::audioRequested(float* buffer, int numFrames, int numChannels)
{
//	try
//	{
//		libpd_process_float(inbuf, outbuf);
//		memcpy(outbuf, buffer, numFrames*numChannels*sizeof(float));
//	}
//	catch(...)
//	{
//		ofLogFatalError("ofxPd") << "request error";
//	}
}

void ofxPd::audioReceived(float* buffer, int numFrames, int numChannels)
{
//	try
//	{
//		memcpy(buffer, inbuf, numFrames*numChannels*sizeof(float));
//	}
//	catch(...)
//	{
//		ofLogFatalError("ofxPd") << "request error";
//	}
}

/* ***** PRIVATE ***** */

//----------------------------------------------------------
void ofxPd::_print(const char* s)
{
	string line(s);
	
	// trim the trailing newline each print line has ... blarg again
	if(line.size() > 0 && line.at(line.size()-1) == '\n')
	{
		line.erase(line.end()-1);
	}
	
	ofLogDebug("ofxPd") << "[" << line << "]";
	thisPd->pdPrintReceived(line);
}
		
void ofxPd::_bang(const char* source)
{
	ofLogDebug("ofxPd") << "bang: " << source;
}

void ofxPd::_float(const char* source, float value)
{
	ofLogDebug("ofxPd") << "float: " << source << " " << value;
}

void ofxPd::_symbol(const char* source, const char* symbol)
{
	ofLogDebug("ofxPd") << "symbol: " << source << symbol;
}

void ofxPd::_list(const char* source, int argc, t_atom* argv)
{
	ofLogDebug("ofxPd") << "list: " << source;
	for(int i = 0; i < argc; i++)
	{  
		t_atom a = argv[i];  
		if(a.a_type == A_FLOAT)
		{  
			float x = a.a_w.w_float;  
			ofLogDebug("ofxPd") << "	" << x; 
		}
		else if(a.a_type == A_SYMBOL)
		{  
			char *s = a.a_w.w_symbol->s_name;  
			ofLogDebug("ofxPd") << "	" << s;  
		}
	}
}

void ofxPd::_message(const char* source, const char *symbol, int argc, t_atom *argv)
{
	ofLogDebug("ofxPd") << "message: " << source << " " << symbol;
	for(int i = 0; i < argc; i++)
	{  
		t_atom a = argv[i];  
		if(a.a_type == A_FLOAT)
		{  
			float x = a.a_w.w_float;  
			ofLogDebug("ofxPd") << "	" << x; 
		}
		else if(a.a_type == A_SYMBOL)
		{  
			char *s = a.a_w.w_symbol->s_name;  
			ofLogDebug("ofxPd") << "	" << s;  
		}
	}
}

void ofxPd::_noteon(int channel, int pitch, int velocity)
{
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
