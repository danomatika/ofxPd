#include "ofxPd.h"

#include "ofUtils.h"

#include <Poco/Path.h>

// needed for libpd audio passing
#define USEAPI_DUMMY

// the listeners
vector<ofxPdListener*> _listeners;

//--------------------------------------------------------------------
ofxPd::ofxPd() {
	
	bPdInited = false;
	
	sampleRate = 0;
	numInChannels = 0;
	numOutChannels = 0;
	inputBuffer = NULL;
	
	bMsgInProgress = false;
	msgType = LIST;
	midiPort = 0;
}

//--------------------------------------------------------------------
ofxPd::~ofxPd() {
    clear();
}

//--------------------------------------------------------------------
bool ofxPd::init(const int numInChannels, 
	const int numOutChannels,  const int sampleRate) {
	
	clear();
	
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
	
	libpd_midibytehook = (t_libpd_midibytehook) _midibyte;
	
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

void ofxPd::clear() {
	if(bPdInited) {
		if(inputBuffer)	delete[] inputBuffer;
	}
	bPdInited = false;
}

void ofxPd::addToSearchPath(const string& path) {
	libpd_add_to_search_path(path.c_str());
}
		
void ofxPd::clearSearchPath() {
	libpd_clear_search_path();
}

//
//	references http://pocoproject.org/docs/Poco.Path.html
//
void ofxPd::openPatch(const string& patch) {

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

void ofxPd::closePatch(const string& name) {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Closing name: "+name);

	// [; pd-name menuclose 1(
	string patchname = (string) "pd-"+name;
	libpd_start_message();
	libpd_add_float(1.0f);
	libpd_finish_message(patchname.c_str(), "menuclose");
}

void ofxPd::dspOn() {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Dsp on");
	
	// [; pd dsp 1(
	libpd_start_message();
	libpd_add_float(1.0f);
	libpd_finish_message("pd", "dsp");
}

void ofxPd::dspOff() {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Dsp off");
	
	// [; pd dsp 0(
	libpd_start_message();
	libpd_add_float(0.0f);
	libpd_finish_message("pd", "dsp");
}

//----------------------------------------------------------
void ofxPd::sendBang(const std::string& dest) {
	libpd_bang(dest.c_str());
}

void ofxPd::sendFloat(const std::string& dest, float value) {
	libpd_float(dest.c_str(), value);
}

void ofxPd::sendSymbol(const std::string& dest, const std::string& symbol) {
	libpd_symbol(dest.c_str(), symbol.c_str());
}

//----------------------------------------------------------
void ofxPd::startList(const std::string& dest) {
	
	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not start list, message in progress");
		return;
	}
	
	libpd_start_message();
	bMsgInProgress = true;
	msgType = LIST;
	msgDest = dest;
}

void ofxPd::startMessage(const std::string& dest, const std::string& msg) {
	
	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not start message, message in progress");
		return;
	}
	
	libpd_start_message();
	bMsgInProgress = true;
	msgType = MSG;
	msgDest = dest;
	msgMsg = msg;
}

void ofxPd::addFloat(const float value) {

	if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add float, message not in progress");
		return;
	}
	
	if(msgType != LIST && msgType != MSG) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add float, midi message in progress");
		return;
	}
	
	libpd_add_float(value);
}

void ofxPd::addSymbol(const std::string& symbol) {

	if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add symbol, message not in progress");
		return;
	}
	
	if(msgType != LIST && msgType != MSG) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add float, midi message in progress");
		return;
	}
	
	libpd_add_symbol(symbol.c_str());
}

void ofxPd::finish() {

	if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not finish message, message not in progress");
		return;
	}
	
	switch(msgType) {
		
		case LIST:
			libpd_finish_list(msgDest.c_str());
			break;
		
		case MSG:
			libpd_finish_message(msgDest.c_str(), msgMsg.c_str());
			break;
	}
	
	bMsgInProgress = false;
}

//----------------------------------------------------------
void ofxPd::sendNote(const int pitch, const int velocity, const int channel) {
	libpd_noteon(channel, pitch, velocity);
}

void ofxPd::sendControlChange(const int control, const int value, int channel) {
	libpd_controlchange(channel, control, value);
}

void ofxPd::sendProgramChange(int program, const int channel) {
	libpd_programchange(channel, program);
}

void ofxPd::sendPitchBend(const int value, const int channel) {
	libpd_pitchbend(channel, value);
}

void ofxPd::sendAftertouch(const int value, const int channel) {
	libpd_aftertouch(channel, value);
}

void ofxPd::sendPolyAftertouch(int note, int value, const int channel) {
	libpd_polyaftertouch(channel, note, value);
}

//----------------------------------------------------------
void ofxPd::sendMidiByte(const int value, const int port) {
	libpd_midibyte(port, value);
}

void ofxPd::sendSysExByte(const int value, const int port) {
	libpd_sysex(port, value);
}

void ofxPd::sendSysRealtimeByte(const int value, const int port) {
	libpd_sysrealtime(port, value);
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const Bang& var) {

	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not send Bang, message in progress");
		return *this;
	}
	
	libpd_bang(var.dest.c_str());
    
    return *this;
}

ofxPd& ofxPd::operator<<(const Float& var) {

	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not send Float, message in progress");
		return *this;
	}
	
	libpd_float(var.dest.c_str(), var.value);
    
    return *this;
}

ofxPd& ofxPd::operator<<(const Symbol& var) {

	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not send Symbol, message in progress");
		return *this;
	}
	
	libpd_symbol(var.dest.c_str(), var.symbol.c_str());
    
    return *this;
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const StartList& var) {
	startList(var.dest);
    return *this;
}

ofxPd& ofxPd::operator<<(const StartMessage& var) {
	startMessage(var.dest, var.msg);
    return *this;
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const bool var) {
	addFloat((float) var);
	return *this;
}

ofxPd& ofxPd::operator<<(const int var) {
    
	switch(msgType) {
	
		case LIST: case MSG:
			addFloat((float) var);
			break;
			
		case MIDI:
			libpd_midibyte(midiPort, var);
			break;
			
		case SYSEX:
			libpd_sysex(midiPort, var);
			break;
			
		case SYSRT:
			libpd_sysrealtime(midiPort, var);
			break;
	}

	return *this;
}

ofxPd& ofxPd::operator<<(const float var) {
    addFloat((float) var);
	return *this;
}

ofxPd& ofxPd::operator<<(const double var) {  
    addFloat((float) var);
	return *this;
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const char var) {
	std::string s;
	s = var;
	addSymbol(s);
	return *this;	
}

ofxPd& ofxPd::operator<<(const char* var) {
	addSymbol((string) var);
	return *this;	
}

ofxPd& ofxPd::operator<<(const std::string& var) {
	addSymbol(var);
	return *this;	
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const Note& var) {
	libpd_noteon(var.channel, var.pitch, var.velocity);
	return *this;
}

ofxPd& ofxPd::operator<<(const ControlChange& var) {
	libpd_controlchange(var.channel, var.controller, var.value);
	return *this;
}

ofxPd& ofxPd::operator<<(const ProgramChange& var) {
	libpd_programchange(var.channel, var.value);
	return *this;
}

ofxPd& ofxPd::operator<<(const PitchBend& var) {
	libpd_pitchbend(var.channel, var.value);
	return *this;
}

ofxPd& ofxPd::operator<<(const Aftertouch& var) {
	libpd_aftertouch(var.channel, var.value);
	return *this;
}

ofxPd& ofxPd::operator<<(const PolyAftertouch& var) {
	libpd_polyaftertouch(var.channel, var.pitch, var.value);
	return *this;
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const StartMidi& var) {
	
	if(bMsgInProgress) {
		ofLog(OF_LOG_ERROR, "ofxPd: Can not start MidiByte stream, message in progress");
		return *this;
	}
	
	bMsgInProgress = true;
	msgType = MIDI;
	midiPort = var.port;

	return *this;
}

ofxPd& ofxPd::operator<<(const StartSysEx& var) {

	if(bMsgInProgress) {
		ofLog(OF_LOG_ERROR, "ofxPd: Can not start SysEx stream, message in progress");
		return *this;
	}
	
	bMsgInProgress = true;
	msgType = SYSEX;
	midiPort = var.port;

	return *this;
}

ofxPd& ofxPd::operator<<(const StartSysRealtime& var) {

	if(bMsgInProgress) {
		ofLog(OF_LOG_ERROR, "ofxPd: Can not start SysRealtime stream, message in progress");
		return *this;
	}
	
	bMsgInProgress = true;
	msgType = SYSRT;
	midiPort = var.port;

	return *this;
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const Finish& var) {
	finish();
    return *this;
}

//----------------------------------------------------------
void ofxPd::bind(const string& source)
{
	libpd_bind(source.c_str());
}

void ofxPd::unbind(const string& source)
{
	//libpd_unbind(source.c_str());
}

//--------------------------------------------------------------------
void ofxPd::addListener(ofxPdListener *listener) {
	_listeners.push_back(listener);
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
		ofLog(OF_LOG_ERROR, (string) "ofxPd: could not copy input buffer, " +
			"check your buffersize and num channels");
	}
}

void ofxPd::audioOut(float * output, int bufferSize, int nChannels) {
	
	if(libpd_process_float(inputBuffer, output) != 0) {
		ofLog(OF_LOG_ERROR, (string) "ofxPd: could not process output buffer, " +
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
	for(int i = 0; i < _listeners.size(); i++) {
		_listeners[i]->printReceived(line);
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
	for(int i = 0; i < _listeners.size(); i++) {
		_listeners[i]->noteReceived(channel, pitch, velocity);
	}
}

void ofxPd::_controlchange(int channel, int controller, int value)
{
	for(int i = 0; i < _listeners.size(); i++) {
		_listeners[i]->controlChangeReceived(channel, controller, value);
	}
}

void ofxPd::_programchange(int channel, int value)
{
	for(int i = 0; i < _listeners.size(); i++) {
		_listeners[i]->programChangeReceived(channel, value);
	}
}

void ofxPd::_pitchbend(int channel, int value)
{
	for(int i = 0; i < _listeners.size(); i++) {
		_listeners[i]->pitchBendReceived(channel, value);
	}
}

void ofxPd::_aftertouch(int channel, int value)
{
	for(int i = 0; i < _listeners.size(); i++) {
		_listeners[i]->aftertouchReceived(channel, value);
	}
}

void ofxPd::_polyaftertouch(int channel, int pitch, int value)
{
	for(int i = 0; i < _listeners.size(); i++) {
		_listeners[i]->polyAftertouchReceived(channel, pitch, value);
	}
}

void ofxPd::_midibyte(int port, int byte) {
	
	for(int i = 0; i < _listeners.size(); i++) {
		_listeners[i]->midiByteReceived(port, byte);
	}
}
