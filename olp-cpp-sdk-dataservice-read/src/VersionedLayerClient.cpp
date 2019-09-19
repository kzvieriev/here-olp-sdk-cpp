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

#include <olp/dataservice/read/VersionedLayerClient.h>

#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/thread/TaskScheduler.h>

#include <olp/core/client/OlpClient.h>
#include <olp/core/context/Context.h>

#include "generated/api/BlobApi.h"
#include "generated/api/QueryApi.h"

#include "olp/dataservice/read/Condition.h"

#include "ApiClientLookup.h"
#include "generated/model/Api.h"

#include "repositories/ExecuteOrSchedule.inl"

#include <algorithm>

namespace olp {
namespace dataservice {
namespace read {

VersionedLayerClient::VersionedLayerClient(
    std::shared_ptr<olp::client::OlpClientSettings> client_settings,
    client::HRN hrn, std::string layer_id, std::int64_t layer_version)
    : client_settings_(client_settings),
      hrn_(std::move(hrn)),
      layer_id_(std::move(layer_id)),
      layer_version_(layer_version) {
  // TODO: Consider move olp client as constructor argument
  // TODO: nullptr
  olp_client_ = olp::client::OlpClientFactory::Create(*client_settings);
}

olp::client::CancellationToken VersionedLayerClient::GetDataByPartitionId(
    const std::string& partition_id, DataResponseCallback callback) {
  auto context = std::make_shared<olp::client::CancellationContext>();
  olp::client::CancellationToken token(
      [context]() mutable { context->CancelOperation(); });

  auto olp_client = olp_client_;
  auto client_settings = client_settings_;
  auto hrn = hrn_;
  auto layer_id = layer_id_;
  auto layer_version = layer_version_;

  auto task = [context, callback, partition_id, olp_client, client_settings,
               hrn, layer_id, layer_version]() {
    Condition condition(*context);
    auto wait_and_check = [&] {
      if (!condition.Wait() && !context->IsCancelled()) {
        callback({{olp::client::ErrorCode::RequestTimeout,
                   "Network request timed out.", true}});
        return false;
      }
      return true;
    };

    // Step 1. Get query service

    ApiClientLookup::ApiClientResponse apis_response;

    context->ExecuteOrCancelled([&]() {
      return ApiClientLookup::LookupApiClient(
          olp_client, "query", "v1", hrn,
          [&](ApiClientLookup::ApiClientResponse response) {
            apis_response = std::move(response);
            condition.Notify();
          });
    });

    // TODO: collapse these 2x4 checks into a lambda calls
    if (!wait_and_check()) {
      return;
    }
    if (!apis_response.IsSuccessful()) {
      callback({{olp::client::ErrorCode::ServiceUnavailable,
                 "Query request unsuccessful.", true}});
      return;
    }

    auto query_client = apis_response.GetResult();

    // Step 2. Use query service to acquire metadata

    std::vector<std::string> paritions;
    paritions.push_back(partition_id);
    QueryApi::PartitionsResponse partitions_response;
    context->ExecuteOrCancelled([&]() {
      return olp::dataservice::read::QueryApi::GetPartitionsbyId(
          query_client, layer_id, paritions, layer_version, boost::none,
          boost::none, [&](QueryApi::PartitionsResponse response) {
            partitions_response = std::move(response);
            condition.Notify();
          });
    });

    if (!wait_and_check()) {
      return;
    }
    if (!partitions_response.IsSuccessful()) {
      callback({{olp::client::ErrorCode::ServiceUnavailable,
                 "Metadata request unsuccessful.", true}});
      return;
    }

    // Step 3. Get blob service

    context->ExecuteOrCancelled([&]() {
      return ApiClientLookup::LookupApiClient(
          olp_client, "blob", "v1", hrn,
          [&](ApiClientLookup::ApiClientResponse response) {
            apis_response = std::move(response);
            condition.Notify();
          });
    });

    if (!wait_and_check()) {
      return;
    }
    if (!apis_response.IsSuccessful()) {
      callback({{olp::client::ErrorCode::ServiceUnavailable,
                 "Blob request unsuccessful.", true}});
      return;
    }

    // Step 4. Use metadata in blob service to acquire data for user

    auto partitions = partitions_response.GetResult();
    auto partition_it = std::find_if(partitions.GetPartitions().begin(),
                                     partitions.GetPartitions().end(),
                                     [&](const model::Partition& p) {
                                       return p.GetPartition() == partition_id;
                                     });
    if (partition_it == partitions.GetPartitions().end()) {
      callback(DataResponse(model::Data()));
      return;
    }
    auto data_handle = partition_it->GetDataHandle();
    auto blob_client = apis_response.GetResult();
    BlobApi::DataResponse data_response;
    context->ExecuteOrCancelled([&]() {
      return olp::dataservice::read::BlobApi::GetBlob(
          blob_client, layer_id, data_handle, boost::none, boost::none,
          [&](BlobApi::DataResponse response) {
            data_response = std::move(response);
            condition.Notify();
          });
    });

    if (!wait_and_check()) {
      return;
    }
    if (!data_response.IsSuccessful()) {
      callback({{olp::client::ErrorCode::ServiceUnavailable,
                 "Data request unsuccessful.", true}});
      return;
    }

    auto data = data_response.GetResult();
    callback(data);
  };

  repository::ExecuteOrSchedule(client_settings_.get(), task);

  return token;
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
