// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/libpkgstate.h>

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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
file(const char* value)
{
  return {pkgstate::package_path::parse(value),
          pkgstate::owned_entry_type::non_directory};
}

pkgstate::installed_package
package(const char* name,
        const char* version,
        const pkgstate::state_target_binding& target,
        char application,
        std::optional<char> transaction,
        std::vector<pkgstate::owned_entry> manifest)
{
  const pkgstate::package_release release =
      pkgstate::package_release::make(name, version, "1");

  std::vector<pkgstate::control_provenance> provenance;
  if (application != '\0')
  {
    provenance.push_back(pkgstate::control_provenance::make(
        pkgstate::control_provenance_kind::application_evidence,
        identity<pkgstate::application_evidence_identity>(application)
            .string()));
  }
  if (transaction.has_value())
  {
    provenance.push_back(pkgstate::control_provenance::make(
        pkgstate::control_provenance_kind::transaction_evidence,
        identity<pkgstate::transaction_evidence_identity>(*transaction)
            .string()));
  }

  const pkgstate::installed_control control =
      pkgstate::installed_control::make(
          release, {}, {}, {}, {}, std::move(provenance));
  return pkgstate::installed_package::make(
      release, control, target, std::move(manifest));
}

} // namespace

int
main()
{
  static_assert(!std::is_same_v<pkgstate::operation_plan_identity,
                                pkgstate::application_evidence_identity>);
  static_assert(!std::is_same_v<pkgstate::application_evidence_identity,
                                pkgstate::transaction_evidence_identity>);
  static_assert(!std::is_constructible_v<
                pkgstate::operation_plan_identity,
                pkgstate::application_evidence_identity>);

  const pkgstate::state_target_binding target = binding('1');
  const pkgstate::installed_package old_base =
      package("base", "1.0", target, '\0', std::nullopt,
              {file("usr/bin/base")});
  const pkgstate::snapshot expected =
      pkgstate::snapshot::make(target, {old_base});

  const auto transaction =
      identity<pkgstate::transaction_evidence_identity>('c');
  const pkgstate::installed_package new_base =
      package("base", "2.0", target, 'b', 'c',
              {file("usr/bin/base"), file("usr/lib/libbase.so")});
  const pkgstate::installed_package new_tool =
      package("tool", "1.0", target, 'e', 'c',
              {file("usr/bin/tool")});

  const pkgstate::package_state_delta replace =
      pkgstate::package_state_delta::replace(
          old_base.identity(), new_base,
          identity<pkgstate::operation_plan_identity>('a'),
          identity<pkgstate::application_evidence_identity>('b'));
  const pkgstate::package_state_delta install =
      pkgstate::package_state_delta::install(
          new_tool,
          identity<pkgstate::operation_plan_identity>('d'),
          identity<pkgstate::application_evidence_identity>('e'));

  const pkgstate::state_publication_request request =
      pkgstate::state_publication_request::make(
          expected, {install, replace}, transaction);

  CHECK(request.schema_version() == 1);
  CHECK(request.identity().string() ==
        "v1:sha256:"
        "c297bf7daeb031978c3386ff26d17742"
        "b319bb9456b3923aebd3aa803392ea8a");
  CHECK(request.expected_snapshot() == expected.identity());
  CHECK(request.target_binding() == target);
  CHECK(request.transaction_evidence().has_value());
  CHECK(*request.transaction_evidence() == transaction);
  CHECK(request.deltas().size() == 2);
  CHECK(request.deltas()[0].package_name() == "base");
  CHECK(request.deltas()[0].kind() ==
        pkgstate::package_state_delta_kind::replace);
  CHECK(request.deltas()[0].expected_package().has_value());
  CHECK(*request.deltas()[0].expected_package() == old_base.identity());
  CHECK(request.deltas()[0].proposed_package().has_value());
  CHECK(request.deltas()[0].proposed_package()->identity() ==
        new_base.identity());
  CHECK(request.deltas()[1].package_name() == "tool");
  CHECK(request.deltas()[1].kind() ==
        pkgstate::package_state_delta_kind::install);
  CHECK(!request.deltas()[1].expected_package().has_value());
  CHECK(request.deltas()[1].proposed_package().has_value());

  const pkgstate::state_publication_request permuted =
      pkgstate::state_publication_request::make(
          expected, {replace, install}, transaction);
  CHECK(permuted.identity() == request.identity());

  const pkgstate::package_state_delta changed_plan =
      pkgstate::package_state_delta::replace(
          old_base.identity(), new_base,
          identity<pkgstate::operation_plan_identity>('f'),
          identity<pkgstate::application_evidence_identity>('b'));
  const pkgstate::state_publication_request changed =
      pkgstate::state_publication_request::make(
          expected, {changed_plan, install}, transaction);
  CHECK(changed.identity() != request.identity());

  const pkgstate::package_state_delta remove =
      pkgstate::package_state_delta::remove(
          "base", old_base.identity(),
          identity<pkgstate::operation_plan_identity>('7'),
          identity<pkgstate::application_evidence_identity>('8'));
  const pkgstate::state_publication_request removal =
      pkgstate::state_publication_request::make(expected, {remove});
  CHECK(removal.deltas().size() == 1);
  CHECK(removal.deltas()[0].kind() ==
        pkgstate::package_state_delta_kind::remove);
  CHECK(!removal.deltas()[0].proposed_package().has_value());
  CHECK(!removal.transaction_evidence().has_value());

  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_request::make(expected, {}));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_request::make(
        expected, {replace, install}));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_request::make(
        expected, {remove, remove}, transaction));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_request::make(
        expected, {pkgstate::package_state_delta::install(
            new_base,
            identity<pkgstate::operation_plan_identity>('a'),
            identity<pkgstate::application_evidence_identity>('b'))}));
  });

  const pkgstate::installed_package absent =
      package("absent", "1", target, '9', std::nullopt,
              {file("usr/bin/absent")});
  check_throws<pkgstate::state_error>([&] {
    const auto delta = pkgstate::package_state_delta::replace(
        old_base.identity(), absent,
        identity<pkgstate::operation_plan_identity>('a'),
        identity<pkgstate::application_evidence_identity>('9'));
    static_cast<void>(pkgstate::state_publication_request::make(
        expected, {delta}));
  });
  check_throws<pkgstate::state_error>([&] {
    const auto delta = pkgstate::package_state_delta::remove(
        "absent", old_base.identity(),
        identity<pkgstate::operation_plan_identity>('a'),
        identity<pkgstate::application_evidence_identity>('b'));
    static_cast<void>(pkgstate::state_publication_request::make(
        expected, {delta}));
  });
  check_throws<pkgstate::state_error>([&] {
    const auto delta = pkgstate::package_state_delta::remove(
        "base", identity<pkgstate::installed_package_identity>('a'),
        identity<pkgstate::operation_plan_identity>('b'),
        identity<pkgstate::application_evidence_identity>('c'));
    static_cast<void>(pkgstate::state_publication_request::make(
        expected, {delta}));
  });

  const pkgstate::state_target_binding other_target = binding('a');
  const pkgstate::installed_package wrong_target =
      package("tool", "1.0", other_target, 'e', std::nullopt,
              {file("usr/bin/tool")});
  check_throws<pkgstate::state_error>([&] {
    const auto delta = pkgstate::package_state_delta::install(
        wrong_target,
        identity<pkgstate::operation_plan_identity>('d'),
        identity<pkgstate::application_evidence_identity>('e'));
    static_cast<void>(pkgstate::state_publication_request::make(
        expected, {delta}));
  });

  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::package_state_delta::replace(
        new_base.identity(), new_base,
        identity<pkgstate::operation_plan_identity>('a'),
        identity<pkgstate::application_evidence_identity>('b')));
  });

  const pkgstate::installed_package missing_application =
      package("missing", "1", target, '\0', std::nullopt,
              {file("usr/bin/missing")});
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::package_state_delta::install(
        missing_application,
        identity<pkgstate::operation_plan_identity>('a'),
        identity<pkgstate::application_evidence_identity>('b')));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::package_state_delta::install(
        new_tool,
        identity<pkgstate::operation_plan_identity>('a'),
        identity<pkgstate::application_evidence_identity>('f')));
  });

  const pkgstate::installed_package missing_transaction =
      package("single", "1", target, 'a', std::nullopt,
              {file("usr/bin/single")});
  check_throws<pkgstate::state_error>([&] {
    const auto delta = pkgstate::package_state_delta::install(
        missing_transaction,
        identity<pkgstate::operation_plan_identity>('9'),
        identity<pkgstate::application_evidence_identity>('a'));
    static_cast<void>(pkgstate::state_publication_request::make(
        expected, {delta}, transaction));
  });

  check_throws<pkgstate::state_error>([] {
    static_cast<void>(pkgstate::package_state_delta::remove(
        "bad\nname",
        identity<pkgstate::installed_package_identity>('1'),
        identity<pkgstate::operation_plan_identity>('2'),
        identity<pkgstate::application_evidence_identity>('3')));
  });

  return 0;
}
