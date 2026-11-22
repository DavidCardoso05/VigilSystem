// /etc/vigil/broker.conf (guide §11.1). Switching EMQX <-> Mosquitto only changes this file.
#pragma once

#include <string>

namespace vigil {

struct BrokerConfig {
    std::string host;
    int         port       = 8883;
    std::string caFile;
    std::string clientId   = "vigil-device-home1";
    std::string username   = "vigil-device";
    std::string secretFile;          // file with the password (chmod 600); never in broker.conf
    int         keepaliveS = 30;

    // Throws std::runtime_error when the file is missing or a required key is absent.
    static BrokerConfig load(const std::string& path);
    // Reads the password from secretFile (trailing newline removed). Never log the result.
    std::string readSecret() const;
};

}  // namespace vigil
