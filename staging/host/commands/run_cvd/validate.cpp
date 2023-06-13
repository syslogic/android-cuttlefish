/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "host/commands/run_cvd/validate.h"

#include <sys/utsname.h>

#include <iostream>

#include <android-base/logging.h>
#include <fruit/fruit.h>

#include "common/libs/utils/network.h"
#include "common/libs/utils/result.h"
#include "host/libs/config/cuttlefish_config.h"
#include "host/libs/config/feature.h"
#include "host/libs/vm_manager/host_configuration.h"

namespace cuttlefish {
namespace {

using vm_manager::ValidateHostConfiguration;

class ValidateTapDevicesImpl : public ValidateTapDevices {
 public:
  INJECT(ValidateTapDevicesImpl(
      const CuttlefishConfig::InstanceSpecific& instance))
      : instance_(instance) {}

  std::string Name() const override { return "ValidateTapDevices"; }
  bool Enabled() const override { return true; }

 private:
  std::unordered_set<SetupFeature*> Dependencies() const override { return {}; }
  Result<void> ResultSetup() override {
    CF_EXPECT(TestTapDevices(),
              "There appears to be another cuttlefish device"
              " already running, using the requested host "
              "resources. Try `cvd reset` or `pkill run_cvd` "
              "and `pkill crosvm`");
    return {};
  }

  Result<void> TestTapDevices() {
    auto taps = TapInterfacesInUse();
    auto wifi = instance_.wifi_tap_name();
    CF_EXPECTF(taps.count(wifi) == 0, "Device \"{}\" in use", wifi);
    auto mobile = instance_.mobile_tap_name();
    CF_EXPECTF(taps.count(mobile) == 0, "Device \"{}\" in use", mobile);
    auto eth = instance_.ethernet_tap_name();
    CF_EXPECTF(taps.count(eth) == 0, "Device \"{}\" in use", eth);
    return {};
  }

  const CuttlefishConfig::InstanceSpecific& instance_;
};

class ValidateHostConfigurationFeature : public SetupFeature {
 public:
  INJECT(ValidateHostConfigurationFeature()) {}

  bool Enabled() const override {
#ifndef __ANDROID__
    return true;
#else
    return false;
#endif
  }
  std::string Name() const override { return "ValidateHostConfiguration"; }

 private:
  std::unordered_set<SetupFeature*> Dependencies() const override { return {}; }
  Result<void> ResultSetup() override {
    // Check host configuration
    std::vector<std::string> config_commands;
    CF_EXPECTF(ValidateHostConfiguration(&config_commands),
               "Validation of user configuration failed.\n"
               "Execute the following to correctly configure: \n[{}]\n",
               "You may need to logout for the changes to take effect.\n",
               fmt::join(config_commands, "\n"));
    return {};
  }
};

class ValidateHostKernelFeature : public SetupFeature {
 public:
  INJECT(ValidateHostKernelFeature()) {}

  bool Enabled() const override { return true; }
  std::string Name() const override { return "ValidateHostKernel"; }

 private:
  std::unordered_set<SetupFeature*> Dependencies() const override { return {}; }
  Result<void> ResultSetup() override {
    struct utsname uname_data;
    CF_EXPECT_EQ(uname(&uname_data), 0, "uname failed: " << strerror(errno));
    LOG(DEBUG) << "uts.sysname = \"" << uname_data.sysname << "\"";
    LOG(DEBUG) << "uts.nodename = \"" << uname_data.nodename << "\"";
    LOG(DEBUG) << "uts.release = \"" << uname_data.release << "\"";
    LOG(DEBUG) << "uts.version = \"" << uname_data.version << "\"";
    LOG(DEBUG) << "uts.machine = \"" << uname_data.machine << "\"";
#ifdef _GNU_SOURCE
    LOG(DEBUG) << "uts.domainname = \"" << uname_data.domainname << "\"";
#endif
    return {};
  }
};

}  // namespace

fruit::Component<fruit::Required<const CuttlefishConfig::InstanceSpecific>,
                 ValidateTapDevices>
validationComponent() {
  return fruit::createComponent()
      .addMultibinding<SetupFeature, ValidateHostConfigurationFeature>()
      .addMultibinding<SetupFeature, ValidateHostKernelFeature>()
      .bind<ValidateTapDevices, ValidateTapDevicesImpl>()
      .addMultibinding<SetupFeature, ValidateTapDevicesImpl>();
}

}  // namespace cuttlefish
