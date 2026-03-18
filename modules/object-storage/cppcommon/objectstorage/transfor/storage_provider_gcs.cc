#include "storage_provider_gcs.h"

#include <alibabacloud/oss/OssClient.h>
#include <alibabacloud/oss/auth/CredentialsProvider.h>
#include <google/cloud/credentials.h>
#include <google/cloud/storage/client.h>
#include <google/cloud/storage/client_options.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "cppcommon/extends/abseil/absl.h"
#include "cppcommon/objectstorage/transfor/object_transfor.h"
#include "cppcommon/objectstorage/transfor/storage_provider.h"
#include "spdlog/spdlog.h"

namespace cppcommon::os {
GcsStorageProvider::GcsStorageProvider(const std::string &service_account_json_string) {
  auto cred = google::cloud::MakeServiceAccountCredentials(service_account_json_string);
  auto co = google::cloud::Options{}.set<google::cloud::UnifiedCredentialsOption>(cred);
  client_ = std::make_shared<gcs::Client>(std::move(co));
}

GcsStorageProvider::GcsStorageProvider() {
  auto co = gcs::ClientOptions::CreateDefaultClientOptions().value();
  client_ = std::make_shared<gcs::Client>(std::move(co));
}

absl::StatusOr<FileList> GcsStorageProvider::List(const std::string &bucket, const std::string &path) {
  std::vector<std::string> keys;
  for (auto &&object_metadata : client_->ListObjects(bucket, gcs::Prefix(path))) {
    if (!object_metadata) {
      auto &s = object_metadata.status();
      return absl::Status(absl::StatusCode::kInternal,
                          absl::StrFormat("[GCS::List] %s: %s", s.error_info().reason(), s.message()));
    }
    keys.emplace_back(object_metadata->name());
  }
  return keys;
}

absl::Status GcsStorageProvider::Upload(const TransferMeta &m) {
  std::ifstream source(m.local_file_path, std::ios::binary);
  if (!source.is_open()) {
    return absl::NotFoundError(absl::StrFormat("Cannot open file: %s", m.local_file_path));
  }
  auto writer = client_->WriteObject(m.bucket, m.remote_file_path);
  writer << source.rdbuf();
  source.close();
  writer.Close();
  return absl::OkStatus();
}

absl::Status GcsStorageProvider::DownloadFile(const TransferMeta &m) {
  OkOrRet(PreDownloadFile(m));
  auto rfp = TryRemoveCloudStoragePrefix(ServiceProvider::GCS, m.bucket, m.remote_file_path);
  auto writer = client_->ReadObject(m.bucket, rfp);
  ExpectOrInternal(
      writer, FMT("Failed to read GCS object. [bucket={}, path={}, fixed_path={}]", m.bucket, m.remote_file_path, rfp));
  std::ofstream out(m.local_file_path, std::ios::binary);
  ExpectOrInternal(out, FMT("Failed to open local file. [path={}]", m.local_file_path));
  out << writer.rdbuf();
  ExpectOrInternal(out, FMT("Failed to write to local file. [path={}]", m.local_file_path));
  return absl::OkStatus();
}

absl::StatusOr<FilePathList> GcsStorageProvider::Download(const TransferMeta &m) {
  ExpectOrInternal(client_, "client not inited");
  ExpectOrInternal(fs::is_directory(m.local_file_path), "local file path must be directory");

  std::filesystem::path base = std::filesystem::path(m.local_file_path);
  std::vector<std::filesystem::path> result;
  // list files
  auto rfp = TryRemoveCloudStoragePrefix(ServiceProvider::GCS, m.bucket, m.remote_file_path);
  google::cloud::storage::ListObjectsReader reader =
      client_->ListObjects(m.bucket, google::cloud::storage::Prefix(rfp));

  for (auto const &object : reader) {
    if (!object) {
      spdlog::error("Failed to list GCS objects. [err={}]", object.status().message());
      continue;
    }

    auto key = object->name();
    if (key.back() == '/') continue;

    auto cur = GetObjLocalFilePath(m.remote_file_path, m.local_file_path, key);
    auto dm = TransferMeta{.bucket = m.bucket, .remote_file_path = key, .local_file_path = cur};
    auto s = DownloadFile(dm);
    if (!s.ok()) {
      spdlog::error("Download gcs object failed. [info={}, error={}]", dm.ToString(), s.ToString());
    } else {
      spdlog::info("Download gcs object success. [info={}]", dm.ToString());
    }
    result.emplace_back(cur);
  }
  return result;
}
}  // namespace cppcommon::os
