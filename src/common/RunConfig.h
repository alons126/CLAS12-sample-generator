#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
namespace samples {
// Strict key/value configuration: file values, then command-line overrides.
class RunConfig {
   public:
    static RunConfig parse(int argc, char** argv, bool genie);
    std::string get(const std::string& key) const;
    double number(const std::string& key) const;
    std::uint64_t integer(const std::string& key) const;
    const std::map<std::string, std::string>& values() const { return values_; }
    void validate(bool genie) const;

   private:
    std::map<std::string, std::string> values_;
};
std::string help(bool genie);
std::string jsonString(const std::string& text);
}  // namespace samples
