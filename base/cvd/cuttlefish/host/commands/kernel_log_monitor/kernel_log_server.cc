/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "cuttlefish/host/commands/kernel_log_monitor/kernel_log_server.h"

#include <fcntl.h>
#include <sys/types.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <android-base/logging.h>
#include <android-base/strings.h>

#include "cuttlefish/common/libs/fs/shared_fd.h"
#include "cuttlefish/common/libs/fs/shared_select.h"
#include "cuttlefish/host/libs/config/cuttlefish_config.h"
#include "cuttlefish/host/libs/config/config_constants.h"

namespace cuttlefish::monitor {
namespace {

constexpr struct {
  std::string_view match;   // Substring to match in the kernel logs
  std::string_view prefix;  // Prefix value to output, describing the entry
} kInformationalPatterns[] = {
    {"U-Boot ", "GUEST_UBOOT_VERSION: "},
    {"] Linux version ", "GUEST_KERNEL_VERSION: "},
    {"GUEST_BUILD_FINGERPRINT: ", "GUEST_BUILD_FINGERPRINT: "},
};

enum EventFormat {
  kBare,          // Just an event, no extra data
  kKeyValuePair,  // <stage> <key>=<value>
  kPrefix,        // <stage> <more data>
};

constexpr struct {
  std::string_view stage;  // substring in the log identifying the stage
  Event event;             // emitted when the stage is encountered
  EventFormat format;      // how the log message is formatted
} kStageTable[] = {
    {kBootStartedMessage, Event::BootStarted, kBare},
    {kBootPendingMessage, Event::BootPending, kPrefix},
    {kBootCompletedMessage, Event::BootCompleted, kBare},
    {kBootFailedMessage, Event::BootFailed, kPrefix},
    {kMobileNetworkConnectedMessage, Event::MobileNetworkConnected, kBare},
    {kWifiConnectedMessage, Event::WifiNetworkConnected, kBare},
    {kEthernetConnectedMessage, Event::EthernetNetworkConnected, kBare},
    {kAdbdStartedMessage, Event::AdbdStarted, kBare},
    {kFastbootdStartedMessage, Event::FastbootStarted, kBare},
    {kFastbootStartedMessage, Event::FastbootStarted, kBare},
    {kScreenChangedMessage, Event::ScreenChanged, kKeyValuePair},
    {kBootloaderLoadedMessage, Event::BootloaderLoaded, kBare},
    {kKernelLoadedMessage, Event::KernelLoaded, kBare},
    {kDisplayPowerModeChangedMessage, Event::DisplayPowerModeChanged,
     kKeyValuePair},
    {kHibernationExitMessage, Event::HibernationExited, kBare},
    {kHibernationExitMessage, Event::AdbdStarted, kBare},
};

void ProcessSubscriptions(Json::Value message,
                          std::vector<EventCallback>* subscribers) {
  auto active_subscription_count = subscribers->size();
  std::size_t idx = 0;
  while (idx < active_subscription_count) {
    // Call the callback
    auto action = (*subscribers)[idx](message);
    if (action == SubscriptionAction::ContinueSubscription) {
      ++idx;
    } else {
      // Cancel the subscription by swapping it with the last active subscription
      // and decreasing the active subscription count
      --active_subscription_count;
      std::swap((*subscribers)[idx], (*subscribers)[active_subscription_count]);
    }
  }
  // Keep only the active subscriptions
  subscribers->resize(active_subscription_count);
}
}  // namespace

KernelLogServer::KernelLogServer(SharedFD pipe_fd, const std::string& log_name)
    : pipe_fd_(pipe_fd),
      log_fd_(SharedFD::Open(log_name.c_str(), O_CREAT | O_RDWR | O_APPEND,
                             0666)) {}

void KernelLogServer::BeforeSelect(SharedFDSet* fd_read) const {
  fd_read->Set(pipe_fd_);
}

void KernelLogServer::AfterSelect(const SharedFDSet& fd_read) {
  if (fd_read.IsSet(pipe_fd_)) {
    HandleIncomingMessage();
  }
}

void KernelLogServer::SubscribeToEvents(EventCallback callback) {
  subscribers_.push_back(callback);
}

bool KernelLogServer::HandleIncomingMessage() {
  const size_t buf_len = 256;
  char buf[buf_len];
  ssize_t ret = pipe_fd_->Read(buf, buf_len);
  if (ret < 0) {
    LOG(ERROR) << "Could not read kernel logs: " << pipe_fd_->StrError();
    return false;
  }
  if (ret == 0) {
    return false;
  }
  // Write the log to a file
  if (log_fd_->Write(buf, ret) < 0) {
    LOG(ERROR) << "Could not write kernel log to file: " << log_fd_->StrError();
    return false;
  }

  // Detect VIRTUAL_DEVICE_BOOT_*
  for (ssize_t i=0; i<ret; i++) {
    if ('\n' == buf[i]) {
      for (auto& [match, prefix] : kInformationalPatterns) {
        auto pos = line_.find(match);
        if (std::string::npos != pos) {
          LOG(INFO) << prefix << line_.substr(pos + match.size());
        }
      }
      for (const auto& [stage, event, format] : kStageTable) {
        auto pos = line_.find(stage);
        if (std::string::npos != pos) {
          // Log the stage
          if (format == kPrefix) {
            LOG(INFO) << line_.substr(pos);
          } else {
            LOG(INFO) << stage;
          }

          Json::Value message;
          message["event"] = event;
          Json::Value metadata;

          if (format == kKeyValuePair) {
            // Expect space-separated key=value pairs in the log message.
            const auto& fields =
                android::base::Split(line_.substr(pos + stage.size()), " ");
            for (std::string field : fields) {
              field = android::base::Trim(field);
              if (field.empty()) {
                // Expected; android::base::Split() always returns at least
                // one (possibly empty) string.
                LOG(DEBUG) << "Empty field for line: " << line_;
                continue;
              }
              const auto& keyvalue = android::base::Split(field, "=");
              if (keyvalue.size() != 2) {
                LOG(WARNING) << "Field is not in key=value format: " << field;
                continue;
              }
              metadata[keyvalue[0]] = keyvalue[1];
            }
          }
          message["metadata"] = metadata;
          ProcessSubscriptions(message, &subscribers_);
        }
      }
      line_.clear();
    }
    line_.append(1, buf[i]);
  }

  return true;
}

}  // namespace cuttlefish::monitor
