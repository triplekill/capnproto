// Copyright (c) 2013, Kenton Varda <temporal@gmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#define CAPNPROTO_PRIVATE
#include "test-import.capnp.h"
#include "message.h"
#include "logging.h"
#include <gtest/gtest.h>
#include "test-util.h"

namespace capnproto {
namespace internal {
namespace {

template <typename T, typename U>
void checkList(T reader, std::initializer_list<U> expected) {
  ASSERT_EQ(expected.size(), reader.size());
  for (uint i = 0; i < expected.size(); i++) {
    EXPECT_EQ(expected.begin()[i], reader[i]);
  }
}

template <typename T>
void checkList(T reader, std::initializer_list<float> expected) {
  ASSERT_EQ(expected.size(), reader.size());
  for (uint i = 0; i < expected.size(); i++) {
    EXPECT_FLOAT_EQ(expected.begin()[i], reader[i]);
  }
}

template <typename T>
void checkList(T reader, std::initializer_list<double> expected) {
  ASSERT_EQ(expected.size(), reader.size());
  for (uint i = 0; i < expected.size(); i++) {
    EXPECT_DOUBLE_EQ(expected.begin()[i], reader[i]);
  }
}

TEST(Encoding, AllTypes) {
  MallocMessageBuilder builder;

  initTestMessage(builder.initRoot<TestAllTypes>());
  checkTestMessage(builder.getRoot<TestAllTypes>());
  checkTestMessage(builder.getRoot<TestAllTypes>().asReader());

  SegmentArrayMessageReader reader(builder.getSegmentsForOutput());

  checkTestMessage(reader.getRoot<TestAllTypes>());

  ASSERT_EQ(1u, builder.getSegmentsForOutput().size());

  checkTestMessage(readMessageTrusted<TestAllTypes>(builder.getSegmentsForOutput()[0].begin()));
}

TEST(Encoding, AllTypesMultiSegment) {
  MallocMessageBuilder builder(0, AllocationStrategy::FIXED_SIZE);

  initTestMessage(builder.initRoot<TestAllTypes>());
  checkTestMessage(builder.getRoot<TestAllTypes>());
  checkTestMessage(builder.getRoot<TestAllTypes>().asReader());

  SegmentArrayMessageReader reader(builder.getSegmentsForOutput());

  checkTestMessage(reader.getRoot<TestAllTypes>());
}

TEST(Encoding, Defaults) {
  AlignedData<1> nullRoot = {{0, 0, 0, 0, 0, 0, 0, 0}};
  ArrayPtr<const word> segments[1] = {arrayPtr(nullRoot.words, 1)};
  SegmentArrayMessageReader reader(arrayPtr(segments, 1));

  checkTestMessage(reader.getRoot<TestDefaults>());
  checkTestMessage(readMessageTrusted<TestDefaults>(nullRoot.words));
}

TEST(Encoding, DefaultInitialization) {
  MallocMessageBuilder builder;

  checkTestMessage(builder.getRoot<TestDefaults>());  // first pass initializes to defaults
  checkTestMessage(builder.getRoot<TestDefaults>().asReader());

  checkTestMessage(builder.getRoot<TestDefaults>());  // second pass just reads the initialized structure
  checkTestMessage(builder.getRoot<TestDefaults>().asReader());

  SegmentArrayMessageReader reader(builder.getSegmentsForOutput());

  checkTestMessage(reader.getRoot<TestDefaults>());
}

TEST(Encoding, DefaultInitializationMultiSegment) {
  MallocMessageBuilder builder(0, AllocationStrategy::FIXED_SIZE);

  // first pass initializes to defaults
  checkTestMessage(builder.getRoot<TestDefaults>());
  checkTestMessage(builder.getRoot<TestDefaults>().asReader());

  // second pass just reads the initialized structure
  checkTestMessage(builder.getRoot<TestDefaults>());
  checkTestMessage(builder.getRoot<TestDefaults>().asReader());

  SegmentArrayMessageReader reader(builder.getSegmentsForOutput());

  checkTestMessage(reader.getRoot<TestDefaults>());
}

TEST(Encoding, DefaultsFromEmptyMessage) {
  AlignedData<1> emptyMessage = {{0, 0, 0, 0, 0, 0, 0, 0}};

  ArrayPtr<const word> segments[1] = {arrayPtr(emptyMessage.words, 1)};
  SegmentArrayMessageReader reader(arrayPtr(segments, 1));

  checkTestMessage(reader.getRoot<TestDefaults>());
  checkTestMessage(readMessageTrusted<TestDefaults>(emptyMessage.words));
}

TEST(Encoding, GenericObjects) {
  MallocMessageBuilder builder;
  auto root = builder.getRoot<test::TestObject>();

  initTestMessage(root.initObjectField<TestAllTypes>());
  checkTestMessage(root.getObjectField<TestAllTypes>());
  checkTestMessage(root.asReader().getObjectField<TestAllTypes>());

  root.setObjectField<Text>("foo");
  EXPECT_EQ("foo", root.getObjectField<Text>());
  EXPECT_EQ("foo", root.asReader().getObjectField<Text>());

  root.setObjectField<Data>("foo");
  EXPECT_EQ("foo", root.getObjectField<Data>());
  EXPECT_EQ("foo", root.asReader().getObjectField<Data>());

  {
    {
      List<uint32_t>::Builder list = root.initObjectField<List<uint32_t>>(3);
      ASSERT_EQ(3u, list.size());
      list.copyFrom({123, 456, 789});
    }

    {
      List<uint32_t>::Builder list = root.getObjectField<List<uint32_t>>();
      ASSERT_EQ(3u, list.size());
      EXPECT_EQ(123u, list[0]);
      EXPECT_EQ(456u, list[1]);
      EXPECT_EQ(789u, list[2]);
    }

    {
      List<uint32_t>::Reader list = root.asReader().getObjectField<List<uint32_t>>();
      ASSERT_EQ(3u, list.size());
      EXPECT_EQ(123u, list[0]);
      EXPECT_EQ(456u, list[1]);
      EXPECT_EQ(789u, list[2]);
    }
  }

  {
    {
      List<Text>::Builder list = root.initObjectField<List<Text>>(2);
      ASSERT_EQ(2u, list.size());
      list.copyFrom({"foo", "bar"});
    }

    {
      List<Text>::Builder list = root.getObjectField<List<Text>>();
      ASSERT_EQ(2u, list.size());
      EXPECT_EQ("foo", list[0]);
      EXPECT_EQ("bar", list[1]);
    }

    {
      List<Text>::Reader list = root.asReader().getObjectField<List<Text>>();
      ASSERT_EQ(2u, list.size());
      EXPECT_EQ("foo", list[0]);
      EXPECT_EQ("bar", list[1]);
    }
  }

  {
    {
      List<TestAllTypes>::Builder list = root.initObjectField<List<TestAllTypes>>(2);
      ASSERT_EQ(2u, list.size());
      initTestMessage(list[0]);
    }

    {
      List<TestAllTypes>::Builder list = root.getObjectField<List<TestAllTypes>>();
      ASSERT_EQ(2u, list.size());
      checkTestMessage(list[0]);
      checkTestMessageAllZero(list[1]);
    }

    {
      List<TestAllTypes>::Reader list = root.asReader().getObjectField<List<TestAllTypes>>();
      ASSERT_EQ(2u, list.size());
      checkTestMessage(list[0]);
      checkTestMessageAllZero(list[1]);
    }
  }
}

#ifdef NDEBUG
#define EXPECT_DEBUG_ANY_THROW(EXP)
#else
#define EXPECT_DEBUG_ANY_THROW EXPECT_ANY_THROW
#endif

TEST(Encoding, Unions) {
  MallocMessageBuilder builder;
  TestUnion::Builder root = builder.getRoot<TestUnion>();

  EXPECT_EQ(TestUnion::Union0::U0F0S0, root.getUnion0().which());
  EXPECT_EQ(Void::VOID, root.getUnion0().getU0f0s0());
  EXPECT_DEBUG_ANY_THROW(root.getUnion0().getU0f0s1());

  root.getUnion0().setU0f0s1(true);
  EXPECT_EQ(TestUnion::Union0::U0F0S1, root.getUnion0().which());
  EXPECT_TRUE(root.getUnion0().getU0f0s1());
  EXPECT_DEBUG_ANY_THROW(root.getUnion0().getU0f0s0());

  root.getUnion0().setU0f0s8(123);
  EXPECT_EQ(TestUnion::Union0::U0F0S8, root.getUnion0().which());
  EXPECT_EQ(123, root.getUnion0().getU0f0s8());
  EXPECT_DEBUG_ANY_THROW(root.getUnion0().getU0f0s1());
}

struct UnionState {
  uint discriminants[4];
  int dataOffset;

  UnionState(std::initializer_list<uint> discriminants, int dataOffset)
      : dataOffset(dataOffset) {
    memcpy(this->discriminants, discriminants.begin(), sizeof(discriminants));
  }

  bool operator==(const UnionState& other) const {
    for (uint i = 0; i < 4; i++) {
      if (discriminants[i] != other.discriminants[i]) {
        return false;
      }
    }

    return dataOffset == other.dataOffset;
  }
};

std::ostream& operator<<(std::ostream& os, const UnionState& us) {
  os << "UnionState({";

  for (uint i = 0; i < 4; i++) {
    if (i > 0) os << ", ";
    os << us.discriminants[i];
  }

  return os << "}, " << us.dataOffset << ")";
}

template <typename StructType, typename Func>
UnionState initUnion(Func&& initializer) {
  // Use the given setter to initialize the given union field and then return a struct indicating
  // the location of the data that was written as well as the values of the four union
  // discriminants.

  MallocMessageBuilder builder;
  initializer(builder.getRoot<StructType>());
  ArrayPtr<const word> segment = builder.getSegmentsForOutput()[0];

  CHECK(segment.size() > 2, segment.size());

  // Find the offset of the first set bit after the union discriminants.
  int offset = 0;
  for (const uint8_t* p = reinterpret_cast<const uint8_t*>(segment.begin() + 2);
       p < reinterpret_cast<const uint8_t*>(segment.end()); p++) {
    if (*p != 0) {
      uint8_t bits = *p;
      while ((bits & 1) == 0) {
        ++offset;
        bits >>= 1;
      }
      goto found;
    }
    offset += 8;
  }
  offset = -1;

found:
  const uint8_t* discriminants = reinterpret_cast<const uint8_t*>(segment.begin() + 1);
  return UnionState({discriminants[0], discriminants[2], discriminants[4], discriminants[6]},
                    offset);
}

TEST(Encoding, UnionLayout) {
#define INIT_UNION(setter) \
  initUnion<TestUnion>([](TestUnion::Builder b) {b.setter;})

  EXPECT_EQ(UnionState({ 0,0,0,0},  -1), INIT_UNION(getUnion0().setU0f0s0(Void::VOID)));
  EXPECT_EQ(UnionState({ 1,0,0,0},   0), INIT_UNION(getUnion0().setU0f0s1(1)));
  EXPECT_EQ(UnionState({ 2,0,0,0},   0), INIT_UNION(getUnion0().setU0f0s8(1)));
  EXPECT_EQ(UnionState({ 3,0,0,0},   0), INIT_UNION(getUnion0().setU0f0s16(1)));
  EXPECT_EQ(UnionState({ 4,0,0,0},   0), INIT_UNION(getUnion0().setU0f0s32(1)));
  EXPECT_EQ(UnionState({ 5,0,0,0},   0), INIT_UNION(getUnion0().setU0f0s64(1)));
  EXPECT_EQ(UnionState({ 6,0,0,0}, 448), INIT_UNION(getUnion0().setU0f0sp("1")));

  EXPECT_EQ(UnionState({ 7,0,0,0},  -1), INIT_UNION(getUnion0().setU0f1s0(Void::VOID)));
  EXPECT_EQ(UnionState({ 8,0,0,0},   0), INIT_UNION(getUnion0().setU0f1s1(1)));
  EXPECT_EQ(UnionState({ 9,0,0,0},   0), INIT_UNION(getUnion0().setU0f1s8(1)));
  EXPECT_EQ(UnionState({10,0,0,0},   0), INIT_UNION(getUnion0().setU0f1s16(1)));
  EXPECT_EQ(UnionState({11,0,0,0},   0), INIT_UNION(getUnion0().setU0f1s32(1)));
  EXPECT_EQ(UnionState({12,0,0,0},   0), INIT_UNION(getUnion0().setU0f1s64(1)));
  EXPECT_EQ(UnionState({13,0,0,0}, 448), INIT_UNION(getUnion0().setU0f1sp("1")));

  EXPECT_EQ(UnionState({0, 0,0,0},  -1), INIT_UNION(getUnion1().setU1f0s0(Void::VOID)));
  EXPECT_EQ(UnionState({0, 1,0,0},  65), INIT_UNION(getUnion1().setU1f0s1(1)));
  EXPECT_EQ(UnionState({0, 2,0,0},  65), INIT_UNION(getUnion1().setU1f1s1(1)));
  EXPECT_EQ(UnionState({0, 3,0,0},  72), INIT_UNION(getUnion1().setU1f0s8(1)));
  EXPECT_EQ(UnionState({0, 4,0,0},  72), INIT_UNION(getUnion1().setU1f1s8(1)));
  EXPECT_EQ(UnionState({0, 5,0,0},  80), INIT_UNION(getUnion1().setU1f0s16(1)));
  EXPECT_EQ(UnionState({0, 6,0,0},  80), INIT_UNION(getUnion1().setU1f1s16(1)));
  EXPECT_EQ(UnionState({0, 7,0,0},  96), INIT_UNION(getUnion1().setU1f0s32(1)));
  EXPECT_EQ(UnionState({0, 8,0,0},  96), INIT_UNION(getUnion1().setU1f1s32(1)));
  EXPECT_EQ(UnionState({0, 9,0,0}, 128), INIT_UNION(getUnion1().setU1f0s64(1)));
  EXPECT_EQ(UnionState({0,10,0,0}, 128), INIT_UNION(getUnion1().setU1f1s64(1)));
  EXPECT_EQ(UnionState({0,11,0,0}, 512), INIT_UNION(getUnion1().setU1f0sp("1")));
  EXPECT_EQ(UnionState({0,12,0,0}, 512), INIT_UNION(getUnion1().setU1f1sp("1")));

  EXPECT_EQ(UnionState({0,13,0,0},  -1), INIT_UNION(getUnion1().setU1f2s0(Void::VOID)));
  EXPECT_EQ(UnionState({0,14,0,0}, 128), INIT_UNION(getUnion1().setU1f2s1(1)));
  EXPECT_EQ(UnionState({0,15,0,0}, 128), INIT_UNION(getUnion1().setU1f2s8(1)));
  EXPECT_EQ(UnionState({0,16,0,0}, 128), INIT_UNION(getUnion1().setU1f2s16(1)));
  EXPECT_EQ(UnionState({0,17,0,0}, 128), INIT_UNION(getUnion1().setU1f2s32(1)));
  EXPECT_EQ(UnionState({0,18,0,0}, 128), INIT_UNION(getUnion1().setU1f2s64(1)));
  EXPECT_EQ(UnionState({0,19,0,0}, 512), INIT_UNION(getUnion1().setU1f2sp("1")));

  EXPECT_EQ(UnionState({0,0,0,0}, 192), INIT_UNION(getUnion2().setU2f0s1(1)));
  EXPECT_EQ(UnionState({0,0,0,0}, 193), INIT_UNION(getUnion3().setU3f0s1(1)));
  EXPECT_EQ(UnionState({0,0,1,0}, 200), INIT_UNION(getUnion2().setU2f0s8(1)));
  EXPECT_EQ(UnionState({0,0,0,1}, 208), INIT_UNION(getUnion3().setU3f0s8(1)));
  EXPECT_EQ(UnionState({0,0,2,0}, 224), INIT_UNION(getUnion2().setU2f0s16(1)));
  EXPECT_EQ(UnionState({0,0,0,2}, 240), INIT_UNION(getUnion3().setU3f0s16(1)));
  EXPECT_EQ(UnionState({0,0,3,0}, 256), INIT_UNION(getUnion2().setU2f0s32(1)));
  EXPECT_EQ(UnionState({0,0,0,3}, 288), INIT_UNION(getUnion3().setU3f0s32(1)));
  EXPECT_EQ(UnionState({0,0,4,0}, 320), INIT_UNION(getUnion2().setU2f0s64(1)));
  EXPECT_EQ(UnionState({0,0,0,4}, 384), INIT_UNION(getUnion3().setU3f0s64(1)));

#undef INIT_UNION
}

TEST(Encoding, UnionDefault) {
  MallocMessageBuilder builder;
  TestUnionDefaults::Reader reader = builder.getRoot<TestUnionDefaults>().asReader();

  {
    auto field = reader.getS16s8s64s8Set();
    EXPECT_EQ(TestUnion::Union0::U0F0S16, field.getUnion0().which());
    EXPECT_EQ(TestUnion::Union1::U1F0S8 , field.getUnion1().which());
    EXPECT_EQ(TestUnion::Union2::U2F0S64, field.getUnion2().which());
    EXPECT_EQ(TestUnion::Union3::U3F0S8 , field.getUnion3().which());
    EXPECT_EQ(321, field.getUnion0().getU0f0s16());
    EXPECT_EQ(123, field.getUnion1().getU1f0s8());
    EXPECT_EQ(12345678901234567ll, field.getUnion2().getU2f0s64());
    EXPECT_EQ(55, field.getUnion3().getU3f0s8());
  }

  {
    auto field = reader.getS0sps1s32Set();
    EXPECT_EQ(TestUnion::Union0::U0F1S0 , field.getUnion0().which());
    EXPECT_EQ(TestUnion::Union1::U1F0SP , field.getUnion1().which());
    EXPECT_EQ(TestUnion::Union2::U2F0S1 , field.getUnion2().which());
    EXPECT_EQ(TestUnion::Union3::U3F0S32, field.getUnion3().which());
    EXPECT_EQ(Void::VOID, field.getUnion0().getU0f1s0());
    EXPECT_EQ("foo", field.getUnion1().getU1f0sp());
    EXPECT_EQ(true, field.getUnion2().getU2f0s1());
    EXPECT_EQ(12345678, field.getUnion3().getU3f0s32());
  }
}

// =======================================================================================

TEST(Encoding, ListDefaults) {
  MallocMessageBuilder builder;
  TestListDefaults::Builder root = builder.getRoot<TestListDefaults>();

  checkTestMessage(root.asReader());
  checkTestMessage(root);
  checkTestMessage(root.asReader());
}

TEST(Encoding, BuildListDefaults) {
  MallocMessageBuilder builder;
  TestListDefaults::Builder root = builder.getRoot<TestListDefaults>();

  initTestMessage(root);
  checkTestMessage(root.asReader());
  checkTestMessage(root);
  checkTestMessage(root.asReader());
}

TEST(Encoding, SmallStructLists) {
  // In this test, we will manually initialize TestListDefaults.lists to match the default
  // value and verify that we end up with the same encoding that the compiler produces.

  MallocMessageBuilder builder;
  auto root = builder.getRoot<TestListDefaults>();
  auto sl = root.initLists();

  // Verify that all the lists are actually empty.
  EXPECT_EQ(0u, sl.getList0 ().size());
  EXPECT_EQ(0u, sl.getList1 ().size());
  EXPECT_EQ(0u, sl.getList8 ().size());
  EXPECT_EQ(0u, sl.getList16().size());
  EXPECT_EQ(0u, sl.getList32().size());
  EXPECT_EQ(0u, sl.getList64().size());
  EXPECT_EQ(0u, sl.getListP ().size());
  EXPECT_EQ(0u, sl.getInt32ListList().size());
  EXPECT_EQ(0u, sl.getTextListList().size());
  EXPECT_EQ(0u, sl.getStructListList().size());

  { auto l = sl.initList0 (2); l[0].setF(Void::VOID);        l[1].setF(Void::VOID); }
  { auto l = sl.initList1 (4); l[0].setF(true);              l[1].setF(false);
                               l[2].setF(true);              l[3].setF(true); }
  { auto l = sl.initList8 (2); l[0].setF(123u);              l[1].setF(45u); }
  { auto l = sl.initList16(2); l[0].setF(12345u);            l[1].setF(6789u); }
  { auto l = sl.initList32(2); l[0].setF(123456789u);        l[1].setF(234567890u); }
  { auto l = sl.initList64(2); l[0].setF(1234567890123456u); l[1].setF(2345678901234567u); }
  { auto l = sl.initListP (2); l[0].setF("foo");             l[1].setF("bar"); }

  {
    auto l = sl.initInt32ListList(3);
    l.init(0, 3).copyFrom({1, 2, 3});
    l.init(1, 2).copyFrom({4, 5});
    l.init(2, 1).copyFrom({12341234});
  }

  {
    auto l = sl.initTextListList(3);
    l.init(0, 2).copyFrom({"foo", "bar"});
    l.init(1, 1).copyFrom({"baz"});
    l.init(2, 2).copyFrom({"qux", "corge"});
  }

  {
    auto l = sl.initStructListList(2);
    l.init(0, 2);
    l.init(1, 1);

    l[0][0].setInt32Field(123);
    l[0][1].setInt32Field(456);
    l[1][0].setInt32Field(789);
  }

  ArrayPtr<const word> segment = builder.getSegmentsForOutput()[0];

  // Initialize another message such that it copies the default value for that field.
  MallocMessageBuilder defaultBuilder;
  defaultBuilder.getRoot<TestListDefaults>().getLists();
  ArrayPtr<const word> defaultSegment = defaultBuilder.getSegmentsForOutput()[0];

  // Should match...
  EXPECT_EQ(defaultSegment.size(), segment.size());

  for (size_t i = 0; i < std::min(segment.size(), defaultSegment.size()); i++) {
    EXPECT_EQ(reinterpret_cast<const uint64_t*>(defaultSegment.begin())[i],
              reinterpret_cast<const uint64_t*>(segment.begin())[i]);
  }
}

// =======================================================================================

TEST(Encoding, ListUpgrade) {
  MallocMessageBuilder builder;
  auto root = builder.initRoot<test::TestObject>();

  root.initObjectField<List<uint16_t>>(3).copyFrom({12, 34, 56});

  checkList(root.getObjectField<List<uint8_t>>(), {12, 34, 56});

  {
    auto l = root.getObjectField<List<test::TestLists::Struct8>>();
    ASSERT_EQ(3u, l.size());
    EXPECT_EQ(12u, l[0].getF());
    EXPECT_EQ(34u, l[1].getF());
    EXPECT_EQ(56u, l[2].getF());
  }

  checkList(root.getObjectField<List<uint16_t>>(), {12, 34, 56});

  auto reader = root.asReader();

  checkList(reader.getObjectField<List<uint8_t>>(), {12, 34, 56});

  {
    auto l = reader.getObjectField<List<test::TestLists::Struct8>>();
    ASSERT_EQ(3u, l.size());
    EXPECT_EQ(12u, l[0].getF());
    EXPECT_EQ(34u, l[1].getF());
    EXPECT_EQ(56u, l[2].getF());
  }

  try {
    reader.getObjectField<List<uint32_t>>();
    ADD_FAILURE() << "Expected exception.";
  } catch (const Exception& e) {
    // expected
  }

  {
    auto l = reader.getObjectField<List<test::TestLists::Struct32>>();
    ASSERT_EQ(3u, l.size());

    // These should return default values because the structs aren't big enough.
    EXPECT_EQ(0u, l[0].getF());
    EXPECT_EQ(0u, l[1].getF());
    EXPECT_EQ(0u, l[2].getF());
  }

  checkList(reader.getObjectField<List<uint16_t>>(), {12, 34, 56});
}

TEST(Encoding, BitListDowngrade) {
  MallocMessageBuilder builder;
  auto root = builder.initRoot<test::TestObject>();

  root.initObjectField<List<uint16_t>>(4).copyFrom({0x1201u, 0x3400u, 0x5601u, 0x7801u});

  checkList(root.getObjectField<List<bool>>(), {true, false, true, true});

  {
    auto l = root.getObjectField<List<test::TestLists::Struct1>>();
    ASSERT_EQ(4u, l.size());
    EXPECT_TRUE(l[0].getF());
    EXPECT_FALSE(l[1].getF());
    EXPECT_TRUE(l[2].getF());
    EXPECT_TRUE(l[3].getF());
  }

  checkList(root.getObjectField<List<uint16_t>>(), {0x1201u, 0x3400u, 0x5601u, 0x7801u});

  auto reader = root.asReader();

  checkList(reader.getObjectField<List<bool>>(), {true, false, true, true});

  {
    auto l = reader.getObjectField<List<test::TestLists::Struct1>>();
    ASSERT_EQ(4u, l.size());
    EXPECT_TRUE(l[0].getF());
    EXPECT_FALSE(l[1].getF());
    EXPECT_TRUE(l[2].getF());
    EXPECT_TRUE(l[3].getF());
  }

  checkList(reader.getObjectField<List<uint16_t>>(), {0x1201u, 0x3400u, 0x5601u, 0x7801u});
}

TEST(Encoding, BitListUpgrade) {
  MallocMessageBuilder builder;
  auto root = builder.initRoot<test::TestObject>();

  root.initObjectField<List<bool>>(4).copyFrom({true, false, true, true});

  {
    auto l = root.getObjectField<List<test::TestFieldZeroIsBit>>();
    ASSERT_EQ(4u, l.size());
    EXPECT_TRUE(l[0].getBit());
    EXPECT_FALSE(l[1].getBit());
    EXPECT_TRUE(l[2].getBit());
    EXPECT_TRUE(l[3].getBit());
  }

  auto reader = root.asReader();

  try {
    reader.getObjectField<List<uint8_t>>();
    ADD_FAILURE() << "Expected exception.";
  } catch (const Exception& e) {
    // expected
  }

  {
    auto l = reader.getObjectField<List<test::TestFieldZeroIsBit>>();
    ASSERT_EQ(4u, l.size());
    EXPECT_TRUE(l[0].getBit());
    EXPECT_FALSE(l[1].getBit());
    EXPECT_TRUE(l[2].getBit());
    EXPECT_TRUE(l[3].getBit());

    // Other fields are defaulted.
    EXPECT_TRUE(l[0].getSecondBit());
    EXPECT_TRUE(l[1].getSecondBit());
    EXPECT_TRUE(l[2].getSecondBit());
    EXPECT_TRUE(l[3].getSecondBit());
    EXPECT_EQ(123u, l[0].getThirdField());
    EXPECT_EQ(123u, l[1].getThirdField());
    EXPECT_EQ(123u, l[2].getThirdField());
    EXPECT_EQ(123u, l[3].getThirdField());
  }

  checkList(reader.getObjectField<List<bool>>(), {true, false, true, true});
}

// =======================================================================================
// Tests of generated code, not really of the encoding.
// TODO(cleanup):  Move to a different test?

TEST(Encoding, NestedTypes) {
  // This is more of a test of the generated code than the encoding.

  MallocMessageBuilder builder;
  TestNestedTypes::Reader reader = builder.getRoot<TestNestedTypes>().asReader();

  EXPECT_EQ(TestNestedTypes::NestedEnum::BAR, reader.getOuterNestedEnum());
  EXPECT_EQ(TestNestedTypes::NestedStruct::NestedEnum::QUUX, reader.getInnerNestedEnum());

  TestNestedTypes::NestedStruct::Reader nested = reader.getNestedStruct();
  EXPECT_EQ(TestNestedTypes::NestedEnum::BAR, nested.getOuterNestedEnum());
  EXPECT_EQ(TestNestedTypes::NestedStruct::NestedEnum::QUUX, nested.getInnerNestedEnum());
}

TEST(Encoding, Imports) {
  // Also just testing the generated code.

  MallocMessageBuilder builder;
  TestImport::Builder root = builder.getRoot<TestImport>();
  initTestMessage(root.initField());
  checkTestMessage(root.asReader().getField());
}

TEST(Encoding, Using) {
  MallocMessageBuilder builder;
  TestUsing::Reader reader = builder.getRoot<TestUsing>().asReader();
  EXPECT_EQ(TestNestedTypes::NestedEnum::BAR, reader.getOuterNestedEnum());
  EXPECT_EQ(TestNestedTypes::NestedStruct::NestedEnum::QUUX, reader.getInnerNestedEnum());
}

}  // namespace
}  // namespace internal
}  // namespace capnproto
