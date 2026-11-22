#include "vigil/net/BrokerConfig.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "vigil/core/IniFile.hpp"

namespace vigil {

BrokerConfig BrokerConfig::load(const std::string& path) {
    IniFile ini = IniFile::load(path);
    BrokerConfig c;
    auto require = [&](const char* key) {
        auto v = ini.get("broker", key);
        if (!v || v->empty()) throw std::runtime_error(path + ": missing [broker] " + key);
        return *v;
    };
    c.host       = require("host");
    c.caFile     = require("ca_file");
    c.secretFile = require("secret_file");
    c.port       = static_cast<int>(ini.getInt("broker", "port", c.port, 1, 65535));
    c.clientId   = ini.getString("broker", "client_id", c.clientId);
    c.username   = ini.getString("broker", "username", c.username);
    c.keepaliveS = static_cast<int>(ini.getInt("broker", "keepalive_s", c.keepaliveS, 5, 3600));
    return c;
}

std::string BrokerConfig::readSecret() const {
    std::ifstream f(secretFile);
    if (!f) throw std::runtime_error("cannot read the MQTT secret file " + secretFile);
    std::stringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    if (s.empty()) throw std::runtime_error("empty MQTT secret file " + secretFile);
    return s;
}

}  // namespace vigil
