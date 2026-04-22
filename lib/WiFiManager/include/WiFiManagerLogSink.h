/**
 * WiFiManagerLogSink.h
 *
 * @author alexhopeoconnor
 * @license MIT
 *
 * Optional host-provided sink for WiFiManager log output. When set, logs are
 * delivered here instead of the configured Print stream (usually Serial).
 */

#ifndef WiFiManagerLogSink_h
#define WiFiManagerLogSink_h

#include "WiFiManagerLogLevel.h"

class WiFiManagerLogSink {
public:
    virtual void log(const WiFiManagerLogMessage& msg) = 0;
    virtual ~WiFiManagerLogSink() = default;
};

#endif
