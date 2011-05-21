#include "ofxPd.h"

#include "ofUtils.h"

#include <algorithm>
#include <Poco/Path.h>

// needed for libpd audio passing
#define USEAPI_DUMMY

using namespace std;

// pointer for static member functions
ofxPd* pdPtr = NULL;


#ifdef TARGET_LINUX

#include <pthread.h>
static pthread_mutex_t mutex;
#define _LOCK() pthread_mutex_lock(&mutex)
#define _UNLOCK() pthread_mutex_unlock( &mutex )

#else

#define _LOCK()
#define _UNLOCK()

#endif


//--------------------------------------------------------------------
ofxPd::ofxPd() {
	pdPtr = this;
	bPdInited = false;
	inputBuffer = NULL;
	clear();

#ifdef TARGET_LINUX
	pthread_mutex_init( &mutex, NULL );
#endif
}

//--------------------------------------------------------------------
ofxPd::~ofxPd() {
    clear();
#ifdef TARGET_LINUX
	pthread_mutex_destroy( &mutex );
#endif
	sources.clear();
}

//--------------------------------------------------------------------
bool ofxPd::init(const int numInChannels, 
	const int numOutChannels,  const int sampleRate) {
	
	clear();
	
	this->sampleRate = sampleRate;
	this->numInChannels = numInChannels;
	this->numOutChannels = numOutChannels;
	
	// allocate buffers
	inputBuffer = new float[numInChannels*getBlockSize()];
	
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
	_LOCK();
	libpd_init();
	if(libpd_init_audio(numInChannels, numOutChannels,
		this->sampleRate*2, 1) != 0) {
		_UNLOCK();
		ofLog(OF_LOG_ERROR, "ofxPd: Could not init");
		return false;
	}
	_UNLOCK();
	
    bPdInited = true;
	ostringstream status;
	status 	<< "Inited"
			<< " samplerate: " << sampleRate
			<< " channels in: " << numInChannels
			<< " out: " << numOutChannels
			<< " blocksize: " << getBlockSize();
	ofLog(OF_LOG_VERBOSE, "ofxPd: "+status.str());

    return bPdInited;
}

void ofxPd::clear() {
	_LOCK();
	if(bPdInited) {
		if(inputBuffer)	delete[] inputBuffer;

	}
	
	bPdInited = false;
	
	sampleRate = 0;
	numInChannels = 0;
	numOutChannels = 0;
	inputBuffer = NULL;
	
	bMsgInProgress = false;
	msgType = LIST;
	midiPort = 0;
	_UNLOCK();

	clearSources();
}

void ofxPd::addToSearchPath(const std::string& path) {
	_LOCK();
	libpd_add_to_search_path(path.c_str());
	_UNLOCK();
}
		
void ofxPd::clearSearchPath() {
	_LOCK();
	libpd_clear_search_path();
	_UNLOCK();
}

//
//	references http://pocoproject.org/docs/Poco.Path.html
//
void ofxPd::openPatch(const std::string& patch) {

	Poco::Path path(ofToDataPath(patch));
	string folder = path.parent().toString();
	
	// trim the trailing slash Poco::Path always adds ... blarg
	if(folder.size() > 0 && folder.at(folder.size()-1) == '/') {
		folder.erase(folder.end()-1);
	}
	
	ofLog(OF_LOG_VERBOSE, "ofxPd: Opening filename: "+path.getFileName()+
						  " path: "+folder);

	// [; pd open file folder(
	_LOCK();
	libpd_start_message();
	libpd_add_symbol(path.getFileName().c_str());
	libpd_add_symbol(folder.c_str());
	libpd_finish_message("pd", "open");
	_UNLOCK();
}

void ofxPd::closePatch(const std::string& name) {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Closing name: "+name);

	// [; pd-name menuclose 1(
	string patchname = (string) "pd-"+name;
	_LOCK();
	libpd_start_message();
	libpd_add_float(1.0f);
	libpd_finish_message(patchname.c_str(), "menuclose");
	_UNLOCK();
}

void ofxPd::dspOn() {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Dsp on");
	
	// [; pd dsp 1(
	_LOCK();
	libpd_start_message();
	libpd_add_float(1.0f);
	libpd_finish_message("pd", "dsp");
	_UNLOCK();
}

void ofxPd::dspOff() {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Dsp off");
	
	// [; pd dsp 0(
	_LOCK();
	libpd_start_message();
	libpd_add_float(0.0f);
	libpd_finish_message("pd", "dsp");
	_UNLOCK();
}

//--------------------------------------------------------------------
void ofxPd::addListener(ofxPdListener& listener, const std::string& source) {
	
	pair<set<ofxPdListener*>::iterator, bool> ret;
	ret = listeners.insert(&listener);
	if(!ret.second) {
		ofLog(OF_LOG_WARNING, "ofxPd: addListener: ignoring duplicate listener");
		return;
	}

	// add source if not existing
	if(!sourceExists(source)) {
		addSource(source);
	}
	
	// subscribe to source
	sources.find(source)->second.addListener(&listener);
}

void ofxPd::removeListener(ofxPdListener& listener) {
	
	// exists?
	set<ofxPdListener*>::iterator l_iter;
	l_iter = listeners.find(&listener);
	if(l_iter == listeners.end()) {
		ofLog(OF_LOG_WARNING, "ofxPd: removeListener: ignoring unknown listener");
		return;
	}
	listeners.erase(l_iter);

	// remove from all sources
	map<string, Source>::iterator s_iter = sources.begin();
	for(s_iter = sources.begin(); s_iter != sources.end(); ++s_iter) {
		s_iter->second.removeListener(&listener);
	}
		
}

bool ofxPd::listenerExists(ofxPdListener& listener) {
	if(listeners.find(&listener) != listeners.end())
		return true;
	return false;
}

void ofxPd::clearListeners() {

	listeners.clear();
	
	map<string, Source>::iterator iter;
	for(iter = sources.begin(); iter != sources.end(); ++iter) {
		iter->second.listeners.clear();
	}
}

//----------------------------------------------------------
void ofxPd::addSource(const std::string& source) {
	if(sourceExists(source)) {
		ofLog(OF_LOG_WARNING, "ofxPd: addSource: ignoring duplicate source");
		return;
	}
	
	Source s;
	s.pointer = libpd_bind(source.c_str());
	sources.insert(pair<string, Source>(source, s));
}

void ofxPd::removeSource(const std::string& source) {
	
	map<string, Source>::iterator iter;
	iter = sources.find(source);
	if(iter == sources.end()) {
		ofLog(OF_LOG_WARNING, "ofxPd: removeSource: ignoring unknown source");
		return;
	}
	
	libpd_unbind(iter->second.pointer);
	sources.erase(iter);
}

bool ofxPd::sourceExists(const std::string& source) {
	if(sources.find(source) != sources.end())
		return true;
	return false;
}

void ofxPd::clearSources() {
	
	sources.clear();

	// add default global source
	Source s;
	s.pointer = NULL;
	sources.insert(make_pair("", s));
}

//----------------------------------------------------------
void ofxPd::subscribe(ofxPdListener& listener, const std::string& source) {

	if(!listenerExists(listener)) {
		ofLog(OF_LOG_WARNING, "ofxPd: subscribe: unknown listener, use addListener first");
		return;
	}

	map<string, Source>::iterator iter;
	iter = sources.find(source);
	if(iter == sources.end()) {
		ofLog(OF_LOG_WARNING, "ofxPd: unsubscribe: unknown source, use addSource first");
		return;
	}
	
	iter->second.addListener(&listener);
}

void ofxPd::unsubscribe(ofxPdListener& listener, const std::string& source) {

	if(!listenerExists(listener)) {
		ofLog(OF_LOG_WARNING, "ofxPd: unsubscribe: ignoring unknown listener");
		return;
	}

	map<string, Source>::iterator iter;
	iter = sources.find(source);
	if(iter == sources.end()) {
		ofLog(OF_LOG_WARNING, "ofxPd: unsubscribe: ignoring unknown source");
		return;
	}
	
	iter->second.removeListener(&listener);
}

//----------------------------------------------------------
void ofxPd::sendBang(const std::string& dest) {
	_LOCK();
	libpd_bang(dest.c_str());
	_UNLOCK();
}

void ofxPd::sendFloat(const std::string& dest, float value) {
	_LOCK();
	libpd_float(dest.c_str(), value);
	_UNLOCK();
}

void ofxPd::sendSymbol(const std::string& dest, const std::string& symbol) {
	_LOCK();
	libpd_symbol(dest.c_str(), symbol.c_str());
	_UNLOCK();
}

//----------------------------------------------------------
void ofxPd::startList(const std::string& dest) {
	
	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not start list, message in progress");
		return;
	}

	_LOCK();	
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

	_LOCK();	
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
	_UNLOCK();
	
	bMsgInProgress = false;
}

//----------------------------------------------------------
void ofxPd::sendNote(const int pitch, const int velocity, const int channel) {
	_LOCK();
	libpd_noteon(channel, pitch, velocity);
	_UNLOCK();
}

void ofxPd::sendControlChange(const int control, const int value, int channel) {
	_LOCK();
	libpd_controlchange(channel, control, value);
	_UNLOCK();
}

void ofxPd::sendProgramChange(int program, const int channel) {
	_LOCK();
	libpd_programchange(channel, program);
	_UNLOCK();
}

void ofxPd::sendPitchBend(const int value, const int channel) {
	_LOCK();
	libpd_pitchbend(channel, value);
	_UNLOCK();
}

void ofxPd::sendAftertouch(const int value, const int channel) {
	_LOCK();
	libpd_aftertouch(channel, value);
	_UNLOCK();
}

void ofxPd::sendPolyAftertouch(int note, int value, const int channel) {
	_LOCK();
	libpd_polyaftertouch(channel, note, value);
	_UNLOCK();
}

//----------------------------------------------------------
void ofxPd::sendMidiByte(const int value, const int port) {
	_LOCK();
	libpd_midibyte(port, value);
	_UNLOCK();
}

void ofxPd::sendSysExByte(const int value, const int port) {
	_LOCK();
	libpd_sysex(port, value);
	_UNLOCK();
}

void ofxPd::sendSysRealtimeByte(const int value, const int port) {
	_LOCK();
	libpd_sysrealtime(port, value);
	_UNLOCK();
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
	string s;
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
int ofxPd::getBlockSize() {
	_LOCK();
	int bs = libpd_blocksize();
	_UNLOCK();
	return bs;
}

//----------------------------------------------------------
void ofxPd::audioIn(float * input, int bufferSize, int nChannels) {
	
	try {
	_LOCK();
		if ( inputBuffer )
			memcpy(inputBuffer, input, bufferSize*nChannels);
	_UNLOCK();
	}
	catch (...) {
		ofLog(OF_LOG_ERROR, (string) "ofxPd: could not copy input buffer, " +
			"check your buffersize and num channels");
	}
}

void ofxPd::audioOut(float * output, int bufferSize, int nChannels) {
	
	_LOCK();
	if(libpd_process_float(inputBuffer, output) != 0) {
		ofLog(OF_LOG_ERROR, (string) "ofxPd: could not process output buffer, " +
			"check your buffersize and num channels");
	}
	_UNLOCK();
}

/* ***** PRIVATE ***** */

//----------------------------------------------------------
void ofxPd::_print(const char* s)
{
	string line(s);
	
	if(line.size() > 0 && line.at(line.size()-1) == '\n') {
		
		// build the message
		if(line.size() > 1) {
			line.erase(line.end()-1);
			pdPtr->printMsg += line;
		}
		
		ofLog(OF_LOG_VERBOSE, "ofxPd: print: %s", pdPtr->printMsg.c_str());
		
		// broadcast
		set<ofxPdListener*>& listeners = pdPtr->listeners;
		set<ofxPdListener*>::iterator iter;
		for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
			(*iter)->printReceived(pdPtr->printMsg);
		}
	
		pdPtr->printMsg = "";
		return;
	}
		
	// build the message
	pdPtr->printMsg += line;
}
		
void ofxPd::_bang(const char* source)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: bang: %s", source);
	
	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->bangReceived((string) source);
	}
}

void ofxPd::_float(const char* source, float value)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: float: %s %d", source, value);
	
	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->floatReceived((string) source, value);
	}
}

void ofxPd::_symbol(const char* source, const char* symbol)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: symbol: %s %s", source, symbol);
	
	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->symbolReceived((string) source, (string) symbol);
	}
}

void ofxPd::_list(const char* source, int argc, t_atom* argv)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: list: %s", source);
	
	for(int i = 0; i < argc; i++) {
		
		t_atom a = argv[i];  
		
		if(a.a_type == A_FLOAT) {  
			float x = a.a_w.w_float;  
			ofLog(OF_LOG_VERBOSE, "ofxPd: %d", x); 
		}
		else if(a.a_type == A_SYMBOL) {  
			char *s = a.a_w.w_symbol->s_name;  
			ofLog(OF_LOG_VERBOSE, "ofxPd: %s", s);  
		}
	}
}

void ofxPd::_message(const char* source, const char *symbol, int argc, t_atom *argv)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: message: %s %s", source, symbol);
	
	for(int i = 0; i < argc; i++) {  
		
		t_atom a = argv[i];  
		
		if(a.a_type == A_FLOAT) {  
			float x = a.a_w.w_float;  
			ofLog(OF_LOG_VERBOSE, "ofxPd: %d", x); 
		}
		else if(a.a_type == A_SYMBOL) {  
			char *s = a.a_w.w_symbol->s_name;  
			ofLog(OF_LOG_VERBOSE, "ofxPd: %s", s);  
		}
	}
}

void ofxPd::_noteon(int channel, int pitch, int velocity)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: note: %d %d %d", channel, pitch, velocity);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->noteReceived(channel, pitch, velocity);
	}
}

void ofxPd::_controlchange(int channel, int controller, int value)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: control change: %d %d %d", channel, controller, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->controlChangeReceived(channel, controller, value);
	}
}

void ofxPd::_programchange(int channel, int value)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: program change: %d %d", channel, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->programChangeReceived(channel, value);
	}
}

void ofxPd::_pitchbend(int channel, int value)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: pitchbend: %d %d", channel, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->pitchBendReceived(channel, value);
	}
}

void ofxPd::_aftertouch(int channel, int value)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: aftertouch: %d %d", channel, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->aftertouchReceived(channel, value);
	}
}

void ofxPd::_polyaftertouch(int channel, int pitch, int value) {

	ofLog(OF_LOG_VERBOSE, "ofxPd: polyaftertouch: %d %d %d", channel, pitch, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->polyAftertouchReceived(channel, pitch, value);
	}
}

void ofxPd::_midibyte(int port, int byte) {
	
	ofLog(OF_LOG_VERBOSE, "ofxPd: midibyte: %d %d", port, byte);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->midiByteReceived(port, byte);
	}
}
