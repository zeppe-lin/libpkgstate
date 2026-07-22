// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/canonical_store.h>
#include <libpkgstate/legacy_import.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "generation_codec.h"
#include "test.h"

namespace {

template<typename Identity>
Identity id(unsigned char byte)
{
  pkgstate::sha256_digest_bytes bytes{};
  bytes.fill(byte);
  return Identity::from_sha256(bytes);
}

pkgstate::owned_entry file(const char* path)
{
  return {pkgstate::package_path::parse(path),
          pkgstate::owned_entry_type::non_directory};
}

std::vector<pkgstate::legacy_import_provenance> provenance()
{
  return {
      pkgstate::legacy_import_provenance::historically_unavailable(
          pkgstate::control_provenance_kind::candidate_control),
      pkgstate::legacy_import_provenance::historically_unavailable(
          pkgstate::control_provenance_kind::artifact),
      pkgstate::legacy_import_provenance::historically_unavailable(
          pkgstate::control_provenance_kind::artifact_manifest),
      pkgstate::legacy_import_provenance::supplied(
          pkgstate::control_provenance_kind::application_evidence,
          id<pkgstate::application_evidence_identity>(0x31).string()),
      pkgstate::legacy_import_provenance::historically_unavailable(
          pkgstate::control_provenance_kind::transaction_evidence),
  };
}

pkgstate::state_target_binding destination_binding()
{
  return pkgstate::state_target_binding::make(
      id<pkgstate::managed_target_identity>(0x01),
      id<pkgstate::state_store_identity>(0x02),
      id<pkgstate::root_view_identity>(0x03),
      id<pkgstate::state_backend_identity>(0x04),
      id<pkgstate::publication_domain_identity>(0x05));
}

pkgstate::legacy_import_source source_binding()
{
  return pkgstate::legacy_import_source::make(
      id<pkgstate::managed_target_identity>(0x01),
      id<pkgstate::state_store_identity>(0x12),
      id<pkgstate::root_view_identity>(0x03),
      id<pkgstate::state_backend_identity>(0x14));
}

pkgstate::legacy_snapshot source_state()
{
  return pkgstate::legacy_snapshot({
      pkgstate::legacy_installed_package(
          pkgstate::package_identity::make("base", "1.0-1"),
          {file("usr/bin/base")}),
  });
}

pkgstate::legacy_import_request request()
{
  const auto source = source_state();
  const auto input = pkgstate::legacy_package_import::make(
      source.packages().front(),
      pkgstate::package_release::make("base", "1.0", "1"),
      pkgstate::installed_control_fact_state::historically_unavailable, {},
      pkgstate::installed_control_fact_state::supplied_by_migration, {},
      pkgstate::installed_control_fact_state::historically_unavailable, {},
      provenance());
  return pkgstate::legacy_import_request::make(
      source_binding(), source,
      pkgstate::snapshot::make(destination_binding(), {}), {input},
      id<pkgstate::legacy_migration_evidence_identity>(0x41));
}

class transaction final : public pkgstate::canonical_publication_transaction {
public:
  transaction(pkgstate::snapshot current, bool& published)
      : current_(std::move(current)), published_(published) {}
  const pkgstate::snapshot& current() const noexcept override
  {
    return current_;
  }
  const std::string& storage_format() const noexcept override
  {
    return format_;
  }
  pkgstate::state_publication_backend_result
  publish(const pkgstate::snapshot& result) override
  {
    published_ = true;
    published_identity_ = result.identity().string();
    return pkgstate::state_publication_backend_result::published(
        pkgstate::state_storage_atomicity_boundary::
            immutable_generation_selection);
  }
  std::string published_identity_;
private:
  pkgstate::snapshot current_;
  bool& published_;
  std::string format_ = "pkgstate-generation-v1";
};

class store final : public pkgstate::canonical_store {
public:
  explicit store(pkgstate::snapshot current) : current_(std::move(current)) {}
  pkgstate::snapshot read() const override { return current_; }
  mutable bool published = false;
protected:
  std::unique_ptr<pkgstate::canonical_publication_transaction>
  begin_publication() const override
  {
    return std::make_unique<transaction>(current_, published);
  }
private:
  pkgstate::snapshot current_;
};

} // namespace

int main()
{
  const auto source = source_state();
  const auto import = request();
  CHECK(import.source_snapshot() == source.observation_identity());
  CHECK(import.resulting_snapshot().size() == 1);
  const auto* base = import.resulting_snapshot().find_package("base");
  CHECK(base != nullptr);
  CHECK(base->manifest() == source.packages().front().manifest());
  CHECK(base->control().completeness().runtime_dependencies ==
        pkgstate::installed_control_fact_state::historically_unavailable);
  CHECK(base->control().completeness().removal_lifecycle ==
        pkgstate::installed_control_fact_state::supplied_by_migration);
  CHECK(base->control().provenance().size() == 4);
  CHECK(import.packages().front().provenance().size() == 5);
  const auto encoded =
      pkgstate::detail::encode_generation_snapshot(import.resulting_snapshot());
  const auto decoded = pkgstate::detail::decode_generation_snapshot(
      std::string_view(reinterpret_cast<const char*>(encoded.data()),
                       encoded.size()));
  CHECK(decoded.identity() == import.resulting_snapshot().identity());
  CHECK(decoded.find_package("base")->control().provenance().size() == 4);
  CHECK(import.identity().string() ==
        "v1:sha256:162547fa54834edbc06958e654a9c974"
        "ad1a4f3effccca5335db751922153ecd");

  const auto validated = pkgstate::legacy_import_receipt::validated(import);
  CHECK(validated.outcome() == pkgstate::legacy_import_outcome::validated_only);
  CHECK(validated.resulting_snapshot() ==
        import.resulting_snapshot().identity());
  CHECK(!validated.storage_format());
  CHECK(validated.identity().string() ==
        "v1:sha256:d0cd07af21dfa5a0749c8e6f43db491a"
        "9bfcf97daa4095d9b295dc19777b6557");

  store good(pkgstate::snapshot::make(destination_binding(), {}));
  const auto imported = good.import_legacy(import);
  CHECK(good.published);
  CHECK(imported.outcome() == pkgstate::legacy_import_outcome::imported);
  CHECK(imported.durability() ==
        pkgstate::state_publication_durability::confirmed);
  CHECK(imported.identity().string() ==
        "v1:sha256:9c69eacf4281facf14324f3bca7402aa"
        "fab647566d109e401ebfee190690a123");

  const auto existing_control = base->control();
  const auto existing = pkgstate::installed_package::make(
      base->release(), existing_control, destination_binding(),
      base->manifest());
  store stale(pkgstate::snapshot::make(destination_binding(), {existing}));
  const auto refused = stale.import_legacy(import);
  CHECK(!stale.published);
  CHECK(refused.outcome() ==
        pkgstate::legacy_import_outcome::stale_destination);

  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::legacy_package_import::make(
        source.packages().front(),
        pkgstate::package_release::make("other", "1", "1"),
        pkgstate::installed_control_fact_state::historically_unavailable, {},
        pkgstate::installed_control_fact_state::historically_unavailable, {},
        pkgstate::installed_control_fact_state::historically_unavailable, {},
        provenance()));
  });
  check_throws<pkgstate::state_error>([&] {
    auto incomplete = provenance();
    incomplete.pop_back();
    static_cast<void>(pkgstate::legacy_package_import::make(
        source.packages().front(),
        pkgstate::package_release::make("base", "1.0", "1"),
        pkgstate::installed_control_fact_state::historically_unavailable, {},
        pkgstate::installed_control_fact_state::historically_unavailable, {},
        pkgstate::installed_control_fact_state::historically_unavailable, {},
        std::move(incomplete)));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::legacy_import_request::make(
        source_binding(), source,
        pkgstate::snapshot::make(destination_binding(), {existing}),
        import.packages(), import.migration_evidence()));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::legacy_import_request::make(
        source_binding(), source,
        pkgstate::snapshot::make(destination_binding(), {}), {},
        import.migration_evidence()));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::legacy_package_import::make(
        source.packages().front(),
        pkgstate::package_release::make("base", "1.0", "1"),
        pkgstate::installed_control_fact_state::historically_unavailable,
        {pkgstate::runtime_dependency_declaration::make("libc")},
        pkgstate::installed_control_fact_state::historically_unavailable, {},
        pkgstate::installed_control_fact_state::historically_unavailable, {},
        provenance()));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(
        pkgstate::legacy_import_provenance::historically_unavailable(
            pkgstate::control_provenance_kind::legacy_package_observation));
  });
  check_throws<pkgstate::state_error>([&] {
    const auto aliased = pkgstate::legacy_import_source::make(
        destination_binding().managed_target(),
        destination_binding().state_store(),
        destination_binding().root_view(),
        id<pkgstate::state_backend_identity>(0x24));
    static_cast<void>(pkgstate::legacy_import_request::make(
        aliased, source,
        pkgstate::snapshot::make(destination_binding(), {}),
        import.packages(), import.migration_evidence()));
  });

  const pkgstate::legacy_snapshot two_source({
      pkgstate::legacy_installed_package(
          pkgstate::package_identity::make("base", "1.0-1"),
          {file("usr/bin/base")}),
      pkgstate::legacy_installed_package(
          pkgstate::package_identity::make("tools", "2.0-3"),
          {file("usr/bin/tool")}),
  });
  const auto base_input = pkgstate::legacy_package_import::make(
      *two_source.find_package("base"),
      pkgstate::package_release::make("base", "1.0", "1"),
      pkgstate::installed_control_fact_state::historically_unavailable, {},
      pkgstate::installed_control_fact_state::historically_unavailable, {},
      pkgstate::installed_control_fact_state::historically_unavailable, {},
      provenance());
  const auto tools_input = pkgstate::legacy_package_import::make(
      *two_source.find_package("tools"),
      pkgstate::package_release::make("tools", "2.0", "3"),
      pkgstate::installed_control_fact_state::historically_unavailable, {},
      pkgstate::installed_control_fact_state::historically_unavailable, {},
      pkgstate::installed_control_fact_state::historically_unavailable, {},
      provenance());
  const auto ordered = pkgstate::legacy_import_request::make(
      source_binding(), two_source,
      pkgstate::snapshot::make(destination_binding(), {}),
      {base_input, tools_input}, import.migration_evidence());
  const auto reversed = pkgstate::legacy_import_request::make(
      source_binding(), two_source,
      pkgstate::snapshot::make(destination_binding(), {}),
      {tools_input, base_input}, import.migration_evidence());
  CHECK(ordered.identity() == reversed.identity());
  CHECK(ordered.resulting_snapshot().identity() ==
        reversed.resulting_snapshot().identity());

  return 0;
}
