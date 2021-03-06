#include "test.h"

TEST_START("functional test")
  // Basics: return + assign
  FUN_TEST("return 1", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  FUN_TEST("a = 32\nreturn a", {
    assert(HValue::As<HNumber>(result)->value() == 32);
  })

  FUN_TEST("a = b = 32\nreturn a", {
    assert(HValue::As<HNumber>(result)->value() == 32);
  })

  FUN_TEST("a = 32\nb = a\nreturn b", {
    assert(HValue::As<HNumber>(result)->value() == 32);
  })

  FUN_TEST("a = nil\nreturn a", {
    assert(result == NULL);
  })

  FUN_TEST("return 'abcdef'", {
    HString* str = HValue::As<HString>(result);
    str = str;
    assert(str->length() == 6);
    assert(strncmp(str->value(), "abcdef", str->length()) == 0);
  })

  // Boolean
  FUN_TEST("return true", {
    assert(HValue::As<HBoolean>(result)->is_true());
  })

  FUN_TEST("return false", {
    assert(HValue::As<HBoolean>(result)->is_false());
  })

  // Functions
  FUN_TEST("a() {}\nreturn a", {
    assert(HValue::As<HFunction>(result) != NULL);
  })

  FUN_TEST("a() { return 1 }\nreturn a()", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  FUN_TEST("a(b) { return b }\nreturn a(3) + a(4)", {
    assert(HValue::As<HNumber>(result)->value() == 7);
  })

  FUN_TEST("a(b) { return b }\nreturn a()", {
    assert(result == NULL);
  })

  FUN_TEST("b() {\nreturn 1\n}\na(c) {\nreturn c()\n}\nreturn a(b)", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  FUN_TEST("a(c) {\nreturn c()\n}\nreturn a(() {\nreturn 1\n})", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  // Context slots
  FUN_TEST("b = 13589\na() { scope b }\nreturn b", {
    assert(HValue::As<HNumber>(result)->value() == 13589);
  })

  FUN_TEST("a() { scope a, b\nb = 1234 }\nb = 13589\na()\nreturn b", {
    assert(HValue::As<HNumber>(result)->value() == 1234);
  })

  FUN_TEST("a() { a = 1\nreturn b() { scope a\nreturn a} }\nreturn a()()", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  });

  // Binary ops
  FUN_TEST("return 1 + 2 * 3 + 4 / 2 + (3 | 2) + (5 & 3) + (3 ^ 2)", {
    assert(HValue::As<HNumber>(result)->value() == 14);
  })

  FUN_TEST("a = 1\na = 1 - 1\nreturn a", {
    assert(HValue::As<HNumber>(result)->value() == 0);
  })

  FUN_TEST("a() { a = 1\nreturn b() { scope a\na = a + 1\nreturn a} }\n"
           "c = a()\nreturn c() + c() + c()", {
    assert(HValue::As<HNumber>(result)->value() == 9);
  });

  FUN_TEST("return ({}) + ({})", {
    assert(result == NULL);
  })

  // Unary ops
  FUN_TEST("a = 1\nreturn ++a", {
    assert(HValue::As<HNumber>(result)->value() == 2);
  })

  FUN_TEST("a = 1\nreturn a++ + a", {
    assert(HValue::As<HNumber>(result)->value() == 3);
  })

  // Objects
  FUN_TEST("return {}", {
    assert(result != NULL);
  })

  FUN_TEST("return { a : 1 }", {
    assert(result != NULL);
  })

  FUN_TEST("a = {a:1,b:2,c:3,d:4,e:5,f:6,g:7}\n"
           "return a.a + a.b + a.c + a.d + a.e + a.f + a.g", {
    assert(HValue::As<HNumber>(result)->value() == 28);
  })

  FUN_TEST("a = { a : { b : 1 } }\n"
           "a = { x: { y: a } }\n"
           "return a.x.y.a.b", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  // Rehash and growing
  FUN_TEST("a = {}\n"
           "a.a = a.b = a.c = a.d = a.e = a.f = a.g = a.h = 1\n"
           "return a.a + a.b + a.c + a.d + a.e + a.f + a.g + a.h", {
    assert(HValue::As<HNumber>(result)->value() == 8);
  })

  FUN_TEST("a = { a: 1, b: 2 }\nreturn a.c", {
    assert(result == NULL);
  })

  FUN_TEST("a = { a: 1, b: 2 }\nreturn a.c = (2 + a.a) + a.b", {
    assert(HValue::As<HNumber>(result)->value() == 5);
  })

  FUN_TEST("a = { a: { b: 2 } }\nreturn a.a.b", {
    assert(HValue::As<HNumber>(result)->value() == 2);
  })

  FUN_TEST("key() {\nreturn 'key'\n}\na = { key: 2 }\nreturn a[key()]", {
    assert(HValue::As<HNumber>(result)->value() == 2);
  })

  // Numeric keys
  FUN_TEST("a = { 1: 2, 2: 3}\nreturn a[1] + a[2] + a['1'] + a['2']", {
    assert(HValue::As<HNumber>(result)->value() == 10);
  });

  FUN_TEST("a = { 1.1: 2, 2.2: 3}\n"
           "return a[1.1] + a[2.2] + a['1.1'] + a['2.2']", {
    assert(HValue::As<HNumber>(result)->value() == 10);
  });

  // Arrays
  FUN_TEST("a = [ 1, 2, 3, 4 ]\nreturn a[0] + a[1] + a[2] + a[3]", {
    assert(HValue::As<HNumber>(result)->value() == 10);
  })

  FUN_TEST("a = [ 1, 2, 3, 4 ]\nreturn a.length", {
    assert(HValue::As<HNumber>(result)->value() == 4);
  })

  // Global lookup
  FUN_TEST("scope a\na = 1\nreturn a", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  // If
  FUN_TEST("if (true) {\n return 1\n} else {\nreturn 2\n}", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  FUN_TEST("if (false) {\n return 1\n} else {\nreturn 2\n}", {
    assert(HValue::As<HNumber>(result)->value() == 2);
  })

  FUN_TEST("if (1) {\n return 1\n} else {\nreturn 2\n}", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  FUN_TEST("if (0) {\n return 1\n} else {\nreturn 2\n}", {
    assert(HValue::As<HNumber>(result)->value() == 2);
  })

  FUN_TEST("if ('123') {\n return 1\n} else {\nreturn 2\n}", {
    assert(HValue::As<HNumber>(result)->value() == 1);
  })

  FUN_TEST("if ('') {\n return 1\n} else {\nreturn 2\n}", {
    assert(HValue::As<HNumber>(result)->value() == 2);
  })

  FUN_TEST("if (nil) {\n return 1\n} else {\nreturn 2\n}", {
    assert(HValue::As<HNumber>(result)->value() == 2);
  })

  // While
  FUN_TEST("i = 10\nj = 0\n"
           "while (i--) {scope i, j\nj = j + 1\n}\n"
           "return j", {
    assert(HValue::As<HNumber>(result)->value() == 10);
  })

  // Runtime errors
  FUN_TEST("() {}", {
    assert(s.CaughtException() == true);
  })

  FUN_TEST("++1", {
    assert(s.CaughtException() == true);
  })
TEST_END("functional test")
