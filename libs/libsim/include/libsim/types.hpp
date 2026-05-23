#ifndef GRID_TYPES_HPP_INCLUDED
#define GRID_TYPES_HPP_INCLUDED

#include <cmath>
#include <ostream>
#include <chrono>

/**
 * @brief This file defines various types used in the grid library.
 */
/**
 * @brief The grid::libsim namespace.
*/
namespace grid::libsim {
    /**
     * @brief Represents the different needs of an agent.
     * 
     * The Need enum class defines the different needs that an agent may have.
     * These needs include survival, safety, social interaction, esteem, and self-actualization.
     */
    enum class Need {
        SURVIVAL,    // Basic needs for survival (e.g., food, water)
        SAFETY,      // Safety and security needs (e.g., avoiding danger)
        SOCIAL,      // Social needs (e.g., interacting with other agents)
        ESTEEM,      // Esteem needs (e.g., gaining status or recognition)
        SELF_ACTUALIZATION // Self-actualization needs (e.g., achieving goals)
    };
    std::ostream& operator<<(std::ostream& os, const Need& need);

    /**
     * @brief Represents the senses and extensions of an agent.
     * 
     * The Senses enum class defines the different senses and extensions that an agent may have.
     * These include sight, hearing, smell, taste, and touch.
     */
    enum class Senses {
        Sight,
        Hearing,
        Smell,
        Taste,
        Touch
    };
    std::ostream& operator<<(std::ostream& os, const Senses& sense);

    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    struct VectX {
        static constexpr size_t num_dimensions = NUM_DIMENSIONS;
        NUMBER_TYPE values[NUM_DIMENSIONS];

        NUMBER_TYPE magnitude() const {
            NUMBER_TYPE result = 0;
            for (size_t i = 0; i < NUM_DIMENSIONS; ++i) {
                result += values[i] * values[i];
            }
            return std::sqrt(result);
        }

        NUMBER_TYPE& operator[](size_t index) {
            if (index >= NUM_DIMENSIONS) {
                throw std::out_of_range("Index out of range");
            }
            return values[index];
        }

        const NUMBER_TYPE& operator[](size_t index) const {
            return values[index];
        }

        NUMBER_TYPE* data() {
            return values;
        }

        const NUMBER_TYPE* data() const {
            return values;
        }

        /**
         * @brief Operator +
         * 
         * @param ref 
         * @return VectX 
         */
        VectX operator+(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& ref) const {
            VectX<NUMBER_TYPE, NUM_DIMENSIONS> result;
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                result.values[i] = this->values[i] + ref.values[i];
            }
            return result;
        }

        /**
         * @brief Operator +=
         * 
         * @param ref 
         * @return VectX& 
         */
        VectX& operator+=(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& ref) {
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                this->values[i] += ref.values[i];
            }
            return *this;
        }

        /**
         * @brief Operator -
         * 
         * @param ref 
         * @return VectX 
         */
        VectX operator-(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& ref) const {
            VectX<NUMBER_TYPE, NUM_DIMENSIONS> result;
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                result.values[i] = this->values[i] - ref.values[i];
            }
            return result;
        }

        /**
         * @brief Operator -=
         * 
         * @param ref 
         * @return VectX& 
         */
        VectX& operator-=(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& ref) {
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                this->values[i] -= ref.values[i];
            }
            return *this;
        }

        /**
         * @brief Operator *
         * 
         * @param scalar 
         * @return VectX 
         */
        VectX operator*(const NUMBER_TYPE& scalar) const {
            VectX<NUMBER_TYPE, NUM_DIMENSIONS> result;
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                result.values[i] = this->values[i] * scalar;
            }
            return result;
        }

        /**
         * @brief Operator *=
         * 
         * @param scalar 
         * @return VectX& 
         */
        VectX& operator*=(const NUMBER_TYPE& scalar) {
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                this->values[i] *= scalar;
            }
            return *this;
        }

        /**
         * @brief Operator /
         * 
         * @param scalar 
         * @return VectX 
         */
        VectX operator/(const NUMBER_TYPE& scalar) const {
            VectX<NUMBER_TYPE, NUM_DIMENSIONS> result;
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                result.values[i] = this->values[i] / scalar;
            }
            return result;
        }

        /**
         * @brief Operator /=
         * 
         * @param scalar 
         * @return VectX& 
         */
        VectX& operator/=(const NUMBER_TYPE& scalar) {
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                this->values[i] /= scalar;
            }
            return *this;
        }

        /**
         * @brief Operator ==
         * 
         * @param rhs 
         * @return bool 
         */
        bool operator==(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& rhs) const {
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                if (this->values[i] != rhs.values[i]) {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Operator !=
         * 
         * @param rhs 
         * @return bool 
         */
        bool operator!=(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& rhs) const {
            return !(*this == rhs);
        }

        /**
         * @brief Operator <<
         * 
         * @param out 
         * @param vector 
         * @return std::ostream& 
         */
        friend std::ostream& operator<< (std::ostream &out, const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& vector) {
            out << "(";
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                out << vector.values[i];
                if (i != NUM_DIMENSIONS - 1) {
                    out << ", ";
                }
            }
            out << ")";
            return out;
        }
    };

    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    struct RangeXD {
        std::pair<NUMBER_TYPE, NUMBER_TYPE> ranges[NUM_DIMENSIONS];
        bool contains(const VectX<NUMBER_TYPE, NUM_DIMENSIONS>& point) const {
            for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
                if (point.values[i] < ranges[i].first || point.values[i] > ranges[i].second) {
                    return false;
                }
            }
            return true;
        }
    };

    /**
     * @brief Represents the range of a 1-dimensional shape.
     * 
     * The Range1D struct represents the range of a 1-dimensional shape.
     * It defines the minimum and maximum values of the range.
     * 
     * @tparam NUMBER_TYPE The number type of the range.
     */
    template<typename NUMBER_TYPE>
    struct Range1D {
        NUMBER_TYPE min; /**< The minimum value of the range. */
        NUMBER_TYPE max; /**< The maximum value of the range. */
    };

    /**
     * @brief Represents the range of a 2-dimensional shape.
     * 
     * The Range2D struct represents the range of a 2-dimensional shape.
     * It defines the minimum and maximum values for both the x and y dimensions.
     * 
     * @tparam NUMBER_TYPE The number type of the range.
     */
    template<typename NUMBER_TYPE>
    struct Range2D {
        Range1D<NUMBER_TYPE> xRange; /**< The range for the x dimension. */
        Range1D<NUMBER_TYPE> yRange; /**< The range for the y dimension. */
    };

    /**
     * @brief Represents the range of a 3-dimensional shape.
     * 
     * The Range3D struct represents the range of a 3-dimensional shape.
     * It defines the minimum and maximum values for the x, y, and z dimensions.
     * 
     * @tparam NUMBER_TYPE The number type of the range.
     */
    template<typename NUMBER_TYPE>
    struct Range3D {
        Range1D<NUMBER_TYPE> xRange; /**< The range for the x dimension. */
        Range1D<NUMBER_TYPE> yRange; /**< The range for the y dimension. */
        Range1D<NUMBER_TYPE> zRange; /**< The range for the z dimension. */
    };

    using DEFAULT_COORD_TYPE = VectX<double, 2>;

    // /**
    //  * @brief Make the default vector type use 2D only.
    //  * 
    //  * The Vect alias is a convenience alias for the Vect2 class with the default element type.
    //  */
    // using Vect = Vect2<VECTOR_TYPE>;
    // /**
    //  * @brief CoordType is a convenience naming for VectType
    //  * 
    //  * The CoordType alias is a convenience alias for the VectType.
    //  */
    // using CoordType = VECTOR_TYPE;
    // /**
    //  * @brief Coord is a convenience name for the default Vector.
    //  * 
    //  * The Coord alias is a convenience alias for the Vect class.
    //  */
    // using Coord = Vect;

    
    using DeltaType = std::chrono::duration<float>;

} // namespace grid::libsim

#endif//GRID_TYPES_HPP_INCLUDED
