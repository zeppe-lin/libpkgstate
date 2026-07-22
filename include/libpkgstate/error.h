// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file error.h
 * \brief Typed errors reported by libpkgstate.
 */

#pragma once

#include <stdexcept>
#include <string>

namespace pkgstate {

/*!
 * \brief Base class for all libpkgstate failures.
 */
class error : public std::runtime_error {
public:
  /*!
   * \brief Construct an error with a human-readable diagnostic.
   */
  explicit error(const std::string& message);
};

/*!
 * \brief Reports an invalid package identity.
 */
class identity_error : public error {
public:
  using error::error;
};

/*!
 * \brief Reports an invalid canonical installed package path.
 */
class path_error : public error {
public:
  using error::error;
};

/*!
 * \brief Reports invalid installed state or publication model construction.
 */
class state_error : public error {
public:
  using error::error;
};

/*!
 * \brief Reports package-state storage, parsing, locking, or commit failure.
 */
class store_error : public error {
public:
  using error::error;
};

/*!
 * \brief Reports invalid use of a state write transaction.
 */
class transaction_error : public error {
public:
  using error::error;
};

} // namespace pkgstate
