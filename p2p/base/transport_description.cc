/*
 *  Copyright 2013 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "p2p/base/transport_description.h"

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "p2p/base/p2p_constants.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/strings/string_builder.h"

using webrtc::RTCError;
using webrtc::RTCErrorOr;
using webrtc::RTCErrorType;

namespace cricket {
namespace {

bool IsIceChar(char c) {
  return absl::ascii_isalnum(c) || c == '+' || c == '/';
}

RTCErrorOr<std::string> ParseIceUfrag(absl::string_view raw_ufrag) {
  if (!(ICE_UFRAG_MIN_LENGTH <= raw_ufrag.size() &&
        raw_ufrag.size() <= ICE_UFRAG_MAX_LENGTH)) {
    rtc::StringBuilder sb;
    sb << "ICE ufrag must be between " << ICE_UFRAG_MIN_LENGTH << " and "
       << ICE_UFRAG_MAX_LENGTH << " characters long.";
    return RTCError(RTCErrorType::SYNTAX_ERROR, sb.Release());
  }

  if (!absl::c_all_of(raw_ufrag, IsIceChar)) {
    return RTCError(
        RTCErrorType::SYNTAX_ERROR,
        "ICE ufrag must contain only alphanumeric characters, '+', and '/'.");
  }

  return std::string(raw_ufrag);
}

RTCErrorOr<std::string> ParseIcePwd(absl::string_view raw_pwd) {
  if (!(ICE_PWD_MIN_LENGTH <= raw_pwd.size() &&
        raw_pwd.size() <= ICE_PWD_MAX_LENGTH)) {
    rtc::StringBuilder sb;
    sb << "ICE pwd must be between " << ICE_PWD_MIN_LENGTH << " and "
       << ICE_PWD_MAX_LENGTH << " characters long.";
    return RTCError(RTCErrorType::SYNTAX_ERROR, sb.Release());
  }

  if (!absl::c_all_of(raw_pwd, IsIceChar)) {
    return RTCError(
        RTCErrorType::SYNTAX_ERROR,
        "ICE pwd must contain only alphanumeric characters, '+', and '/'.");
  }

  return std::string(raw_pwd);
}

}  // namespace

// static
RTCErrorOr<IceParameters> IceParameters::Parse(absl::string_view raw_ufrag,
                                               absl::string_view raw_pwd) {
  // For legacy protocols.
  // TODO(zhihuang): Remove this once the legacy protocol is no longer
  // supported.
  if (raw_ufrag.empty() && raw_pwd.empty()) {
    return IceParameters();
  }

  auto ufrag_result = ParseIceUfrag(raw_ufrag);
  if (!ufrag_result.ok()) {
    return ufrag_result.MoveError();
  }

  auto pwd_result = ParseIcePwd(raw_pwd);
  if (!pwd_result.ok()) {
    return pwd_result.MoveError();
  }

  IceParameters parameters;
  parameters.ufrag = ufrag_result.MoveValue();
  parameters.pwd = pwd_result.MoveValue();
  return parameters;
}

bool StringToConnectionRole(const std::string& role_str, ConnectionRole* role) {
  const char* const roles[] = {
      CONNECTIONROLE_ACTIVE_STR, CONNECTIONROLE_PASSIVE_STR,
      CONNECTIONROLE_ACTPASS_STR, CONNECTIONROLE_HOLDCONN_STR};

  for (size_t i = 0; i < arraysize(roles); ++i) {
    if (absl::EqualsIgnoreCase(roles[i], role_str)) {
      *role = static_cast<ConnectionRole>(CONNECTIONROLE_ACTIVE + i);
      return true;
    }
  }
  return false;
}

bool ConnectionRoleToString(const ConnectionRole& role, std::string* role_str) {
  switch (role) {
    case cricket::CONNECTIONROLE_ACTIVE:
      *role_str = cricket::CONNECTIONROLE_ACTIVE_STR;
      break;
    case cricket::CONNECTIONROLE_ACTPASS:
      *role_str = cricket::CONNECTIONROLE_ACTPASS_STR;
      break;
    case cricket::CONNECTIONROLE_PASSIVE:
      *role_str = cricket::CONNECTIONROLE_PASSIVE_STR;
      break;
    case cricket::CONNECTIONROLE_HOLDCONN:
      *role_str = cricket::CONNECTIONROLE_HOLDCONN_STR;
      break;
    default:
      return false;
  }
  return true;
}

TransportDescription::TransportDescription()
    : ice_mode(ICEMODE_FULL), connection_role(CONNECTIONROLE_NONE) {}

TransportDescription::TransportDescription(
    const std::vector<std::string>& transport_options,
    const std::string& ice_ufrag,
    const std::string& ice_pwd,
    IceMode ice_mode,
    ConnectionRole role,
    const rtc::SSLFingerprint* identity_fingerprint)
    : transport_options(transport_options),
      ice_ufrag(ice_ufrag),
      ice_pwd(ice_pwd),
      ice_mode(ice_mode),
      connection_role(role),
      identity_fingerprint(CopyFingerprint(identity_fingerprint)) {}

TransportDescription::TransportDescription(const std::string& ice_ufrag,
                                           const std::string& ice_pwd)
    : ice_ufrag(ice_ufrag),
      ice_pwd(ice_pwd),
      ice_mode(ICEMODE_FULL),
      connection_role(CONNECTIONROLE_NONE) {}

TransportDescription::TransportDescription(const TransportDescription& from)
    : transport_options(from.transport_options),
      ice_ufrag(from.ice_ufrag),
      ice_pwd(from.ice_pwd),
      ice_mode(from.ice_mode),
      connection_role(from.connection_role),
      identity_fingerprint(CopyFingerprint(from.identity_fingerprint.get())),
      opaque_parameters(from.opaque_parameters) {}

TransportDescription::~TransportDescription() = default;

TransportDescription& TransportDescription::operator=(
    const TransportDescription& from) {
  // Self-assignment
  if (this == &from)
    return *this;

  transport_options = from.transport_options;
  ice_ufrag = from.ice_ufrag;
  ice_pwd = from.ice_pwd;
  ice_mode = from.ice_mode;
  connection_role = from.connection_role;

  identity_fingerprint.reset(CopyFingerprint(from.identity_fingerprint.get()));
  opaque_parameters = from.opaque_parameters;
  return *this;
}

}  // namespace cricket
