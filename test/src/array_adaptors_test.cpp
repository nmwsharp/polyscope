
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
TEST(ArrayAdaptorTests, access_Iterable) {
  EXPECT_EQ(polyscope::standardizeArray<double>(arr_listdouble)[0], .1);
}

// Test that standardizeArray works with a custom accessor function
TEST(ArrayAdaptorTests, access_FuncAccess) {
  EXPECT_EQ(polyscope::standardizeArray<double>(userArray_funcAccess)[0], .1);

  // ensures the conversion code path works
  EXPECT_NEAR(polyscope::standardizeArray<float>(userArray_funcAccess)[0], .1, 1e-5);
}
