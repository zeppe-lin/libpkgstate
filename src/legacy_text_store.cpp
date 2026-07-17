/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <libpkgstate/legacy_text_store.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libpkgimage/error.h>
#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

std::string
system_message(const std::string& action,
               const std::filesystem::path& path,
               int error_number)
{
  return action + " " + path.string() + ": " +
         std::string(std::strerror(error_number));
}

std::filesystem::path
parent_directory(const std::filesystem::path& database)
{
  const std::filesystem::path parent = database.parent_path();
  return parent.empty() ? std::filesystem::path(".") : parent;
}

class directory_lock final {
public:
  directory_lock(const std::filesystem::path& directory, int operation)
      : directory_(directory)
  {
    fd_ = ::open(directory_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd_ == -1)
      throw store_error(system_message("could not open directory", directory_, errno));

    if (::flock(fd_, operation | LOCK_NB) == -1)
    {
      const int saved = errno;
      ::close(fd_);
      fd_ = -1;

      if (saved == EWOULDBLOCK || saved == EAGAIN)
        throw store_error("package state is currently locked by another process");

      throw store_error(system_message("could not lock directory", directory_, saved));
    }
  }

  directory_lock(const directory_lock&) = delete;
  directory_lock& operator=(const directory_lock&) = delete;

  ~directory_lock()
  {
    if (fd_ != -1)
    {
      static_cast<void>(::flock(fd_, LOCK_UN));
      static_cast<void>(::close(fd_));
    }
  }

  [[nodiscard]] int descriptor() const noexcept
  {
    return fd_;
  }

private:
  std::filesystem::path directory_;
  int fd_ = -1;
};

[[noreturn]] void
parse_failure(const std::filesystem::path& database,
              std::size_t line,
              const std::string& message)
{
  throw store_error(database.string() + ":" + std::to_string(line) +
                    ": " + message);
}

snapshot
read_database(const std::filesystem::path& database)
{
  std::ifstream input(database, std::ios::binary);
  if (!input)
    throw store_error(system_message("could not open", database, errno));

  std::vector<installed_package> packages;
  std::string line;
  std::size_t line_number = 0;

  while (std::getline(input, line))
  {
    ++line_number;
    if (line.empty())
      continue;

    const std::size_t name_line = line_number;
    const std::string name = line;

    std::string version;
    if (!std::getline(input, version))
      parse_failure(database, name_line, "package record has no version line");
    ++line_number;

    std::vector<owned_entry> manifest;
    while (std::getline(input, line))
    {
      ++line_number;
      if (line.empty())
        break;

      try
      {
        const bool directory = line.back() == '/';
        manifest.push_back(owned_entry{
            pkgimage::package_path::parse(line),
            directory ? owned_entry_type::directory
                      : owned_entry_type::non_directory,
        });
      }
      catch (const pkgimage::path_error& failure)
      {
        parse_failure(database, line_number,
                      std::string("invalid owned path: ") + failure.what());
      }
    }

    try
    {
      packages.emplace_back(package_identity::make(name, version),
                            std::move(manifest));
    }
    catch (const identity_error& failure)
    {
      parse_failure(database, name_line,
                    std::string("invalid package identity: ") + failure.what());
    }
    catch (const state_error& failure)
    {
      parse_failure(database, name_line,
                    std::string("invalid installed manifest: ") + failure.what());
    }
  }

  if (!input.eof())
    throw store_error(system_message("could not read", database, errno));

  try
  {
    return snapshot(std::move(packages));
  }
  catch (const state_error& failure)
  {
    throw store_error(database.string() + ": invalid package state: " +
                      failure.what());
  }
}

std::string
serialize(const snapshot& state)
{
  std::ostringstream output;
  for (const installed_package& package : state.packages())
  {
    output << package.identity().name() << '\n';
    output << package.identity().version() << '\n';
    for (const owned_entry& entry : package.manifest())
    {
      output << entry.path.string();
      if (entry.type == owned_entry_type::directory)
        output << '/';
      output << '\n';
    }
    output << '\n';
  }
  return output.str();
}

void
write_all(int fd, const std::string& data, const std::filesystem::path& path)
{
  std::size_t written = 0;
  while (written < data.size())
  {
    const ssize_t result =
        ::write(fd, data.data() + written, data.size() - written);
    if (result == -1)
    {
      if (errno == EINTR)
        continue;
      throw store_error(system_message("could not write", path, errno));
    }
    if (result == 0)
      throw store_error("could not write " + path.string() +
                        ": write made no progress");
    written += static_cast<std::size_t>(result);
  }
}

class temporary_file final {
public:
  explicit temporary_file(const std::filesystem::path& database)
  {
    std::string pattern = database.string() + ".incomplete.XXXXXX";
    std::vector<char> mutable_pattern(pattern.begin(), pattern.end());
    mutable_pattern.push_back('\0');

    fd_ = ::mkstemp(mutable_pattern.data());
    if (fd_ == -1)
      throw store_error(system_message("could not create temporary state", database,
                                       errno));

    path_ = mutable_pattern.data();

    const int flags = ::fcntl(fd_, F_GETFD);
    if (flags == -1 || ::fcntl(fd_, F_SETFD, flags | FD_CLOEXEC) == -1)
    {
      const int saved = errno;
      cleanup();
      throw store_error(system_message("could not secure temporary state", path_,
                                       saved));
    }
  }

  temporary_file(const temporary_file&) = delete;
  temporary_file& operator=(const temporary_file&) = delete;

  ~temporary_file()
  {
    cleanup();
  }

  [[nodiscard]] int descriptor() const noexcept
  {
    return fd_;
  }

  [[nodiscard]] const std::filesystem::path& path() const noexcept
  {
    return path_;
  }

  void close_file()
  {
    if (fd_ == -1)
      return;

    if (::close(fd_) == -1)
    {
      const int saved = errno;
      fd_ = -1;
      throw store_error(system_message("could not close", path_, saved));
    }
    fd_ = -1;
  }

  void release_path() noexcept
  {
    path_.clear();
  }

private:
  void cleanup() noexcept
  {
    if (fd_ != -1)
    {
      static_cast<void>(::close(fd_));
      fd_ = -1;
    }
    if (!path_.empty())
      static_cast<void>(::unlink(path_.c_str()));
  }

  std::filesystem::path path_;
  int fd_ = -1;
};

void
publish_database(const std::filesystem::path& database,
                 const snapshot& state,
                 int directory_fd)
{
  temporary_file temporary(database);
  const std::string encoded = serialize(state);

  if (::fchmod(temporary.descriptor(), 0444) == -1)
    throw store_error(system_message("could not set permissions on",
                                     temporary.path(), errno));

  write_all(temporary.descriptor(), encoded, temporary.path());

  if (::fsync(temporary.descriptor()) == -1)
    throw store_error(system_message("could not synchronize", temporary.path(),
                                     errno));

  temporary.close_file();

  std::filesystem::path backup = database;
  backup += ".backup";

  if (::unlink(backup.c_str()) == -1 && errno != ENOENT)
    throw store_error(system_message("could not remove", backup, errno));

  if (::link(database.c_str(), backup.c_str()) == -1 && errno != ENOENT)
    throw store_error(system_message("could not create", backup, errno));

  if (::rename(temporary.path().c_str(), database.c_str()) == -1)
    throw store_error(system_message("could not publish", database, errno));

  temporary.release_path();

  if (::fsync(directory_fd) == -1)
    throw store_error(system_message("state published but directory sync failed",
                                     parent_directory(database), errno));
}

void
validate_package_name(std::string_view name)
{
  static_cast<void>(package_identity::make(name, "validation"));
}

class legacy_write_transaction final : public write_transaction {
public:
  legacy_write_transaction(std::filesystem::path database,
                           std::unique_ptr<directory_lock> lock,
                           snapshot initial)
      : database_(std::move(database)), lock_(std::move(lock))
  {
    for (const installed_package& package : initial.packages())
      packages_.emplace(package.identity().name(), package);
  }

  [[nodiscard]] snapshot current() const override
  {
    std::vector<installed_package> packages;
    packages.reserve(packages_.size());
    for (const auto& item : packages_)
      packages.push_back(item.second);
    return snapshot(std::move(packages));
  }

  void put(installed_package package) override
  {
    ensure_open();
    packages_.insert_or_assign(package.identity().name(), std::move(package));
  }

  bool erase(std::string_view package_name) override
  {
    ensure_open();
    validate_package_name(package_name);
    return packages_.erase(std::string(package_name)) != 0;
  }

  void commit() override
  {
    ensure_open();
    publish_database(database_, current(), lock_->descriptor());
    committed_ = true;
    lock_.reset();
  }

  [[nodiscard]] bool committed() const noexcept override
  {
    return committed_;
  }

private:
  void ensure_open() const
  {
    if (committed_)
      throw transaction_error("package-state transaction is already committed");
  }

  std::filesystem::path database_;
  std::unique_ptr<directory_lock> lock_;
  std::map<std::string, installed_package> packages_;
  bool committed_ = false;
};

} // namespace

legacy_text_store::legacy_text_store(std::filesystem::path database)
    : database_(std::move(database))
{
  if (database_.empty())
    throw store_error("package-state database path must not be empty");
}

const std::filesystem::path&
legacy_text_store::database_path() const noexcept
{
  return database_;
}

snapshot
legacy_text_store::read() const
{
  directory_lock lock(parent_directory(database_), LOCK_SH);
  return read_database(database_);
}

std::unique_ptr<write_transaction>
legacy_text_store::begin_write() const
{
  auto lock =
      std::make_unique<directory_lock>(parent_directory(database_), LOCK_EX);
  snapshot initial = read_database(database_);
  return std::make_unique<legacy_write_transaction>(
      database_, std::move(lock), std::move(initial));
}

} // namespace pkgstate
