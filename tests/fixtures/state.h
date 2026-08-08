// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <libpkgstate/libpkgstate.h>

namespace state_fixture {

template<typename Identity>
Identity identity(std::uint8_t seed)
{
  pkgstate::sha256_digest_bytes bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index)
    bytes[index] = static_cast<std::uint8_t>(seed + index);
  return Identity::from_sha256(bytes);
}

inline pkgstate::declaration_provenance at(
    std::string path, std::uint32_t line = 1,
    std::string document = "recipe.yml", std::uint32_t column = 3)
{
  return pkgstate::declaration_provenance(
      std::move(document), std::move(path), line, column);
}

inline pkgstate::state_target_binding target(std::uint8_t seed = 1)
{
  return pkgstate::state_target_binding::make(
      identity<pkgstate::managed_target_identity>(seed),
      identity<pkgstate::state_store_identity>(seed + 1),
      identity<pkgstate::root_view_identity>(seed + 2),
      identity<pkgstate::state_backend_identity>(seed + 3),
      identity<pkgstate::publication_domain_identity>(seed + 4));
}

inline pkgstate::package_requirement requirement(
    std::string name, std::string path = "requirements.run[0]")
{
  return pkgstate::package_requirement(
      pkgstate::package_reference(std::move(name)),
      {pkgstate::requirement_origin(at(std::move(path)), {})});
}

inline pkgstate::package_requirement expanded_requirement(
    std::string name = "libbar")
{
  const std::string package_name = name;
  std::vector<pkgstate::profile_expansion_step> expansion;
  expansion.emplace_back(
      pkgstate::profile_reference("@toolchain"),
      pkgstate::requirement_member_kind::profile, "@compiler",
      at("profiles.toolchain[0]", 4, "profiles.yml", 5));
  expansion.emplace_back(
      pkgstate::profile_reference("@compiler"),
      pkgstate::requirement_member_kind::package, package_name,
      at("profiles.compiler[0]", 8, "profiles.yml", 7));
  return pkgstate::package_requirement(
      pkgstate::package_reference(std::move(name)),
      {pkgstate::requirement_origin(
          at("requirements.run[1]", 12), std::move(expansion))});
}

inline pkgstate::package_source_record source(
    std::string name = "example", std::uint8_t seed = 20,
    std::uint32_t release_number = 1)
{
  pkgstate::package_release release(
      identity<pkgstate::package_release_identity>(seed),
      pkgstate::package_reference(std::move(name)), "1.2.3", release_number);

  std::vector<pkgstate::package_requirement> runtime;
  runtime.push_back(requirement("libfoo"));

  std::vector<pkgstate::lifecycle_program> programs;
  programs.emplace_back(
      pkgstate::lifecycle_action::post_install,
      pkgstate::program(pkgstate::program_language::posix_shell,
                        "echo installed\n"));
  programs.emplace_back(
      pkgstate::lifecycle_action::pre_remove,
      pkgstate::program(pkgstate::program_language::posix_shell,
                        "echo removing\n"));

  std::vector<pkgstate::lifecycle_requirement> lifecycle;
  lifecycle.emplace_back(pkgstate::lifecycle_action::post_install,
                         requirement("desktop-file-utils",
                                     "requirements.lifecycle.post-install[0]"));

  std::vector<pkgstate::selected_profile> profiles;
  profiles.emplace_back(
      pkgstate::profile_reference("@toolchain"),
      identity<pkgstate::source_profile_identity>(seed + 1),
      std::vector<pkgstate::declaration_provenance>{
          at("requirements.build[0]", 7)});

  return pkgstate::package_source_record::make(
      std::move(release),
      pkgstate::package_metadata(
          "Example package", std::optional<std::string>("Long description\n"),
          std::optional<std::string>("https://example.invalid"),
          {"GPL-3.0-or-later"}),
      std::move(runtime), std::move(programs), std::move(lifecycle),
      pkgstate::architecture_binding::make(
          {pkgstate::architecture_reference("x86_64")},
          {pkgstate::architecture_reference("x86_64")},
          pkgstate::architecture_reference("x86_64"),
          pkgstate::architecture_reference("x86_64")),
      std::move(profiles),
      identity<pkgstate::source_snapshot_identity>(seed + 2));
}

inline pkgstate::package_source_record rich_source(
    std::string name = "example", std::uint8_t seed = 20)
{
  pkgstate::package_release release(
      identity<pkgstate::package_release_identity>(seed),
      pkgstate::package_reference(std::move(name)), "9.8.7_rc1", 4);

  std::vector<pkgstate::package_requirement> runtime = {
      requirement("libfoo", "requirements.run[0]"),
      expanded_requirement("libbar"),
  };

  std::vector<pkgstate::lifecycle_program> programs;
  for (const auto& [action, material] : {
           std::pair{pkgstate::lifecycle_action::pre_install, "echo pre-install\n"},
           std::pair{pkgstate::lifecycle_action::post_install, "echo post-install\n"},
           std::pair{pkgstate::lifecycle_action::pre_remove, "echo pre-remove\n"},
           std::pair{pkgstate::lifecycle_action::post_remove, "echo post-remove\n"},
       })
  {
    programs.emplace_back(
        action,
        pkgstate::program(pkgstate::program_language::posix_shell, material));
  }

  std::vector<pkgstate::lifecycle_requirement> lifecycle;
  lifecycle.emplace_back(
      pkgstate::lifecycle_action::pre_install,
      requirement("shadow", "requirements.lifecycle.pre-install[0]"));
  lifecycle.emplace_back(
      pkgstate::lifecycle_action::post_install,
      requirement("desktop-file-utils",
                  "requirements.lifecycle.post-install[0]"));
  lifecycle.emplace_back(
      pkgstate::lifecycle_action::pre_remove,
      requirement("busybox", "requirements.lifecycle.pre-remove[0]"));
  lifecycle.emplace_back(
      pkgstate::lifecycle_action::post_remove,
      requirement("coreutils", "requirements.lifecycle.post-remove[0]"));

  std::vector<pkgstate::selected_profile> profiles;
  profiles.emplace_back(
      pkgstate::profile_reference("@compiler"),
      identity<pkgstate::source_profile_identity>(seed + 1),
      std::vector<pkgstate::declaration_provenance>{
          at("requirements.build[1]", 18),
          at("requirements.check[0]", 21)});
  profiles.emplace_back(
      pkgstate::profile_reference("@toolchain"),
      identity<pkgstate::source_profile_identity>(seed + 2),
      std::vector<pkgstate::declaration_provenance>{
          at("requirements.build[0]", 17)});

  return pkgstate::package_source_record::make(
      std::move(release),
      pkgstate::package_metadata(
          "Rich example", std::optional<std::string>("line one\nline two\tdata\n"),
          std::optional<std::string>("https://example.invalid/rich"),
          {"MIT", "GPL-3.0-or-later"}),
      std::move(runtime), std::move(programs), std::move(lifecycle),
      pkgstate::architecture_binding::make(
          {pkgstate::architecture_reference("aarch64"),
           pkgstate::architecture_reference("x86_64")},
          {pkgstate::architecture_reference("riscv64"),
           pkgstate::architecture_reference("x86_64")},
          pkgstate::architecture_reference("aarch64"),
          pkgstate::architecture_reference("riscv64")),
      std::move(profiles),
      identity<pkgstate::source_snapshot_identity>(seed + 3));
}

inline pkgstate::build_provenance build(
    const pkgstate::package_source_record& source_record,
    std::uint8_t seed = 20)
{
  return pkgstate::build_provenance(
      source_record.identity(),
      identity<pkgstate::build_request_identity>(seed + 4),
      identity<pkgstate::build_input_set_identity>(seed + 6),
      identity<pkgstate::environment_policy_identity>(seed + 7),
      identity<pkgstate::build_policy_identity>(seed + 8),
      identity<pkgstate::build_result_identity>(seed + 9),
      identity<pkgstate::payload_manifest_identity>(seed + 10),
      identity<pkgstate::build_artifact_identity>(seed + 11),
      identity<pkgstate::artifact_content_identity>(seed + 12),
      identity<pkgstate::artifact_binding_identity>(seed + 13),
      identity<pkgstate::execution_evidence_identity>(seed + 14),
      identity<pkgstate::build_image_identity>(seed + 15),
      identity<pkgstate::artifact_image_identity>(seed + 16),
      identity<pkgstate::artifact_inspection_identity>(seed + 17));
}

inline pkgstate::installed_control control_from_source(
    pkgstate::package_source_record source_record,
    std::uint8_t seed = 20,
    pkgstate::installation_reason reason =
        pkgstate::installation_reason::explicit_request())
{
  pkgstate::build_provenance provenance = build(source_record, seed);
  return pkgstate::installed_control::make(
      std::move(source_record), std::move(reason), std::move(provenance));
}

inline pkgstate::installed_control control(
    std::string name = "example", std::uint8_t seed = 20,
    pkgstate::installation_reason reason =
        pkgstate::installation_reason::explicit_request())
{
  return control_from_source(source(std::move(name), seed), seed,
                             std::move(reason));
}

inline pkgstate::installed_object_metadata regular_object(
    std::uint8_t seed, std::uint32_t mode = 0644,
    std::int64_t seconds = 1700000000,
    std::optional<pkgstate::package_path> hardlink_anchor = std::nullopt)
{
  return pkgstate::installed_object_metadata(
      pkgstate::owned_object_kind::regular, mode, 1000, 100,
      pkgstate::installed_object_timestamp(seconds, 123456789),
      std::uint64_t{7},
      identity<pkgstate::installed_regular_content_identity>(seed),
      std::nullopt, std::nullopt, std::move(hardlink_anchor));
}

inline pkgstate::installed_object_metadata simple_object(
    pkgstate::owned_object_kind kind, std::uint32_t mode = 0644,
    std::int64_t seconds = 1700000000)
{
  return pkgstate::installed_object_metadata(
      kind, mode, 1000, 100,
      pkgstate::installed_object_timestamp(seconds, 987654321));
}

inline pkgstate::installed_object_metadata symlink_object(
    std::string target_text = "../lib/example")
{
  return pkgstate::installed_object_metadata(
      pkgstate::owned_object_kind::symlink, 0777, 1000, 100,
      pkgstate::installed_object_timestamp(-9, 42), std::nullopt, std::nullopt,
      std::move(target_text));
}

inline pkgstate::installed_object_metadata device_object(
    pkgstate::owned_object_kind kind, std::uint64_t major,
    std::uint64_t minor)
{
  return pkgstate::installed_object_metadata(
      kind, 0600, 0, 6, pkgstate::installed_object_timestamp(11, 12),
      std::nullopt, std::nullopt, std::nullopt,
      pkgstate::installed_device_number(major, minor));
}

inline pkgstate::owned_entry entry(
    std::string path, pkgstate::installed_object_metadata object,
    pkgstate::active_object_origin origin =
        pkgstate::active_object_origin::incoming_payload,
    std::optional<pkgstate::rejected_object_reference> rejected = std::nullopt)
{
  return pkgstate::owned_entry::make(
      pkgstate::package_path::parse(std::move(path)), std::move(object), origin,
      std::move(rejected));
}

inline std::vector<pkgstate::owned_entry> manifest(std::uint8_t seed = 20)
{
  std::vector<pkgstate::owned_entry> result;
  result.push_back(entry(
      "usr/bin/example", regular_object(seed + 12, 0755, -1),
      pkgstate::active_object_origin::incoming_payload));
  result.push_back(entry(
      "etc/example.conf", regular_object(seed + 13),
      pkgstate::active_object_origin::retained_existing,
      pkgstate::rejected_object_reference(
          pkgstate::rejected_object_side::incoming,
          identity<pkgstate::rejected_object_identity>(seed + 17))));
  return result;
}

inline std::vector<pkgstate::owned_entry> rich_manifest(std::uint8_t seed = 20)
{
  std::vector<pkgstate::owned_entry> result;
  result.push_back(entry(
      "usr/bin/tool", regular_object(seed + 30, 0755, -2),
      pkgstate::active_object_origin::incoming_payload));
  result.push_back(entry(
      "usr/bin/tool-link",
      regular_object(seed + 30, 0755, -2,
                     pkgstate::package_path::parse("usr/bin/tool")),
      pkgstate::active_object_origin::incoming_payload));
  result.push_back(entry(
      "usr/share/example", simple_object(pkgstate::owned_object_kind::directory, 0755)));
  result.push_back(entry("usr/lib/example-current", symlink_object()));
  result.push_back(entry(
      "run/example.fifo", simple_object(pkgstate::owned_object_kind::fifo, 0640)));
  result.push_back(entry(
      "dev/example-char",
      device_object(pkgstate::owned_object_kind::character_device, 1, 7)));
  result.push_back(entry(
      "dev/example-block",
      device_object(pkgstate::owned_object_kind::block_device, 8, 9)));
  result.push_back(entry(
      "run/example.sock", simple_object(pkgstate::owned_object_kind::socket, 0660)));
  result.push_back(entry(
      "var/lib/example.other", simple_object(pkgstate::owned_object_kind::other, 0600),
      pkgstate::active_object_origin::retained_existing,
      pkgstate::rejected_object_reference(
          pkgstate::rejected_object_side::prior_installed,
          identity<pkgstate::rejected_object_identity>(seed + 31))));
  return result;
}

inline pkgstate::installation_receipt receipt_from_control(
    pkgstate::installed_control control_value,
    std::uint8_t seed = 20,
    pkgstate::state_target_binding binding = target(),
    std::vector<pkgstate::owned_entry> owned = {},
    std::optional<pkgstate::transaction_evidence_identity> transaction =
        std::nullopt)
{
  return pkgstate::installation_receipt::make(
      std::move(control_value), std::move(binding), std::move(owned),
      identity<pkgstate::operation_plan_identity>(seed + 18),
      identity<pkgstate::application_evidence_identity>(seed + 19),
      std::move(transaction));
}

inline pkgstate::installation_receipt receipt(
    std::string name = "example", std::uint8_t seed = 20,
    pkgstate::state_target_binding binding = target(),
    std::optional<pkgstate::transaction_evidence_identity> transaction =
        std::nullopt)
{
  return receipt_from_control(
      control(std::move(name), seed), seed, std::move(binding), manifest(seed),
      std::move(transaction));
}

inline pkgstate::installation_receipt empty_receipt(
    std::string name = "example", std::uint8_t seed = 20,
    pkgstate::state_target_binding binding = target(),
    std::optional<pkgstate::transaction_evidence_identity> transaction =
        std::nullopt)
{
  return receipt_from_control(
      control(std::move(name), seed), seed, std::move(binding), {},
      std::move(transaction));
}

inline pkgstate::installed_package package_from_receipt(
    pkgstate::installation_receipt receipt_value)
{
  return pkgstate::installed_package::make(std::move(receipt_value));
}

inline pkgstate::installed_package package(
    std::string name = "example", std::uint8_t seed = 20,
    pkgstate::state_target_binding binding = target(),
    std::optional<pkgstate::transaction_evidence_identity> transaction =
        std::nullopt)
{
  return package_from_receipt(
      receipt(std::move(name), seed, std::move(binding), std::move(transaction)));
}

inline pkgstate::installed_package empty_package(
    std::string name = "example", std::uint8_t seed = 20,
    pkgstate::state_target_binding binding = target())
{
  return package_from_receipt(
      empty_receipt(std::move(name), seed, std::move(binding)));
}

inline pkgstate::installed_package rich_package(
    std::string name = "example", std::uint8_t seed = 20,
    pkgstate::state_target_binding binding = target())
{
  pkgstate::package_source_record source_record = rich_source(std::move(name), seed);
  pkgstate::installed_control control_value = control_from_source(
      std::move(source_record), seed,
      pkgstate::installation_reason::profile_membership(
          pkgstate::profile_reference("@toolchain"),
          identity<pkgstate::source_profile_identity>(seed + 2)));
  return package_from_receipt(receipt_from_control(
      std::move(control_value), seed, std::move(binding), rich_manifest(seed),
      identity<pkgstate::transaction_evidence_identity>(seed + 40)));
}

inline pkgstate::snapshot state_with_package(
    std::string name = "example", std::uint8_t seed = 20,
    pkgstate::state_target_binding binding = target())
{
  pkgstate::installed_package value = package(name, seed, binding);
  return pkgstate::snapshot::make(std::move(binding), {std::move(value)});
}

inline pkgstate::package_state_delta install_delta(pkgstate::installed_package value)
{
  const pkgstate::operation_plan_identity plan = value.receipt().operation_plan();
  const pkgstate::application_evidence_identity evidence =
      value.receipt().application_evidence();
  return pkgstate::package_state_delta::install(
      std::move(value), plan, evidence);
}

inline pkgstate::state_publication_request install_request(
    const pkgstate::snapshot& prior, std::string name = "example",
    std::uint8_t seed = 20)
{
  return pkgstate::state_publication_request::make(
      prior,
      {install_delta(package(std::move(name), seed, prior.target_binding()))});
}

} // namespace state_fixture
