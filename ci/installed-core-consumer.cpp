// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/libpkgstate.h>

#include <filesystem>
#include <string>

namespace {

template<typename Identity>
Identity
identity(char digit)
{
  return Identity::parse("v1:sha256:" + std::string(64, digit));
}

pkgstate::state_target_binding
target_binding()
{
  return pkgstate::state_target_binding::make(
      identity<pkgstate::managed_target_identity>('1'),
      identity<pkgstate::state_store_identity>('2'),
      identity<pkgstate::root_view_identity>('3'),
      identity<pkgstate::state_backend_identity>('4'),
      identity<pkgstate::publication_domain_identity>('5'));
}

} // namespace

int
main(int argc, char** argv)
{
  if (argc != 1 && argc != 2)
    return 2;

  const pkgstate::state_target_binding target = target_binding();
  const pkgstate::snapshot state = pkgstate::snapshot::make(target, {});
  if (state.size() != 0 ||
      state.target_binding().identity().string() != target.identity().string() ||
      state.identity().string().rfind("v1:sha256:", 0) != 0)
  {
    return 1;
  }

  if (argc == 2)
  {
    pkgstate::canonical_generation_store store(
        std::filesystem::path(argv[1]), target);
    if (store.read().identity().string() != state.identity().string())
      return 1;
  }

  return 0;
}
