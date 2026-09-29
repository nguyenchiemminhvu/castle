# Geofence

## Overview

Deterministic point-in-geofence containment tests for geographic coordinates.

This module answers a single question: **is a geographic point inside a geofence?**

Three common geofencing models are provided:

- **Circular geofences** using a center point and radius in meters.
- **Polygon geofences** using an ordered set of boundary vertices.
- **Boundary utility functions** for performing containment checks against externally owned polygon vertex data.

The implementation is designed for embedded and deterministic environments:

- No dynamic memory allocation.
- Fixed-capacity polygon storage.
- Reusable fence objects.
- Geographic coordinates represented with `geo_point<T>`.
- `constexpr` support where practical.

Circular containment uses the actual geodesic distance calculation (`haversine_distance()`), while polygon containment uses a lightweight equirectangular projection combined with a ray-casting point-in-polygon algorithm.

---

## Header

```cpp
#include "castle_ext/geodetic/geofence.hpp"
```

---

## Dependencies

- coord.md
- datum.md
- distance.md
- ../../core/compiler.md
- ../../core/error_handler.md
- ../../core/traits.md
- ../../core/types.md
- ../../container/array.md
- [`../../ntainer/array_view.md
- ../../error/status.md
- [`../../math/geometry/geometry.md`](../../ublic API

### Projection Utilities

| API | Description |
|------|-------------|
| `to_plane_point(point)` | Converts a `geo_point<T>` into a `castle::math::point2d<T>` using an equirectangular (plate carrée) projection where longitude becomes X and latitude becomes Y. |

### Circular Geofencing

| API | Description |
|------|-------------|
| `point_in_circle(point, center, radius_m, margin_m)` | Tests whether a point lies within a circular geofence. |
| `circular_geofence<T, Ellipsoid>` | Reusable circular fence storing a center point and radius. |
| `circular_geofence()` | Constructs a fence centered at `(0,0)` with zero radius. |
| `circular_geofence(center, radius_m)` | Constructs a fence from a center point and radius. |
| `center()` | Returns the stored center coordinate. |
| `radius()` | Returns the configured radius in meters. |
| `contains(point, margin_m)` | Tests whether a point is within the fence. |

### Polygon Geofencing

| API | Description |
|------|-------------|
| `point_in_boundary(point, boundary, count, epsilon_deg)` | Tests containment against a raw vertex array. |
| `point_in_boundary(point, array_view, epsilon_deg)` | Tests containment against an `array_view` boundary. |
| `point_in_boundary(point, region, epsilon_deg)` | Tests containment against a `polygon_geofence`. |
| `geo_boundary_view<T>` | Read-only boundary view alias. |
| `polygon_geofence<T, Capacity>` | Fixed-capacity reusable polygon geofence. |
| `size()` | Returns the number of stored vertices. |
| `capacity()` | Returns the maximum allowed vertices. |
| `empty()` | Returns whether the geofence contains no vertices. |
| `full()` | Returns whether the vertex storage is full. |
| `operator[]` | Accesses a boundary vertex. |
| `push_back(vertex)` | Appends a vertex to the boundary. |
| `pop_back()` | Removes the last vertex. |
| `clear()` | Removes all vertices. |
| `data()` | Returns the underlying vertex storage pointer. |
| `begin()`, `end()` | Iterator access to boundary vertices. |
| `contains(point, epsilon_deg)` | Tests whether a point lies inside the polygon. |

### Generic Containment

| API | Description |
|------|-------------|
| `is_inside(point, fence)` | Generic dispatcher that calls `fence.contains(point)`. Enables custom fence types that expose a compatible `contains()` member function. |

---

## Usage Examples

### Circular Geofence

```cpp
#include "castle_ext/geodetic/geofence.hpp"

using castle::geodetic::geo_point;

constexpr geo_point<double> center(21.0278, 105.8342);
constexpr geo_point<double> vehicle(21.0300, 105.8400);

castle::geodetic::circular_geofence<double> zone(center, 1000.0);

bool inside = zone.contains(vehicle);
```

### Polygon Geofence

```cpp
#include "castle_ext/geodetic/geofence.hpp"

using castle::geodetic::geo_point;

castle::geodetic::polygon_geofence<double, 8U> region;

region.push_back({20.9, 105.5});
region.push_back({21.3, 105.5});
region.push_back({21.3, 106.1});
region.push_back({20.9, 106.1});

geo_point<double> hanoi(21.0278, 105.8342);

bool inside = region.contains(hanoi);
```

### Boundary Check Using Existing Vertex Data

```cpp
#include "castle_ext/geodetic/geofence.hpp"

using castle::geodetic::geo_point;

const geo_point<double> boundary[] =
{
    {20.9, 105.5},
    {21.3, 105.5},
    {21.3, 106.1},
    {20.9, 106.1}
};

geo_point<double> test_point(21.0278, 105.8342);

bool inside =
    castle::geodetic::point_in_boundary(
        test_point,
        boundary,
        4U);
```

### Generic Fence Interface

```cpp
#include "castle_ext/geodetic/geofence.hpp"

using castle::geodetic::geo_point;

castle::geodetic::circular_geofence<double> fence(
    geo_point<double>(21.0278, 105.8342),
    5000.0);

geo_point<double> point(21.0300, 105.8400);

bool inside =
    castle::geodetic::is_inside(point, fence);
```

---

## Geometric Model

### Circular Fence Evaluation

Circular geofences are evaluated using:

```cpp
haversine_distance(center, point)
    <= radius_m + margin_m
```

This avoids the inaccuracies that would result from treating latitude and longitude as ordinary Cartesian coordinates.

The implementation:

- Uses great-circle distance.
- Remains valid at all latitudes.
- Supports optional expansion or shrinkage through `margin_m`.

### Polygon Fence Evaluation

Polygon boundaries are projected onto a plane using:

```cpp
x = longitude
y = latitude
```

The resulting planar polygon is then evaluated using a ray-casting containment test.

Boundary edges are treated as inclusive:

```cpp
point_on_segment(...)
```

is evaluated before ray casting, so points lying directly on an edge are reported as inside.

---

## Extending With Custom Fence Types

Any type exposing:

```cpp
bool contains(const geo_point<T>& point) const;
```

can be used with:

```cpp
is_inside(point, fence);
```

Example:

```cpp
class custom_fence
{
public:
    bool contains(
        const castle::geodetic::geo_point<double>& point) const
    {
        return point.latitude() > 0.0;
    }
};

custom_fence fence;

bool inside =
    castle::geodetic::is_inside(point, fence);
```

No modification to `geofence.hpp` is required.

---

## Constraints & Notes

### Type Requirements

- `T` must be a floating-point type.
- Violations produce compile-time errors through `static_assert`.

### Circular Geofences

- `radius_m` must be non-negative.
- Negative radii violate the documented precondition.
- Distance calculations use `haversine_distance()`.

### Polygon Geofences

- `polygon_geofence<T, Capacity>` requires `Capacity >= 3`.
- Fewer than three vertices cannot form a valid region.
- `contains()` returns `false` when the active vertex count is less than three.

### Boundary Functions

`point_in_boundary()` returns `false` when:

```cpp
boundary == nullptr
```

or

```cpp
count < 3
```

### Projection Assumptions

Polygon containment relies on an equirectangular (plate carrée) projection.

The caller is responsible for ensuring that:

- Boundaries do not cross the ±180° antimeridian.
- Boundaries remain sufficiently far from the poles.
- Polygon edges are interpreted as straight lines in projected latitude/longitude space rather than true geodesics.

This matches the simplification used by many lightweight GIS and geofencing systems.

### Memory Characteristics

- No dynamic allocation.
- Fixed-capacity storage.
- Deterministic runtime behavior.
- Suitable for embedded and safety-oriented environments.

---

## Related Components

- `coord.hpp` for geographic coordinate representation.
- `distance.hpp` for Haversine and geodesic distance calculations.
- `datum.hpp` for ellipsoid definitions.
- `castle::math::geometry` for planar geometric primitives and algorithms.
- `castle::container::array` and `array_view` for polygon boundary storage and views.
