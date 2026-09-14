#pragma once
#include <TRandom3.h>
#include <TVector3.h>

#include <string>
#include <utility>
namespace samples {
class TargetGeometry {
   public:
    explicit TargetGeometry(std::string name) : name_(std::move(name)) { validate(name_); }
    static void validate(const std::string& name);
    TVector3 sample(TRandom3& random) const;

   private:
    std::string name_;
};
}  // namespace samples
