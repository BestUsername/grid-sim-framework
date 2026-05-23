#include <gtest/gtest.h>
#include <sstream>
#include "libsim/types.hpp"

using namespace grid::libsim;

TEST(OperatorTest, NeedOutputOperator) {
    std::ostringstream oss;

    oss << Need::SURVIVAL;
    EXPECT_EQ(oss.str(), "Survival");

    oss.str("");
    oss << Need::SAFETY;
    EXPECT_EQ(oss.str(), "Safety");

    oss.str("");
    oss << Need::SOCIAL;
    EXPECT_EQ(oss.str(), "Social");

    oss.str("");
    oss << Need::ESTEEM;
    EXPECT_EQ(oss.str(), "Esteem");

    oss.str("");
    oss << Need::SELF_ACTUALIZATION;
    EXPECT_EQ(oss.str(), "Self Actualization");
}

TEST(OperatorTest, SensesOutputOperator) {
    std::ostringstream oss;

    oss << Senses::Sight;
    EXPECT_EQ(oss.str(), "Sight");

    oss.str("");
    oss << Senses::Hearing;
    EXPECT_EQ(oss.str(), "Hearing");

    oss.str("");
    oss << Senses::Smell;
    EXPECT_EQ(oss.str(), "Smell");

    oss.str("");
    oss << Senses::Taste;
    EXPECT_EQ(oss.str(), "Taste");

    oss.str("");
    oss << Senses::Touch;
    EXPECT_EQ(oss.str(), "Touch");
}

TEST(OperatorTest, Vect3OutputOperator) {
    std::ostringstream oss;

    VectX<int, 3> v1{1, 2, 3};
    oss << v1;
    EXPECT_EQ(oss.str(), "(1, 2, 3)");

    oss.str("");
    VectX<int, 3> v2{4, 5, 6};
    oss << v2;
    EXPECT_EQ(oss.str(), "(4, 5, 6)");

    oss.str("");
    VectX<int, 3> v3{7, 8, 9};
    oss << v3;
    EXPECT_EQ(oss.str(), "(7, 8, 9)");
}

TEST(Vect2Test, Magnitude) {
    VectX<double, 2> v1{3.0, 4.0};
    EXPECT_DOUBLE_EQ(v1.magnitude(), 5.0);

    VectX<double, 2> v2{0.0, 0.0};
    EXPECT_DOUBLE_EQ(v2.magnitude(), 0.0);

    VectX<double, 2> v3{-2.0, -2.0};
    EXPECT_DOUBLE_EQ(v3.magnitude(), 2.8284271247461903);
}

TEST(Vect2Test, Addition) {
    VectX<int, 2> v1{1, 2};
    VectX<int, 2> v2{3, 4};
    VectX<int, 2> result = v1 + v2;
    EXPECT_EQ(result[0], 4);
    EXPECT_EQ(result[1], 6);
}

TEST(Vect2Test, Subtraction) {
    VectX<int, 2> v1{5, 6};
    VectX<int, 2> v2{3, 4};
    VectX<int, 2> result = v1 - v2;
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 2);
}

TEST(Vect2Test, CompoundAddition) {
    VectX<int, 2> v1{1, 2};
    VectX<int, 2> v2{3, 4};
    v1 += v2;
    EXPECT_EQ(v1[0], 4);
    EXPECT_EQ(v1[1], 6);
}

TEST(Vect2Test, CompoundSubtraction) {
    VectX<int, 2> v1{5, 6};
    VectX<int, 2> v2{3, 4};
    v1 -= v2;
    EXPECT_EQ(v1[0], 2);
    EXPECT_EQ(v1[1], 2);
}

TEST(Vect2Test, OutputOperator) {
    std::ostringstream oss;

    VectX<int, 2> v1{1, 2};
    oss << v1;
    EXPECT_EQ(oss.str(), "(1, 2)");

    oss.str("");
    VectX<int, 2> v2{3, 4};
    oss << v2;
    EXPECT_EQ(oss.str(), "(3, 4)");

    oss.str("");
    VectX<int, 2> v3{5, 6};
    oss << v3;
    EXPECT_EQ(oss.str(), "(5, 6)");
}
TEST(OperatorTest, NeedOutputOperatorInvalid) {
    std::ostringstream oss;
    Need invalidNeed = static_cast<Need>(100); // Invalid enum value
    oss << invalidNeed;
    EXPECT_TRUE(oss.fail()); // Expect stringstream's fail bit to be set
}

TEST(OperatorTest, SensesOutputOperatorInvalid) {
    std::ostringstream oss;
    Senses invalidSense = static_cast<Senses>(100); // Invalid enum value
    oss << invalidSense;
    EXPECT_TRUE(oss.fail()); // Expect stringstream's fail bit to be set
}

TEST(Vect3Test, Magnitude) {
    VectX<double, 3> v1{1.0, 2.0, 3.0};
    EXPECT_DOUBLE_EQ(v1.magnitude(), 3.7416573867739413);

    VectX<double, 3> v2{0.0, 0.0, 0.0};
    EXPECT_DOUBLE_EQ(v2.magnitude(), 0.0);

    VectX<double, 3> v3{-2.0, -2.0, -2.0};
    EXPECT_DOUBLE_EQ(v3.magnitude(), 3.4641016151377544);
}

TEST(Vect3Test, Addition) {
    VectX<int, 3> v1{1, 2, 3};
    VectX<int, 3> v2{4, 5, 6};
    VectX<int, 3> result = v1 + v2;
    EXPECT_EQ(result[0], 5);
    EXPECT_EQ(result[1], 7);
    EXPECT_EQ(result[2], 9);
}

TEST(Vect3Test, Subtraction) {
    VectX<int, 3> v1{5, 6, 7};
    VectX<int, 3> v2{3, 4, 5};
    VectX<int, 3> result = v1 - v2;
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 2);
    EXPECT_EQ(result[2], 2);
}

TEST(Vect3Test, CompoundAddition) {
    VectX<int, 3> v1{1, 2, 3};
    VectX<int, 3> v2{4, 5, 6};
    v1 += v2;
    EXPECT_EQ(v1[0], 5);
    EXPECT_EQ(v1[1], 7);
    EXPECT_EQ(v1[2], 9);
}

TEST(Vect3Test, CompoundSubtraction) {
    VectX<int, 3> v1{5, 6, 7};
    VectX<int, 3> v2{3, 4, 5};
    v1 -= v2;
    EXPECT_EQ(v1[0], 2);
    EXPECT_EQ(v1[1], 2);
    EXPECT_EQ(v1[2], 2);
}

TEST(Vect3Test, OutputOperator) {
    std::ostringstream oss;

    VectX<int, 3> v1{1, 2, 3};
    oss << v1;
    EXPECT_EQ(oss.str(), "(1, 2, 3)");

    oss.str("");
    VectX<int, 3> v2{4, 5, 6};
    oss << v2;
    EXPECT_EQ(oss.str(), "(4, 5, 6)");

    oss.str("");
    VectX<int, 3> v3{7, 8, 9};
    oss << v3;
    EXPECT_EQ(oss.str(), "(7, 8, 9)");
}
