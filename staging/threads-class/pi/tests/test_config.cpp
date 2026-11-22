#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <unistd.h>

#include "vigil/core/Config.hpp"
#include "vigil/core/IniFile.hpp"
#include "vigil/net/BrokerConfig.hpp"

using namespace vigil;

static std::string writeTemp(const std::string& text) {
    char path[] = "/tmp/vigil-test-XXXXXX";
    int fd = mkstemp(path);
    close(fd);
    std::ofstream(path) << text;
    return path;
}

TEST(IniFile, SectionsAndInlineComments) {
    auto ini = IniFile::parse(
        "; comment\n[broker]\nhost = x.emqxsl.com   ; or the laptop's IP\nport=8883\n"
        "# another\n[other]\nhost = y\n");
    EXPECT_EQ(ini.get("broker", "host").value(), "x.emqxsl.com");
    EXPECT_EQ(ini.getInt("broker", "port", 0, 1, 65535), 8883);
    EXPECT_EQ(ini.get("other", "host").value(), "y");
    EXPECT_FALSE(ini.get("broker", "missing").has_value());
}

TEST(IniFile, RejectsMalformed) {
    EXPECT_THROW(IniFile::parse("[broker\n"), std::runtime_error);
    EXPECT_THROW(IniFile::parse("[b]\nnovalue\n"), std::runtime_error);
    auto ini = IniFile::parse("[b]\nport = abc\nbig = 70000\n");
    EXPECT_THROW(ini.getInt("b", "port", 1, 1, 65535), std::runtime_error);
    EXPECT_THROW(ini.getInt("b", "big", 1, 1, 65535), std::runtime_error);
}

TEST(Config, DefaultsWhenMissing) {
    Config c = Config::load("/nonexistent/vigil.conf");
    EXPECT_EQ(c.exitDelayS, 60);
    EXPECT_EQ(c.entryDelayS, 30);
    EXPECT_EQ(c.pinEntryTimeoutS, 10);
    EXPECT_EQ(c.mq2WarmUpS, 60);
    EXPECT_EQ(c.mq2RequiredReads, 5);
}

TEST(Config, OverridesFromFile) {
    std::string p = writeTemp("[timing]\nmq2_warmup_s = 10\npin_entry_timeout_s = 5\n");
    Config c = Config::load(p);
    EXPECT_EQ(c.mq2WarmUpS, 10);
    EXPECT_EQ(c.pinEntryTimeoutS, 5);
    EXPECT_EQ(c.exitDelayS, 60);
    std::remove(p.c_str());
}

TEST(BrokerConfig, LoadsExampleAndReadsSecret) {
    std::string secret = writeTemp("s3cret\n");
    std::string p = writeTemp("[broker]\nhost = localhost\nport = 8883\nca_file = ca.crt ; test CA\n"
                              "secret_file = " + secret + "\n");
    BrokerConfig b = BrokerConfig::load(p);
    EXPECT_EQ(b.host, "localhost");
    EXPECT_EQ(b.caFile, "ca.crt");
    EXPECT_EQ(b.clientId, "vigil-device-home1");
    EXPECT_EQ(b.username, "vigil-device");
    EXPECT_EQ(b.keepaliveS, 30);
    EXPECT_EQ(b.readSecret(), "s3cret");
    std::remove(p.c_str());
    std::remove(secret.c_str());
}

TEST(BrokerConfig, MissingRequiredKeyThrows) {
    std::string p = writeTemp("[broker]\nhost = localhost\n");
    EXPECT_THROW(BrokerConfig::load(p), std::runtime_error);
    std::remove(p.c_str());
}
