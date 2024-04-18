#include "serialization.h"
#include <catch2/catch_test_macros.hpp>

namespace {
  struct X {
    explicit X(int x) : x(x) {}
    int x;
  };

  inline bool operator==(X const& lhs, X const& rhs) noexcept {
    return lhs.x == rhs.x;
  }

  inline std::ostream& operator<<(std::ostream& ostr, X const& x) noexcept {
    ostr << "X(" << x.x << ")";
    return ostr;
  }
}

namespace serialization::detail {
  template <>
  struct serializer<X> {
    static void serialize(X const& x, std::vector<char>& stream) {
      serialization::serialize(x.x, stream);
    }

    static X deserialize(std::vector<char>& stream) {
      return X(serialization::deserialize<int>(stream));
    }
  };
}

namespace {

  using namespace std::string_literals;

  void round_trip(auto obj) {
    std::vector<char> stream;
    serialization::serialize(obj, stream);
    REQUIRE(serialization::deserialize<decltype(obj)>(stream) == obj);
  }

  TEST_CASE("int round trip") {
    round_trip(3);
  }

  TEST_CASE("double round trip") {
    round_trip(3.0);
  }

  TEST_CASE("string round trip") {
    round_trip("test"s);
  }

  TEST_CASE("non-trivial type round trip") {
    round_trip("test"s);
  }

  TEST_CASE("Custom object round trip") {
    round_trip(X(3));
  }

  TEST_CASE("empty vector round trip") {
    round_trip(std::vector<char>());
  }

  TEST_CASE("vector of trivial types round trip") {
    round_trip(std::vector{ 1,2,3 });
  }

  TEST_CASE("vector of strings round trip") {
    round_trip(std::vector{ "one"s, "two"s, "three"s });
  }

  TEST_CASE("vector of booleans round trip") {
    round_trip(std::vector{ true, false, true, true, false, false });
  }

  TEST_CASE("vector of vectors round trip") {
    round_trip(std::vector{ std::vector{ 1,2,3 }, std::vector{ 4,5 } });
  }

  TEST_CASE("unordered_map round trip") {
    round_trip(std::unordered_map<std::string, int>{ {"one"s, 1}, { "two"s, 2 }, { "three"s, 3 } });
  }

  TEST_CASE("map string to vector round trip") {
    round_trip(std::map<std::string, std::vector<int>>{{"one"s, std::vector{ 1,2,3 }}, { "two"s, std::vector{4,5} }, { "three"s, std::vector{6,7,8} } });
  }

  TEST_CASE("multiple objects") {
    std::vector<char> stream;

    for (int i = 0; i != 10; ++i) {
      serialization::serialize(1, stream);
      serialization::serialize(5.0, stream);
      serialization::serialize(std::vector{ 1,2,3 }, stream);
      serialization::serialize("test"s, stream);
    }
    
    for (int i = 0; i != 10; ++i) {
      REQUIRE(serialization::deserialize<int>(stream) == 1);
      REQUIRE(serialization::deserialize<double>(stream) == 5.0);
      REQUIRE(serialization::deserialize<std::vector<int>>(stream) == std::vector{ 1,2,3 });
      REQUIRE(serialization::deserialize<std::string>(stream) == "test"s);
    }

  }

}
