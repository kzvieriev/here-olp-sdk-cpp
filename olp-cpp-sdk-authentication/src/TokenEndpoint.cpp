/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#include <olp/authentication/TokenEndpoint.h>

#include <olp/authentication/TokenResult.h>
#include "olp/authentication/AuthenticationClient.h"
#include "olp/authentication/AuthenticationCredentials.h"
#include "olp/authentication/AutoRefreshingToken.h"
#include "olp/authentication/ErrorResponse.h"
#include "olp/authentication/Settings.h"
#include "olp/authentication/SignInResult.h"
#include "olp/authentication/TokenRequest.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/warning_disable.h"

namespace {
const std::string kOauth2TokenEndpoint = "/oauth2/token";
constexpr auto kLogTag = "here::account::oauth2::TokenEndpoint";
}  // namespace

namespace olp {
namespace authentication {
struct TokenEndpoint::Impl {
  explicit Impl(Settings settings)
      : auth_client_(std::move(settings.token_endpoint_url)),
        auth_credentials_(std::move(settings.credentials)) {
    PORTING_PUSH_WARNINGS()
    PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")
    if (settings.network_proxy_settings) {
      auth_client_.SetNetworkProxySettings(
          settings.network_proxy_settings.get());
    }
    auth_client_.SetNetwork(std::move(settings.network_request_handler));
    auth_client_.SetTaskScheduler(std::move(settings.task_scheduler));
    PORTING_POP_WARNINGS()
  }

  PORTING_PUSH_WARNINGS()
  PORTING_CLANG_GCC_DISABLE_WARNING("-Wdeprecated-declarations")
  client::CancellationToken RequestToken(const TokenRequest& token_request,
                                         const RequestTokenCallback& callback) {
    return auth_client_.SignInClient(
        auth_credentials_,
        [callback](
            const AuthenticationClient::SignInClientResponse& signInResponse) {
          if (signInResponse.IsSuccessful()) {
            TokenResult result(signInResponse.GetResult().GetAccessToken(),
                               signInResponse.GetResult().GetExpiresIn(),
                               signInResponse.GetResult().GetStatus(),
                               signInResponse.GetResult().GetErrorResponse());
            callback(TokenResponse(result));
          } else {
            callback(signInResponse.GetError());
          }
        },
        token_request.GetExpiresIn());
  }

  std::future<TokenResponse> RequestToken(
      client::CancellationToken& cancellation_token,
      const TokenRequest& token_request);

  olp::authentication::AuthenticationClient auth_client_;
  olp::authentication::AuthenticationCredentials auth_credentials_;
};  // namespace authentication

std::future<TokenEndpoint::TokenResponse> TokenEndpoint::Impl::RequestToken(
    client::CancellationToken& cancellation_token,
    const TokenRequest& token_request) {
  auto p = std::make_shared<std::promise<TokenResponse> >();
  cancellation_token = RequestToken(
      token_request,
      [p](TokenResponse tokenResponse) { p->set_value(tokenResponse); });
  return p->get_future();
}

TokenEndpoint::TokenEndpoint(Settings settings) {
  // The underlying auth library expects a base URL and appends /oauth2/token
  // endpoint to it. Therefore if /oauth2/token is found it is stripped from the
  // endpoint URL provided. The underlying auth library should be updated to
  // supoprt an arbitrary token endpoint URL.
  auto pos = settings.token_endpoint_url.find(kOauth2TokenEndpoint);
  if (pos != std::string::npos) {
    settings.token_endpoint_url.erase(pos, kOauth2TokenEndpoint.size());
  } else {
    OLP_SDK_LOG_ERROR(
        kLogTag,
        "Expected '/oauth2/token' endpoint in the tokenEndpointUrl. Only "
        "standard "
        "OAuth2 token endpoint URLs are supported.");
  }

  impl_ = std::make_shared<TokenEndpoint::Impl>(std::move(settings));
}

client::CancellationToken TokenEndpoint::RequestToken(
    const TokenRequest& token_request,
    const RequestTokenCallback& callback) const {
  return impl_->RequestToken(token_request, callback);
}

std::future<TokenEndpoint::TokenResponse> TokenEndpoint::RequestToken(
    client::CancellationToken& cancellation_token,
    const TokenRequest& token_request) const {
  return impl_->RequestToken(cancellation_token, token_request);
}

std::future<TokenEndpoint::TokenResponse> TokenEndpoint::RequestToken(
    const TokenRequest& token_request) const {
  client::CancellationToken cancellationToken;
  return impl_->RequestToken(cancellationToken, token_request);
}

AutoRefreshingToken TokenEndpoint::RequestAutoRefreshingToken(
    const TokenRequest& token_request) {
  return AutoRefreshingToken(*this, token_request);
}
PORTING_POP_WARNINGS()

}  // namespace authentication
}  // namespace olp
