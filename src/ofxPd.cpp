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
using namespace pd;

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
	msgType = MSG;
	midiPort = 0;
	_UNLOCK();

	unsubscribeAll();
    
    channels.clear();
    
    // add default global channel
	Channel c;
	channels.insert(make_pair(-1, c));
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

	ofLog(OF_LOG_VERBOSE, "ofxPd: Closing path: "+patch);

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
void ofxPd::start() {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Audio processing on");
	
	// [; pd dsp 1(
	_LOCK();
	libpd_start_message(maxMsgLen);
	libpd_add_float(1.0f);
	libpd_finish_message("pd", "dsp");
	_UNLOCK();
}

void ofxPd::stop() {

	ofLog(OF_LOG_VERBOSE, "ofxPd: Audio processing off");
	
	// [; pd dsp 0(
	_LOCK();
	libpd_start_message(maxMsgLen);
	libpd_add_float(0.0f);
	libpd_finish_message("pd", "dsp");
	_UNLOCK();
}

//----------------------------------------------------------
void ofxPd::subscribe(const std::string& source) {

	if(isSubscribed(source)) {
		ofLog(OF_LOG_WARNING, "ofxPd: subscribe: ignoring duplicate source");
		return;
	}
	
	Source s;
	s.pointer = libpd_bind(source.c_str());
	sources.insert(pair<string, Source>(source, s));
}

void ofxPd::unsubscribe(const std::string& source) {
	
	map<string, Source>::iterator iter;
	iter = sources.find(source);
	if(iter == sources.end()) {
		ofLog(OF_LOG_WARNING, "ofxPd: unsubscribe: ignoring unknown source");
		return;
	}
	
	libpd_unbind(iter->second.pointer);
	sources.erase(iter);
}

bool ofxPd::isSubscribed(const std::string& source) {
	if(sources.find(source) != sources.end())
		return true;
	return false;
}

void ofxPd::unsubscribeAll(){
	
	sources.clear();

	// add default global source
	Source s;
	s.pointer = NULL;
	sources.insert(make_pair("", s));
}

//--------------------------------------------------------------------
void ofxPd::addReceiver(PdReceiver& receiver) {
	
	pair<set<PdReceiver*>::iterator, bool> ret;
	ret = receivers.insert(&receiver);
	if(!ret.second) {
		ofLog(OF_LOG_WARNING, "ofxPd: addReceiver: ignoring duplicate receiver");
		return;
	}
    
    // receive from all sources by default
    receive(receiver);
}

void ofxPd::removeReceiver(PdReceiver& receiver) {
	
	// exists?
	set<PdReceiver*>::iterator r_iter;
	r_iter = receivers.find(&receiver);
	if(r_iter == receivers.end()) {
		ofLog(OF_LOG_WARNING, "ofxPd: removeReceiver: ignoring unknown receiver");
		return;
	}
	receivers.erase(r_iter);

	// remove from all sources
	ignore(receiver);		
}

bool ofxPd::receiverExists(PdReceiver& receiver) {
	if(receivers.find(&receiver) != receivers.end())
		return true;
	return false;
}

void ofxPd::clearReceivers() {

	receivers.clear();
	
	map<string,Source>::iterator iter;
	for(iter = sources.begin(); iter != sources.end(); ++iter) {
		iter->second.receivers.clear();
	}
}

//----------------------------------------------------------
void ofxPd::receive(PdReceiver& receiver, const std::string& source) {

	if(!receiverExists(receiver)) {
		ofLog(OF_LOG_WARNING, "ofxPd: receive: unknown receiver, call addReceiver first");
		return;
	}
	
	if(!isSubscribed(source)) {
		ofLog(OF_LOG_WARNING, "ofxPd: receive: unknown source, call subscribe first");
		return;
	}
	
	// global source (all sources)
	map<string,Source>::iterator g_iter;
	g_iter = sources.find("");
	
	// subscribe to specific source
	if(source != "") {
	
		// make sure global source is ignored
		if(g_iter->second.receiverExists(&receiver)) {
			g_iter->second.removeReceiver(&receiver);
		}
		
		// receive from specific source
		map<string,Source>::iterator s_iter;
		s_iter = sources.find(source);
		s_iter->second.addReceiver(&receiver);
	}
	else {
		// make sure all sources are ignored
		ignore(receiver);
	
		// receive from the global source
		g_iter->second.addReceiver(&receiver);
	}
}

void ofxPd::ignore(PdReceiver& receiver, const std::string& source) {

	if(!receiverExists(receiver)) {
		ofLog(OF_LOG_WARNING, "ofxPd: ignore: ignoring unknown receiver");
		return;
	}

	if(!isSubscribed(source)) {
		ofLog(OF_LOG_WARNING, "ofxPd: ignore: ignoring unknown source");
		return;
	}
	
	map<string,Source>::iterator s_iter;
	
	// unsubscribe from specific source
	if(source != "") {
	
		// global source (all sources)
		map<string,Source>::iterator g_iter;
		g_iter = sources.find("");
	
		// negation from global
		if(g_iter->second.receiverExists(&receiver)) {
			
			// remove from global
			g_iter->second.removeReceiver(&receiver);
			
			// add to *all* other sources
			for(s_iter = sources.begin(); s_iter != sources.end(); ++s_iter) {
				if(s_iter != g_iter) {
					s_iter->second.addReceiver(&receiver);
				}
			}
		}
		
		// remove from source
		s_iter = sources.find(source);
		s_iter->second.removeReceiver(&receiver);
	}
	else {	// ignore all sources	
		for(s_iter = sources.begin(); s_iter != sources.end(); ++s_iter) {
			s_iter->second.removeReceiver(&receiver);
		}
	}
}

bool ofxPd::isReceiving(PdReceiver& receiver, const std::string& source) {
	map<string,Source>::iterator s_iter;
	s_iter = sources.find(source);
	if(s_iter != sources.end() && s_iter->second.receiverExists(&receiver))
		return true;
	return false;
}

//----------------------------------------------------------
void ofxPd::addMidiReceiver(PdMidiReceiver& receiver) {
	
    pair<set<PdMidiReceiver*>::iterator, bool> ret;
	ret = midiReceivers.insert(&receiver);
	if(!ret.second) {
		ofLog(OF_LOG_WARNING, "ofxPd: addMidiReceiver: ignoring duplicate receiver");
		return;
	}
    
    // receive from all channels by default
    receiveMidi(receiver);
}

void ofxPd::removeMidiReceiver(PdMidiReceiver& receiver) {

	// exists?
	set<PdMidiReceiver*>::iterator r_iter;
	r_iter = midiReceivers.find(&receiver);
	if(r_iter == midiReceivers.end()) {
		ofLog(OF_LOG_WARNING, "ofxPd: removeMidiReceiver: ignoring unknown receiver");
		return;
	}
	midiReceivers.erase(r_iter);

	// remove from all sources
	ignoreMidi(receiver);	
}

bool ofxPd::midiReceiverExists(PdMidiReceiver& receiver) {
	if(midiReceivers.find(&receiver) != midiReceivers.end())
		return true;
	return false;
}

void ofxPd::clearMidiReceivers() {

	midiReceivers.clear();
	
	map<int,Channel>::iterator iter;
	for(iter = channels.begin(); iter != channels.end(); ++iter) {
		iter->second.receivers.clear();
	}
}

//----------------------------------------------------------
void ofxPd::receiveMidi(PdMidiReceiver& receiver, int channel) {

	if(!midiReceiverExists(receiver)) {
		ofLog(OF_LOG_WARNING, "ofxPd: receiveMidi: unknown receiver, call addMidiReceiver first");
		return;
	}
	
    // handle bad channel numbers
    if(channel < 0)
        channel = 0;
    
    // insert channel if it dosen't exist yet
    if(!channelExists(channel)) {
        Channel c;
        channels.insert(pair<int,Channel>(channel, c));
    }
    
	// global channel (all channels)
	map<int,Channel>::iterator g_iter;
	g_iter = channels.find(0);
	
	// subscribe to specific channel
	if(channel != 0) {
	
		// make sure global channel is ignored
		if(g_iter->second.midiReceiverExists(&receiver)) {
			g_iter->second.removeMidiReceiver(&receiver);
		}
		
		// receive from specific channel
		map<int,Channel>::iterator c_iter;
		c_iter = channels.find(channel);
		c_iter->second.addMidiReceiver(&receiver);
	}
	else {
		// make sure all channels are ignored
		ignoreMidi(receiver);
	
		// receive from the global channel
		g_iter->second.addMidiReceiver(&receiver);
	}
}

void ofxPd::ignoreMidi(PdMidiReceiver& receiver, int channel) {

	if(!midiReceiverExists(receiver)) {
		ofLog(OF_LOG_WARNING, "ofxPd: ignoreMidi: ignoring unknown receiver");
		return;
	} 
	
    // handle bad channel numbers
    if(channel < 0)
        channel = 0;
    
    // insert channel if it dosen't exist yet
    if(!channelExists(channel)) {
        Channel c;
        channels.insert(pair<int,Channel>(channel, c));
    }
    
	map<int,Channel>::iterator c_iter;
	
	// unsubscribe from specific channel
	if(channel != 0) {
	
		// global channel (all channels)
		map<int,Channel>::iterator g_iter;
		g_iter = channels.find(0);
	
		// negation from global
		if(g_iter->second.midiReceiverExists(&receiver)) {
			
			// remove from global
			g_iter->second.removeMidiReceiver(&receiver);
			
			// add to *all* other channels
			for(c_iter = channels.begin(); c_iter != channels.end(); ++c_iter) {
				if(c_iter != g_iter) {
					c_iter->second.addMidiReceiver(&receiver);
				}
			}
		}
		
		// remove from channel
		c_iter = channels.find(channel);
		c_iter->second.removeMidiReceiver(&receiver);
	}
	else {	// ignore all sources	
		for(c_iter = channels.begin(); c_iter != channels.end(); ++c_iter) {
			c_iter->second.removeMidiReceiver(&receiver);
		}
	}
}

bool ofxPd::isReceivingMidi(PdMidiReceiver& receiver, int channel) {
	
    // handle bad channel numbers
    if(channel < 0)
        channel = 0;
    
    map<int,Channel>::iterator c_iter;
	c_iter = channels.find(channel);
	if(c_iter != channels.end() && c_iter->second.midiReceiverExists(&receiver))
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
void ofxPd::startMsg() {
	
	if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not start message, message in progress");
		return;
	}

	_LOCK();	
	libpd_start_message(maxMsgLen);
	_UNLOCK();
	
	bMsgInProgress = true;
    msgType = MSG;
}

void ofxPd::addFloat(const float value) {

	if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add float, message not in progress");
		return;
	}
	
	if(msgType != MSG) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add float, midi byte stream in progress");
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
	
	if(msgType != MSG) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not add symbol, midi byte stream in progress");
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

void ofxPd::finishList(const std::string& dest) {

	if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not finish list, message not in progress");
		return;
	}
	
    if(msgType != MSG) {
        ofLog(OF_LOG_ERROR, "ofxPd: Can not finish list, midi byte stream in progress");
		return;
    }
    
    _LOCK();
    libpd_finish_list(dest.c_str());
    _UNLOCK();
	
	bMsgInProgress = false;
    curMsgLen = 0;
}

void ofxPd::finishMsg(const std::string& dest, const std::string& msg) {

	if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not finish message, message not in progress");
		return;
	}
	
    if(msgType != MSG) {
        ofLog(OF_LOG_ERROR, "ofxPd: Can not finish message, midi byte stream in progress");
		return;
    }
    
    _LOCK();
    libpd_finish_message(dest.c_str(), msg.c_str());
    _UNLOCK();
	
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
	libpd_start_message(list.len());
	_UNLOCK();
	
	bMsgInProgress = true;
    
    // step through list
    for(int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i))
			addFloat(list.asFloat(i));
		else if(list.isSymbol(i))
			addSymbol(list.asSymbol(i));
	}
    
    finishList(dest);
}

void ofxPd::sendMsg(const std::string& dest, const std::string& msg, const List& list) {

    if(bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not send message, message in progress");
		return;
	}

	_LOCK();	
	libpd_start_message(list.len());
	_UNLOCK();
	
	bMsgInProgress = true;
    
    // step through list
    for(int i = 0; i < list.len(); ++i) {
		if(list.isFloat(i))
			addFloat(list.asFloat(i));
		else if(list.isSymbol(i))
			addSymbol(list.asSymbol(i));
	}
    
    finishMsg(dest, msg);
}

//----------------------------------------------------------
void ofxPd::sendNote(const int channel, const int pitch, const int velocity) {
	_LOCK();
	libpd_noteon(channel-1, pitch, velocity);
	_UNLOCK();
}

void ofxPd::sendCtl(const int channel, const int control, const int value) {
	_LOCK();
	libpd_controlchange(channel-1, control, value);
	_UNLOCK();
}

void ofxPd::sendPgm(const int channel, int program) {
	_LOCK();
	libpd_programchange(channel-1, program-1);
	_UNLOCK();
}

void ofxPd::sendBend(const int channel, const int value) {
	_LOCK();
	libpd_pitchbend(channel-1, value);
	_UNLOCK();
}

void ofxPd::sendTouch(const int channel, const int value) {
	_LOCK();
	libpd_aftertouch(channel-1, value);
	_UNLOCK();
}

void ofxPd::sendPolyTouch(const int channel, int pitch, int value) {
	_LOCK();
	libpd_polyaftertouch(channel-1, pitch, value);
	_UNLOCK();
}

//----------------------------------------------------------
void ofxPd::sendMidiByte(const int port, const int value) {
	_LOCK();
	libpd_midibyte(port, value);
	_UNLOCK();
}

void ofxPd::sendSysExByte(const int port, const int value) {
	_LOCK();
	libpd_sysex(port, value);
	_UNLOCK();
}

void ofxPd::sendSysRtByte(const int port, const int value) {
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
ofxPd& ofxPd::operator<<(const StartMsg& var) {
	startMsg();
    return *this;
}

ofxPd& ofxPd::operator<<(const FinishList& var) {
	finishList(var.dest);
    return *this;
}

ofxPd& ofxPd::operator<<(const FinishMsg& var) {
	finishMsg(var.dest, var.msg);
    return *this;
}

//----------------------------------------------------------
ofxPd& ofxPd::operator<<(const bool var) {
	addFloat((float) var);
	return *this;
}

ofxPd& ofxPd::operator<<(const int var) {
    
	switch(msgType) {
	
		case MSG:
			addFloat((float) var);
			break;
			
		case MIDI:
			sendMidiByte(midiPort, var);
			break;
			
		case SYSEX:
			sendSysExByte(midiPort, var);
			break;
			
		case SYSRT:
			sendSysRtByte(midiPort, var);
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
	sendPgm(var.channel, var.value);
	return *this;
}

ofxPd& ofxPd::operator<<(const Bend& var) {
	sendBend(var.channel, var.value);
	return *this;
}

ofxPd& ofxPd::operator<<(const Touch& var) {
	sendTouch(var.channel, var.value);
	return *this;
}

ofxPd& ofxPd::operator<<(const PolyTouch& var) {
	sendPolyTouch(var.channel, var.pitch, var.value);
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

ofxPd& ofxPd::operator<<(const StartSysRt& var) {

	if(bMsgInProgress) {
		ofLog(OF_LOG_ERROR, "ofxPd: Can not start SysRealtime stream, message in progress");
		return *this;
	}
	
	bMsgInProgress = true;
	msgType = SYSRT;
	midiPort = var.port;

	return *this;
}

ofxPd& ofxPd::operator<<(const Finish& var) {
    
    if(!bMsgInProgress) {
    	ofLog(OF_LOG_ERROR, "ofxPd: Can not finish midi byte stream, stream not in progress");
		return *this;
	}
	
    if(msgType == MSG) {
        ofLog(OF_LOG_ERROR, "ofxPd: Can not finish midi byte stream, message in progress");
		return *this;
    }
    
	bMsgInProgress = false;
    curMsgLen = 0;
    return *this;
}

//----------------------------------------------------------
int ofxPd::getArraySize(const std::string& arrayName) {
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
bool ofxPd::channelExists(int channel) {
	
    // handle bad channel numbers
    if(channel < 0)
        channel = 0;
        
    if(channels.find(channel) != channels.end())
		return true;
	return false;
}

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
		set<PdReceiver*>& receivers = pdPtr->receivers;
		set<PdReceiver*>::iterator iter;
		for(iter = receivers.begin(); iter != receivers.end(); ++iter) {
			(*iter)->receivePrint(pdPtr->printMsg);
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
	
	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* receivers;
	
	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	receivers = &g_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveBang((string) source);
	}
	
	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	receivers = &s_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveBang((string) source);
	}
}

void ofxPd::_float(const char* source, float value)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: float: %s %f", source, value);
	
	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* receivers;
	
	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	receivers = &g_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveFloat((string) source, value);
	}
	
	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	receivers = &s_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveFloat((string) source, value);
	}
}

void ofxPd::_symbol(const char* source, const char* symbol)
{
	ofLog(OF_LOG_VERBOSE, "ofxPd: symbol: %s %s", source, symbol);
	
	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* receivers;
	
	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	receivers = &g_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveSymbol((string) source, (string) symbol);
	}
	
	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	receivers = &s_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveSymbol((string) source, (string) symbol);
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
	
	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* receivers;
	
	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	receivers = &g_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveList((string) source, list);
	}
	
	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	receivers = &s_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveList((string) source, list);
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
	
	set<PdReceiver*>::iterator r_iter;
	set<PdReceiver*>* receivers;
	
	// send to global receivers
	map<string,Source>::iterator g_iter;
	g_iter = pdPtr->sources.find("");
	receivers = &g_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveMessage((string) source, (string) symbol, list);
	}
	
	// send to subscribed receivers
	map<string,Source>::iterator s_iter;
	s_iter = pdPtr->sources.find((string) source);
	receivers = &s_iter->second.receivers;
	for(r_iter = receivers->begin(); r_iter != receivers->end(); ++r_iter) {
		(*r_iter)->receiveMessage((string) source, (string) symbol, list);
	}
}

void ofxPd::_noteon(int channel, int pitch, int velocity) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: note: %d %d %d", channel, pitch, velocity);
	
    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* midiReceivers;
    
	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = pdPtr->channels.find(0);
	midiReceivers = &g_iter->second.receivers;
	for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
		(*r_iter)->receiveNote(channel, pitch, velocity);
	}
	
	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = pdPtr->channels.find(channel);
    if(c_iter != pdPtr->channels.end()) {
        midiReceivers = &c_iter->second.receivers;
        for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
            (*r_iter)->receiveNote(channel, pitch, velocity);
        }
    }
}

void ofxPd::_controlchange(int channel, int controller, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: control change: %d %d %d", channel, controller, value);

    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* midiReceivers;
    
	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = pdPtr->channels.find(0);
	midiReceivers = &g_iter->second.receivers;
	for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
		(*r_iter)->receiveCtl(channel, controller, value);
	}
	
	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = pdPtr->channels.find(channel);
    if(c_iter != pdPtr->channels.end()) {
        midiReceivers = &c_iter->second.receivers;
        for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
            (*r_iter)->receiveCtl(channel, controller, value);
        }
    }
}

void ofxPd::_programchange(int channel, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: program change: %d %d", channel, value+1);

    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* midiReceivers;
    
	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = pdPtr->channels.find(0);
	midiReceivers = &g_iter->second.receivers;
	for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
		(*r_iter)->receivePgm(channel, value);
	}
	
	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = pdPtr->channels.find(channel);
    if(c_iter != pdPtr->channels.end()) {
        midiReceivers = &c_iter->second.receivers;
        for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
            (*r_iter)->receivePgm(channel, value);
        }
    }
}

void ofxPd::_pitchbend(int channel, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: pitchbend: %d %d", channel, value);

    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* midiReceivers;
    
	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = pdPtr->channels.find(0);
	midiReceivers = &g_iter->second.receivers;
	for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
		(*r_iter)->receiveBend(channel, value);
	}
	
	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = pdPtr->channels.find(channel);
    if(c_iter != pdPtr->channels.end()) {
        midiReceivers = &c_iter->second.receivers;
        for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
            (*r_iter)->receivePgm(channel, value);
        }
    }
}

void ofxPd::_aftertouch(int channel, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: aftertouch: %d %d", channel, value);

    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* midiReceivers;
    
	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = pdPtr->channels.find(0);
	midiReceivers = &g_iter->second.receivers;
	for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
		(*r_iter)->receiveTouch(channel, value);
	}
	
	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = pdPtr->channels.find(channel);
    if(c_iter != pdPtr->channels.end()) {
        midiReceivers = &c_iter->second.receivers;
        for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
            (*r_iter)->receiveTouch(channel, value);
        }
    }
}

void ofxPd::_polyaftertouch(int channel, int pitch, int value) {
	channel++;
	ofLog(OF_LOG_VERBOSE, "ofxPd: polyaftertouch: %d %d %d", channel, pitch, value);

    set<PdMidiReceiver*>::iterator r_iter;
	set<PdMidiReceiver*>* midiReceivers;
    
	// send to global receivers
	map<int,Channel>::iterator g_iter;
	g_iter = pdPtr->channels.find(0);
	midiReceivers = &g_iter->second.receivers;
	for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
		(*r_iter)->receivePolyTouch(channel, pitch, value);
	}
	
	// send to subscribed receivers
	map<int,Channel>::iterator c_iter;
	c_iter = pdPtr->channels.find(channel);
    if(c_iter != pdPtr->channels.end()) {
        midiReceivers = &c_iter->second.receivers;
        for(r_iter = midiReceivers->begin(); r_iter != midiReceivers->end(); ++r_iter) {
            (*r_iter)->receivePolyTouch(channel, pitch, value);
        }
    }
}

void ofxPd::_midibyte(int port, int byte) {

	ofLog(OF_LOG_VERBOSE, "ofxPd: midibyte: %d %d", port, byte);

	set<PdMidiReceiver*>& midiReceivers = pdPtr->midiReceivers;
	set<PdMidiReceiver*>::iterator iter;
	for(iter = midiReceivers.begin(); iter != midiReceivers.end(); ++iter) {
		(*iter)->receiveMidiByte(port, byte);
	}
}
