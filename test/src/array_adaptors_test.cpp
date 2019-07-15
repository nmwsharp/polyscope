
#include "gtest/gtest.h"

#include <iostream>
#include <list>
#include <string>
#include <vector>


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
size_t adaptorF_size(const UserArray& c) { return c.bigness(); }


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
// (Eigen works this way, but don't want to depend on Eigen)
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

} // namespace


// This include is intentionally after the definitions above, so it can pick them up
#include "polyscope/standardize_data_array.h"


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
TEST(ArrayAdaptorTests, validateSize_Custom) { polyscope::validateSize(userArray_sizeFunc, 5, "test"); }


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
