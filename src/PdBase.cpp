/*
 * Copyright (c) 2011 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/PdBase for documentation
 *
 * This project uses libpd, copyrighted by Miller Puckette and others using the
 * "Standard Improved BSD License". See the file "LICENSE.txt" in src/pd.
 *
 * See http://gitorious.org/pdlib/pages/Libpd for documentation
 *
 */
#include "PdBase.h"

#include <iostream>

// needed for libpd audio passing
#define USEAPI_DUMMY

using namespace std;

namespace pd {

// pointer for static member functions
PdBase* pdPtr = NULL;

//--------------------------------------------------------------------
PdBase::PdBase() {
	pdPtr = this;
	bPdInited = false;
    receiver = NULL;
    midiReceiver = NULL;
	clear();
    maxMsgLen = 32;
}

//--------------------------------------------------------------------
PdBase::~PdBase() {
    clear();
}

//--------------------------------------------------------------------
bool PdBase::init(const int numInChannels, const int numOutChannels, 
				 const int sampleRate, const int ticksPerBuffer) {
	clear();
	
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
	libpd_init();
	if(libpd_init_audio(numInChannels, numOutChannels, sampleRate, ticksPerBuffer) != 0) {
		return false;
	}
    bPdInited = true;

    return bPdInited;
}

void PdBase::clear() {	
	bPdInited = false;
	
	bMsgInProgress = false;
    curMsgLen = 0;
	msgType = MSG;
	midiPort = 0;

	unsubscribeAll();
}

//--------------------------------------------------------------------
void PdBase::addToSearchPath(const std::string& path) {
	libpd_add_to_search_path(path.c_str());
}
		
void PdBase::clearSearchPath() {
	libpd_clear_search_path();
}

//--------------------------------------------------------------------
//
//	references http://pocoproject.org/docs/Poco.Path.html
//
Patch PdBase::openPatch(const std::string& patch, const std::string& path) {
    // [; pd open file folder(
	void* handle = libpd_openfile(patch.c_str(), path.c_str());
	if(handle == NULL) {
		return Patch(); // return empty Patch
	}
	int dollarZero = libpd_getdollarzero(handle);
	return Patch(handle, dollarZero, patch, path);
}

void PdBase::closePatch(const std::string& patch) {
	// [; pd-name menuclose 1(
	string patchname = (string) "pd-"+patch;
	libpd_start_message(maxMsgLen);
	libpd_add_float(1.0f);
	libpd_finish_message(patchname.c_str(), "menuclose");
}

void PdBase::closePatch(Patch& patch) {
	libpd_closefile(patch.handle());
	patch.clear();
}	

//--------------------------------------------------------------------
bool PdBase::processRaw(float* inBuffer, float* outBuffer) {
    return libpd_process_raw(inBuffer, outBuffer) == 0;
}

bool PdBase::processShort(short* inBuffer, short* outBuffer) {
    return libpd_process_short(inBuffer, outBuffer) == 0;
}

bool PdBase::processFloat(float* inBuffer, float* outBuffer) {
    return libpd_process_float(inBuffer, outBuffer) == 0;
}

bool PdBase::processDouble(double* inBuffer, double* outBuffer) {
    return libpd_process_double(inBuffer, outBuffer) == 0;
}


//--------------------------------------------------------------------
void PdBase::computeAudio(bool state) {
	// [; pd dsp $1(
	libpd_start_message(1);
	libpd_add_float((float) state);
	libpd_finish_message("pd", "dsp");
}

//----------------------------------------------------------
void PdBase::subscribe(const std::string& source) {

	if(exists(source)) {
        cerr << "Pd: unsubscribe: ignoring duplicate source" << endl;
		return;
	}
	
	void* pointer = libpd_bind(source.c_str());
	if(pointer != NULL)
        sources.insert(pair<string,void*>(source, pointer));
}

void PdBase::unsubscribe(const std::string& source) {
	
	map<string,void*>::iterator iter;
	iter = sources.find(source);
	if(iter == sources.end()) {
		cerr << "Pd: unsubscribe: ignoring unknown source" << endl;
		return;
	}
	
	libpd_unbind(iter->second);
	sources.erase(iter);
}

bool PdBase::exists(const std::string& source) {
	if(sources.find(source) != sources.end())
		return true;
	return false;
}

void PdBase::unsubscribeAll(){
    map<string,void*>::iterator iter;
    for(iter = sources.begin(); iter != sources.end(); ++iter)
        libpd_unbind(iter->second);
    sources.clear();
}

//--------------------------------------------------------------------
void PdBase::setReceiver(PdReceiver* receiver) {
	this->receiver = receiver;
}


void PdBase::setMidiReceiver(PdMidiReceiver* midiReceiver) {
    this->midiReceiver = midiReceiver;
}

//----------------------------------------------------------
void PdBase::sendBang(const std::string& dest) {
	libpd_bang(dest.c_str());
}

void PdBase::sendFloat(const std::string& dest, float value) {
	libpd_float(dest.c_str(), value);
}

void PdBase::sendSymbol(const std::string& dest, const std::string& symbol) {
	libpd_symbol(dest.c_str(), symbol.c_str());
}

//----------------------------------------------------------
void PdBase::startMsg() {
	
	if(bMsgInProgress) {
    	cerr << "Pd: Can not start message, message in progress" << endl;
		return;
	}
	
	libpd_start_message(maxMsgLen);
	
	bMsgInProgress = true;
    msgType = MSG;
}

void PdBase::addFloat(const float value) {

	if(!bMsgInProgress) {
    	cerr << "Pd: Can not add float, message not in progress" << endl;
		return;
	}
	
	if(msgType != MSG) {
    	cerr << "Pd: Can not add float, midi byte stream in progress" << endl;
		return;
	}
    
    if(curMsgLen+1 >= maxMsgLen) {
        cerr << "Pd: Can not add float, max msg len of " << maxMsgLen<< " reached" << endl;
		return;
    }
	
	libpd_add_float(value);
    curMsgLen++;
}

void PdBase::addSymbol(const std::string& symbol) {

	if(!bMsgInProgress) {
        cerr << "Pd: Can not add symbol, message not in progress" << endl;;
		return;
	}
	
	if(msgType != MSG) {
    	cerr << "Pd: Can not add symbol, midi byte stream in progress" << endl;;
		return;
	}
	
    if(curMsgLen+1 >= maxMsgLen) {
        cerr << "Pd: Can not add symbol, max msg len of " << maxMsgLen << " reached" << endl;
		return;
    }
    
	libpd_add_symbol(symbol.c_str());
    curMsgLen++;
}

void PdBase::finishList(const std::string& dest) {

	if(!bMsgInProgress) {
    	cerr << "Pd: Can not finish list, message not in progress" << endl;
		return;
	}
	
    if(msgType != MSG) {
        cerr << "Pd: Can not finish list, midi byte stream in progress" << endl;
		return;
    }
    
    libpd_finish_list(dest.c_str());
	
	bMsgInProgress = false;
    curMsgLen = 0;
}

void PdBase::finishMsg(const std::string& dest, const std::string& msg) {

	if(!bMsgInProgress) {
    	cerr << "Pd: Can not finish message, message not in progress" << endl;
		return;
	}
	
    if(msgType != MSG) {
        cerr << "Pd: Can not finish message, midi byte stream in progress" << endl;
		return;
    }
    
    libpd_finish_message(dest.c_str(), msg.c_str());
	
	bMsgInProgress = false;
    curMsgLen = 0;
}

//----------------------------------------------------------
void PdBase::sendList(const std::string& dest, const List& list) {
    
    if(bMsgInProgress) {
    	cerr << "Pd: Can not send list, message in progress" << endl;
		return;
	}
	
	libpd_start_message(list.len());
	
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

void PdBase::sendMsg(const std::string& dest, const std::string& msg, const List& list) {

    if(bMsgInProgress) {
    	cerr << "Pd: Can not send message, message in progress" << endl;
		return;
	}
	
	libpd_start_message(list.len());
	
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
void PdBase::sendNote(const int channel, const int pitch, const int velocity) {
	libpd_noteon(channel, pitch, velocity);
}

void PdBase::sendCtl(const int channel, const int control, const int value) {
	libpd_controlchange(channel, control, value);
}

void PdBase::sendPgm(const int channel, int program) {
	libpd_programchange(channel, program);
}

void PdBase::sendBend(const int channel, const int value) {
	libpd_pitchbend(channel, value);
}

void PdBase::sendTouch(const int channel, const int value) {
	libpd_aftertouch(channel, value);
}

void PdBase::sendPolyTouch(const int channel, int pitch, int value) {
	libpd_polyaftertouch(channel, pitch, value);
}

//----------------------------------------------------------
void PdBase::sendMidiByte(const int port, const int value) {
	libpd_midibyte(port, value);
}

void PdBase::sendSysExByte(const int port, const int value) {
	libpd_sysex(port, value);
}

void PdBase::sendSysRtByte(const int port, const int value) {
	libpd_sysrealtime(port, value);
}

//----------------------------------------------------------
PdBase& PdBase::operator<<(const Bang& var) {

	if(bMsgInProgress) {
    	cerr << "Pd: Can not send Bang, message in progress" << endl;
		return *this;
	}
	
	sendBang(var.dest.c_str());
    
    return *this;
}

PdBase& PdBase::operator<<(const Float& var) {

	if(bMsgInProgress) {
    	cerr << "Pd: Can not send Float, message in progress" << endl;
		return *this;
	}
	
	sendFloat(var.dest.c_str(), var.value);
    
    return *this;
}

PdBase& PdBase::operator<<(const Symbol& var) {

	if(bMsgInProgress) {
    	cerr << "Pd: Can not send Symbol, message in progress" << endl;
		return *this;
	}
	
	sendSymbol(var.dest.c_str(), var.symbol.c_str());
    
    return *this;
}

//----------------------------------------------------------
PdBase& PdBase::operator<<(const StartMsg& var) {
	startMsg();
    return *this;
}

PdBase& PdBase::operator<<(const FinishList& var) {
	finishList(var.dest);
    return *this;
}

PdBase& PdBase::operator<<(const FinishMsg& var) {
	finishMsg(var.dest, var.msg);
    return *this;
}

//----------------------------------------------------------
PdBase& PdBase::operator<<(const bool var) {
	addFloat((float) var);
	return *this;
}

PdBase& PdBase::operator<<(const int var) {
    
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

PdBase& PdBase::operator<<(const float var) {
    addFloat((float) var);
	return *this;
}

PdBase& PdBase::operator<<(const double var) {  
    addFloat((float) var);
	return *this;
}

//----------------------------------------------------------
PdBase& PdBase::operator<<(const char var) {
	string s;
	s = var;
	addSymbol(s);
	return *this;	
}

PdBase& PdBase::operator<<(const char* var) {
	addSymbol((string) var);
	return *this;	
}

PdBase& PdBase::operator<<(const std::string& var) {
	addSymbol(var);
	return *this;	
}

//----------------------------------------------------------
PdBase& PdBase::operator<<(const Note& var) {
	sendNote(var.channel, var.pitch, var.velocity);
	return *this;
}

PdBase& PdBase::operator<<(const Ctl& var) {
	sendCtl(var.channel, var.controller, var.value);
	return *this;
}

PdBase& PdBase::operator<<(const Pgm& var) {
	sendPgm(var.channel, var.value);
	return *this;
}

PdBase& PdBase::operator<<(const Bend& var) {
	sendBend(var.channel, var.value);
	return *this;
}

PdBase& PdBase::operator<<(const Touch& var) {
	sendTouch(var.channel, var.value);
	return *this;
}

PdBase& PdBase::operator<<(const PolyTouch& var) {
	sendPolyTouch(var.channel, var.pitch, var.value);
	return *this;
}

//----------------------------------------------------------
PdBase& PdBase::operator<<(const StartMidi& var) {
	
	if(bMsgInProgress) {
		cerr << "Pd: Can not start MidiByte stream, message in progress" << endl;
		return *this;
	}
	
	bMsgInProgress = true;
	msgType = MIDI;
	midiPort = var.port;

	return *this;
}

PdBase& PdBase::operator<<(const StartSysEx& var) {

	if(bMsgInProgress) {
		cerr << "Pd: Can not start SysEx stream, message in progress" << endl;
		return *this;
	}
	
	bMsgInProgress = true;
	msgType = SYSEX;
	midiPort = var.port;

	return *this;
}

PdBase& PdBase::operator<<(const StartSysRt& var) {

	if(bMsgInProgress) {
        cerr << "Pd: Can not start SysRealtime stream, message in progress" << endl;
		return *this;
	}
	
	bMsgInProgress = true;
	msgType = SYSRT;
	midiPort = var.port;

	return *this;
}

PdBase& PdBase::operator<<(const Finish& var) {
    
    if(!bMsgInProgress) {
    	cerr << "Pd: Can not finish midi byte stream, stream not in progress" << endl;
		return *this;
	}
	
    if(msgType == MSG) {
        cerr << "Pd: Can not finish midi byte stream, message in progress" << endl;
		return *this;
    }
    
	bMsgInProgress = false;
    curMsgLen = 0;
    return *this;
}

//----------------------------------------------------------
int PdBase::arraySize(const std::string& arrayName) {
	int len = libpd_arraysize(arrayName.c_str());;
	if(len < 0) {
		cerr << "Pd: Cannot get size of unknown array \"" << arrayName << "\"" << endl;
		return 0;
	}
	return len;
}
		
bool PdBase::readArray(const std::string& arrayName, std::vector<float>& dest, int readLen, int offset) {
	
	int arrayLen = libpd_arraysize(arrayName.c_str());
	if(arrayLen < 0) {
		cerr << "Pd: Cannot read unknown array \"" << arrayName << "\"" << endl;
		return false;
	}
	
	// full array len?
	if(readLen < 0) {
		readLen = arrayLen;
	}
	// check read len
	else if(readLen > arrayLen) {
		cerr << "Pd: Given read len " << readLen << " > len "
             << arrayLen << " of array \"" << arrayName << "\"" << endl;
		return false;
	}
	
	// check offset
	if(offset+readLen > arrayLen) {
		cerr << "Pd: Given read len and offset > len " << readLen
             << " of array \"" << arrayName << "\"" << endl;
		return false;
	}
	
	// resize if necessary
	if(dest.size() != readLen) {
		dest.resize(readLen, 0);
	}
	
	if(libpd_read_array(&dest[0], arrayName.c_str(), offset, readLen) < 0) {
		cerr << "Pd: libpd_read_array failed for array \""
             << arrayName << "\"" << endl;
		return false;
	}
	return true;
}
		
bool PdBase::writeArray(const std::string& arrayName, std::vector<float>& source, int writeLen, int offset) {

	int arrayLen = libpd_arraysize(arrayName.c_str());
	if(arrayLen < 0) {
		cerr << "Pd: Cannot write to unknown array \"" << arrayName << "\"" << endl;
		return false;
	}
	
	// full array len?
	if(writeLen < 0) {
		writeLen = arrayLen;
	}
	// check write len
	else if(writeLen > arrayLen) {
		cerr << "Pd: Given write len " << writeLen << " > len " << arrayLen
             << " of array \"" << arrayName << "\"" << endl;
		return false;
	}
	
	// check offset
	if(offset+writeLen > arrayLen) {
		cerr << "Pd: Given write len and offset > len " << writeLen
             << " of array \"" << arrayName << "\"" << endl;
		return false;
	}
	
	if(libpd_write_array(arrayName.c_str(), offset, &source[0], writeLen) < 0) {
		cerr << "Pd: libpd_write_array failed for array \"" << arrayName << "\"" << endl;
		return false;
	}
	return true;
}

void PdBase::clearArray(const std::string& arrayName, int value) {

	int arrayLen = libpd_arraysize(arrayName.c_str());
	if(arrayLen < 0) {
		cerr << "Pd: Cannot clear unknown array \"" << arrayName << "\"" << endl;
		return;
	}
	
	std::vector<float> array;
	array.resize(arrayLen, value);
	
	if(libpd_write_array(arrayName.c_str(), 0, &array[0], arrayLen) < 0) {
		cerr << "Pd: libpd_write_array failed while clearing array \""
             << arrayName << "\"" << endl;
	}
}

//----------------------------------------------------------
bool PdBase::isInited() {
    return bPdInited;
}

int PdBase::blockSize() {
	return libpd_blocksize();
}

void PdBase::setMaxMsgLength(unsigned int len) {
    maxMsgLen = len;
}

unsigned int PdBase::maxMsgLength() {
    return maxMsgLen;
}

/* ***** PRIVATE ***** */

//----------------------------------------------------------
void PdBase::_print(const char* s)
{
	string line(s);
	
	if(line.size() > 0 && line.at(line.size()-1) == '\n') {
		
		// build the message
		if(line.size() > 1) {
			line.erase(line.end()-1);
			pdPtr->printMsg += line;
		}
		
		if(pdPtr->receiver)
            pdPtr->receiver->receivePrint(pdPtr->printMsg);
	
		pdPtr->printMsg = "";
		return;
	}
		
	// build the message
	pdPtr->printMsg += line;
}
		
void PdBase::_bang(const char* source)
{	
    if(pdPtr->receiver)
        pdPtr->receiver->receiveBang((string) source);
}

void PdBase::_float(const char* source, float value)
{
    if(pdPtr->receiver)
        pdPtr->receiver->receiveFloat((string) source, value);
}

void PdBase::_symbol(const char* source, const char* symbol)
{
    if(pdPtr->receiver)
        pdPtr->receiver->receiveSymbol((string) source, (string) symbol);
}

void PdBase::_list(const char* source, int argc, t_atom* argv)
{
	List list((string) source);
	
	for(int i = 0; i < argc; i++) {
		
		t_atom a = argv[i];  
		
		if(a.a_type == A_FLOAT) {  
			float f = a.a_w.w_float;
			list.addFloat(f);
		}
		else if(a.a_type == A_SYMBOL) {  
			char* s = a.a_w.w_symbol->s_name;
			list.addSymbol((string) s);  
		}
	}
	
    if(pdPtr->receiver)
        pdPtr->receiver->receiveList((string) source, list);
}

void PdBase::_message(const char* source, const char *symbol, int argc, t_atom *argv)
{
	List list((string) source);
	
	for(int i = 0; i < argc; i++) {
		
		t_atom a = argv[i];  
		
		if(a.a_type == A_FLOAT) {  
			float f = a.a_w.w_float;
			list.addFloat(f); 
		}
		else if(a.a_type == A_SYMBOL) {  
			char* s = a.a_w.w_symbol->s_name;
			list.addSymbol((string) s);  
		}
	}
	
    if(pdPtr->receiver)
        pdPtr->receiver->receiveMessage((string) source, (string) symbol, list);
}

void PdBase::_noteon(int channel, int pitch, int velocity) {
    if(pdPtr->midiReceiver)
        pdPtr->midiReceiver->receiveNote(channel, pitch, velocity);
}

void PdBase::_controlchange(int channel, int controller, int value) {
    if(pdPtr->midiReceiver)
        pdPtr->midiReceiver->receiveCtl(channel, controller, value);
}

void PdBase::_programchange(int channel, int value) {
    if(pdPtr->midiReceiver)
        pdPtr->midiReceiver->receivePgm(channel, value);
}

void PdBase::_pitchbend(int channel, int value) {
    if(pdPtr->midiReceiver)
        pdPtr->midiReceiver->receiveBend(channel, value);
}

void PdBase::_aftertouch(int channel, int value) {
    if(pdPtr->midiReceiver)
        pdPtr->midiReceiver->receiveTouch(channel, value);
}

void PdBase::_polyaftertouch(int channel, int pitch, int value) {
    if(pdPtr->midiReceiver)
        pdPtr->midiReceiver->receivePolyTouch(channel, pitch, value);
}

void PdBase::_midibyte(int port, int byte) {
	if(pdPtr->midiReceiver)
        pdPtr->midiReceiver->receiveMidiByte(port, byte);
}

} // namespace
