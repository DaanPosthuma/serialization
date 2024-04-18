#pragma once

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>

namespace serialization {

  using Stream = std::vector<char>;

  // fwd declarations as these are used in concrete serialization functions

  template <class T>
  void serialize(T const& x, Stream& stream);

  template <class T>
  T deserialize(Stream& stream);

  namespace detail {

    template <class T>
    struct serializer;

    // concepts

    template<class T>
    concept trivial_type = std::is_trivial<T>::value;

    template<template<typename...> class Template, typename Class>
    struct is_instantiation : std::false_type {};

    template<template<typename...> class Template, typename... Args>
    struct is_instantiation<Template, Template<Args...>> : std::true_type {};

    template<typename Class, template<typename...> class Template>
    concept is_instantiation_of = is_instantiation<Template, Class>::value;

    template<typename T>
    concept map_type = is_instantiation_of<T, std::map> || is_instantiation_of<T, std::unordered_map>;

    template<typename T>
    concept vector_type = is_instantiation_of<T, std::vector>;

    // (de)serialize objects

    template <class T>
    inline void serialize_object(T const& x, Stream& stream) {
      serializer<T>::serialize(x, stream);
    }

    template <class T>
    inline T deserialize_object(Stream& stream) {
      return serializer<T>::deserialize(stream);
    }

    template <trivial_type T>
    inline static void serialize_object(T const& x, Stream& stream) {
      auto const& data = reinterpret_cast<char const*>(&x);
      stream.insert(stream.end(), data, data + sizeof(T));
    }

    template <trivial_type T>
    inline T deserialize_object(Stream& stream) {
      T out;
      std::ranges::copy_n(stream.begin(), sizeof(T), reinterpret_cast<char*>(&out));
      stream.erase(stream.begin(), stream.begin() + sizeof(T));
      return out;
    }

    template <>
    inline void serialize_object<std::string>(std::string const& str, Stream& stream) {
      serialize_object<size_t>(str.size(), stream);
      stream.insert(stream.end(), str.data(), str.data() + str.size());
    }

    template <>
    inline std::string deserialize_object<std::string>(Stream& stream) {
      auto const len = serialization::deserialize<size_t>(stream);
      std::string out(stream.begin(), stream.begin() + len);
      stream.erase(stream.begin(), stream.begin() + len);
      return out;
    }

    // (de)serialize vectors

    template <class T>
    inline void serialize_vector(std::vector<T> const& vec, Stream& stream) {
      serialize_object<size_t>(vec.size(), stream);
      for (T const& el : vec) {
        serialization::serialize(el, stream);
      }
    }

    template <trivial_type T>
    inline void serialize_vector(std::vector<T> const& vec, Stream& stream) {
      serialize_object<size_t>(vec.size(), stream);
      auto const& data = reinterpret_cast<char const*>(vec.data());
      stream.insert(stream.end(), data, data + sizeof(T) * vec.size());
    }

    template <>
    inline void serialize_vector<bool>(std::vector<bool> const& vec, Stream& stream) {
      serialize_object<size_t>(vec.size(), stream);
      for (bool b : vec) {
        serialize(b, stream);
      }
    }

    template <class T>
    inline std::vector<T> deserialize_vector(Stream& stream) {
      auto const num = serialization::deserialize<size_t>(stream);
      auto vec = std::vector<T>();
      for (int i = 0; i != num; ++i) {
        vec.push_back(serialization::deserialize<T>(stream));
      }
      return vec;
    }

    template <trivial_type T>
    inline std::vector<T> deserialize_vector(Stream& stream) {
      auto const num = serialization::deserialize<size_t>(stream);
      auto vec = std::vector<T>(num);
      std::ranges::copy_n(stream.begin(), num * sizeof(T), reinterpret_cast<char*>(vec.data()));
      stream.erase(stream.begin(), stream.begin() + num * sizeof(T));
      return vec;
    }

    template <>
    inline std::vector<bool> deserialize_vector<bool>(Stream& stream) {
      auto const num = serialization::deserialize<size_t>(stream);
      auto vec = std::vector<bool>();
      for (int i = 0; i != num; ++i) {
        vec.push_back(serialization::deserialize<bool>(stream));
      }
      return vec;
    }

    // (de)serialize maps

    template <class MapT>
    inline void serialize_map(MapT const& map, Stream& stream) {
      serialize_object<size_t>(map.size(), stream);
      for (auto const& [k, v] : map) {
        serialization::serialize(k, stream);
        serialization::serialize(v, stream);
      }
    }

    template <class MapT>
    inline MapT deserialize_map(Stream& stream) {
      auto const num = deserialize<size_t>(stream);
      auto map = MapT();
      for (int i = 0; i != num; ++i) {
        auto k = serialization::deserialize<typename MapT::key_type>(stream);
        auto v = serialization::deserialize<typename MapT::mapped_type>(stream);
        map.emplace(std::move(k), std::move(v));
      }
      return map;
    }

    // (de)serialize generic/vector/map specializations

    template <class T>
    void serialize(T const& x, Stream& stream) {
      serialize_object<T>(x, stream);
    }
    template <vector_type T>
    void serialize(T const& x, Stream& stream) {
      serialize_vector<typename T::value_type>(x, stream);
    }
    template <map_type T>
    void serialize(T const& map, Stream& stream) {
      serialize_map(map, stream);
    }

    template <class T>
    T deserialize(Stream& stream) {
      return deserialize_object<T>(stream);
    }
    template <vector_type T>
    T deserialize(Stream& stream) {
      return deserialize_vector<typename T::value_type>(stream);
    }
    template <map_type T>
    T deserialize(Stream& stream) {
      return deserialize_map<T>(stream);
    }

  }

  template <class T>
  void serialize(T const& x, Stream& stream) {
    detail::serialize<T>(x, stream);
  }

  template <class T>
  T deserialize(Stream& stream) {
    return detail::deserialize<T>(stream);
  }

}
