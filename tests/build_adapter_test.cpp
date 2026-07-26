// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "build_fixture.h"
#include "native_fixture.h"
#include "test.h"

#include <libpkgstate-build/adapter.h>

namespace {

void retains_complete_build_authority()
{
  const pkgbuild::build_result build = build_fixture::result();
  const pkgimage::inspected_package_image image = build_fixture::inspected();
  const pkgstate::build_adapter::build_authority authority =
      pkgstate::build_adapter::project_build(
          build_fixture::state_source(), build, image);
  const pkgstate::build_provenance& provenance = authority.provenance();
  TEST_EQ(authority.source(), build_fixture::state_source());

  TEST_EQ(provenance.request().string(),
          "v1:sha256:" + build.request().identity().hex());
  TEST_EQ(provenance.source_materials().string(),
          "v1:sha256:" + build.request().sources().identity().hex());
  TEST_EQ(provenance.build_inputs().string(),
          "v1:sha256:" + build.request().inputs().identity().hex());
  TEST_EQ(provenance.environment_policy().string(),
          "v1:sha256:" +
              build.request().policy().environment().identity().hex());
  TEST_EQ(provenance.build_policy().string(),
          "v1:sha256:" + build.request().policy().identity().hex());
  TEST_EQ(provenance.build_result().string(),
          "v1:sha256:" + build.identity().hex());
  TEST_EQ(provenance.payload_manifest().string(),
          "v1:sha256:" + build.payload()->identity().hex());
  TEST_EQ(provenance.artifact().string(),
          "v1:sha256:" + build.artifact()->identity().hex());
  TEST_EQ(provenance.artifact_content().string(),
          image.receipt().archive_digest().string());
  TEST_EQ(provenance.artifact_binding().string(),
          "v1:sha256:" + build.artifact_binding()->hex());
  TEST_EQ(provenance.execution_evidence().string(),
          "v1:sha256:" + build.execution_evidence().hex());
  TEST_EQ(provenance.artifact_image().string(),
          image.image().identity().string());
  TEST_EQ(provenance.artifact_inspection().string(),
          image.receipt().identity().string());
}

void rejects_failed_or_incomplete_builds()
{
  const pkgbuild::build_result failed = pkgbuild::build_result::failed(
      build_fixture::request(),
      pkgbuild::execution_evidence_identity::from_sha256(std::string(64, '7')),
      pkgbuild::failure_evidence_identity::from_sha256(std::string(64, '8')));
  TEST_THROWS(pkgstate::build_adapter::projection_error,
              pkgstate::build_adapter::project_build(
                  build_fixture::state_source(), failed,
                  build_fixture::inspected()));
}

void rejects_mismatched_authorities()
{
  TEST_THROWS(pkgstate::build_adapter::projection_error,
              pkgstate::build_adapter::project_build(
                  native_fixture::source("other", 80),
                  build_fixture::result(), build_fixture::inspected()));

  TEST_THROWS(pkgstate::build_adapter::projection_error,
              pkgstate::build_adapter::project_build(
                  build_fixture::state_source(), build_fixture::result(),
                  build_fixture::inspected('2')));

  TEST_THROWS(pkgstate::build_adapter::projection_error,
              pkgstate::build_adapter::project_build(
                  build_fixture::state_source(), build_fixture::result(),
                  build_fixture::inspected('1', 0644)));
}

} // namespace

int main()
{
  retains_complete_build_authority();
  rejects_failed_or_incomplete_builds();
  rejects_mismatched_authorities();
}
