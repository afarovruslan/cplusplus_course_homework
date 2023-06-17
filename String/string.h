#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>

class String {
 public:
  String(const char* c_string);
  String(size_t n, char symbol);
  String();
  String(const String& string);
  String(char symbol);
  ~String();
  String& operator=(String string);
  const char& operator[](size_t ind) const { return string_[ind]; }
  char& operator[](size_t ind) { return string_[ind]; };
  size_t length() const { return length_; };
  size_t capacity() const { return capacity_; };
  size_t size() const { return length_; };
  void push_back(char symbol);
  void pop_back() { --length_; };
  char& front() { return string_[0]; };
  const char& front() const { return string_[0]; };
  char& back() { return string_[length_ - 1]; };
  const char& back() const { return string_[length_ - 1]; };
  String& operator+=(const String& string);
  String& operator+=(char symbol);
  size_t find(const String& substring) const;
  size_t rfind(const String& substring) const;
  String substr(size_t start, size_t count) const;
  bool empty() const { return length_ == 0; };
  void clear();
  void shrink_to_fit();
  char* data() { return string_; };
  const char* data() const { return string_; };

 private:
  size_t length_;
  size_t capacity_;
  char* string_;

  void swap(String& string);
  void reserve(size_t new_size);
};

bool operator==(const String& string1, const String& string2);
bool operator!=(const String& string1, const String& string2);
bool operator<(const String& string1, const String& string2);
bool operator>(const String& string1, const String& string2);
bool operator<=(const String& string1, const String& string2);
bool operator>=(const String& string1, const String& string2);
String operator+(String string1, const String& string2);
std::ostream& operator<<(std::ostream& out, const String& string);
std::istream& operator>>(std::istream& flow_in, String& string);