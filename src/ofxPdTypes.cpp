#include "ofxPdTypes.h"
	
//----------------------------------------------------------
bool List::isFloat(const unsigned int index) const {
	if(index < objects.size())
		if(objects[index].type == OFX_PD_FLOAT)
			return true;
	return false;
}

bool List::isSymbol(const unsigned int index) const {
	if(index < objects.size())
		if(objects[index].type == OFX_PD_SYMBOL)
			return true;
	return false;
}

//----------------------------------------------------------
float List::asFloat(const unsigned int index) const {
	if(!isFloat(index)) {
		ofLog(OF_LOG_WARNING, "ofxPd: List: object is not a float");
		return 0;
	}
	return objects[index].value;
}

std::string List::asSymbol(const unsigned int index) const {
	if(!isSymbol(index)) {
		ofLog(OF_LOG_WARNING, "ofxPd: List: object is not a symbol");
		return "";
	}
	return objects[index].symbol;
}

//----------------------------------------------------------
void List::addFloat(const float value) {
	MsgObject o;
	o.type = OFX_PD_FLOAT;
	o.value = value;
	objects.push_back(o);
	typeString += 'f';
}

void List::addSymbol(const std::string& symbol) {
	MsgObject o;
	o.type = OFX_PD_SYMBOL;
	o.symbol = symbol;
	objects.push_back(o);
	typeString += 's';
}

//----------------------------------------------------------
const unsigned int List::len() const {
	return objects.size();
}

const std::string& List::types() const {
	return typeString;
}

void List::clear() {
	typeString = "";
	objects.clear();
}

std::string List::toString() const {
	
	string line;
	
	for(int i = 0; i < objects.size(); ++i) {
		if(isFloat(i))
			line += ofToString(asFloat(i));
		else
			line += asSymbol(i);
		line += " ";
	}
	
	return line;
}

std::ostream& operator<<(std::ostream& os, const List& from) {
	return os << from.toString();
}
		