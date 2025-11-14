#ifndef WiFiManagerParameter_h
#define WiFiManagerParameter_h

#include <Arduino.h>

class WiFiManager;

#ifndef WIFI_MANAGER_MAX_PARAMS
    #define WIFI_MANAGER_MAX_PARAMS 5 // params will autoincrement and realloc by this amount when max is reached
#endif

#define WFM_LABEL_BEFORE 1
#define WFM_LABEL_AFTER 2
#define WFM_NO_LABEL 0
#define WFM_LABEL_DEFAULT 1

class WiFiManagerParameter {
  public:
    /**
        Create custom parameters that can be added to the WiFiManager setup web page
        @id is used for HTTP queries and must not contain spaces nor other special characters
    */
    WiFiManagerParameter();
    explicit WiFiManagerParameter(const char *custom);
    WiFiManagerParameter(const char *id, const char *label);
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length);
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom);
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement);
    ~WiFiManagerParameter();
    // WiFiManagerParameter& operator=(const WiFiManagerParameter& rhs);

    const char *getID() const;
    const char *getValue() const;
    const char *getLabel() const;
    const char *getPlaceholder() const; // @deprecated, use getLabel
    int         getValueLength() const;
    int         getLabelPlacement() const;
    virtual const char *getCustomHTML() const;
    void        setValue(const char *defaultValue, int length);

  protected:
    void init(const char *id, const char *label, const char *defaultValue, int length, const char *custom, int labelPlacement);

    WiFiManagerParameter& operator=(const WiFiManagerParameter&);
    const char *_id;
    const char *_label;
    char       *_value;
    int         _length;
    int         _labelPlacement;

    const char *_customHTML;
    friend class WiFiManager;
};

#endif // WiFiManagerParameter_h

