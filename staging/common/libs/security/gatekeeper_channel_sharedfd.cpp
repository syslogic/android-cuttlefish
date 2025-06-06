/*
 * Copyright 2020 The Android Open Source Project
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

#include "common/libs/security/gatekeeper_channel_sharedfd.h"

#include <android-base/logging.h>
#include "keymaster/android_keymaster_utils.h"

namespace cuttlefish {

SharedFdGatekeeperChannel::SharedFdGatekeeperChannel(SharedFD input,
                                                     SharedFD output)
    : channel_(transport::SharedFdChannel(std::move(input), std::move(output))) {}

bool SharedFdGatekeeperChannel::SendRequest(
    uint32_t command, const gatekeeper::GateKeeperMessage& message) {
  return SendMessage(command, false, message);
}

bool SharedFdGatekeeperChannel::SendResponse(
    uint32_t command, const gatekeeper::GateKeeperMessage& message) {
  return SendMessage(command, true, message);
}

bool SharedFdGatekeeperChannel::SendMessage(uint32_t command, bool is_response,
                                            const gatekeeper::GateKeeperMessage& message) {
  LOG(DEBUG) << "Sending message with id: " << command;
  auto payload_size = message.GetSerializedSize();
  auto to_send_result = transport::CreateMessage(command, payload_size);
  if (!to_send_result.ok()) {
    LOG(ERROR) << "Could not allocate Gatekeeper Message: "
               << to_send_result.error().FormatForEnv();
    return false;
  }
  auto to_send = std::move(to_send_result.value());
  message.Serialize(to_send->payload, to_send->payload + payload_size);

  auto result = is_response ? channel_.SendResponse(*to_send) : channel_.SendRequest(*to_send);
  if (!result.ok()) {
    LOG(ERROR) << "Could not write Gatekeeper Message: "
               << result.error().FormatForEnv();
  }
  return result.ok();
}

transport::ManagedMessage SharedFdGatekeeperChannel::ReceiveMessage() {
  auto result = channel_.ReceiveMessage();
  if (!result.ok()) {
    return {};
  }
  return std::move(result.value());
}

}  // namespace cuttlefish
