// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/publication_codec.h>

#include <libpkgstate/generation_codec.h>
#include "publication_projection.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <openssl/evp.h>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

constexpr std::array<std::uint8_t, 8> request_magic = {
    'Z', 'L', 'S', 'P', 'R', 'Q', 'S', 'T',
};
constexpr std::array<std::uint8_t, 8> receipt_magic = {
    'Z', 'L', 'S', 'P', 'R', 'C', 'P', 'T',
};
constexpr std::array<std::uint8_t, 29> legacy_request_magic = {
    'p','k','g','s','t','a','t','e','-','p','u','b','l','i','c','a','t','i','o','n','-','r','e','q','u','e','s','t',0,
};
constexpr std::array<std::uint8_t, 29> legacy_receipt_magic = {
    'p','k','g','s','t','a','t','e','-','p','u','b','l','i','c','a','t','i','o','n','-','r','e','c','e','i','p','t',0,
};
constexpr std::uint16_t legacy_state_publication_encoding_version = 1;
enum class publication_wire_format : std::uint8_t {
  legacy_v1 = 1,
  house_v2 = 2,
};
constexpr std::size_t checksum_size = 32;
constexpr std::size_t maximum_collection_count = 1024ULL * 1024ULL;

[[noreturn]] void fail(state_publication_codec_error_code code,
                       std::string message)
{
  throw state_publication_codec_error(code, std::move(message));
}

class writer final {
public:
  template<std::size_t Size>
  void raw(const std::array<std::uint8_t, Size>& value)
  {
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void raw(const std::uint8_t* data, std::size_t size)
  {
    if (size != 0)
      bytes_.insert(bytes_.end(), data, data + size);
  }

  void u8(std::uint8_t value) { bytes_.push_back(value); }

  void u16(std::uint16_t value)
  {
    bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes_.push_back(static_cast<std::uint8_t>(value & 0xffU));
  }

  void u64(std::uint64_t value)
  {
    for (int shift = 56; shift >= 0; shift -= 8)
      bytes_.push_back(static_cast<std::uint8_t>(value >> static_cast<unsigned>(shift)));
  }

  void boolean(bool value) { u8(value ? 1U : 0U); }

  void bytes(std::string_view value)
  {
    u64(static_cast<std::uint64_t>(value.size()));
    raw(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
  }

  void bytes(const std::vector<std::uint8_t>& value)
  {
    u64(static_cast<std::uint64_t>(value.size()));
    raw(value.data(), value.size());
  }

  template<typename Identity>
  void digest(const Identity& identity)
  {
    bytes(identity.string());
  }

  [[nodiscard]] const std::vector<std::uint8_t>& value() const noexcept
  {
    return bytes_;
  }

  [[nodiscard]] std::vector<std::uint8_t> take() { return std::move(bytes_); }

private:
  std::vector<std::uint8_t> bytes_;
};

class reader final {
public:
  explicit reader(std::string_view bytes) : bytes_(bytes) {}

  template<std::size_t Size>
  void expect(const std::array<std::uint8_t, Size>& expected,
              const char* label)
  {
    require(Size, label);
    const auto* actual = reinterpret_cast<const std::uint8_t*>(
        bytes_.data() + position_);
    if (!std::equal(expected.begin(), expected.end(), actual))
      fail(state_publication_codec_error_code::invalid_magic,
           std::string("invalid ") + label);
    position_ += Size;
  }

  [[nodiscard]] std::uint8_t u8(const char* label)
  {
    require(1, label);
    return static_cast<std::uint8_t>(bytes_[position_++]);
  }

  [[nodiscard]] std::uint16_t u16(const char* label)
  {
    require(2, label);
    const std::uint16_t value =
        (static_cast<std::uint16_t>(
             static_cast<std::uint8_t>(bytes_[position_])) << 8U) |
        static_cast<std::uint8_t>(bytes_[position_ + 1]);
    position_ += 2;
    return value;
  }

  [[nodiscard]] std::uint64_t u64(const char* label)
  {
    require(8, label);
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < 8; ++index)
      value = (value << 8U) |
          static_cast<std::uint8_t>(bytes_[position_ + index]);
    position_ += 8;
    return value;
  }

  [[nodiscard]] bool boolean(const char* label)
  {
    const std::uint8_t value = u8(label);
    if (value > 1)
      fail(state_publication_codec_error_code::invalid_value,
           std::string("invalid ") + label);
    return value == 1;
  }

  [[nodiscard]] std::string bytes(const char* label)
  {
    const std::uint64_t size64 = u64(label);
    if (size64 > remaining() ||
        size64 > static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max()))
    {
      fail(state_publication_codec_error_code::truncated,
           std::string(label) + " length exceeds record");
    }
    const std::size_t size = static_cast<std::size_t>(size64);
    std::string value(bytes_.substr(position_, size));
    position_ += size;
    return value;
  }

  [[nodiscard]] std::size_t count(const char* label)
  {
    const std::uint64_t value = u64(label);
    if (value > maximum_collection_count || value > remaining())
    {
      fail(state_publication_codec_error_code::limit_exceeded,
           std::string(label) + " exceeds the supported bound");
    }
    return static_cast<std::size_t>(value);
  }

  void finish()
  {
    if (position_ != bytes_.size())
      fail(state_publication_codec_error_code::trailing_data,
           "trailing bytes in publication evidence record");
  }

private:
  [[nodiscard]] std::size_t remaining() const noexcept
  {
    return bytes_.size() - position_;
  }

  void require(std::size_t size, const char* label)
  {
    if (size > remaining())
      fail(state_publication_codec_error_code::truncated,
           std::string("truncated ") + label);
  }

  std::string_view bytes_;
  std::size_t position_ = 0;
};

template<std::size_t Size>
bool has_prefix(std::string_view bytes,
                const std::array<std::uint8_t, Size>& expected)
{
  if (bytes.size() < Size)
    return false;
  return std::equal(
      expected.begin(), expected.end(),
      reinterpret_cast<const std::uint8_t*>(bytes.data()));
}

publication_wire_format read_request_prefix(reader& input,
                                            std::string_view body)
{
  if (has_prefix(body, request_magic))
  {
    input.expect(request_magic, "publication request magic");
    if (input.u16("publication request encoding version") !=
        state_publication_request_encoding_version)
    {
      fail(state_publication_codec_error_code::unsupported_version,
           "unsupported publication request encoding version");
    }
    return publication_wire_format::house_v2;
  }

  if (has_prefix(body, legacy_request_magic))
  {
    input.expect(legacy_request_magic, "legacy publication request magic");
    if (input.u16("legacy publication request encoding version") !=
        legacy_state_publication_encoding_version)
    {
      fail(state_publication_codec_error_code::unsupported_version,
           "unsupported legacy publication request encoding version");
    }
    return publication_wire_format::legacy_v1;
  }

  fail(state_publication_codec_error_code::invalid_magic,
       "invalid publication request magic");
}

publication_wire_format read_receipt_prefix(reader& input,
                                            std::string_view body)
{
  if (has_prefix(body, receipt_magic))
  {
    input.expect(receipt_magic, "publication receipt magic");
    if (input.u16("publication receipt encoding version") !=
        state_publication_receipt_encoding_version)
    {
      fail(state_publication_codec_error_code::unsupported_version,
           "unsupported publication receipt encoding version");
    }
    return publication_wire_format::house_v2;
  }

  if (has_prefix(body, legacy_receipt_magic))
  {
    input.expect(legacy_receipt_magic, "legacy publication receipt magic");
    if (input.u16("legacy publication receipt encoding version") !=
        legacy_state_publication_encoding_version)
    {
      fail(state_publication_codec_error_code::unsupported_version,
           "unsupported legacy publication receipt encoding version");
    }
    return publication_wire_format::legacy_v1;
  }

  fail(state_publication_codec_error_code::invalid_magic,
       "invalid publication receipt magic");
}

sha256_digest_bytes sha256(std::string_view bytes)
{
  using context_ptr =
      std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  context_ptr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!context)
    fail(state_publication_codec_error_code::invalid_value,
         "could not allocate publication-codec digest context");

  if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(context.get(), bytes.data(), bytes.size()) != 1)
  {
    fail(state_publication_codec_error_code::invalid_value,
         "could not compute publication-codec checksum");
  }

  sha256_digest_bytes result{};
  unsigned int size = 0;
  if (EVP_DigestFinal_ex(context.get(), result.data(), &size) != 1 ||
      size != result.size())
  {
    fail(state_publication_codec_error_code::invalid_value,
         "could not finalize publication-codec checksum");
  }
  return result;
}

std::string_view as_string_view(const std::vector<std::uint8_t>& bytes)
{
  return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void append_checksum(writer& output, std::size_t maximum_size)
{
  if (output.value().size() > maximum_size - checksum_size)
    fail(state_publication_codec_error_code::limit_exceeded,
         "publication evidence encoding exceeds the supported size");
  const sha256_digest_bytes checksum = sha256(as_string_view(output.value()));
  output.raw(checksum.data(), checksum.size());
}

std::string_view verified_body(const std::vector<std::uint8_t>& encoding,
                               std::size_t maximum_size)
{
  if (encoding.size() > maximum_size)
    fail(state_publication_codec_error_code::limit_exceeded,
         "publication evidence encoding exceeds the supported size");
  if (encoding.size() < checksum_size)
    fail(state_publication_codec_error_code::truncated,
         "publication evidence encoding is truncated");

  const std::size_t body_size = encoding.size() - checksum_size;
  const std::string_view body(
      reinterpret_cast<const char*>(encoding.data()), body_size);
  const sha256_digest_bytes expected = sha256(body);
  if (!std::equal(expected.begin(), expected.end(),
                  encoding.begin() + static_cast<std::ptrdiff_t>(body_size)))
  {
    fail(state_publication_codec_error_code::checksum_mismatch,
         "publication evidence checksum does not match");
  }
  return body;
}

template<typename Identity>
Identity read_digest(reader& input, const char* label)
{
  try
  {
    return Identity::parse(input.bytes(label));
  }
  catch (const error& failure)
  {
    fail(state_publication_codec_error_code::invalid_value,
         std::string("invalid ") + label + ": " + failure.what());
  }
}

package_state_delta_kind read_delta_kind(reader& input)
{
  switch (input.u8("package-state delta kind"))
  {
    case 1: return package_state_delta_kind::install;
    case 2: return package_state_delta_kind::replace;
    case 3: return package_state_delta_kind::remove;
  }
  fail(state_publication_codec_error_code::invalid_value,
       "invalid package-state delta kind");
}

std::uint8_t encode_delta_kind(package_state_delta_kind kind)
{
  switch (kind)
  {
    case package_state_delta_kind::install: return 1;
    case package_state_delta_kind::replace: return 2;
    case package_state_delta_kind::remove: return 3;
  }
  fail(state_publication_codec_error_code::invalid_value,
       "invalid package-state delta kind");
}

state_publication_outcome read_outcome(reader& input)
{
  switch (input.u8("publication outcome"))
  {
    case 1: return state_publication_outcome::published;
    case 2: return state_publication_outcome::stale_expected_state;
    case 3: return state_publication_outcome::request_rejected;
    case 4: return state_publication_outcome::failed_before_publication;
    case 5: return state_publication_outcome::published_durability_unconfirmed;
    case 6: return state_publication_outcome::indeterminate;
  }
  fail(state_publication_codec_error_code::invalid_value,
       "invalid publication outcome");
}

std::uint8_t encode_outcome(state_publication_outcome value)
{
  switch (value)
  {
    case state_publication_outcome::published: return 1;
    case state_publication_outcome::stale_expected_state: return 2;
    case state_publication_outcome::request_rejected: return 3;
    case state_publication_outcome::failed_before_publication: return 4;
    case state_publication_outcome::published_durability_unconfirmed: return 5;
    case state_publication_outcome::indeterminate: return 6;
  }
  fail(state_publication_codec_error_code::invalid_value,
       "invalid publication outcome");
}

state_publication_durability read_durability(reader& input)
{
  switch (input.u8("publication durability"))
  {
    case 1: return state_publication_durability::not_attempted;
    case 2: return state_publication_durability::confirmed;
    case 3: return state_publication_durability::unconfirmed;
    case 4: return state_publication_durability::indeterminate;
  }
  fail(state_publication_codec_error_code::invalid_value,
       "invalid publication durability");
}

std::uint8_t encode_durability(state_publication_durability value)
{
  switch (value)
  {
    case state_publication_durability::not_attempted: return 1;
    case state_publication_durability::confirmed: return 2;
    case state_publication_durability::unconfirmed: return 3;
    case state_publication_durability::indeterminate: return 4;
  }
  fail(state_publication_codec_error_code::invalid_value,
       "invalid publication durability");
}

state_storage_atomicity_boundary read_atomicity(reader& input)
{
  switch (input.u8("publication atomicity boundary"))
  {
    case 1: return state_storage_atomicity_boundary::none;
    case 2: return state_storage_atomicity_boundary::complete_state_object_replace;
    case 3: return state_storage_atomicity_boundary::immutable_generation_selection;
  }
  fail(state_publication_codec_error_code::invalid_value,
       "invalid publication atomicity boundary");
}

std::uint8_t encode_atomicity(state_storage_atomicity_boundary value)
{
  switch (value)
  {
    case state_storage_atomicity_boundary::none: return 1;
    case state_storage_atomicity_boundary::complete_state_object_replace: return 2;
    case state_storage_atomicity_boundary::immutable_generation_selection: return 3;
  }
  fail(state_publication_codec_error_code::invalid_value,
       "invalid publication atomicity boundary");
}

std::vector<std::uint8_t> encode_proposed_package(
    const installed_package& package)
{
  return encode_generation_snapshot(
      snapshot::make(package.target_binding(), {package}));
}

installed_package decode_proposed_package(
    std::string_view bytes,
    const state_target_binding& expected_binding,
    std::string_view expected_name)
{
  const snapshot proposed = decode_generation_snapshot(bytes);
  if (proposed.target_binding() != expected_binding || proposed.size() != 1)
    fail(state_publication_codec_error_code::invalid_value,
         "proposed package record has invalid target or cardinality");
  const installed_package& package = proposed.packages().front();
  if (package.release().name() != expected_name)
    fail(state_publication_codec_error_code::invalid_value,
         "proposed package record names another package");
  return package;
}

void require_request_shape(const package_state_delta& delta)
{
  switch (delta.kind())
  {
    case package_state_delta_kind::install:
      if (delta.expected_package() || !delta.proposed_package())
        fail(state_publication_codec_error_code::invalid_value,
             "invalid installation delta shape");
      return;
    case package_state_delta_kind::replace:
      if (!delta.expected_package() || !delta.proposed_package())
        fail(state_publication_codec_error_code::invalid_value,
             "invalid replacement delta shape");
      return;
    case package_state_delta_kind::remove:
      if (!delta.expected_package() || delta.proposed_package())
        fail(state_publication_codec_error_code::invalid_value,
             "invalid removal delta shape");
      return;
  }
}

template<std::size_t LegacyMagicSize>
std::vector<std::uint8_t> legacy_encoding(
    const std::vector<std::uint8_t>& current,
    const std::array<std::uint8_t, LegacyMagicSize>& legacy_magic,
    std::size_t maximum_size)
{
  constexpr std::size_t current_prefix_size = request_magic.size() + 2U;
  if (current.size() < current_prefix_size + checksum_size)
    fail(state_publication_codec_error_code::truncated,
         "current publication evidence encoding is truncated");

  writer output;
  output.raw(legacy_magic);
  output.u16(legacy_state_publication_encoding_version);
  output.raw(current.data() + current_prefix_size,
             current.size() - current_prefix_size - checksum_size);
  append_checksum(output, maximum_size);
  return output.take();
}

void require_canonical_request(
    const state_publication_request_encoding& encoded,
    const state_publication_request& request,
    publication_wire_format format)
{
  state_publication_request_encoding expected =
      encode_state_publication_request(request);
  if (format == publication_wire_format::legacy_v1)
  {
    expected = legacy_encoding(
        expected, legacy_request_magic,
        maximum_state_publication_request_encoding_size);
  }
  if (expected != encoded)
    fail(state_publication_codec_error_code::invalid_value,
         "publication request is not canonically encoded");
}

void require_canonical_receipt(
    const state_publication_receipt_encoding& encoded,
    const state_publication_receipt& receipt,
    publication_wire_format format)
{
  state_publication_receipt_encoding expected =
      encode_state_publication_receipt(receipt);
  if (format == publication_wire_format::legacy_v1)
  {
    expected = legacy_encoding(
        expected, legacy_receipt_magic,
        maximum_state_publication_receipt_encoding_size);
  }
  if (expected != encoded)
    fail(state_publication_codec_error_code::invalid_value,
         "publication receipt is not canonically encoded");
}

} // namespace

state_publication_codec_error::state_publication_codec_error(
    state_publication_codec_error_code code,
    std::string message)
    : std::invalid_argument(std::move(message)), code_(code)
{
}

state_publication_codec_error::~state_publication_codec_error() = default;

state_publication_codec_error_code
state_publication_codec_error::code() const noexcept
{
  return code_;
}

state_publication_request_encoding
encode_state_publication_request(const state_publication_request& request)
{
  writer output;
  output.raw(request_magic);
  output.u16(state_publication_request_encoding_version);
  output.digest(request.identity());
  output.digest(request.expected_snapshot());
  output.digest(request.target_binding().identity());
  output.u64(static_cast<std::uint64_t>(request.deltas().size()));

  for (const package_state_delta& delta : request.deltas())
  {
    require_request_shape(delta);
    output.u8(encode_delta_kind(delta.kind()));
    output.bytes(delta.package_name());
    output.digest(delta.operation_plan());
    output.digest(delta.application_evidence());
    output.boolean(delta.expected_package().has_value());
    if (delta.expected_package())
      output.digest(*delta.expected_package());
    output.boolean(delta.proposed_package().has_value());
    if (delta.proposed_package())
      output.bytes(encode_proposed_package(*delta.proposed_package()));
  }

  output.boolean(request.transaction_evidence().has_value());
  if (request.transaction_evidence())
    output.digest(*request.transaction_evidence());
  append_checksum(output, maximum_state_publication_request_encoding_size);
  return output.take();
}

state_publication_request
decode_state_publication_request(
    const state_publication_request_encoding& encoding,
    const snapshot& expected_snapshot)
{
  try
  {
    const std::string_view body = verified_body(
        encoding, maximum_state_publication_request_encoding_size);
    reader input(body);
    const publication_wire_format format = read_request_prefix(input, body);

    const state_publication_request_identity encoded_identity =
        read_digest<state_publication_request_identity>(
            input, "publication request identity");
    const installed_state_snapshot_identity encoded_expected =
        read_digest<installed_state_snapshot_identity>(
            input, "expected snapshot identity");
    const state_target_binding_identity encoded_target =
        read_digest<state_target_binding_identity>(
            input, "target binding identity");

    if (encoded_expected != expected_snapshot.identity() ||
        encoded_target != expected_snapshot.target_binding().identity())
    {
      fail(state_publication_codec_error_code::expected_snapshot_mismatch,
           "publication request belongs to another expected snapshot");
    }

    std::vector<package_state_delta> deltas;
    const std::size_t count = input.count("publication delta count");
    deltas.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
    {
      const package_state_delta_kind kind = read_delta_kind(input);
      const std::string package_name = input.bytes("publication package name");
      operation_plan_identity operation_plan =
          read_digest<operation_plan_identity>(input,
                                               "operation plan identity");
      application_evidence_identity application_evidence =
          read_digest<application_evidence_identity>(
              input, "application evidence identity");
      std::optional<installed_package_identity> expected_package;
      if (input.boolean("expected package presence"))
      {
        expected_package = read_digest<installed_package_identity>(
            input, "expected package identity");
      }
      std::optional<installed_package> proposed_package;
      if (input.boolean("proposed package presence"))
      {
        const std::string proposed_bytes = input.bytes("proposed package body");
        proposed_package = decode_proposed_package(
            proposed_bytes, expected_snapshot.target_binding(), package_name);
      }

      switch (kind)
      {
        case package_state_delta_kind::install:
          if (expected_package || !proposed_package)
            fail(state_publication_codec_error_code::invalid_value,
                 "invalid installation delta encoding");
          deltas.push_back(package_state_delta::install(
              std::move(*proposed_package), std::move(operation_plan),
              std::move(application_evidence)));
          break;
        case package_state_delta_kind::replace:
          if (!expected_package || !proposed_package)
            fail(state_publication_codec_error_code::invalid_value,
                 "invalid replacement delta encoding");
          deltas.push_back(package_state_delta::replace(
              std::move(*expected_package), std::move(*proposed_package),
              std::move(operation_plan), std::move(application_evidence)));
          break;
        case package_state_delta_kind::remove:
          if (!expected_package || proposed_package)
            fail(state_publication_codec_error_code::invalid_value,
                 "invalid removal delta encoding");
          deltas.push_back(package_state_delta::remove(
              package_name, std::move(*expected_package),
              std::move(operation_plan), std::move(application_evidence)));
          break;
      }
    }

    std::optional<transaction_evidence_identity> transaction;
    if (input.boolean("transaction evidence presence"))
    {
      transaction = read_digest<transaction_evidence_identity>(
          input, "transaction evidence identity");
    }
    input.finish();

    state_publication_request result = state_publication_request::make(
        expected_snapshot, std::move(deltas), std::move(transaction));
    if (result.identity() != encoded_identity)
      fail(state_publication_codec_error_code::identity_mismatch,
           "publication request identity does not match reconstructed evidence");
    require_canonical_request(encoding, result, format);
    return result;
  }
  catch (const state_publication_codec_error&)
  {
    throw;
  }
  catch (const error& failure)
  {
    fail(state_publication_codec_error_code::invalid_value,
         std::string("invalid publication request encoding: ") + failure.what());
  }
}

state_publication_receipt_encoding
encode_state_publication_receipt(const state_publication_receipt& receipt)
{
  writer output;
  output.raw(receipt_magic);
  output.u16(state_publication_receipt_encoding_version);
  output.digest(receipt.identity());
  output.digest(receipt.request());
  output.digest(receipt.expected_prior_snapshot());
  output.digest(receipt.actual_prior_snapshot());
  output.digest(receipt.target_binding().identity());
  output.bytes(receipt.storage_format());
  output.u8(encode_outcome(receipt.outcome()));
  output.u8(encode_durability(receipt.durability()));
  output.u8(encode_atomicity(receipt.atomicity_boundary()));
  output.boolean(receipt.resulting_snapshot().has_value());
  if (receipt.resulting_snapshot())
    output.digest(*receipt.resulting_snapshot());
  output.u64(static_cast<std::uint64_t>(receipt.subordinate_evidence().size()));
  for (const state_publication_evidence_identity& evidence :
       receipt.subordinate_evidence())
  {
    output.digest(evidence);
  }
  append_checksum(output, maximum_state_publication_receipt_encoding_size);
  return output.take();
}

state_publication_receipt
decode_state_publication_receipt(
    const state_publication_receipt_encoding& encoding,
    const state_publication_request& request,
    const snapshot& actual_prior)
{
  try
  {
    const std::string_view body = verified_body(
        encoding, maximum_state_publication_receipt_encoding_size);
    reader input(body);
    const publication_wire_format format = read_receipt_prefix(input, body);

    const state_publication_receipt_identity encoded_identity =
        read_digest<state_publication_receipt_identity>(
            input, "publication receipt identity");
    const state_publication_request_identity encoded_request =
        read_digest<state_publication_request_identity>(
            input, "publication request identity");
    const installed_state_snapshot_identity encoded_expected =
        read_digest<installed_state_snapshot_identity>(
            input, "expected prior snapshot identity");
    const installed_state_snapshot_identity encoded_actual =
        read_digest<installed_state_snapshot_identity>(
            input, "actual prior snapshot identity");
    const state_target_binding_identity encoded_target =
        read_digest<state_target_binding_identity>(
            input, "target binding identity");

    if (encoded_request != request.identity() ||
        encoded_expected != request.expected_snapshot())
    {
      fail(state_publication_codec_error_code::request_mismatch,
           "publication receipt belongs to another request");
    }
    if (encoded_actual != actual_prior.identity())
    {
      fail(state_publication_codec_error_code::actual_prior_mismatch,
           "publication receipt belongs to another actual prior snapshot");
    }
    if (encoded_target != request.target_binding().identity() ||
        actual_prior.target_binding() != request.target_binding())
    {
      fail(state_publication_codec_error_code::actual_prior_mismatch,
           "publication receipt actual prior belongs to another target");
    }

    std::string storage_format = input.bytes("publication storage format");
    const state_publication_outcome outcome = read_outcome(input);
    const state_publication_durability durability = read_durability(input);
    const state_storage_atomicity_boundary atomicity = read_atomicity(input);
    std::optional<installed_state_snapshot_identity> resulting;
    if (input.boolean("resulting snapshot presence"))
    {
      resulting = read_digest<installed_state_snapshot_identity>(
          input, "resulting snapshot identity");
    }
    std::vector<state_publication_evidence_identity> evidence;
    const std::size_t evidence_count =
        input.count("subordinate evidence count");
    evidence.reserve(evidence_count);
    for (std::size_t index = 0; index < evidence_count; ++index)
    {
      evidence.push_back(read_digest<state_publication_evidence_identity>(
          input, "subordinate evidence identity"));
    }
    input.finish();

    std::optional<snapshot> projected;
    auto require_projected = [&]() -> const snapshot& {
      if (!projected)
        projected = detail::project_publication_request(request, actual_prior);
      if (!resulting || *resulting != projected->identity())
      {
        fail(state_publication_codec_error_code::invalid_value,
             "publication receipt cites an impossible resulting snapshot");
      }
      return *projected;
    };

    state_publication_receipt result = [&]() {
      switch (outcome)
      {
        case state_publication_outcome::published:
          if (durability != state_publication_durability::confirmed ||
              atomicity == state_storage_atomicity_boundary::none)
            fail(state_publication_codec_error_code::invalid_value,
                 "published receipt has invalid durability or atomicity");
          return state_publication_receipt::published(
              request, actual_prior, require_projected(),
              std::move(storage_format), atomicity, std::move(evidence));

        case state_publication_outcome::stale_expected_state:
          if (durability != state_publication_durability::not_attempted ||
              atomicity != state_storage_atomicity_boundary::none || resulting)
            fail(state_publication_codec_error_code::invalid_value,
                 "stale receipt has invalid durability, atomicity, or result");
          return state_publication_receipt::stale_expected_state(
              request, actual_prior, std::move(storage_format),
              std::move(evidence));

        case state_publication_outcome::request_rejected:
          if (durability != state_publication_durability::not_attempted ||
              atomicity != state_storage_atomicity_boundary::none || resulting)
            fail(state_publication_codec_error_code::invalid_value,
                 "rejected receipt has invalid durability, atomicity, or result");
          return state_publication_receipt::request_rejected(
              request, actual_prior, std::move(storage_format),
              std::move(evidence));

        case state_publication_outcome::failed_before_publication:
          if (durability != state_publication_durability::not_attempted ||
              atomicity != state_storage_atomicity_boundary::none || resulting)
            fail(state_publication_codec_error_code::invalid_value,
                 "failed receipt has invalid durability, atomicity, or result");
          return state_publication_receipt::failed_before_publication(
              request, actual_prior, std::move(storage_format),
              std::move(evidence));

        case state_publication_outcome::published_durability_unconfirmed:
          if (durability != state_publication_durability::unconfirmed ||
              atomicity == state_storage_atomicity_boundary::none)
            fail(state_publication_codec_error_code::invalid_value,
                 "unconfirmed receipt has invalid durability or atomicity");
          return state_publication_receipt::published_but_durability_unconfirmed(
              request, actual_prior, require_projected(),
              std::move(storage_format), atomicity, std::move(evidence));

        case state_publication_outcome::indeterminate:
          if (durability != state_publication_durability::indeterminate ||
              atomicity == state_storage_atomicity_boundary::none)
            fail(state_publication_codec_error_code::invalid_value,
                 "indeterminate receipt has invalid durability or atomicity");
          return state_publication_receipt::indeterminate(
              request, actual_prior, std::move(resulting),
              std::move(storage_format), atomicity, std::move(evidence));
      }
      fail(state_publication_codec_error_code::invalid_value,
           "invalid publication receipt outcome");
    }();

    if (result.identity() != encoded_identity)
      fail(state_publication_codec_error_code::identity_mismatch,
           "publication receipt identity does not match reconstructed evidence");
    require_canonical_receipt(encoding, result, format);
    return result;
  }
  catch (const state_publication_codec_error&)
  {
    throw;
  }
  catch (const error& failure)
  {
    fail(state_publication_codec_error_code::invalid_value,
         std::string("invalid publication receipt encoding: ") + failure.what());
  }
}

} // namespace pkgstate
