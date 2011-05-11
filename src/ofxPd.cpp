#include "ofxPd.h"

#include "ofUtils.h"

#include <Poco/Path.h>

// needed for libpd audio passing
#define USEAPI_DUMMY

// the listeners 
vector<ofxPdListener*> pdListeners;

//--------------------------------------------------------------------
ofxPd::ofxPd() {
	bPdInited = false;
	sampleRate = 0;
	numInChannels = 0;
	numOutChannels = 0;
	inputBuffer = NULL;
}

//--------------------------------------------------------------------
ofxPd::~ofxPd() {
    pdClear();
}

void ofxPd::addListener(ofxPdListener *listener) {
	pdListeners.push_back(listener);
}

//--------------------------------------------------------------------
bool ofxPd::pdInit(const int numInChannels, 
	const int numOutChannels,  const int sampleRate) {
	
	pdClear();
	
	this->sampleRate = sampleRate;
	this->numInChannels = numInChannels;
	this->numOutChannels = numOutChannels;
	
	// allocate buffers
	inputBuffer = new float[numInChannels*getBlocksize()];
	
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
	libpd_init();
	if(libpd_init_audio(numInChannels, numOutChannels,
		this->sampleRate, 1) != 0) {
		ofLog(OF_LOG_FATAL_ERROR, "ofxPd: Could not init");
		return false;
	}
	
    bPdInited = true;
	ostringstream status;
	status 	<< "Inited"
			<< " samplerate: " << sampleRate
			<< " channels in: " << numInChannels
			<< " out: " << numOutChannels
			<< " blocksize: " << getBlocksize();
	ofLog(OF_LOG_VERBOSE, "ofxPd: "+status.str());

    return bPdInited;
}

void ofxPd::pdClear()
{
	if(bPdInited) {
		if(inputBuffer)	delete[] inputBuffer;
	}
	bPdInited = false;
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
	if(folder.size() > 0 && folder.at(folder.size()-1) == '/') {
		folder.erase(folder.end()-1);
	}
	
	ofLog(OF_LOG_VERBOSE, "ofxPd: Opening filename: "+path.getFileName()+
						  " path: "+folder);

	// [; pd open file folder(
	libpd_start_message();
	libpd_add_symbol(path.getFileName().c_str());
	libpd_add_symbol(folder.c_str());
	libpd_finish_message("pd", "open");
}

void ofxPd::pdClosePatch(const string& name)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: Closing name: "+name);

	// [; pd-name menuclose 1(
	string patchname = (string) "pd-"+name;
	libpd_start_message();
	libpd_add_float(1.0f);
	libpd_finish_message(patchname.c_str(), "menuclose");
}

void ofxPd::pdDspOn()
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: Dsp on");
	
	// [; pd dsp 1(
	libpd_start_message();
	libpd_add_float(1.0f);
	libpd_finish_message("pd", "dsp");
}

void ofxPd::pdDspOff()
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: Dsp off");
	
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

//----------------------------------------------------------
int ofxPd::getBlocksize() {
	return libpd_blocksize();
}

//----------------------------------------------------------
void ofxPd::audioIn(float * input, int bufferSize, int nChannels) {
	try {
		memcpy(inputBuffer, input, bufferSize*nChannels);
	}
	catch (...) {
		ofLog(OF_LOG_WARNING, (string) "ofxPd: could not copy input buffer, " +
			"check your buffersize and num channels");
	}
}

void ofxPd::audioOut(float * output, int bufferSize, int nChannels) {
	try {
		libpd_process_float(inputBuffer, output);
	}
	catch (...) {
		ofLog(OF_LOG_WARNING, (string) "ofxPd: could not process output buffer, " +
			"check your buffersize and num channels");
	}
}

/* ***** PRIVATE ***** */

//----------------------------------------------------------
void ofxPd::_print(const char* s)
{
	string line(s);
	
	// trim the trailing newline each print line has ... blarg again
	if(line.size() > 0 && line.at(line.size()-1) == '\n') {
		line.erase(line.end()-1);
	}
	
	ofLog(OF_LOG_VERBOSE, "ofxPd: "+line);
	for(int i = 0; i < pdListeners.size(); i++) {
		pdListeners[i]->pdPrintReceived(line);
	}
}
		
void ofxPd::_bang(const char* source)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: bang: " + (string) source);
}

void ofxPd::_float(const char* source, float value)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: float: " + (string) source + " " + ofToString(value));
}

void ofxPd::_symbol(const char* source, const char* symbol)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: symbol: " + (string) source + " " + (string) symbol);
}

void ofxPd::_list(const char* source, int argc, t_atom* argv)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: list: " + (string) source);
	
	for(int i = 0; i < argc; i++) {
		
		t_atom a = argv[i];  
		
		if(a.a_type == A_FLOAT) {  
			float x = a.a_w.w_float;  
			ofLog(OF_LOG_VERBOSE, "ofxPd: "	+ ofToString(x)); 
		}
		else if(a.a_type == A_SYMBOL) {  
			char *s = a.a_w.w_symbol->s_name;  
			ofLog(OF_LOG_VERBOSE, "ofxPd: "	+ (string) s);  
		}
	}
}

void ofxPd::_message(const char* source, const char *symbol, int argc, t_atom *argv)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: message: " + (string) source + " " + (string) symbol);
	
	for(int i = 0; i < argc; i++) {  
		
		t_atom a = argv[i];  
		
		if(a.a_type == A_FLOAT) {  
			float x = a.a_w.w_float;  
			ofLog(OF_LOG_VERBOSE, "ofxPd: "	+ ofToString(x)); 
		}
		else if(a.a_type == A_SYMBOL) {  
			char *s = a.a_w.w_symbol->s_name;  
			ofLog(OF_LOG_VERBOSE, "ofxPd: "	+ (string) s);  
		}
	}
}

void ofxPd::_noteon(int channel, int pitch, int velocity)
{
	for(int i = 0; i < pdListeners.size(); i++) {
		pdListeners[i]->pdNoteonReceived(channel, pitch, velocity);
	}
}

void ofxPd::_controlchange(int channel, int controller, int val)
{
	for(int i = 0; i < pdListeners.size(); i++) {
		pdListeners[i]->pdControlChangeReceived(channel, controller, val);
	}
}

void ofxPd::_programchange(int channel, int program)
{
	for(int i = 0; i < pdListeners.size(); i++) {
		pdListeners[i]->pdProgramChangeReceived(channel, program);
	}
}

void ofxPd::_pitchbend(int channel, int val)
{
	for(int i = 0; i < pdListeners.size(); i++) {
		pdListeners[i]->pdPitchbendReceived(channel, val);
	}
}

void ofxPd::_aftertouch(int channel, int val)
{
	for(int i = 0; i < pdListeners.size(); i++) {
		pdListeners[i]->pdAftertouchReceived(channel, val);
	}
}

void ofxPd::_polyaftertouch(int channel, int pitch, int val)
{
	for(int i = 0; i < pdListeners.size(); i++) {
		pdListeners[i]->pdPolyAftertouchReceived(channel, pitch, val);
	}
}
