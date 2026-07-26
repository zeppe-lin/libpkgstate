// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-source/adapter.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <libpkgsource/model.h>
#include <libpkgsource/profile.h>
#include <libpkgsource/recipe.h>

namespace pkgstate::source_adapter {
namespace {

template<typename Target, typename Source>
Target translate_identity(const Source& source)
{
  try
  {
    return Target::parse(std::string("v1:sha256:") + source.hex());
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::identity_translation,
        std::string("cannot translate source identity: ") + error.what());
  }
}

declaration_provenance translate(const pkgsource::declaration_provenance& source)
{
  return declaration_provenance(source.document(), source.path(),
                                source.line(), source.column());
}

profile_expansion_step translate(const pkgsource::profile_expansion_step& source)
{
  requirement_member_kind kind;
  switch (source.member().kind())
  {
    case pkgsource::requirement_subject_kind::package:
      kind = requirement_member_kind::package;
      break;
    case pkgsource::requirement_subject_kind::profile:
      kind = requirement_member_kind::profile;
      break;
    default:
      throw projection_error(projection_error_code::vocabulary_translation,
                             "unknown source requirement subject kind");
  }

  return profile_expansion_step(
      profile_reference(source.profile().name()), kind,
      source.member().text(), translate(source.provenance()));
}

package_requirement translate(const pkgsource::resolved_requirement& source)
{
  std::vector<requirement_origin> origins;
  origins.reserve(source.origins().size());
  for (const pkgsource::requirement_origin& origin : source.origins())
  {
    std::vector<profile_expansion_step> expansion;
    expansion.reserve(origin.expansion().size());
    for (const pkgsource::profile_expansion_step& step : origin.expansion())
      expansion.push_back(translate(step));
    origins.emplace_back(translate(origin.declaration()), std::move(expansion));
  }
  return package_requirement(package_reference(source.package().name()),
                             std::move(origins));
}

lifecycle_action translate(pkgsource::lifecycle_action source)
{
  switch (source)
  {
    case pkgsource::lifecycle_action::pre_install:
      return lifecycle_action::pre_install;
    case pkgsource::lifecycle_action::post_install:
      return lifecycle_action::post_install;
    case pkgsource::lifecycle_action::pre_remove:
      return lifecycle_action::pre_remove;
    case pkgsource::lifecycle_action::post_remove:
      return lifecycle_action::post_remove;
  }
  throw projection_error(projection_error_code::vocabulary_translation,
                         "unknown source lifecycle action");
}

program_language translate(pkgsource::program_language source)
{
  switch (source)
  {
    case pkgsource::program_language::posix_shell:
      return program_language::posix_shell;
  }
  throw projection_error(projection_error_code::vocabulary_translation,
                         "unknown source program language");
}

std::vector<architecture_reference> translate_architectures(
    const std::vector<pkgsource::architecture_reference>& source)
{
  std::vector<architecture_reference> result;
  result.reserve(source.size());
  for (const pkgsource::architecture_reference& value : source)
    result.emplace_back(value.name());
  return result;
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

package_source_record project_source(
    const pkgsource::source_snapshot& source,
    const pkgsource::architecture_reference& selected_build,
    const pkgsource::architecture_reference& selected_target)
{
  try
  {
    const pkgsource::sealed_recipe& recipe = source.recipe();
    const pkgsource::package_release& source_release = recipe.release();

    package_release release(
        translate_identity<package_release_identity>(source_release.identity()),
        package_reference(source_release.package().name()),
        source_release.version(), source_release.release());

    const pkgsource::package_metadata& source_metadata = recipe.metadata();
    package_metadata metadata(
        source_metadata.summary(), source_metadata.description(),
        source_metadata.homepage(), source_metadata.licenses());

    std::vector<package_requirement> runtime;
    for (const pkgsource::resolved_requirement& requirement :
         recipe.run_requirements())
      runtime.push_back(translate(requirement));

    std::vector<lifecycle_program> programs;
    programs.reserve(recipe.lifecycle_programs().size());
    for (const pkgsource::lifecycle_program& lifecycle :
         recipe.lifecycle_programs())
    {
      programs.emplace_back(
          translate(lifecycle.action()),
          program(translate(lifecycle.value().language()),
                  lifecycle.value().material()));
    }

    std::vector<lifecycle_requirement> lifecycle_requirements;
    for (const pkgsource::lifecycle_action action : {
             pkgsource::lifecycle_action::pre_install,
             pkgsource::lifecycle_action::post_install,
             pkgsource::lifecycle_action::pre_remove,
             pkgsource::lifecycle_action::post_remove})
    {
      for (const pkgsource::resolved_requirement& requirement :
           recipe.lifecycle_requirements(action))
      {
        lifecycle_requirements.emplace_back(translate(action),
                                            translate(requirement));
      }
    }

    std::vector<selected_profile> profiles;
    profiles.reserve(recipe.selected_build_profiles().size());
    for (const pkgsource::selected_profile& selected :
         recipe.selected_build_profiles())
    {
      std::vector<declaration_provenance> declarations;
      declarations.reserve(selected.declarations().size());
      for (const pkgsource::declaration_provenance& declaration :
           selected.declarations())
        declarations.push_back(translate(declaration));
      profiles.emplace_back(
          profile_reference(selected.profile().name()),
          translate_identity<source_profile_identity>(selected.identity()),
          std::move(declarations));
    }

    const pkgsource::architecture_requirements& source_architectures =
        recipe.architectures();
    architecture_binding architectures = architecture_binding::make(
        translate_architectures(source_architectures.build()),
        translate_architectures(source_architectures.target()),
        architecture_reference(selected_build.name()),
        architecture_reference(selected_target.name()));

    return package_source_record::make(
        std::move(release), std::move(metadata), std::move(runtime),
        std::move(programs), std::move(lifecycle_requirements),
        std::move(architectures), std::move(profiles),
        translate_identity<source_recipe_identity>(recipe.identity()),
        translate_identity<source_snapshot_identity>(source.identity()));
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const state_error& error)
  {
    const std::string message = error.what();
    const projection_error_code code =
        message.find("architecture") != std::string::npos
            ? projection_error_code::architecture_selection
            : projection_error_code::record_construction;
    throw projection_error(code,
                           std::string("state projection rejected source: ") +
                               message);
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::record_construction,
        std::string("cannot project sealed source authority: ") + error.what());
  }
}

} // namespace pkgstate::source_adapter
