#include "gtest/gtest.h"


#include "glm/glm.hpp"

#include <array>
#include <iostream>
#include <list>
#include <string>
#include <vector>

#define POLYSCOPE_NO_STANDARDIZE_FALLTHROUGH

#include "polyscope/standardize_data_array.h"

using std::cout;
using std::endl;

// ============================================================
// =============== Array adaptor tests
// ============================================================

// A few examples to test
namespace {

// std arrays
std::vector<double> arr_vecdouble{0.1, 0.2, 0.3, 0.4, 0.5};
std::vector<float> arr_vecfloat{0.1, 0.2, 0.3, 0.4, 0.5};
std::vector<int> arr_vecint{1, 2, 3, 4, 5};
std::array<double, 5> arr_arrdouble{0.1, 0.2, 0.3, 0.4, 0.5};
std::list<double> arr_listdouble{0.1, 0.2, 0.3, 0.4, 0.5};

// == A custom array which needs all the adaptors
struct UserArray {
  std::vector<double> myData;
  size_t bigness() const { return myData.size(); }
};
UserArray userArray_sizeFunc{{0.1, 0.2, 0.3, 0.4, 0.5}};

// Size function for custom array
size_t adaptorF_custom_size(const UserArray& c) { return c.bigness(); }


// == A type that we access with callable (paren)
// (Eigen works this way, but don't want to depend on Eigen)
struct UserArrayCallable {
  std::vector<double> myData;
  size_t size() const { return myData.size(); }
  double operator()(size_t i) const { return myData[i]; }
};
UserArrayCallable userArray_callableAccess{{0.1, 0.2, 0.3, 0.4, 0.5}};


// == A type that we access with callable (paren), and needs an int (rather than a size_t)
struct UserArrayCallableInt {
  std::vector<double> myData;
  size_t size() const { return myData.size(); }
  double operator()(int i) const { return myData[i]; }
};
UserArrayCallableInt userArray_callableAccessInt{{0.1, 0.2, 0.3, 0.4, 0.5}};


// == A type that requires a custom access function
struct UserArrayFuncAccess {
  std::vector<double> myData;
  size_t size() const { return myData.size(); }
};
UserArrayFuncAccess userArray_funcAccess{{0.1, 0.2, 0.3, 0.4, 0.5}};

std::vector<double> adaptorF_custom_convertToStdVector(const UserArrayFuncAccess& c) {
  std::vector<double> out;
  for (auto x : c.myData) {
    out.push_back(x);
  }
  return out;
}


// A vector type with x-y access
struct UserVector2XY {
  double x;
  double y;
};
UserVector2XY userVec2_xy{0.1, 0.2};

// A vector type with u-v access
struct UserVector2UV {
  double u;
  double v;
};
UserVector2UV userVec2_uv{0.1, 0.2};

// A vector2 type with crazy custom access
struct UserVector2Custom {
  double foo;
  double bar;
};
UserVector2Custom userVec2_custom{0.1, 0.2};
double adaptorF_custom_accessVector2Value(const UserVector2Custom& v, unsigned int ind) {
  if (ind == 0) return v.foo;
  if (ind == 1) return v.bar;
  throw std::logic_error("bad access");
  return -1.;
}

// A vector type with x y z access
struct UserVector3XYZ {
  double x;
  double y;
  double z;
};
UserVector3XYZ userVec3_xyz{0.1, 0.2, 0.3};

// A vector3 type with crazy custom access
struct UserVector3Custom {
  double foo;
  double bar;
  double baz;
};
UserVector3Custom userVec3_custom{0.1, 0.2, 0.3};
double adaptorF_custom_accessVector3Value(const UserVector3Custom& v, unsigned int ind) {
  if (ind == 0) return v.foo;
  if (ind == 1) return v.bar;
  if (ind == 2) return v.baz;
  throw std::logic_error("bad access");
  return -1.;
}


// A array-vector type with double-callable access
struct UserArrayVectorCallable {
  std::vector<std::array<double, 3>> vals;
  size_t size() const { return vals.size(); }
  double operator()(int i, int j) const { return vals[i][j]; }
};
UserArrayVectorCallable userArrayVector_doubleCallable{{{0.1, 0.2, 0.3}}};

// A array-vector type with custom access
struct UserArrayVectorCustom {
  std::list<UserVector3XYZ> vals;
  size_t size() const { return vals.size(); }
};
std::vector<std::array<double, 3>>
adaptorF_custom_convertArrayOfVectorToStdVector(const UserArrayVectorCustom& inputData) {
  std::vector<std::array<double, 3>> out;
  for (auto v : inputData.vals) {
    out.push_back({v.x, v.y, v.z});
  }
  return out;
}
UserArrayVectorCustom userArrayVector_custom{{{0.1, 0.2, 0.3}}};

// A wannabe Eigen matrix
struct FakeMatrix {
  std::vector<std::array<int, 3>> myData;
  long long int rows() const { return myData.size(); }
  long long int cols() const { return 3; }
  double operator()(int i, int j) const { return myData[i][j]; }
};
FakeMatrix fakeMatrix_int{{{1, 2, 3}, {4, 5, 6}}};


// Nested list access with paren-vector
struct UserArrayParenBracketCustom {
  std::vector<std::vector<int>> myData;
  size_t size() const { return myData.size(); }
  std::vector<int> operator()(int i) const { return myData[i]; }
};
UserArrayParenBracketCustom userArray_parentBracketCustom{{{1, 2, 3}, {4, 5, 6, 7}}};

// A nested list type with custom access
struct UserNestedListCustom {
  std::list<std::vector<int>> vals;
  size_t size() const { return vals.size(); }
};
std::vector<std::vector<int>> adaptorF_custom_convertNestedArrayToStdVector(const UserNestedListCustom& inputData) {
  std::vector<std::vector<int>> out;
  for (auto v : inputData.vals) {
    std::vector<int> inner;
    for (auto x : v) {
      inner.push_back(x);
    }
    out.push_back(inner);
  }
  return out;
}
UserNestedListCustom userArray_nestedListCustom{{{1, 2, 3}, {4, 5, 6, 7}}};

} // namespace


// Test that validateSize works when the type has a .size() member
TEST(ArrayAdaptorTests, validateSize_MemberMethod) {
  polyscope::validateSize(arr_vecdouble, 5, "test");
  polyscope::validateSize(arr_vecfloat, 5, "test");
  polyscope::validateSize(arr_vecint, 5, "test");
  polyscope::validateSize(arr_arrdouble, 5, "test");
  polyscope::validateSize(arr_listdouble, 5, "test");
  polyscope::validateSize(userArray_callableAccess, 5, "test");
}


// Test that validateSize works with a custom overload
TEST(ArrayAdaptorTests, validateSize_Custom) { 
  polyscope::validateSize(userArray_sizeFunc, 5, "test"); 
}


// Test that standardizeArray works with bracket access
TEST(ArrayAdaptorTests, access_BracketOperator) {
  EXPECT_EQ(polyscope::standardizeArray<double>(arr_vecdouble)[0], .1);
  EXPECT_NEAR(polyscope::standardizeArray<double>(arr_vecfloat)[0], .1, 1e-5);
  EXPECT_NEAR(polyscope::standardizeArray<double>(arr_vecint)[0], 1, 1e-5);
  EXPECT_EQ(polyscope::standardizeArray<double>(arr_arrdouble)[0], .1);
}

// Test that standardizeArray works with callable (paren) access
TEST(ArrayAdaptorTests, access_CallableOperator) {
  EXPECT_EQ(polyscope::standardizeArray<double>(userArray_callableAccess)[0], .1);
  EXPECT_EQ(polyscope::standardizeArray<double>(userArray_callableAccessInt)[0], .1);
}


// Test that standardizeArray works with iterable
TEST(ArrayAdaptorTests, access_Iterable) { EXPECT_EQ(polyscope::standardizeArray<double>(arr_listdouble)[0], .1); }

// Test that standardizeArray works with a custom accessor function
TEST(ArrayAdaptorTests, access_FuncAccess) {
  EXPECT_EQ(polyscope::standardizeArray<double>(userArray_funcAccess)[0], .1);

  // ensures the conversion code path works
  EXPECT_NEAR(polyscope::standardizeArray<float>(userArray_funcAccess)[0], .1, 1e-5);
}


// Test that accessVector2 works.
TEST(ArrayAdaptorTests, adaptor_vector2) {
  // Shouldn't compile
  //EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 2>(std::array<double, 2>{0.1, 0.2})), 0.1);

  // bracket access
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 0>(std::array<double, 2>{0.1, 0.2})), 0.1);
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 1>(std::array<double, 2>{0.1, 0.2})), 0.2);

  // x-y access
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 0>(userVec2_xy)), 0.1);
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 1>(userVec2_xy)), 0.2);

  // u-v access
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 0>(userVec2_uv)), 0.1);
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 1>(userVec2_uv)), 0.2);

  // real() imag() access
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 0>(std::complex<double>{0.1, 0.2})), 0.1);
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 1>(std::complex<double>{0.1, 0.2})), 0.2);

  // custom function access
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 0>(userVec2_custom)), 0.1);
  EXPECT_EQ((polyscope::adaptorF_accessVector2Value<double, 1>(userVec2_custom)), 0.2);
}


// Test that accessVector3 works.
TEST(ArrayAdaptorTests, adaptor_vector3) {
  // Shouldn't compile 
  //EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 3>(std::array<double, 3>{0.1, 0.2, 0.3})), 0.1);

  // bracket access
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 0>(std::array<double, 3>{0.1, 0.2, 0.3})), 0.1);
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 1>(std::array<double, 3>{0.1, 0.2, 0.3})), 0.2);
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 2>(std::array<double, 3>{0.1, 0.2, 0.3})), 0.3);

  // x-y access
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 0>(userVec3_xyz)), 0.1);
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 1>(userVec3_xyz)), 0.2);
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 2>(userVec3_xyz)), 0.3);


  // custom function access
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 0>(userVec3_custom)), 0.1);
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 1>(userVec3_custom)), 0.2);
  EXPECT_EQ((polyscope::adaptorF_accessVector3Value<double, 2>(userVec3_custom)), 0.3);
}


// Test that access array of vectors works.
TEST(ArrayAdaptorTests, adaptor_array_vectors) {

  // bracket-bracket access
  EXPECT_NEAR(
      (polyscope::standardizeVectorArray<glm::vec3, 3>(std::vector<std::array<double, 3>>{{0.1, 0.2, 0.3}}))[0][0], 0.1,
      1e-5);

  // double callable access
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec3, 3>(userArrayVector_doubleCallable))[0][0], 0.1, 1e-5);

  // bracket-vector2 access (xy)
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec2, 2>(std::vector<UserVector2XY>{userVec2_xy}))[0][0], 0.1,
              1e-5);

  // bracket-vector2 access (uv)
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec2, 2>(std::vector<UserVector2UV>{userVec2_uv}))[0][0], 0.1,
              1e-5);

  // bracket-vector2 access (real/imag)
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec2, 2>(std::vector<UserVector2UV>{userVec2_uv}))[0][0], 0.1,
              1e-5);

  // list bracket access
  std::list<std::array<double, 3>> arr_listdouble{{0.1, 0.2, 0.3}, {0.4, 0.5, 0.6}};
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec3, 3>(arr_listdouble))[0][0], 0.1, 1e-5);

  // bracket-vector3 access
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec3, 3>(std::vector<UserVector3XYZ>{userVec3_xyz}))[0][0], 0.1,
              1e-5);
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec3, 3>(std::vector<UserVector3XYZ>{userVec3_xyz}))[0][2], 0.3,
              1e-5);

  // custom function access
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec3, 3>(userArrayVector_custom))[0][0], 0.1, 1e-5);
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec3, 3>(userArrayVector_custom))[0][2], 0.3, 1e-5);
  
  // custom inner type  (bracketed)
  std::vector<UserVector3Custom> userVec3sArr{userVec3_custom, userVec3_custom};
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec3, 3>(userVec3sArr))[0][0], 0.1, 1e-5);
  std::vector<UserVector2Custom> userVec2sArr{userVec2_custom, userVec2_custom};
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec2, 2>(userVec2sArr))[0][0], 0.1, 1e-5);
  
  // custom inner type  (iterable)
  std::list<UserVector3Custom> userVec3sList{userVec3_custom, userVec3_custom};
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec3, 3>(userVec3sList))[0][0], 0.1, 1e-5);
  std::list<UserVector2Custom> userVec2sList{userVec2_custom, userVec2_custom};
  EXPECT_NEAR((polyscope::standardizeVectorArray<glm::vec2, 2>(userVec2sList))[0][0], 0.1, 1e-5);
}


// Test that nested access works
TEST(ArrayAdaptorTests, adaptor_nested_array) {

  // Test matrix-style access
  EXPECT_EQ(polyscope::standardizeNestedList<size_t>(fakeMatrix_int)[1][2], 6);

  // Test bracket-bracket access
  std::vector<std::array<int, 3>> testVecBracket{{1, 2, 3}, {4, 5, 6}};
  EXPECT_EQ(polyscope::standardizeNestedList<size_t>(testVecBracket)[1][2], 6);

  // Test paren-braket access
  EXPECT_EQ((polyscope::standardizeNestedList<size_t>(userArray_parentBracketCustom))[1][3], 7);

  // Test iterable-bracket access
  std::list<std::vector<int>> testVecList{{1, 2, 3}, {4, 5, 6, 7}};
  EXPECT_EQ(polyscope::standardizeNestedList<size_t>(testVecList)[1][3], 7);
  
  
  // Test user-specified
  EXPECT_EQ(polyscope::standardizeNestedList<size_t>(userArray_nestedListCustom)[1][3], 7);
}
