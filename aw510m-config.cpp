#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <map>
#include <algorithm>
#include <cstdint>

using namespace std;

// --- Key map ---
static const std::map<std::string, uint8_t> KEY_MAP = {
    // Letters
    {"A", 0x04}, {"B", 0x05}, {"C", 0x06}, {"D", 0x07}, {"E", 0x08},
    {"F", 0x09}, {"G", 0x0A}, {"H", 0x0B}, {"I", 0x0C}, {"J", 0x0D},
    {"K", 0x0E}, {"L", 0x0F}, {"M", 0x10}, {"N", 0x11}, {"O", 0x12},
    {"P", 0x13}, {"Q", 0x14}, {"R", 0x15}, {"S", 0x16}, {"T", 0x17},
    {"U", 0x18}, {"V", 0x19}, {"W", 0x1A}, {"X", 0x1B}, {"Y", 0x1C},
    {"Z", 0x1D},
    // Numbers (top row)
    {"1", 0x1E}, {"2", 0x1F}, {"3", 0x20}, {"4", 0x21}, {"5", 0x22},
    {"6", 0x23}, {"7", 0x24}, {"8", 0x25}, {"9", 0x26}, {"0", 0x27},
    // Controls / punctuation
    {"Enter", 0x28}, {"Esc", 0x29}, {"Backspace", 0x2A}, {"Tab", 0x2B},
    {"Space", 0x2C}, {"Minus", 0x2D}, {"Equal", 0x2E},
    {"LeftBracket", 0x2F}, {"RightBracket", 0x30}, {"Backslash", 0x31},
    {"NonUSHash", 0x32}, {"Semicolon", 0x33}, {"Quote", 0x34},
    {"Grave", 0x35}, {"Comma", 0x36}, {"Period", 0x37}, {"Slash", 0x38},
    {"CapsLock", 0x39},
    // Function keys
    {"F1", 0x3A}, {"F2", 0x3B}, {"F3", 0x3C}, {"F4", 0x3D}, {"F5", 0x3E},
    {"F6", 0x3F}, {"F7", 0x40}, {"F8", 0x41}, {"F9", 0x42}, {"F10", 0x43},
    {"F11", 0x44}, {"F12", 0x45},
    // Sys / navigation
    {"PrintScreen", 0x46}, {"ScrollLock", 0x47}, {"Pause", 0x48},
    {"Insert", 0x49}, {"Home", 0x4A}, {"PageUp", 0x4B}, {"Delete", 0x4C},
    {"End", 0x4D}, {"PageDown", 0x4E},
    // Arrows
    {"Right", 0x4F}, {"Left", 0x50}, {"Down", 0x51}, {"Up", 0x52},
    // Keypad / Num
    {"NumLock", 0x53}, {"KP_Slash", 0x54}, {"KP_Asterisk", 0x55}, {"KP_Minus", 0x56},
    {"KP_Plus", 0x57}, {"KP_Enter", 0x58}, {"KP_1", 0x59}, {"KP_2", 0x5A},
    {"KP_3", 0x5B}, {"KP_4", 0x5C}, {"KP_5", 0x5D}, {"KP_6", 0x5E},
    {"KP_7", 0x5F}, {"KP_8", 0x60}, {"KP_9", 0x61}, {"KP_0", 0x62},
    {"KP_Decimal", 0x63},
    // (některé další klávesy v rozsahu)
    {"IntlBackslash", 0x64}, {"Application", 0x65}, {"Power", 0x66},
    {"KP_Equal", 0x67}, {"F13", 0x68}, {"F14", 0x69}, {"F15", 0x6A},
    {"F16", 0x6B}, {"F17", 0x6C}, {"F18", 0x6D}, {"F19", 0x6E},
    {"F20", 0x6F}, {"F21", 0x70}, {"F22", 0x71}, {"F23", 0x72},
    {"F24", 0x73},
    // Modifier keys
    {"LeftCtrl", 0xE0}, {"LeftShift", 0xE1}, {"LeftAlt", 0xE2}, {"LeftGUI", 0xE3},
    {"RightCtrl", 0xE4}, {"RightShift", 0xE5}, {"RightAlt", 0xE6}, {"RightGUI", 0xE7}
};

// Helper – pads a packet to 64 bytes
static void padPacket(vector<uint8_t>& packet) {
    packet.resize(64, 0x00);
}

// Create generic packet with header and payload
vector<uint8_t> createPacket(const vector<uint8_t>& header,
                             const vector<uint8_t>& payload) {
    vector<uint8_t> packet;
    packet.reserve(64);

    packet.insert(packet.end(), header.begin(), header.end());
    packet.insert(packet.end(), payload.begin(), payload.end());

    padPacket(packet);
    return packet;
}

// Parse "keyX = VALUE"
bool parseKeyLine(const string& line, string& keyIndex, string& keyValue) {
    size_t pos = line.find('=');
    if (pos == string::npos) return false;

    string key = line.substr(0, pos);
    string value = line.substr(pos + 1);

    auto trim = [](string& s) {
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \t") + 1);
    };

    trim(key);
    trim(value);

    if (key.rfind("key", 0) == 0) {
        keyIndex = key.substr(3);
        keyValue = value;
        return true;
    }
    return false;
}

// Parse "color = R G B"
bool parseColorLine(const string& line, string& colorKey, string& rgbValue) {
    size_t pos = line.find('=');
    if (pos == string::npos) return false;

    string key = line.substr(0, pos);
    string value = line.substr(pos + 1);

    auto trim = [](string& s) {
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \t") + 1);
    };

    trim(key);
    trim(value);

    if (key.rfind("color", 0) == 0) {
        colorKey = key;
        rgbValue = value;
        return true;
    }
    return false;
}

// Convert R G B into vector<uint8_t>
vector<uint8_t> parseRGB(const string& val) {
    vector<uint8_t> rgb;
    stringstream ss(val);
    string tok;
    int v;

    while (ss >> tok && rgb.size() < 3) {
        try {
            v = stoi(tok);
            v = max(0, min(255, v));
            rgb.push_back(static_cast<uint8_t>(v));
        } catch (...) {}
    }

    while (rgb.size() < 3) rgb.push_back(0);
    return rgb;
}

// Translate key string into HID byte
uint8_t keyToByte(const string& key) {
    auto it = KEY_MAP.find(key);
    return (it != KEY_MAP.end()) ? it->second : 0x00;
}

// Send binary packet to HIDRAW device
void sendPacket(int fd, const vector<uint8_t>& packet) {
    ssize_t written = write(fd, packet.data(), packet.size());
    if (written < 0) cerr << "Error writing packet" << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <config_file> <hidraw_path>" << endl;
        return 1;
    }

    ifstream config(argv[1]);
    if (!config.is_open()) {
        cerr << "Unable to open config file" << endl;
        return 1;
    }

    int fd = open(argv[2], O_WRONLY);
    if (fd < 0) {
        cerr << "Unable to open HIDRAW device" << endl;
        return 1;
    }

    string line;
    while (getline(config, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '/') continue;

        string keyIndex, value;

        // Handle key packets
        if (parseKeyLine(line, keyIndex, value)) {
            uint8_t keyCode = keyToByte(value);
            int idx = stoi(keyIndex);

            // Unified packet creation
            auto p1 = createPacket({0x40,0x25,0x3c,0x00,0x02,0x00,0x0c,0x00,0x00,0x00,0x01,0x00}, {keyCode,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x19});
            auto p2 = createPacket({0x40,0x1a,0x07,0x00}, {static_cast<uint8_t>(idx),static_cast<uint8_t>(idx)});
            auto p3 = createPacket({0x40,0x60,0x07,0x00,0x9e,0x4e,0x00,0x00,0x01}, {static_cast<uint8_t>(idx),static_cast<uint8_t>(idx)});

            sendPacket(fd, p1); usleep(50000);
            sendPacket(fd, p2); usleep(50000);
            sendPacket(fd, p3); usleep(75000);
            continue;
        }

        // Handle color packets
        if (parseColorLine(line, keyIndex, value)) {
            auto rgb = parseRGB(value);

            auto p1 = createPacket({0x40,0x03,0x01,0x00,0x01}, {});
            auto p2 = createPacket({0x40,0x10,0x0c,0x00,0x01,0x64,0x00}, rgb);
            auto p3 = createPacket({0x40,0x60,0x07,0x00,0x9e,0x4e,0x00,0x01}, {});

            sendPacket(fd, p1); usleep(50000);
            sendPacket(fd, p2); usleep(50000);
            sendPacket(fd, p3); usleep(75000);
            continue;
        }
    }

    close(fd);
    return 0;
}
