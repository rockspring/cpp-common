/**
 * @file storage_tool.cc
 * @brief CLI tool for listing and downloading files from cloud storage (OSS/S3/GCS)
 *
 * Usage:
 *   storage_tool <command> --provider <oss|s3|gcs> --bucket <bucket> --path <path> [options]
 *
 * Commands:
 *   ls            List objects under the given path
 *   download      Download all objects under --path to --local directory
 *   download-file Download a single remote file to --local file path
 *
 * Options:
 *   --provider   oss | s3 | gcs  (required)
 *   --bucket     bucket name     (required)
 *   --path       remote path     (required)
 *   --local      local path      (required for download/download-file)
 *   --no-overwrite               skip existing local files
 *   --help
 *
 * Credentials are read from environment variables:
 *   OSS: OSS_ACCESS_KEY_ID, OSS_ACCESS_KEY_SECRET, OSS_ENDPOINT, OSS_REGION
 *   S3 : AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, AWS_REGION, AWS_ENDPOINT
 *   GCS: GOOGLE_APPLICATION_CREDENTIALS (ADC) or GOOGLE_SERVICE_ACCOUNT_JSON
 */

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#include "cppcommon/objectstorage/transfor/object_transfor.h"
#include "cppcommon/objectstorage/transfor/storage_provider.h"

using namespace cppcommon::os;

// ---------------------------------------------------------------------------
// Tiny argument parser
// ---------------------------------------------------------------------------
struct Args {
  std::string command;   // "ls" | "download" | "download-file"
  std::string provider;  // "oss" | "s3" | "gcs"
  std::string bucket;
  std::string path;
  std::string local{"."};
  bool overwrite{true};
  bool help{false};
};

static void PrintUsage(const char *prog) {
  std::cout << "Usage: " << prog
            << " <ls|download|download-file> --provider <oss|s3|gcs> --bucket <bucket>"
               " --path <path> [--local <path>] [--no-overwrite]\n"
            << "\n"
            << "Commands:\n"
            << "  ls             List objects under --path\n"
            << "  download       Download all objects under --path to --local directory\n"
            << "  download-file  Download a single remote file to --local file path\n"
            << "\n"
            << "Options:\n"
            << "  --provider   oss | s3 | gcs  (required)\n"
            << "  --bucket     bucket name     (required)\n"
            << "  --path       remote path     (required)\n"
            << "  --local      local path      (default: \".\" for download, required for download-file)\n"
            << "  --no-overwrite               skip existing local files\n"
            << "  --help\n"
            << "\n"
            << "Credentials via env vars:\n"
            << "  OSS: OSS_ACCESS_KEY_ID, OSS_ACCESS_KEY_SECRET, OSS_ENDPOINT, OSS_REGION\n"
            << "  S3 : AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, AWS_REGION, AWS_ENDPOINT\n"
            << "  GCS: GOOGLE_APPLICATION_CREDENTIALS (ADC)\n";
}

static Args ParseArgs(int argc, char **argv) {
  Args a;
  if (argc < 2) {
    a.help = true;
    return a;
  }

  // First positional argument is the command
  std::string first(argv[1]);
  if (first == "--help" || first == "-h") {
    a.help = true;
    return a;
  }
  a.command = first;

  for (int i = 2; i < argc; ++i) {
    std::string key(argv[i]);
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) throw std::invalid_argument("missing value for " + key);
      return std::string(argv[++i]);
    };

    if (key == "--provider") {
      a.provider = next();
    } else if (key == "--bucket") {
      a.bucket = next();
    } else if (key == "--path") {
      a.path = next();
    } else if (key == "--local") {
      a.local = next();
    } else if (key == "--no-overwrite") {
      a.overwrite = false;
    } else if (key == "--help" || key == "-h") {
      a.help = true;
    } else {
      throw std::invalid_argument("unknown option: " + key);
    }
  }
  return a;
}

// ---------------------------------------------------------------------------
// Provider helpers
// ---------------------------------------------------------------------------
static ServiceProvider ResolveProvider(const std::string &name) {
  if (name == "oss") return ServiceProvider::OSS;
  if (name == "s3") return ServiceProvider::S3;
  if (name == "gcs") return ServiceProvider::GCS;
  throw std::invalid_argument("unknown provider '" + name + "', must be oss|s3|gcs");
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------
static int CmdLs(std::shared_ptr<StorageProvider> provider, const Args &a) {
  auto result = provider->List(a.bucket, a.path);
  if (!result.ok()) {
    std::cerr << "error: " << result.status().message() << "\n";
    return 1;
  }
  const auto &files = *result;
  if (files.empty()) {
    std::cout << "(no objects found)\n";
    return 0;
  }
  for (const auto &f : files) {
    std::cout << f << "\n";
  }
  std::cout << "\n" << files.size() << " object(s)\n";
  return 0;
}

static int CmdDownloadFile(std::shared_ptr<StorageProvider> provider, const Args &a) {
  if (a.local.empty() || a.local == ".") {
    std::cerr << "error: --local must specify a destination file path for download-file\n";
    return 1;
  }
  TransferMeta meta;
  meta.bucket = a.bucket;
  meta.remote_file_path = a.path;
  meta.local_file_path = a.local;
  meta.overwrite = a.overwrite;

  std::cout << "downloading " << a.bucket << "/" << a.path << " -> " << a.local << " ...\n";

  auto status = provider->DownloadFile(meta);
  if (!status.ok()) {
    std::cerr << "error: " << status.message() << "\n";
    return 1;
  }
  std::cout << "done\n";
  return 0;
}

static int CmdDownload(std::shared_ptr<StorageProvider> provider, const Args &a) {
  TransferMeta meta;
  meta.bucket = a.bucket;
  meta.remote_file_path = a.path;
  meta.local_file_path = a.local;
  meta.overwrite = a.overwrite;

  std::cout << "downloading " << a.bucket << "/" << a.path << " -> " << a.local << " ...\n";

  auto result = provider->Download(meta);
  if (!result.ok()) {
    std::cerr << "error: " << result.status().message() << "\n";
    return 1;
  }
  const auto &downloaded = *result;
  for (const auto &fp : downloaded) {
    std::cout << "  " << fp.string() << "\n";
  }
  std::cout << "\n" << downloaded.size() << " file(s) downloaded\n";
  return 0;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
  Args a;
  try {
    a = ParseArgs(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    PrintUsage(argv[0]);
    return 1;
  }

  if (a.help) {
    PrintUsage(argv[0]);
    return 0;
  }

  // Validate
  if (a.command != "ls" && a.command != "download" && a.command != "download-file") {
    std::cerr << "error: unknown command '" << a.command << "', must be ls|download|download-file\n";
    PrintUsage(argv[0]);
    return 1;
  }
  if (a.provider.empty()) {
    std::cerr << "error: --provider is required\n";
    return 1;
  }
  if (a.bucket.empty()) {
    std::cerr << "error: --bucket is required\n";
    return 1;
  }
  if (a.path.empty()) {
    std::cerr << "error: --path is required\n";
    return 1;
  }

  ServiceProvider sp;
  try {
    sp = ResolveProvider(a.provider);
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }

  std::shared_ptr<StorageProvider> storage;
  try {
    storage = NewObjectTransfor(sp);
  } catch (const std::exception &e) {
    std::cerr << "error: failed to create provider: " << e.what() << "\n";
    return 1;
  }

  if (a.command == "ls") return CmdLs(storage, a);
  if (a.command == "download") return CmdDownload(storage, a);
  if (a.command == "download-file") return CmdDownloadFile(storage, a);

  return 0;
}
