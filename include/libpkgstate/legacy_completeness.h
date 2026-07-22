// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file legacy_completeness.h
 * \brief Explicit provenance and absence of historical compatibility facts.
 */

#pragma once

#include <cstdint>

namespace pkgstate {

/*!
 * \brief Availability of one fact in the historical compatibility format.
 */
enum class legacy_fact_availability : std::uint8_t {
  retained_by_compatibility_format = 1, //!< Retained by the old format.
  derived_from_retained_facts = 2, //!< Derived only from retained facts.
  historically_unavailable = 3, //!< The historical format did not retain it.
};

/*! \brief Completeness profile of one historical package record. */
class legacy_package_completeness final {
public:
  [[nodiscard]] constexpr legacy_fact_availability package_name() const noexcept
  {
    return legacy_fact_availability::retained_by_compatibility_format;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  opaque_version_line() const noexcept
  {
    return legacy_fact_availability::retained_by_compatibility_format;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  ownership_manifest() const noexcept
  {
    return legacy_fact_availability::retained_by_compatibility_format;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  ownership_object_class() const noexcept
  {
    return legacy_fact_availability::retained_by_compatibility_format;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  package_observation_identity() const noexcept
  {
    return legacy_fact_availability::derived_from_retained_facts;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  package_release() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  installed_control() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  state_target_binding() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  installed_package_identity() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  application_evidence() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  transaction_evidence() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }
};

/*! \brief Completeness profile of one historical database observation. */
class legacy_snapshot_completeness final {
public:
  [[nodiscard]] constexpr legacy_fact_availability
  package_records() const noexcept
  {
    return legacy_fact_availability::retained_by_compatibility_format;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  ownership_relation() const noexcept
  {
    return legacy_fact_availability::derived_from_retained_facts;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  package_observation_identities() const noexcept
  {
    return legacy_fact_availability::derived_from_retained_facts;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  snapshot_observation_identity() const noexcept
  {
    return legacy_fact_availability::derived_from_retained_facts;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  state_target_binding() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  ownership_inventory_identity() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  installed_state_snapshot_identity() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }

  [[nodiscard]] constexpr legacy_fact_availability
  publication_history() const noexcept
  {
    return legacy_fact_availability::historically_unavailable;
  }
};

} // namespace pkgstate
