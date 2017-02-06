/*
MIT License

Copyright (c) 2017 Eren Okka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

namespace anisthesia {
namespace util {

template <class T>
class optional {
public:
  using value_type = T;

  optional() : has_value_(false) {}
  optional(const value_type& value) : has_value_(true), value_(value) {}

  ~optional() {}

  value_type& operator=(const value_type& value) {
    value_ = value;
    has_value_ = true;
    return value_;
  }

  operator bool() const {
    return has_value_;
  }

  bool has_value() const {
    return has_value_;
  }

  const value_type* operator->() const {
    return &value_;
  }

  const value_type& operator*() const {
    return value_;
  }

  const value_type& value() const {
    return value_;
  }

  const value_type& value_or(const value_type default_value) const {
    return has_value_ ? value_ : default_value;
  }

  void reset() {
    has_value_ = false;
  }

private:
  bool has_value_;
  value_type value_;
};

}  // namespace util
}  // namespace anisthesia
