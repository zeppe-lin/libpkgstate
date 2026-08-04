// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file error.h
 * \brief Typed errors reported by libpkgstate.
 */

#pragma once

#include <libpkgstate/export.h>

#include <stdexcept>
#include <string>

namespace pkgstate {

/*!
 * \brief Base class for all libpkgstate failures.
 */
class PKGSTATE_API error : public std::runtime_error {
public:
  /*!
   * \brief Construct an error with a human-readable diagnostic.
   * \param message Human-readable diagnostic message.
   */
  explicit error(std::string message);

  /*! \brief Destroy the polymorphic state failure. */
  ~error() override;
};

/*!
 * \brief Reports an invalid package identity.
 */
class PKGSTATE_API identity_error : public error {
public:
  using error::error;

  /*! \brief Destroy the polymorphic identity failure. */
  ~identity_error() override;
};

/*!
 * \brief Reports an invalid canonical installed package path.
 */
class PKGSTATE_API path_error : public error {
public:
  using error::error;

  /*! \brief Destroy the polymorphic path failure. */
  ~path_error() override;
};

/*!
 * \brief Reports invalid installed state or publication model construction.
 */
class PKGSTATE_API state_error : public error {
public:
  using error::error;

  /*! \brief Destroy the polymorphic state-model failure. */
  ~state_error() override;
};

/*!
 * \brief Reports package-state storage, parsing, locking, or commit failure.
 */
class PKGSTATE_API store_error : public error {
public:
  using error::error;

  /*! \brief Destroy the polymorphic store failure. */
  ~store_error() override;
};


} // namespace pkgstate
