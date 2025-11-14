#include "WiFiManagerParameter.h"

#include <cstring>

WiFiManagerParameter::WiFiManagerParameter() {
  WiFiManagerParameter("");
}

WiFiManagerParameter::WiFiManagerParameter(const char *custom) {
  _id             = NULL;
  _label          = NULL;
  _length         = 0;
  _value          = nullptr;
  _labelPlacement = WFM_LABEL_DEFAULT;
  _customHTML     = custom;
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label) {
  init(id, label, "", 0, "", WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length) {
  init(id, label, defaultValue, length, "", WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom) {
  init(id, label, defaultValue, length, custom, WFM_LABEL_DEFAULT);
}

WiFiManagerParameter::WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement) {
  init(id, label, defaultValue, length, custom, labelPlacement);
}

void WiFiManagerParameter::init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement) {
  _id             = id;
  _label          = label;
  _labelPlacement = labelPlacement;
  _customHTML     = custom;
  _length         = 0;
  _value          = nullptr;
  setValue(defaultValue,length);
}

WiFiManagerParameter::~WiFiManagerParameter() {
  if (_value != NULL) {
    delete[] _value;
  }
  _length=0; // setting length 0, ideally the entire parameter should be removed, or added to wifimanager scope so it follows
}

// WiFiManagerParameter& WiFiManagerParameter::operator=(const WiFiManagerParameter& rhs){
//   Serial.println("copy assignment op called");
//   (*this->_value) = (*rhs._value);
//   return *this;
// }

// @note debug is not available in wmparameter class
void WiFiManagerParameter::setValue(const char *defaultValue, int length) {
  if(!_id){
    // Serial.println("cannot set value of this parameter");
    return;
  }

  // if(strlen(defaultValue) > length){
  //   // Serial.println("defaultValue length mismatch");
  //   // return false; //@todo bail
  // }

  if(_length != length || _value == nullptr){
    _length = length;
    if( _value != nullptr){
      delete[] _value;
    }
    _value  = new char[_length + 1];
  }

  memset(_value, 0, _length + 1); // explicit null

  if (defaultValue != NULL) {
    strncpy(_value, defaultValue, _length);
  }
}
const char* WiFiManagerParameter::getValue() const {
  // Serial.println(printf("Address of _value is %p\n", (void *)_value));
  return _value;
}
const char* WiFiManagerParameter::getID() const {
  return _id;
}
const char* WiFiManagerParameter::getPlaceholder() const {
  return _label;
}
const char* WiFiManagerParameter::getLabel() const {
  return _label;
}
int WiFiManagerParameter::getValueLength() const {
  return _length;
}
int WiFiManagerParameter::getLabelPlacement() const {
  return _labelPlacement;
}
const char* WiFiManagerParameter::getCustomHTML() const {
  return _customHTML;
}

