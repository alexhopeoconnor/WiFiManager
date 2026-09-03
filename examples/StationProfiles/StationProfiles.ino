#include <Arduino.h>
#include <EEPROM.h>
#include <WiFiManager.h>

namespace {
constexpr uint32_t kStoreMagic = 0x574D5031;  // "WMP1"

struct StoredProfiles {
    uint32_t magic;
    WiFiManagerStationProfiles profiles;
};

class EepromProfileStore final : public WiFiManagerStationProfileStore {
public:
    bool begin() {
#if defined(ESP32)
        return EEPROM.begin(sizeof(StoredProfiles));
#else
        EEPROM.begin(sizeof(StoredProfiles));
        return true;
#endif
    }

    bool load(WiFiManagerStationProfiles& profiles) override {
        StoredProfiles stored{};
        EEPROM.get(0, stored);
        if (stored.magic != kStoreMagic) {
            return false;
        }
        profiles = stored.profiles;
        return true;
    }

    bool save(const WiFiManagerStationProfiles& profiles) override {
        EEPROM.put(0, StoredProfiles{kStoreMagic, profiles});
        return EEPROM.commit();
    }

    bool clear() override {
        EEPROM.put(0, StoredProfiles{});
        return EEPROM.commit();
    }
};

EepromProfileStore profileStore;
WiFiManager portal;
}  // namespace

void setup() {
    Serial.begin(115200);
    if (!profileStore.begin()) {
        Serial.println("Could not initialise EEPROM profile storage");
        return;
    }

    portal.setStationProfileStore(&profileStore);
    portal.setStationRecoveryInterval(30000);
    portal.startStationConnection("WiFiManager Profiles", "example-pass");
}

void loop() {
    portal.process();
}
