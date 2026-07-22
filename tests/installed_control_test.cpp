// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/installed_control.h>

#include <string>
#include <type_traits>
#include <vector>

#include "test.h"

namespace {

using pkgstate::control_provenance;
using pkgstate::control_provenance_kind;
using pkgstate::installed_control;
using pkgstate::installed_control_completeness;
using pkgstate::installed_control_fact_state;
using pkgstate::removal_lifecycle_declaration;
using pkgstate::removal_lifecycle_phase;
using pkgstate::runtime_dependency_declaration;
using pkgstate::target_profile_fact;

std::string
identity(char digit)
{
  return "v1:sha256:" + std::string(pkgstate::sha256_digest_size * 2, digit);
}

installed_control
complete_control()
{
  return installed_control::make(
      pkgstate::package_release::make("base", "1.0", "1"),
      {},
      {
          runtime_dependency_declaration::make("zlib >= 1.3"),
          runtime_dependency_declaration::make("glibc"),
      },
      {
          removal_lifecycle_declaration::make(
              removal_lifecycle_phase::post_remove,
              "application/x-posix-shell",
              "post\n"),
          removal_lifecycle_declaration::make(
              removal_lifecycle_phase::pre_remove,
              "application/x-posix-shell",
              std::string("pre\0exact", 9)),
      },
      {
          target_profile_fact::make("libc", "glibc"),
          target_profile_fact::make("arch", "x86_64"),
      },
      {
          control_provenance::make(
              control_provenance_kind::artifact_manifest, identity('3')),
          control_provenance::make(
              control_provenance_kind::candidate_control, identity('1')),
          control_provenance::make(
              control_provenance_kind::artifact, identity('2')),
          control_provenance::make(
              control_provenance_kind::transaction_evidence, identity('5')),
          control_provenance::make(
              control_provenance_kind::application_evidence, identity('4')),
      });
}

installed_control
permuted_control()
{
  return installed_control::make(
      pkgstate::package_release::make("base", "1.0", "1"),
      {},
      {
          runtime_dependency_declaration::make("glibc"),
          runtime_dependency_declaration::make("zlib >= 1.3"),
      },
      {
          removal_lifecycle_declaration::make(
              removal_lifecycle_phase::pre_remove,
              "application/x-posix-shell",
              std::string("pre\0exact", 9)),
          removal_lifecycle_declaration::make(
              removal_lifecycle_phase::post_remove,
              "application/x-posix-shell",
              "post\n"),
      },
      {
          target_profile_fact::make("arch", "x86_64"),
          target_profile_fact::make("libc", "glibc"),
      },
      {
          control_provenance::make(
              control_provenance_kind::application_evidence, identity('4')),
          control_provenance::make(
              control_provenance_kind::artifact, identity('2')),
          control_provenance::make(
              control_provenance_kind::transaction_evidence, identity('5')),
          control_provenance::make(
              control_provenance_kind::candidate_control, identity('1')),
          control_provenance::make(
              control_provenance_kind::artifact_manifest, identity('3')),
      });
}

} // namespace

int
main()
{
  static_assert(!std::is_default_constructible_v<installed_control>);
  static_assert(!std::is_constructible_v<
                installed_control,
                pkgstate::installed_control_identity,
                pkgstate::package_release>);

  const installed_control control = complete_control();

  CHECK(control.release() ==
        pkgstate::package_release::make("base", "1.0", "1"));
  CHECK(control.completeness() == installed_control_completeness{});

  CHECK(control.runtime_dependencies().size() == 2);
  CHECK(control.runtime_dependencies()[0].expression() == "glibc");
  CHECK(control.runtime_dependencies()[1].expression() == "zlib >= 1.3");

  CHECK(control.removal_lifecycle().size() == 2);
  CHECK(control.removal_lifecycle()[0].phase() ==
        removal_lifecycle_phase::pre_remove);
  CHECK(control.removal_lifecycle()[0].material() ==
        std::string("pre\0exact", 9));
  CHECK(control.removal_lifecycle()[1].phase() ==
        removal_lifecycle_phase::post_remove);

  CHECK(control.target_profile().size() == 2);
  CHECK(control.target_profile()[0].name() == "arch");
  CHECK(control.target_profile()[1].name() == "libc");

  CHECK(control.provenance().size() == 5);
  CHECK(control.provenance()[0].kind() ==
        control_provenance_kind::candidate_control);
  CHECK(control.provenance()[1].kind() ==
        control_provenance_kind::artifact);
  CHECK(control.provenance()[2].kind() ==
        control_provenance_kind::artifact_manifest);
  CHECK(control.provenance()[3].kind() ==
        control_provenance_kind::application_evidence);
  CHECK(control.provenance()[4].kind() ==
        control_provenance_kind::transaction_evidence);

  CHECK(control.identity().string() ==
        "v1:sha256:"
        "bf87432557e10047a726cec16c6cf99d"
        "6525310a9cc796ff18c2e8b6c484f630");

  const installed_control same = complete_control();
  CHECK(control == same);
  CHECK(!(control != same));
  CHECK(control == permuted_control());

  const installed_control known_empty = installed_control::make(
      pkgstate::package_release::make("empty", "1", "1"),
      {}, {}, {}, {}, {});
  CHECK(pkgstate::is_known(
      known_empty.completeness().runtime_dependencies));
  CHECK(known_empty.runtime_dependencies().empty());

  installed_control_completeness unavailable_completeness;
  unavailable_completeness.runtime_dependencies =
      installed_control_fact_state::historically_unavailable;
  unavailable_completeness.removal_lifecycle =
      installed_control_fact_state::historically_unavailable;
  unavailable_completeness.target_profile =
      installed_control_fact_state::historically_unavailable;
  unavailable_completeness.provenance =
      installed_control_fact_state::historically_unavailable;

  const installed_control unavailable = installed_control::make(
      pkgstate::package_release::make("empty", "1", "1"),
      unavailable_completeness, {}, {}, {}, {});
  CHECK(!pkgstate::is_known(
      unavailable.completeness().runtime_dependencies));
  CHECK(known_empty.identity() != unavailable.identity());

  installed_control_completeness migrated_completeness;
  migrated_completeness.runtime_dependencies =
      installed_control_fact_state::supplied_by_migration;
  const installed_control migrated = installed_control::make(
      pkgstate::package_release::make("empty", "1", "1"),
      migrated_completeness,
      {runtime_dependency_declaration::make("libc")},
      {}, {}, {});
  CHECK(migrated.completeness().runtime_dependencies ==
        installed_control_fact_state::supplied_by_migration);
  CHECK(migrated.identity() != known_empty.identity());

  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(runtime_dependency_declaration::make(""));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(runtime_dependency_declaration::make("bad\ndep"));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(removal_lifecycle_declaration::make(
        removal_lifecycle_phase::pre_remove, "", "body"));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(target_profile_fact::make("bad\nname", "value"));
  });
  check_throws<pkgstate::digest_error>([] {
    static_cast<void>(control_provenance::make(
        control_provenance_kind::candidate_control, "not-a-digest"));
  });

  check_throws<pkgstate::state_error>([] {
    static_cast<void>(removal_lifecycle_declaration::make(
        static_cast<removal_lifecycle_phase>(0), "format", "body"));
  });
  check_throws<pkgstate::state_error>([] {
    static_cast<void>(control_provenance::make(
        static_cast<control_provenance_kind>(0), identity('0')));
  });
  check_throws<pkgstate::state_error>([] {
    installed_control_completeness completeness;
    completeness.runtime_dependencies =
        static_cast<installed_control_fact_state>(0);
    static_cast<void>(installed_control::make(
        pkgstate::package_release::make("base", "1", "1"),
        completeness, {}, {}, {}, {}));
  });

  check_throws<pkgstate::state_error>([] {
    static_cast<void>(installed_control::make(
        pkgstate::package_release::make("base", "1", "1"),
        {},
        {
            runtime_dependency_declaration::make("libc"),
            runtime_dependency_declaration::make("libc"),
        },
        {}, {}, {}));
  });
  check_throws<pkgstate::state_error>([] {
    static_cast<void>(installed_control::make(
        pkgstate::package_release::make("base", "1", "1"),
        {}, {}, {},
        {
            target_profile_fact::make("arch", "x86_64"),
            target_profile_fact::make("arch", "amd64"),
        },
        {}));
  });
  check_throws<pkgstate::state_error>([] {
    static_cast<void>(installed_control::make(
        pkgstate::package_release::make("base", "1", "1"),
        {}, {}, {}, {},
        {
            control_provenance::make(
                control_provenance_kind::candidate_control, identity('1')),
            control_provenance::make(
                control_provenance_kind::candidate_control, identity('2')),
        }));
  });
  check_throws<pkgstate::state_error>([] {
    installed_control_completeness completeness;
    completeness.removal_lifecycle =
        installed_control_fact_state::historically_unavailable;
    static_cast<void>(installed_control::make(
        pkgstate::package_release::make("base", "1", "1"),
        completeness, {},
        {removal_lifecycle_declaration::make(
            removal_lifecycle_phase::pre_remove, "format", "body")},
        {}, {}));
  });

  const removal_lifecycle_declaration first =
      removal_lifecycle_declaration::make(
          removal_lifecycle_phase::pre_remove, "format", "first");
  const removal_lifecycle_declaration second =
      removal_lifecycle_declaration::make(
          removal_lifecycle_phase::pre_remove, "format", "second");
  const installed_control ordered = installed_control::make(
      pkgstate::package_release::make("order", "1", "1"),
      {}, {}, {first, second}, {}, {});
  CHECK(ordered.removal_lifecycle()[0].material() == "first");
  CHECK(ordered.removal_lifecycle()[1].material() == "second");

  const installed_control reversed = installed_control::make(
      pkgstate::package_release::make("order", "1", "1"),
      {}, {}, {second, first}, {}, {});
  CHECK(ordered.identity() != reversed.identity());

  return 0;
}
