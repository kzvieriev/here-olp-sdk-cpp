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

#include <gtest/gtest.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>

#include <olp/dataservice/read/VersionedLayerClient.h>

namespace {
using namespace olp::dataservice::read;
using namespace ::testing;
using namespace olp::tests::common;

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kLayerId = "testlayer";
const auto kHrn = olp::client::HRN::FromString(kCatalog);
const auto kPartitionId = "269";
const auto kTimeout = std::chrono::seconds(5);

constexpr auto kBlobDataHandle = R"(4eed6ed1-0d32-43b9-ae79-043cb4256432)";

TEST(VersionedLayerClientTest, CanBeMoved) {
  VersionedLayerClient client_a(olp::client::HRN(), "", {});
  VersionedLayerClient client_b(std::move(client_a));
  VersionedLayerClient client_c(olp::client::HRN(), "", {});
  client_c = std::move(client_b);
}

TEST(VersionedLayerClientTest, GetData) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  VersionedLayerClient client(kHrn, kLayerId, settings);
  {
    SCOPED_TRACE("Get Data with PartitionId and DataHandle");
    std::promise<DataResponse> promise;
    std::future<DataResponse> future = promise.get_future();

    auto token = client.GetData(
        DataRequest()
            .WithPartitionId(kPartitionId)
            .WithDataHandle(kBlobDataHandle),
        [&](DataResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::PreconditionFailed);
  }
}

}  // namespace
