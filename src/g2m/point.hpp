
/// \file
/// A point in 3D space, and the arithmetic g2m does on one.

#include <cmath>
#include <sstream>

#ifndef POINT_HPP
#define POINT_HPP

//#include <app/cutsim_def.hpp>

namespace g2m {

/// a point in 3D space
struct Point {
    /// Create point at (0,0,0)
    Point():x(0),y(0),z(0) {}
    /// Create point at given coordinates
    /// \param a x-coordinate \param b y-coordinate \param c z-coordinate
    Point(double a, double b, double c):x(a),y(b),z(c) {}
    /// copy-constructor
    /// \param other the point to copy
    Point(const Point& other): x(other.x),y(other.y),z(other.z) {}
    /// distance to given other Point
    /// \param other the far end
    /// \returns the straight-line distance
    double Distance( Point other ) {
        return sqrt( (other.x-x)*(other.x-x) + (other.y-y)*(other.y-y) + (other.z-z)*(other.z-z) );
    }
    /// string representation
    /// \returns the coordinates as "(x, y, z)"
    std::string str() const {
        std::ostringstream o;
        o << "(" << x << ", " << y << ", " << z << ")";
        return o.str();
    }
    /// multiply Point with scalar
    /// \param a the scalar \returns this point, scaled
    Point& operator*=(const double &a) {
        x*=a;
        y*=a;
        z*=a;
        return *this;
    }
    /// multiply Point with scalar
    /// \param a the scalar \returns a scaled copy
    Point  operator*(const double &a) const {
        return Point(*this) *= a;
    }
    /// vector addition
    /// \param p the point to add \returns this point, moved
    Point& operator+=( const Point& p) {
        x+=p.x;
        y+=p.y;
        z+=p.z;
        return *this;
    }
    
    /// vector addition
    /// \param p the point to add \returns the sum
    const Point operator+( const Point &p) const {
        return Point(*this) += p;
    }
    
    /// vector subtraction
    /// \param p the point to subtract \returns this point, moved
    Point& operator-=( const Point& p) {
        x-=p.x;
        y-=p.y;
        z-=p.z;
        return *this;
    }
    /// vector subtraction
    /// \param p the point to subtract \returns the difference
    const Point operator-( const Point &p) const {
        return Point(*this) -= p;
    }
    
// DATA
    /// x-coordinate
    double x;
    /// y-coordinate
    double y;
    /// z-coordinate
    double z;
};

/// a pose is a complete 6-dimensional description of the position and rotation
/// of an object in 3D space (e.g. a tool). This includes the position of the origin
/// of the tool (loc) and the direction of the tool axis (dir)
struct Pose {
    /// Create a pose at the origin, pointing along the origin direction
    Pose() {}
    /// Create Pose with given location and direction
    /// \param a location
    /// \param b direction
    Pose( Point a, Point b ) {
        loc = a;
        dir = b;
    }
    /// location
    Point loc;
    /// direction -> angle
    Point dir;
};

} // end namespace
#endif 
