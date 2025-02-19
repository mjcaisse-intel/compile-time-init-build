#include <log/fmt/logger.hpp>
#include <msg/callback.hpp>
#include <msg/field.hpp>
#include <msg/message.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <iterator>
#include <string>

namespace {
using namespace msg;

bool dispatched = false;

using id_field = field<"id", std::uint32_t>::located<at{0_dw, 31_msb, 24_lsb}>;
using field1 = field<"f1", std::uint32_t>::located<at{0_dw, 15_msb, 0_lsb}>;
using field2 = field<"f2", std::uint32_t>::located<at{1_dw, 23_msb, 16_lsb}>;
using field3 = field<"f3", std::uint32_t>::located<at{1_dw, 15_msb, 0_lsb}>;

using msg_defn =
    message<"msg", id_field::WithRequired<0x80>, field1, field2, field3>;

constexpr auto id_match =
    msg::msg_matcher<msg_defn,
                     msg::equal_to_t<id_field, std::uint32_t, 0x80>>{};

std::string log_buffer{};
} // namespace

template <>
inline auto logging::config<> =
    logging::fmt::config{std::back_inserter(log_buffer)};

TEST_CASE("callback matches message", "[handler]") {
    auto callback = msg::callback<"cb">(id_match, [] {});
    auto const msg_match = std::array{0x8000ba11u, 0x0042d00du};
    CHECK(callback.is_match(msg_match));
}

TEST_CASE("callback matches message (alternative range)", "[handler]") {
    auto callback = msg::callback<"cb">(id_match, [] {});
    auto const msg_match = std::array<std::uint8_t, 8>{0x11, 0xba, 0x00, 0x80,
                                                       0x0d, 0xd0, 0x42, 0x00};
    CHECK(callback.is_match(msg_match));
}

TEST_CASE("callback matches message (typed message)", "[handler]") {
    auto callback = msg::callback<"cb">(id_match, [] {});
    auto const msg_match = msg::owning<msg_defn>{"id"_field = 0x80};
    CHECK(callback.is_match(msg_match));
}

TEST_CASE("callback logs mismatch (raw)", "[handler]") {
    auto callback = msg::callback<"cb">(id_match, [] {});
    auto const msg_nomatch = std::array{0x8100ba11u, 0x0042d00du};
    CHECK(not callback.is_match(msg_nomatch));

    log_buffer.clear();
    callback.log_mismatch(msg_nomatch);
    CAPTURE(log_buffer);
    CHECK(log_buffer.find("cb - F:(id (0x81) == 0x80)") != std::string::npos);
}

TEST_CASE("callback logs mismatch (typed)", "[handler]") {
    auto callback = msg::callback<"cb">(id_match, [] {});
    auto const msg_nomatch = msg::owning<msg_defn>{"id"_field = 0x81};
    CHECK(not callback.is_match(msg_nomatch));

    log_buffer.clear();
    callback.log_mismatch(msg_nomatch);
    CAPTURE(log_buffer);
    CHECK(log_buffer.find("cb - F:(id (0x81) == 0x80)") != std::string::npos);
}

TEST_CASE("callback handles message (raw)", "[handler]") {
    auto callback = msg::callback<"cb">(
        id_match, [](msg::const_view<msg_defn>) { dispatched = true; });
    auto const msg_match = std::array{0x8000ba11u, 0x0042d00du};

    dispatched = false;
    CHECK(callback.handle(msg_match));
    CHECK(dispatched);
}

namespace {
template <typename T>
using uint8_view =
    typename T::template view_t<typename T::access_t::template span_t<uint8_t>>;
template <typename T>
using const_uint8_view = typename T::template view_t<
    typename T::access_t::template span_t<uint8_t const>>;
} // namespace

TEST_CASE("callback handles message (custom raw format)", "[handler]") {
    auto callback = msg::callback<"cb">(
        id_match, [](const_uint8_view<msg_defn>) { dispatched = true; });
    auto const msg_match = std::array<uint8_t, 32>{0x00u, 0xbau, 0x11u, 0x80u,
                                                   0x00u, 0x42u, 0xd0u, 0x0du};

    dispatched = false;
    CHECK(callback.handle(msg_match));
    CHECK(dispatched);
}

TEST_CASE("callback handles message (typed)", "[handler]") {
    auto callback = msg::callback<"cb">(
        id_match, [](msg::const_view<msg_defn>) { dispatched = true; });
    auto const msg_match = msg::owning<msg_defn>{"id"_field = 0x80};

    dispatched = false;
    CHECK(callback.handle(msg_match));
    CHECK(dispatched);
}

TEST_CASE("callback logs match", "[handler]") {
    auto callback = msg::callback<"cb">(
        id_match, [](msg::const_view<msg_defn>) { dispatched = true; });
    auto const msg_match = std::array{0x8000ba11u, 0x0042d00du};

    log_buffer.clear();
    CHECK(callback.handle(msg_match));
    CAPTURE(log_buffer);
    CHECK(log_buffer.find("matched [cb], because [id == 0x80]") !=
          std::string::npos);
}
