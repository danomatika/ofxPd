/*
 * Copyright (c) 2011 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxPd for documentation
 *
 * This project uses libpd, copyrighted by Miller Puckette and others using the
 * "Standard Improved BSD License". See the file "LICENSE.txt" in src/pd.
 *
 * See http://gitorious.org/pdlib/pages/Libpd for documentation
 *
 */
#include "ofxPd.h"

#include "ofUtils.h"

#include <algorithm>
#include <Poco/Path.h>

// needed for libpd audio passing
#define USEAPI_DUMMY

using namespace std;

// pointer for static member functions
ofxPd* pdPtr = NULL;

#define _LOCK() pdPtr->mutex.lock()
#define _UNLOCK() pdPtr->mutex.unlock()

//--------------------------------------------------------------------
ofxPd::ofxPd() {
	pdPtr = this;
	bPdInited = false;
	inputBuffer = NULL;
	clear();
    maxMsgLen = 32;
}

//--------------------------------------------------------------------
ofxPd::~ofxPd() {
    clear();
	sources.clear();
}

//--------------------------------------------------------------------
bool ofxPd::init(const int numOutChannels, const int numInChannels, 
				 const int sampleRate, const int ticksPerBuffer) {
	clear();
	
	this->sampleRate = sampleRate;
	this->ticksPerBuffer = ticksPerBuffer;
	this->numInChannels = numInChannels;
	this->numOutChannels = numOutChannels;
	
	// allocate buffers
	inputBuffer = new float[numInChannels*ticksPerBuffer*getBlockSize()];
	
	// attach callbacks
	libpd_printhook = (t_libpd_printhook) _print;
	
	libpd_banghook = (t_libpd_banghook) _bang;
	libpd_floathook = (t_libpd_floathook) _float;
	libpd_symbolhook = (t_libpd_symbolhook) _symbol;
	libpd_listhook = (t_libpd_listhook) _list;
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
		sampleRate, ticksPerBuffer) != 0) {
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
			<< " ticks: " << ticksPerBuffer
			<< " blocksize: " << getBlockSize();
	ofLog(OF_LOG_VERBOSE, "ofxPd: "+status.str());

    return bPdInited;
}

void ofxPd::clear() {
	_LOCK();
	if(bPdInited) {
		if(inputBuffer) delete[] inputBuffer;
	}
	
	bPdInited = false;
	
	sampleRate = 0;
	ticksPerBuffer = 0;
	numInChannels = 0;
	numOutChannels = 0;
	inputBuffer = NULL;
	
	bMsgInProgress = false;
    curMsgLen = 0;
	msgType = LIST;
	midiPort = 0;
	_UNLOCK();

	clearSources();
}

//--------------------------------------------------------------------
void ofxPd::addToSearchPath(const std::string& path) {
	Poco::Path fullPath(ofToDataPath(path));
	_LOCK();
	libpd_add_to_search_path(fullPath.toString().c_str());
	_UNLOCK();
}
		
void ofxPd::clearSearchPath() {
	_LOCK();
	libpd_clear_search_path();
	_UNLOCK();
}

//--------------------------------------------------------------------
//
//	references http://pocoproject.org/docs/Poco.Path.html
//
Patch ofxPd::openPatch(const std::string& patch) {

	Poco::Path path(ofToDataPath(patch));
	string folder = path.parent().toString();
	
	// trim the trailing slash Poco::Path always adds ... blarg
	if(folder.size() > 0 && folder.at(folder.size()-1) == '/') {
		folder.erase(folder.end()-1);
	}
	
	ofLog(OF_LOG_VERBOSE, "ofxPd: Opening patch: "+path.getFileName()+
						  " path: "+folder);

	// [; pd open file folder(
	_LOCK();
	void* handle = libpd_openfile(path.getFileName().c_str(), folder.c_str());
	if(handle == NULL) {
		_UNLOCK();
		ofLog(OF_LOG_ERROR, "ofxPd: Opening patch \"%s\" failed", path.getFileName().c_str());
		return Patch();
	}
	int dollarZero = libpd_getdollarzero(handle);
	_UNLOCK();
	
	return Patch(handle, dollarZero, path.getFileName(), folder);
}

void ofxPd::closePatch(const std::string& patch) {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Closing patch: "+patch);

	// [; pd-name menuclose 1(
	string patchname = (string) "pd-"+patch;
	_LOCK();
	libpd_start_message(maxMsgLen);
	libpd_add_float(1.0f);
	libpd_finish_message(patchname.c_str(), "menuclose");
	_UNLOCK();
}

void ofxPd::closePatch(Patch& patch) {
	
	ofLog(OF_LOG_VERBOSE, "ofxPd: Closing patch: "+patch.filename());
	
	_LOCK();
	libpd_closefile(patch.handle());
	_UNLOCK();
	patch.clear();
}	

//--------------------------------------------------------------------
void ofxPd::dspOn() {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Dsp on");
	
	// [; pd dsp 1(
	_LOCK();
	libpd_start_message(maxMsgLen);
	libpd_add_float(1.0f);
	libpd_finish_message("pd", "dsp");
	_UNLOCK();
}

void ofxPd::dspOff() {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Dsp off");
	
	// [; pd dsp 0(
	_LOCK();
	libpd_start_message(maxMsgLen);
	libpd_add_float(0.0f);
	libpd_finish_message("pd", "dsp");
	_UNLOCK();
}

//--------------------------------------------------------------------
void ofxPd::addListener(ofxPdListener& listener) {
	
	pair<set<ofxPdListener*>::iterator, bool> ret;
	ret = listeners.insert(&listener);
	if(!ret.second) {
		ofLog(OF_LOG_WARNING, "ofxPd: addListener: ignoring duplicate listener");
		return;
	}
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
	unsubscribe(listener);		
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
		ofLog(OF_LOG_WARNING, "ofxPd: subscribe: unknown listener, call addListener first");
		return;
	}
	
	if(!sourceExists(source)) {
		ofLog(OF_LOG_WARNING, "ofxPd: subscribe: unknown source, call addSource first");
		return;
	}
	
	// global source (all sources)
	map<string,Source>::iterator g_iter;
	g_iter = sources.find("");
	
	// subscribe to specific source
	if(source != "") {
	
		// make sure unsubscribed from global source
		if(g_iter->second.listenerExists(&listener)) {
			g_iter->second.removeListener(&listener);
		}
		
		// subscribe to specific source
		map<string,Source>::iterator s_iter;
		s_iter = sources.find(source);
		s_iter->second.addListener(&listener);
	}
	else {
		// make sure unsubscribed from all sources
		unsubscribe(listener);
	
		// subscribe to global source
		g_iter->second.addListener(&listener);
	}
}

void ofxPd::unsubscribe(ofxPdListener& listener, const std::string& source) {

	if(!listenerExists(listener)) {
		ofLog(OF_LOG_WARNING, "ofxPd: unsubscribe: ignoring unknown listener");
		return;
	}

	if(!sourceExists(source)) {
		ofLog(OF_LOG_WARNING, "ofxPd: unsubscribe: ignoring unknown source");
		return;
	}
	
	map<string,Source>::iterator s_iter;
	
	// unsubscribe from specific source
	if(source != "") {
	
		// global source (all sources)
		map<string,Source>::iterator g_iter;
		g_iter = sources.find("");
	
		// negation from global
		if(g_iter->second.listenerExists(&listener)) {
			
			// remove from global
			g_iter->second.removeListener(&listener);
			
			// add to *all* other sources
			for(s_iter = sources.begin(); s_iter != sources.end(); ++s_iter) {
				if(s_iter != g_iter) {
					s_iter->second.addListener(&listener);
				}
			}
		}
		
		// remove from source
		s_iter = sources.find(source);
		s_iter->second.removeListener(&listener);
	}
	else {	// unsubscribe from all sources	
		for(s_iter = sources.begin(); s_iter != sources.end(); ++s_iter) {
			s_iter->second.removeListener(&listener);
		}
	}
}

bool ofxPd::isSubscribed(ofxPdListener& listener, const std::string& source) {
	map<string,Source>::iterator s_iter;
	s_iter = sources.find(source);
	if(s_iter != sources.end() && s_iter->second.listenerExists(&listener))
		return true;
	return false;
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
	libpd_start_message(maxMsgLen);
	_UNLOCK();
	
	bMsgInProgress = true;
	msgType = LIST;
	msgDest = dest;
}

void ofxPd::startMsg(const std::string& dest, const std::string& msg) {
	
	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not start message, message in progress");
		return;
	}

	_LOCK();	
	libpd_start_message(maxMsgLen);
	_UNLOCK();
	
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
    
    if(curMsgLen+1 >= maxMsgLen) {
        ofLog(OF_LOG_ERROR, "ofxPd: Can not add float, max msg len of %d reached", maxMsgLen);
		return;
    }
	
	_LOCK();
	libpd_add_float(value);
	_UNLOCK();
    curMsgLen++;
}

void ofxPd::addSymbol(const std::string& symbol) {

	if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add symbol, message not in progress");
		return;
	}
	
	if(msgType != LIST && msgType != MSG) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add symbol, midi message in progress");
		return;
	}
	
    if(curMsgLen+1 >= maxMsgLen) {
        ofLog(OF_LOG_ERROR, "ofxPd: Can not add symbol, max msg len of %d reached", maxMsgLen);
		return;
    }
    
	_LOCK();
	libpd_add_symbol(symbol.c_str());
	_UNLOCK();
    curMsgLen++;
}

void ofxPd::finish() {

	if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not finish message, message not in progress");
		return;
	}
	
	switch(msgType) {
		
		case LIST:
			_LOCK();
			libpd_finish_list(msgDest.c_str());
			_UNLOCK();
			break;
		
		case MSG:
			_LOCK();
			libpd_finish_message(msgDest.c_str(), msgMsg.c_str());
			_UNLOCK();
			break;
	}
	
	bMsgInProgress = false;
    curMsgLen = 0;
}

//----------------------------------------------------------
void ofxPd::sendList(const std::string& dest, const List& list) {
    
    if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not send list, message in progress");
		return;
	}

	_LOCK();	
	libpd_start_message();
	_UNLOCK();
	
	bMsgInProgress = true;
	msgType = LIST;
	msgDest = dest;
    
    // step through list
    for(int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i))
			addFloat(list.asFloat(i));
		else if(list.isSymbol(i))
			addSymbol(list.asSymbol(i));
	}
    
    finish();
}

void ofxPd::sendMsg(const std::string& dest, const std::string& msg, const List& list) {

    if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not send message, message in progress");
		return;
	}

	_LOCK();	
	libpd_start_message();
	_UNLOCK();
	
	bMsgInProgress = true;
	msgType = MSG;
	msgDest = dest;
    msgMsg = msg;
    
    // step through list
    for(int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i))
			addFloat(list.asFloat(i));
		else if(list.isSymbol(i))
			addSymbol(list.asSymbol(i));
	}
    
    finish();
}

//----------------------------------------------------------
void ofxPd::sendNote(const int pitch, const int velocity, const int channel) {
	_LOCK();
	libpd_noteon(channel-1, pitch, velocity);
	_UNLOCK();
}

void ofxPd::sendCtl(const int control, const int value, int channel) {
	_LOCK();
	libpd_controlchange(channel-1, control, value);
	_UNLOCK();
}

void ofxPd::sendPgm(int program, const int channel) {
	_LOCK();
	libpd_programchange(channel-1, program);
	_UNLOCK();
}

void ofxPd::sendBend(const int value, const int channel) {
	_LOCK();
	libpd_pitchbend(channel-1, value);
	_UNLOCK();
}

void ofxPd::sendTouch(const int value, const int channel) {
	_LOCK();
	libpd_aftertouch(channel-1, value);
	_UNLOCK();
}

void ofxPd::sendPolyTouch(int note, int value, const int channel) {
	_LOCK();
	libpd_polyaftertouch(channel-1, note, value);
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

void ofxPd::sendSysRTByte(const int value, const int port) {
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
	
	sendBang(var.dest.c_str());
    
    return *this;
}

ofxPd& ofxPd::operator<<(const Float& var) {

	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not send Float, message in progress");
		return *this;
	}
	
	sendFloat(var.dest.c_str(), var.value);
    
    return *this;
}

ofxPd& ofxPd::operator<<(const Symbol& var) {

	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not send Symbol, message in progress");
		return *this;
	}
	
	sendSymbol(var.dest.c_str(), var.symbol.c_str());
    
    return *this;
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const StartList& var) {
	startList(var.dest);
    return *this;
}

ofxPd& ofxPd::operator<<(const StartMsg& var) {
	startMsg(var.dest, var.msg);
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
			sendMidiByte(midiPort, var);
			break;
			
		case SYSEX:
			sendSysExByte(midiPort, var);
			break;
			
		case SYSRT:
			sendSysRTByte(midiPort, var);
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
	sendNote(var.pitch, var.velocity, var.channel);
	return *this;
}

ofxPd& ofxPd::operator<<(const Ctl& var) {
	sendCtl(var.controller, var.value, var.channel);
	return *this;
}

ofxPd& ofxPd::operator<<(const Pgm& var) {
	sendPgm(var.value, var.channel);
	return *this;
}

ofxPd& ofxPd::operator<<(const Bend& var) {
	sendBend(var.value, var.channel);
	return *this;
}

ofxPd& ofxPd::operator<<(const Touch& var) {
	sendTouch(var.value, var.channel);
	return *this;
}

ofxPd& ofxPd::operator<<(const PolyTouch& var) {
	sendPolyTouch(var.pitch, var.value, var.channel);
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

ofxPd& ofxPd::operator<<(const StartSysRT& var) {

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
int ofxPd::getArrayLen(const std::string& arrayName) {
	_LOCK();
	int len = libpd_arraysize(arrayName.c_str());
	_UNLOCK();
	if(len < 0) {
		ofLog(OF_LOG_WARNING, "ofxPd: Cannot get size of unknown array \"%s\"", arrayName.c_str());
		return 0;
	}
	return len;
}
		
bool ofxPd::readArray(const std::string& arrayName, std::vector<float>& dest, int readLen, int offset) {
	
	_LOCK();
	int arrayLen = libpd_arraysize(arrayName.c_str());
	_UNLOCK();
	if(arrayLen < 0) {
		ofLog(OF_LOG_WARNING, "ofxPd: Cannot read unknown array \"%s\"", arrayName.c_str());
		return false;
	}
	
	// full array len?
	if(readLen < 0) {
		readLen = arrayLen;
	}
	// check read len
	else if(readLen > arrayLen) {
		ofLog(OF_LOG_WARNING, "ofxPd: Given read len %d > len %d of array \"%s\"",
			readLen, arrayLen, arrayName.c_str());
		return false;
	}
	
	// check offset
	if(offset+readLen > arrayLen) {
		ofLog(OF_LOG_WARNING, "ofxPd: Given read len and offset > len %d of array \"%s\"",
			readLen, arrayName.c_str());
		return false;
	}
	
	// resize if necessary
	if(dest.size() != readLen) {
		dest.resize(readLen, 0);
	}
	
	_LOCK();
	if(libpd_read_array(&dest[0], arrayName.c_str(), offset, readLen) < 0) {
		_UNLOCK();
		ofLog(OF_LOG_ERROR, "ofxPd: libpd_read_array failed for array \"%s\"", arrayName.c_str());
		return false;
	}
	_UNLOCK();
	return true;
}
		
bool ofxPd::writeArray(const std::string& arrayName, std::vector<float>& source, int writeLen, int offset) {

	_LOCK();
	int arrayLen = libpd_arraysize(arrayName.c_str());
	_UNLOCK();
	if(arrayLen < 0) {
		ofLog(OF_LOG_WARNING, "ofxPd: Cannot write to unknown array \"%s\"", arrayName.c_str());
		return false;
	}
	
	// full array len?
	if(writeLen < 0) {
		writeLen = arrayLen;
	}
	// check write len
	else if(writeLen > arrayLen) {
		ofLog(OF_LOG_WARNING, "ofxPd: Given write len %d > len %d of array \"%s\"",
			writeLen, arrayLen, arrayName.c_str());
		return false;
	}
	
	// check offset
	if(offset+writeLen > arrayLen) {
		ofLog(OF_LOG_WARNING, "ofxPd: Given write len and offset > len %d of array \"%s\"",
			writeLen, arrayName.c_str());
		return false;
	}
	
	_LOCK();
	if(libpd_write_array(arrayName.c_str(), offset, &source[0], writeLen) < 0) {
		_UNLOCK();
		ofLog(OF_LOG_ERROR, "ofxPd: libpd_write_array failed for array \"%s\"", arrayName.c_str());
		return false;
	}
	_UNLOCK();
	return true;
}

void ofxPd::clearArray(const std::string& arrayName, int value) {

	_LOCK();
	int arrayLen = libpd_arraysize(arrayName.c_str());
	_UNLOCK();
	if(arrayLen < 0) {
		ofLog(OF_LOG_WARNING, "ofxPd: Cannot clear unknown array \"%s\"", arrayName.c_str());
		return;
	}
	
	std::vector<float> array;
	array.resize(arrayLen, value);
	
	_LOCK();
	if(libpd_write_array(arrayName.c_str(), 0, &array[0], arrayLen) < 0) {
		ofLog(OF_LOG_ERROR, "ofxPd: libpd_write_array failed while clearing array \"%s\"", arrayName.c_str());
	}
	_UNLOCK();
}

//----------------------------------------------------------
int ofxPd::getBlockSize() {
	_LOCK();
	int bs = libpd_blocksize();
	_UNLOCK();
	return bs;
}

//----------------------------------------------------------
void ofxPd::setMaxMsgLength(unsigned int len) {
    maxMsgLen = len;
}

unsigned int ofxPd::getMaxMsgLength() {
    return maxMsgLen;
}

//----------------------------------------------------------
void ofxPd::audioIn(float * input, int bufferSize, int nChannels) {
	try {
	_LOCK();
		if(inputBuffer)
			memcpy(inputBuffer, input, bufferSize*nChannels*sizeof(float));
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
	
	set<ofxPdListener*>::iterator l_iter;
	set<ofxPdListener*>* listeners;
	
	// send to global listeners
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	listeners = &g_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->bangReceived((string) source);
	}
	
	// send to subscribed listeners
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	listeners = &s_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->bangReceived((string) source);
	}
}

void ofxPd::_float(const char* source, float value)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: float: %s %f", source, value);
	
	set<ofxPdListener*>::iterator l_iter;
	set<ofxPdListener*>* listeners;
	
	// send to global listeners
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	listeners = &g_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->floatReceived((string) source, value);
	}
	
	// send to subscribed listeners
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	listeners = &s_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->floatReceived((string) source, value);
	}
}

void ofxPd::_symbol(const char* source, const char* symbol)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: symbol: %s %s", source, symbol);
	
	set<ofxPdListener*>::iterator l_iter;
	set<ofxPdListener*>* listeners;
	
	// send to global listeners
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	listeners = &g_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->symbolReceived((string) source, (string) symbol);
	}
	
	// send to subscribed listeners
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	listeners = &s_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->symbolReceived((string) source, (string) symbol);
	}
}

void ofxPd::_list(const char* source, int argc, t_atom* argv)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: list: %s", source);
	
	List list((string) source);
	
	for(int i = 0; i < argc; i++) {
		
		t_atom a = argv[i];  
		
		if(a.a_type == A_FLOAT) {  
			float f = a.a_w.w_float;
			list.addFloat(f);
			ofLog(OF_LOG_VERBOSE, "ofxPd:\t%f", f);
		}
		else if(a.a_type == A_SYMBOL) {  
			char* s = a.a_w.w_symbol->s_name;
			list.addSymbol((string) s); 
			ofLog(OF_LOG_VERBOSE, "ofxPd:\t%s", s);  
		}
	}
	
	set<ofxPdListener*>::iterator l_iter;
	set<ofxPdListener*>* listeners;
	
	// send to global listeners
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	listeners = &g_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->listReceived((string) source, list);
	}
	
	// send to subscribed listeners
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	listeners = &s_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->listReceived((string) source, list);
	}
}

void ofxPd::_message(const char* source, const char *symbol, int argc, t_atom *argv)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: message: %s %s", source, symbol);
	
	List list((string) source);
	
	for(int i = 0; i < argc; i++) {
		
		t_atom a = argv[i];  
		
		if(a.a_type == A_FLOAT) {  
			float f = a.a_w.w_float;
			list.addFloat(f);
			ofLog(OF_LOG_VERBOSE, "ofxPd:\t%f", f); 
		}
		else if(a.a_type == A_SYMBOL) {  
			char* s = a.a_w.w_symbol->s_name;
			list.addSymbol((string) s); 
			ofLog(OF_LOG_VERBOSE, "ofxPd:\t%s", s);  
		}
	}
	
	set<ofxPdListener*>::iterator l_iter;
	set<ofxPdListener*>* listeners;
	
	// send to global listeners
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	listeners = &g_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->messageReceived((string) source, (string) symbol, list);
	}
	
	// send to subscribed listeners
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	listeners = &s_iter->second.listeners;
	for(l_iter = listeners->begin(); l_iter != listeners->end(); ++l_iter) {
		(*l_iter)->messageReceived((string) source, (string) symbol, list);
	}
}

void ofxPd::_noteon(int channel, int pitch, int velocity) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: note: %d %d %d", channel, pitch, velocity);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->noteReceived(channel, pitch, velocity);
	}
}

void ofxPd::_controlchange(int channel, int controller, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: control change: %d %d %d", channel, controller, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->ctlReceived(channel, controller, value);
	}
}

void ofxPd::_programchange(int channel, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: program change: %d %d", channel, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->pgmReceived(channel, value);
	}
}

void ofxPd::_pitchbend(int channel, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: pitchbend: %d %d", channel, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->bendReceived(channel, value);
	}
}

void ofxPd::_aftertouch(int channel, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: aftertouch: %d %d", channel, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->touchReceived(channel, value);
	}
}

void ofxPd::_polyaftertouch(int channel, int pitch, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: polyaftertouch: %d %d %d", channel, pitch, value);

	set<ofxPdListener*>& listeners = pdPtr->listeners;
	set<ofxPdListener*>::iterator iter;
	for(iter = listeners.begin(); iter != listeners.end(); ++iter) {
		(*iter)->polyTouchReceived(channel, pitch, value);
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
