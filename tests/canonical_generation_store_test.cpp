// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/libpkgstate.h>

#include <array>
#include <filesystem>
#include <iomanip>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/evp.h>

#include "temp_directory.h"
#include "test.h"

namespace {

template<typename Identity>
Identity
identity(char digit)
{
  return Identity::parse("v1:sha256:" + std::string(64, digit));
}

pkgstate::state_target_binding
binding(char first)
{
  return pkgstate::state_target_binding::make(
      identity<pkgstate::managed_target_identity>(first),
      identity<pkgstate::state_store_identity>(
          static_cast<char>(first + 1)),
      identity<pkgstate::root_view_identity>(
          static_cast<char>(first + 2)),
      identity<pkgstate::state_backend_identity>(
          static_cast<char>(first + 3)),
      identity<pkgstate::publication_domain_identity>(
          static_cast<char>(first + 4)));
}

pkgstate::owned_entry
entry(const char* path, pkgstate::owned_entry_type type)
{
  return {pkgstate::package_path::parse(path), type};
}

pkgstate::installed_package
rich_package(const pkgstate::state_target_binding& target)
{
  const pkgstate::package_release release =
      pkgstate::package_release::make("base", "1.0", "2");

  pkgstate::installed_control_completeness completeness;
  completeness.runtime_dependencies =
      pkgstate::installed_control_fact_state::recorded_at_installation;
  completeness.removal_lifecycle =
      pkgstate::installed_control_fact_state::recorded_at_installation;
  completeness.target_profile =
      pkgstate::installed_control_fact_state::supplied_by_migration;
  completeness.provenance =
      pkgstate::installed_control_fact_state::recorded_at_installation;

  const std::string pre_remove("echo pre\0remove\n", 16);
  const std::string post_remove("echo post\n", 10);
  pkgstate::installed_control control = pkgstate::installed_control::make(
      release,
      completeness,
      {
          pkgstate::runtime_dependency_declaration::make("openssl>=3.5"),
          pkgstate::runtime_dependency_declaration::make("zlib"),
      },
      {
          pkgstate::removal_lifecycle_declaration::make(
              pkgstate::removal_lifecycle_phase::post_remove,
              "application/x-posix-shell",
              post_remove),
          pkgstate::removal_lifecycle_declaration::make(
              pkgstate::removal_lifecycle_phase::pre_remove,
              "application/x-posix-shell",
              pre_remove),
      },
      {
          pkgstate::target_profile_fact::make("arch", "x86_64"),
          pkgstate::target_profile_fact::make("libc", "glibc"),
      },
      {
          pkgstate::control_provenance::make(
              pkgstate::control_provenance_kind::application_evidence,
              identity<pkgstate::application_evidence_identity>('a').string()),
          pkgstate::control_provenance::make(
              pkgstate::control_provenance_kind::candidate_control,
              identity<pkgstate::operation_plan_identity>('b').string()),
      });

  return pkgstate::installed_package::make(
      release,
      std::move(control),
      target,
      {
          entry("usr/lib/libbase.so",
                pkgstate::owned_entry_type::non_directory),
          entry("usr", pkgstate::owned_entry_type::directory),
          entry("usr/bin/base",
                pkgstate::owned_entry_type::non_directory),
      });
}

pkgstate::state_publication_request
install_request(const pkgstate::snapshot& expected,
                const pkgstate::installed_package& proposed)
{
  return pkgstate::state_publication_request::make(
      expected,
      {pkgstate::package_state_delta::install(
          proposed,
          identity<pkgstate::operation_plan_identity>('c'),
          identity<pkgstate::application_evidence_identity>('a'))});
}

[[nodiscard]] std::string
generation_name(const pkgstate::installed_state_snapshot_identity& identity)
{
  const std::string value = identity.string();
  return "v1-sha256-" + value.substr(std::string("v1:sha256:").size());
}

[[nodiscard]] mode_t
mode_of(const std::filesystem::path& path)
{
  struct stat status {};
  CHECK(::stat(path.c_str(), &status) == 0);
  return status.st_mode & 07777;
}

[[nodiscard]] std::size_t
generation_count(const std::filesystem::path& root)
{
  std::size_t result = 0;
  for (const auto& ignored :
       std::filesystem::directory_iterator(root / "generations"))
  {
    static_cast<void>(ignored);
    ++result;
  }
  return result;
}

void
write_binary(const std::filesystem::path& path, const std::string& bytes)
{
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  output.close();
  CHECK(static_cast<bool>(output));
}

[[nodiscard]] std::string
sha256_hex(const std::string& bytes)
{
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  unsigned int size = 0;
  CHECK(EVP_Digest(bytes.data(),
                   bytes.size(),
                   digest.data(),
                   &size,
                   EVP_sha256(),
                   nullptr) == 1);
  CHECK(size == pkgstate::sha256_digest_size);

  std::ostringstream output;
  output << std::hex << std::setfill('0');
  for (unsigned int index = 0; index < size; ++index)
    output << std::setw(2) << static_cast<unsigned int>(digest[index]);
  return output.str();
}

} // namespace

int
main()
{
  static_assert(pkgstate::canonical_generation_storage_version == 1);
  CHECK(pkgstate::canonical_generation_storage_format ==
        "libpkgstate-generation-v1");

  temp_directory temporary;
  const std::filesystem::path root = temporary.path() / "state";
  const pkgstate::state_target_binding target = binding('1');

  pkgstate::canonical_generation_store store(root, target);
  CHECK(store.root_path() == root);
  CHECK(store.target_binding() == target);
  CHECK(std::filesystem::is_directory(root / "generations"));
  CHECK(std::filesystem::is_regular_file(root / "binding"));
  CHECK(mode_of(root / "binding") == 0444);
  CHECK(std::filesystem::is_regular_file(root / "current"));
  CHECK(mode_of(root / "current") == 0444);

  const pkgstate::snapshot initial = pkgstate::snapshot::make(target, {});
  CHECK(store.read().identity() == initial.identity());

  pkgstate::canonical_generation_store existing =
      pkgstate::canonical_generation_store::open_existing(root, target);
  CHECK(existing.read().identity() == initial.identity());
  CHECK(existing.target_binding() == target);
  CHECK(read_text(root / "current") == initial.identity().string() + "\n");
  CHECK(generation_count(root) == 1);

  const std::filesystem::path initial_generation =
      root / "generations" / generation_name(initial.identity());
  const std::string initial_snapshot =
      read_text(initial_generation / "snapshot");
  CHECK(initial_snapshot.size() == 440);
  CHECK(sha256_hex(initial_snapshot) ==
        "334214bb9d2b6147fd85c9a8758fd428bc6b25641faf631f78a81cd8bdc8d24a");

  std::filesystem::create_directory(root / "generations" / "orphan.tmp");
  CHECK(store.read().identity() == initial.identity());
  CHECK(generation_count(root) == 2);

  const int root_fd = ::open(root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  CHECK(root_fd != -1);
  CHECK(::flock(root_fd, LOCK_SH | LOCK_NB) == 0);
  CHECK(store.read().size() == 0);
  CHECK(::flock(root_fd, LOCK_UN) == 0);
  CHECK(::flock(root_fd, LOCK_EX | LOCK_NB) == 0);
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(store.read());
  });
  CHECK(::flock(root_fd, LOCK_UN) == 0);
  CHECK(::close(root_fd) == 0);

  const pkgstate::snapshot empty = store.read();
  CHECK(empty.identity() == initial.identity());
  const pkgstate::installed_package base = rich_package(target);
  const pkgstate::state_publication_request request =
      install_request(empty, base);
  const pkgstate::state_publication_receipt receipt =
      store.compare_and_publish(request);

  CHECK(receipt.outcome() ==
        pkgstate::state_publication_outcome::published);
  CHECK(receipt.durability() ==
        pkgstate::state_publication_durability::confirmed);
  CHECK(receipt.atomicity_boundary() ==
        pkgstate::state_storage_atomicity_boundary::
            immutable_generation_selection);
  CHECK(receipt.storage_format() ==
        pkgstate::canonical_generation_storage_format);
  CHECK(receipt.subordinate_evidence().size() == 1);
  CHECK(receipt.resulting_snapshot().has_value());

  const pkgstate::snapshot installed = store.read();
  CHECK(*receipt.resulting_snapshot() == installed.identity());
  CHECK(installed.size() == 1);
  CHECK(installed.find_package("base") != nullptr);
  CHECK(installed.find_package("base")->identity() == base.identity());
  CHECK(installed.find_package("base")->control().removal_lifecycle()[0]
            .material() == std::string("echo pre\0remove\n", 16));

  const std::string selected_text = installed.identity().string() + "\n";
  CHECK(read_text(root / "current") == selected_text);
  CHECK(mode_of(root / "current") == 0444);

  const std::filesystem::path generation =
      root / "generations" / generation_name(installed.identity());
  const std::filesystem::path snapshot_file = generation / "snapshot";
  CHECK(std::filesystem::is_directory(generation));
  CHECK(std::filesystem::is_regular_file(snapshot_file));
  CHECK(mode_of(generation) == 0555);
  CHECK(mode_of(snapshot_file) == 0444);
  CHECK(generation_count(root) == 3);

  pkgstate::canonical_generation_store reopened(root, target);
  CHECK(reopened.read().identity() == installed.identity());
  CHECK(reopened.read().find_package("base")->control() == base.control());

  const std::string current_before_stale = read_text(root / "current");
  const std::size_t generations_before_stale = generation_count(root);
  const pkgstate::state_publication_receipt stale =
      reopened.compare_and_publish(request);
  CHECK(stale.outcome() ==
        pkgstate::state_publication_outcome::stale_expected_state);
  CHECK(read_text(root / "current") == current_before_stale);
  CHECK(generation_count(root) == generations_before_stale);

  const pkgstate::state_publication_request remove =
      pkgstate::state_publication_request::make(
          installed,
          {pkgstate::package_state_delta::remove(
              "base",
              base.identity(),
              identity<pkgstate::operation_plan_identity>('d'),
              identity<pkgstate::application_evidence_identity>('e'))});
  const pkgstate::state_publication_receipt removed =
      reopened.compare_and_publish(remove);
  CHECK(removed.outcome() ==
        pkgstate::state_publication_outcome::published);
  CHECK(reopened.read().size() == 0);
  const std::size_t generations_after_remove = generation_count(root);

  const pkgstate::state_publication_receipt reinstalled =
      reopened.compare_and_publish(request);
  CHECK(reinstalled.outcome() ==
        pkgstate::state_publication_outcome::published);
  CHECK(reinstalled.resulting_snapshot() == receipt.resulting_snapshot());
  CHECK(generation_count(root) == generations_after_remove);
  CHECK(reopened.read().identity() == installed.identity());

  check_throws<pkgstate::store_error>([&] {
    pkgstate::canonical_generation_store wrong(root, binding('a'));
    static_cast<void>(wrong);
  });

  const std::string original_selector = read_text(root / "current");
  CHECK(::chmod((root / "current").c_str(), 0644) == 0);
  write_text(root / "current", "not-an-identity\n");
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(reopened.read());
  });
  write_text(root / "current", original_selector);
  CHECK(::chmod((root / "current").c_str(), 0444) == 0);

  const std::string binding_bytes = read_text(root / "binding");
  const std::string original_snapshot = read_text(snapshot_file);
  CHECK(binding_bytes.size() == 429);
  CHECK(sha256_hex(binding_bytes) ==
        "ee552fc83fe5108b98e3746e75ec8b6a0259a2e50d5c8d5332ee5286a5d55b23");
  CHECK(original_snapshot.size() == 935);
  CHECK(sha256_hex(original_snapshot) ==
        "0773f3f20f27e55b4f2f057087ee4927a9256a842d5cc6954df701ca46df8f44");
  CHECK(::chmod(snapshot_file.c_str(), 0644) == 0);
  write_binary(snapshot_file, original_snapshot + "x");
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(reopened.read());
  });
  write_binary(snapshot_file, original_snapshot);
  CHECK(::chmod(snapshot_file.c_str(), 0444) == 0);
  CHECK(reopened.read().identity() == installed.identity());

  const std::filesystem::path linked_snapshot = root / "snapshot-link";
  std::filesystem::create_hard_link(snapshot_file, linked_snapshot);
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(reopened.read());
  });
  std::filesystem::remove(linked_snapshot);
  CHECK(reopened.read().identity() == installed.identity());

  CHECK(::chmod(generation.c_str(), 0755) == 0);
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(reopened.read());
  });
  CHECK(::chmod(generation.c_str(), 0555) == 0);
  CHECK(reopened.read().identity() == installed.identity());

  const std::string missing_identity =
      "v1:sha256:" + std::string(64, 'f') + "\n";
  CHECK(::chmod((root / "current").c_str(), 0644) == 0);
  write_text(root / "current", missing_identity);
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(reopened.read());
  });
  write_text(root / "current", original_selector);
  CHECK(::chmod((root / "current").c_str(), 0444) == 0);
  CHECK(reopened.read().identity() == installed.identity());

  {
    temp_directory recovery_temporary;
    const std::filesystem::path recovery_root =
        recovery_temporary.path() / "state";
    pkgstate::canonical_generation_store initialized(recovery_root, target);
    const pkgstate::snapshot recovery_empty = initialized.read();
    CHECK(recovery_empty.size() == 0);
    std::filesystem::remove(recovery_root / "binding");

    pkgstate::canonical_generation_store recovered(recovery_root, target);
    CHECK(recovered.read().identity() == recovery_empty.identity());
    CHECK(std::filesystem::is_regular_file(recovery_root / "binding"));
  }

  {
    temp_directory missing_selector_temporary;
    const std::filesystem::path missing_selector_root =
        missing_selector_temporary.path() / "state";
    pkgstate::canonical_generation_store initialized(
        missing_selector_root, target);
    std::filesystem::remove(missing_selector_root / "current");

    check_throws<pkgstate::store_error>([&] {
      pkgstate::canonical_generation_store corrupted(
          missing_selector_root, target);
      static_cast<void>(corrupted);
    });
    CHECK(!std::filesystem::exists(missing_selector_root / "current"));
  }

  {
    temp_directory absent_temporary;
    const std::filesystem::path absent_root =
        absent_temporary.path() / "absent-state";
    check_throws<pkgstate::store_error>([&] {
      static_cast<void>(
          pkgstate::canonical_generation_store::open_existing(
              absent_root, target));
    });
    CHECK(!std::filesystem::exists(absent_root));
  }

  {
    temp_directory incomplete_temporary;
    const std::filesystem::path incomplete_root =
        incomplete_temporary.path() / "state";
    std::filesystem::create_directory(incomplete_root);
    write_text(incomplete_root / "marker", "unchanged\n");
    check_throws<pkgstate::store_error>([&] {
      static_cast<void>(
          pkgstate::canonical_generation_store::open_existing(
              incomplete_root, target));
    });
    CHECK(read_text(incomplete_root / "marker") == "unchanged\n");
    CHECK(!std::filesystem::exists(incomplete_root / "binding"));
    CHECK(!std::filesystem::exists(incomplete_root / "current"));
    CHECK(!std::filesystem::exists(incomplete_root / "generations"));
  }

  {
    temp_directory unbound_nonempty_temporary;
    const std::filesystem::path unbound_root =
        unbound_nonempty_temporary.path() / "state";
    pkgstate::canonical_generation_store initialized(unbound_root, target);
    const pkgstate::installed_package proposed = rich_package(target);
    const pkgstate::state_publication_receipt published =
        initialized.compare_and_publish(
            install_request(initialized.read(), proposed));
    CHECK(published.outcome() ==
          pkgstate::state_publication_outcome::published);
    std::filesystem::remove(unbound_root / "binding");

    check_throws<pkgstate::store_error>([&] {
      pkgstate::canonical_generation_store refused(unbound_root, target);
      static_cast<void>(refused);
    });
    CHECK(!std::filesystem::exists(unbound_root / "binding"));
  }

  return 0;
}
