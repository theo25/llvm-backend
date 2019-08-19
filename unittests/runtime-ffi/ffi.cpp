#include<boost/test/unit_test.hpp>
#include<gmp.h>
#include<mpfr.h>
#include<cstdint>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<vector>
#include<dlfcn.h>

#include "runtime/header.h"
#include "runtime/header.hpp"
#include "runtime/alloc.h"

#define KCHAR char
#define TYPETAG(type) "Lbl'Hash'" #type "{}"
extern "C" {

  struct point {
    int x;
    int y;
  };

  struct point2 {
    struct point p;
  };

#define NUM_SYMBOLS 6
  const char * symbols[NUM_SYMBOLS] = {TYPETAG(struct), TYPETAG(uint), TYPETAG(sint), TYPETAG(pointer), "inj{SortBytes{}}", "inj{SortFFIType{}}"};

  char * getTerminatedString(string * str);

  uint32_t getTagForSymbolName(const char *s) {
    for (int i = 0; i < NUM_SYMBOLS; i++) {
      if (0 == strcmp(symbols[i], s)) {
        return i + 1;
      }
    }

    return 0;
  }

  struct blockheader getBlockHeaderForSymbol(uint32_t tag) {
    return blockheader {tag};
  }

  void add_hash64(void*, uint64_t) {}

  uint64_t hash_k(block * kitem) {
    return (uint64_t) kitem;
  }

  bool during_gc() {
    return false;
  }

  void printConfigurationInternal(FILE *file, block *subject, const char *sort, bool) {}

  bool hook_KEQUAL_eq(block * lhs, block * rhs) {
    return lhs->h.hdr == rhs->h.hdr;
  }

  mpz_ptr hook_FFI_address(string * fn);
  string * hook_FFI_call(mpz_t addr, List * args, List * types, block * ret);
  string * hook_FFI_call_variadic(mpz_t addr, List * args, List * fixtypes, List * vartypes, block * ret);

  mpz_ptr hook_FFI_bytes_address(string * bytes);
  block * hook_FFI_free(block * kitem);
  block * hook_FFI_bytes_ref(string * bytes);
  string * hook_FFI_alloc(block * kitem, mpz_t size);
  bool hook_FFI_allocated(block * kitem);

  string * makeString(const KCHAR *, int64_t len = -1);

  List hook_LIST_element(block * value);
  List hook_LIST_concat(List * l1, List * l2);
  List hook_LIST_unit();
  size_t hook_LIST_size_long(List * l);
  block * hook_LIST_get_long(List * l, ssize_t idx);

  mpz_ptr move_int(mpz_t i) {
    mpz_ptr result = (mpz_ptr)malloc(sizeof(__mpz_struct));
    *result = *i;
    return result;
  }

  floating *move_float(floating *i) {
    floating *result = (floating *)malloc(sizeof(floating));
    *result = *i;
    return result;
  }

  block D1 = {{1}};
  block * DUMMY1 = &D1;
}

BOOST_AUTO_TEST_SUITE(FfiTest)

BOOST_AUTO_TEST_CASE(address) {
  string * fn = makeString("timesTwo");
  mpz_ptr addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("utimesTwo");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("times");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("increaseX");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("timesPoint");
  addr = hook_FFI_address(fn);
  BOOST_CHECK(0 < mpz_cmp_ui(addr, 0));

  fn = makeString("fakeFunction");
  addr = hook_FFI_address(fn);
  BOOST_CHECK_EQUAL(0, mpz_cmp_ui(addr, 0));
}

BOOST_AUTO_TEST_CASE(call) {
  /* int timesTwo(int x) */
  int x = -3;
  string * xargstr = makeString((char *) &x, sizeof(int)); 

  block * xarg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  xarg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(xarg->children, &xargstr, sizeof(string *));

  List args = hook_LIST_element(xarg);
  block * type_sint = (block *)((((uint64_t)getTagForSymbolName(TYPETAG(sint))) << 32) | 1);

  block * argtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  argtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}}"));
  memcpy(argtype->children, &type_sint, sizeof(block *));

  List types = hook_LIST_element(argtype);

  string * fn = makeString("timesTwo");
  mpz_ptr addr = hook_FFI_address(fn);

  string * bytes = hook_FFI_call(addr, &args, &types, type_sint);

  BOOST_CHECK(bytes != NULL);

  int ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * 2);

  /* unsigned int utimesTwo(unsigned int x) */
  x = 4;
  xargstr = makeString((char *) &x, sizeof(int)); 

  memcpy(xarg->children, &xargstr, sizeof(string *));

  args = hook_LIST_element(xarg);
  block * type_uint = (block *)((((uint64_t)getTagForSymbolName(TYPETAG(uint))) << 32) | 1);
  memcpy(argtype->children, &type_uint, sizeof(block *));

  types = hook_LIST_element(argtype);

  fn = makeString("utimesTwo");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_uint);

  BOOST_CHECK(bytes != NULL);

  ret = *(unsigned int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * 2);

  /* int times(int x, int y) */
  int y = 4;
  string * yargstr = makeString((char *) &y, sizeof(int));

  memcpy(argtype->children, &type_sint, sizeof(block *));

  block * yarg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  yarg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(yarg->children, &yargstr, sizeof(string *));

  List yargs = hook_LIST_element(yarg);

  args = hook_LIST_concat(&args, &yargs);
  types = hook_LIST_concat(&types, &types);

  fn = makeString("times");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x * y);

  /* struct point constructPoint(int x, int y) */
  block * structType = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structType->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(TYPETAG(struct)));

  List * structFields = static_cast<List *>(koreAlloc(sizeof(List)));
  List tmp = hook_LIST_element(argtype);
  tmp = hook_LIST_concat(&tmp, &tmp);
  memcpy(structFields, &tmp, sizeof(List));

  memcpy(structType->children, &structFields, sizeof(List *));

  fn = makeString("constructPoint");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, structType);

  struct point p = *(struct point *) bytes->data;
  BOOST_CHECK_EQUAL(p.x, x);
  BOOST_CHECK_EQUAL(p.y, y);

  /* int getX(void) */
  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, 1);

  /* void increaseX(void) */
  fn = makeString("increaseX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);

  /* int getX(void) */
  fn = makeString("getX");
  addr = hook_FFI_address(fn);
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, 2);

  /* struct point {
   *  int x;
   *  int y;
   * }
   *
   * int timesPoint(struct point p) */
  p = {.x = 2, .y = 5};
  string * pargstr = makeString((char *) &p, sizeof(struct point));

  block * parg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  parg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(parg->children, &pargstr, sizeof(string *));

  args = hook_LIST_element(parg);

  block * new_argtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  new_argtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}}"));
  memcpy(new_argtype->children, &structType, sizeof(block *));
  types = hook_LIST_element(new_argtype);

  fn = makeString("timesPoint");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p.x * p.y);

  /* struct point2 {
   *  struct point p;
   * }
   *
   * int timesPoint2(struct point2 p) */
  struct point2 p2 = {.p = p};
  string * pargstr2 = makeString((char *) &p2, sizeof(struct point2));

  memcpy(parg->children, &pargstr2, sizeof(string *));

  args = hook_LIST_element(parg);

  block * structType2 = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structType2->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName(TYPETAG(struct)));

  block * structArgType = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  structArgType->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}}"));
  memcpy(structArgType->children, &structType, sizeof(block *));

  List * structFields2 = static_cast<List *>(koreAlloc(sizeof(List)));
  List tmp2 = hook_LIST_element(structArgType);
  memcpy(structFields2, &tmp2, sizeof(List));

  memcpy(structType2->children, &structFields2, sizeof(List *));
  
  memcpy(new_argtype->children, &structType2, sizeof(block *));
  types = hook_LIST_element(new_argtype);

  fn = makeString("timesPoint2");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p2.p.x * p2.p.y);

  /* Make sure there is no double free */
  bytes = hook_FFI_call(addr, &args, &types, type_sint);
  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, p2.p.x * p2.p.y);

  /* int pointerTest(int * x) */
  x = 2;
  mpz_t s1;
  mpz_init_set_ui(s1, sizeof(int));
  string * b1 = hook_FFI_alloc(DUMMY1, s1);
  memcpy(b1->data, &x, sizeof(int));

  mpz_ptr addr1 = hook_FFI_bytes_address(b1);
  uintptr_t address1 = mpz_get_ui(addr1);

  string * ptrargstr = makeString((char *) &address1, sizeof(uintptr_t *));

  block * ptrarg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  ptrarg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(ptrarg->children, &ptrargstr, sizeof(string *));

  args = hook_LIST_element(ptrarg);
  block * type_pointer = (block *)((((uint64_t)getTagForSymbolName(TYPETAG(pointer))) << 32) | 1);

  memcpy(argtype->children, &type_pointer, sizeof(block *));

  types = hook_LIST_element(argtype);

  fn = makeString("pointerTest");
  addr = hook_FFI_address(fn);

  bytes = hook_FFI_call(addr, &args, &types, type_sint);

  hook_FFI_free(DUMMY1);

  BOOST_CHECK(bytes != NULL);

  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, x);
}

BOOST_AUTO_TEST_CASE(call_variadic) {
  /* int addInts(int x, ...) */
  int n = 1;
  string * nargstr = makeString((char *) &n, sizeof(int)); 

  block * narg = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  narg->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(narg->children, &nargstr, sizeof(string *));

  List args = hook_LIST_element(narg);

  int arg1 = 1;
  string * arg1str = makeString((char *) &arg1, sizeof(int)); 

  block * arg1block = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  arg1block->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(arg1block->children, &arg1str, sizeof(string *));

  List arg1list = hook_LIST_element(arg1block);

  args = hook_LIST_concat(&args, &arg1list);

  block * type_sint = (block *)((((uint64_t)getTagForSymbolName(TYPETAG(sint))) << 32) | 1);

  block * fixargtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  fixargtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}}"));
  memcpy(fixargtype->children, &type_sint, sizeof(block *));

  block * varargtype = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(block *)));
  varargtype->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortFFIType{}}"));
  memcpy(varargtype->children, &type_sint, sizeof(block *));

  List fixtypes = hook_LIST_element(fixargtype);
  List vartypes = hook_LIST_element(varargtype);

  string * fn = makeString("addInts");
  mpz_ptr addr = hook_FFI_address(fn);

  string * bytes = hook_FFI_call_variadic(addr, &args, &fixtypes, &vartypes, type_sint);

  BOOST_CHECK(bytes != NULL);

  int ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, arg1);

  /* addInts with 2 var args */
  n = 2;
  nargstr = makeString((char *) &n, sizeof(int));
  memcpy(narg->children, &nargstr, sizeof(string *));
  args = hook_LIST_element(narg);

  arg1 = 20;
  arg1str = makeString((char *) &arg1, sizeof(int));
  memcpy(arg1block->children, &arg1str, sizeof(string *));
  arg1list = hook_LIST_element(arg1block);
  args = hook_LIST_concat(&args, &arg1list);

  int arg2 = 15;
  string * arg2str = makeString((char *) &arg2, sizeof(int));

  block * arg2block = static_cast<block *>(koreAlloc(sizeof(block) + sizeof(string *)));
  arg2block->h = getBlockHeaderForSymbol((uint64_t)getTagForSymbolName("inj{SortBytes{}}"));
  memcpy(arg2block->children, &arg2str, sizeof(string *));

  List arg2list = hook_LIST_element(arg2block);

  args = hook_LIST_concat(&args, &arg2list);

  vartypes = hook_LIST_element(varargtype);
  vartypes = hook_LIST_concat(&vartypes, &vartypes);

  bytes = hook_FFI_call_variadic(addr, &args, &fixtypes, &vartypes, type_sint);

  BOOST_CHECK(bytes != NULL);

  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, arg1 + arg2);

  /* addInts with 0 var args */
  n = 0;
  nargstr = makeString((char *) &n, sizeof(int));
  memcpy(narg->children, &nargstr, sizeof(string *));
  args = hook_LIST_element(narg);

  vartypes = hook_LIST_unit();

  bytes = hook_FFI_call_variadic(addr, &args, &fixtypes, &vartypes, type_sint);

  BOOST_CHECK(bytes != NULL);

  ret = *(int *) bytes->data;

  BOOST_CHECK_EQUAL(ret, 0);
}

BOOST_AUTO_TEST_CASE(alloc) {
  mpz_t s1;
  mpz_init_set_ui(s1, 1);

  string * b1 = hook_FFI_alloc(DUMMY1, s1);
  BOOST_CHECK(0 != b1);

  string * b2 = hook_FFI_alloc(DUMMY1, s1);
  BOOST_CHECK_EQUAL(b1, b2);
}

BOOST_AUTO_TEST_CASE(free) {
  mpz_t s1;
  mpz_init_set_ui(s1, 1);

  hook_FFI_alloc(DUMMY1, s1);
  hook_FFI_free(DUMMY1);
}

BOOST_AUTO_TEST_CASE(bytes_ref) {
  mpz_t s1;
  mpz_init_set_ui(s1, 1);

  string * b1 = hook_FFI_alloc(DUMMY1, s1);

  block * i2 = hook_FFI_bytes_ref(b1);

  BOOST_CHECK_EQUAL(DUMMY1, i2);
}

BOOST_AUTO_TEST_CASE(allocated) {
  mpz_t s1;
  mpz_init_set_ui(s1, 1);

  hook_FFI_alloc(DUMMY1, s1);
  BOOST_CHECK_EQUAL(true, hook_FFI_allocated(DUMMY1));

  block * i2 = (block *) 2;
  BOOST_CHECK_EQUAL(false, hook_FFI_allocated(i2));
}

BOOST_AUTO_TEST_SUITE_END()
