// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-build/adapter.h>

#include <optional>
#include <string>
#include <utility>

#include <libpkgstate-source/adapter.h>

namespace pkgstate::build_adapter {
namespace {

template<typename Target, typename Source>
Target translate_build_identity(const Source& source)
{
  try
  {
    return Target::parse(std::string("v1:sha256:") + source.hex());
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::identity_translation,
        std::string("cannot translate build identity: ") + error.what());
  }
}

template<typename Target, typename Source>
Target translate_external_identity(const Source& source)
{
  try
  {
    return Target::parse(source.string());
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::identity_translation,
        std::string("cannot translate inspected artifact identity: ") +
            error.what());
  }
}

pkgimage::entry_type image_type(pkgbuild::payload_entry_type value)
{
  switch (value)
  {
    case pkgbuild::payload_entry_type::regular:
      return pkgimage::entry_type::regular;
    case pkgbuild::payload_entry_type::directory:
      return pkgimage::entry_type::directory;
    case pkgbuild::payload_entry_type::symlink:
      return pkgimage::entry_type::symlink;
    case pkgbuild::payload_entry_type::hardlink:
      return pkgimage::entry_type::hardlink;
    case pkgbuild::payload_entry_type::fifo:
      return pkgimage::entry_type::fifo;
    case pkgbuild::payload_entry_type::character_device:
      return pkgimage::entry_type::character_device;
    case pkgbuild::payload_entry_type::block_device:
      return pkgimage::entry_type::block_device;
  }
  throw projection_error(projection_error_code::payload_mismatch,
                         "unknown build payload entry type");
}

void verify_entry(const pkgbuild::payload_entry& expected,
                  const pkgimage::package_entry& observed)
{
  if (expected.path().string() != observed.path.string() ||
      image_type(expected.type()) != observed.type ||
      expected.mode() != observed.mode || expected.uid() != observed.uid ||
      expected.gid() != observed.gid || expected.size() != observed.size ||
      expected.modification_time().seconds != observed.mtime ||
      expected.modification_time().nanoseconds != observed.mtime_nanoseconds)
  {
    throw projection_error(
        projection_error_code::payload_mismatch,
        "inspected artifact metadata differs at " +
            expected.path().string());
  }

  if (expected.symlink_target() != observed.symlink_target)
  {
    throw projection_error(
        projection_error_code::payload_mismatch,
        "inspected symbolic-link target differs at " +
            expected.path().string());
  }

  const std::optional<std::string> expected_hardlink =
      expected.hardlink_target()
          ? std::optional<std::string>(
                expected.hardlink_target()->string())
          : std::nullopt;
  const std::optional<std::string> observed_hardlink =
      observed.hardlink_target
          ? std::optional<std::string>(observed.hardlink_target->string())
          : std::nullopt;
  if (expected_hardlink != observed_hardlink)
  {
    throw projection_error(
        projection_error_code::payload_mismatch,
        "inspected hard-link target differs at " +
            expected.path().string());
  }

  const std::optional<pkgbuild::device_number> observed_device =
      observed.device
          ? std::optional<pkgbuild::device_number>(
                pkgbuild::device_number{observed.device->major,
                                        observed.device->minor})
          : std::nullopt;
  if (expected.device() != observed_device)
  {
    throw projection_error(
        projection_error_code::payload_mismatch,
        "inspected device number differs at " + expected.path().string());
  }

  const std::optional<std::string> expected_content =
      expected.regular_content()
          ? std::optional<std::string>(
                "v1:sha256:" + expected.regular_content()->hex())
          : std::nullopt;
  const std::optional<std::string> observed_content =
      observed.regular_content
          ? std::optional<std::string>(observed.regular_content->string())
          : std::nullopt;
  if (expected_content != observed_content)
  {
    throw projection_error(
        projection_error_code::payload_mismatch,
        "inspected regular content differs at " +
            expected.path().string());
  }
}

void verify_payload(const pkgbuild::payload_manifest& expected,
                    const pkgimage::package_image& observed)
{
  if (expected.entries().size() != observed.entries().size())
  {
    throw projection_error(
        projection_error_code::payload_mismatch,
        "inspected artifact entry count differs from build payload");
  }
  for (std::size_t index = 0; index < expected.entries().size(); ++index)
    verify_entry(expected.entries()[index], observed.entries()[index]);
}

void verify_source(const package_source_record& source,
                   const pkgbuild::build_request& request)
{
  package_source_record expected = [&] {
    try
    {
      return source_adapter::project_source(
          request.source(), request.architectures().build(),
          request.architectures().target());
    }
    catch (const source_adapter::projection_error& error)
    {
      throw projection_error(
          projection_error_code::source_binding,
          std::string("cannot derive state source authority from build: ") +
              error.what());
    }
  }();

  if (source != expected)
  {
    throw projection_error(
        projection_error_code::source_binding,
        "state source record differs from the build request source authority");
  }
}

} // namespace

projection_error::projection_error(projection_error_code code,
                                   std::string message)
    : std::invalid_argument(std::move(message)), code_(code)
{
}

projection_error_code projection_error::code() const noexcept
{
  return code_;
}

build_authority::build_authority(package_source_record source,
                                   build_provenance provenance)
    : source_(std::move(source)), provenance_(std::move(provenance))
{
  if (provenance_.source_record() != source_.identity())
    throw projection_error(projection_error_code::source_binding,
                           "build authority source binding is inconsistent");
}

const package_source_record& build_authority::source() const noexcept
{
  return source_;
}

const build_provenance& build_authority::provenance() const noexcept
{
  return provenance_;
}

build_authority project_build(
    const package_source_record& source,
    const pkgbuild::build_result& build,
    const pkgimage::inspected_package_image& image)
{
  if (build.outcome() != pkgbuild::build_outcome::succeeded ||
      !build.payload() || !build.artifact() || !build.artifact_binding() ||
      build.failure_evidence())
  {
    throw projection_error(
        projection_error_code::build_result,
        "state admission requires a complete successful build result");
  }

  verify_source(source, build.request());

  const std::string expected_digest =
      "v1:sha256:" + build.artifact()->complete_digest().hex();
  if (image.receipt().archive_digest().string() != expected_digest)
  {
    throw projection_error(
        projection_error_code::artifact_binding,
        "inspected artifact bytes differ from build artifact authority");
  }
  if (image.receipt().image_identity() != image.image().identity() ||
      image.receipt().entry_count() != image.image().size())
  {
    throw projection_error(
        projection_error_code::artifact_binding,
        "inspected artifact receipt does not bind the supplied image");
  }

  verify_payload(*build.payload(), image.image());

  try
  {
    return build_authority(
        source,
        build_provenance(
            source.identity(),
        translate_build_identity<build_request_identity>(
            build.request().identity()),
        translate_build_identity<source_material_set_identity>(
            build.request().sources().identity()),
        translate_build_identity<build_input_set_identity>(
            build.request().inputs().identity()),
        translate_build_identity<environment_policy_identity>(
            build.request().policy().environment().identity()),
        translate_build_identity<build_policy_identity>(
            build.request().policy().identity()),
        translate_build_identity<build_result_identity>(build.identity()),
        translate_build_identity<payload_manifest_identity>(
            build.payload()->identity()),
        translate_build_identity<build_artifact_identity>(
            build.artifact()->identity()),
        translate_external_identity<artifact_content_identity>(
            image.receipt().archive_digest()),
        translate_build_identity<artifact_binding_identity>(
            *build.artifact_binding()),
        translate_build_identity<execution_evidence_identity>(
            build.execution_evidence()),
        translate_external_identity<artifact_image_identity>(
            image.image().identity()),
        translate_external_identity<artifact_inspection_identity>(
            image.receipt().identity())));
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::identity_translation,
        std::string("cannot construct native build provenance: ") +
            error.what());
  }
}

} // namespace pkgstate::build_adapter
