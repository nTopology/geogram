#pragma once

#include <geogram/mesh/mesh.h>

#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

namespace GEO::stream {

static constexpr int64_t k_MaxVectorSize = 1ll << 40;

template <class T>
struct EnableBinarySerialize
{
  // IEEE 754 and no long doubles
  static constexpr bool value = std::is_arithmetic_v<T> ||
                                (std::is_floating_point_v<T> && std::numeric_limits<T>::is_iec559 && sizeof(T) <= 64) ||
                                std::is_enum_v<T>;
};

template <class T, std::size_t N>
struct EnableBinarySerialize<std::array<T, N>>
{
  // No padding in arrays
  static constexpr bool value = EnableBinarySerialize<T>::value && sizeof(std::array<T, N>) == sizeof(T) * N;
};

/// Will fail if 'sz' would be less than 0 or more than k_MaxVectorSize.
/// Designed for vector sizes or similar.
inline int readSize(int64_t& sz, std::istream& ist)
{
  int64_t size = 0;
  ist.read(reinterpret_cast<char*>(&size), sizeof(size));
  if (ist.bad() || size < 0 || size > k_MaxVectorSize) {
    return -1;
  }
  sz = size;
  return 0;
}

/// Return 'length' bytes into 'buffer' from 'stream'. Return 0 on success.
inline int readFromStream(std::istream& ist, char* buffer, size_t length)
{
  if (!ist.good()) {
    return -1;
  }
  ist.read(buffer, length);

  if (ist.bad()) {
    return -1;
  }
  assert(static_cast<size_t>(ist.gcount()) == length);

  return 0;
}

/// Read 'n' number of type T into 't' from binary data istream.
/// NOTE: 'n' must not exceed size of container pointed to by 't'.
/// Return 0 on success
template <typename T>
inline int readBytesFromStream(T* t, size_t n, std::istream& ist)
{
  static_assert(EnableBinarySerialize<T>::value);
  if (!ist.good()) {
    return -1;
  }

  ist.read(reinterpret_cast<char*>(t), n * sizeof(T));

  if (ist.bad()) {
    return -1;
  }

  return 0;
}

/// Read 't' from 'ist'. Return 0 on success
template <typename T>
inline int readBytesFromStream(T& t, std::istream& ist)
{
  return readBytesFromStream(&t, 1, ist);
}

/// Read 'vec' from 'ist' in binary form. Return 0 on success.
template <typename T>
inline int readVector(std::vector<T>& vec, std::istream& ist)
{
  static_assert(EnableBinarySerialize<T>::value);
  int64_t size = 0;
  if (0 != readSize(size, ist)) {
    return -1;
  }

  vec.resize(size);
  return readBytesFromStream(vec.data(), size, ist);
}

/// Write 'sv' to ost. Return 0 on success.
inline int writeToStream(std::ostream& ost, std::string_view sv)
{
  if (!ost.good()) {
    return -1;
  }
  ost.write(sv.data(), sv.size());
  if (!ost.good()) {
    return -1;
  }
  return 0;
}

/// Write memory at 't' of size 'n' into 'ost' in binary form. Return 0 on success.
template <typename T>
inline int writeBytesToStream(const T* t, size_t n, std::ostream& ost)
{
  static_assert(EnableBinarySerialize<T>::value);
  if (!ost.good()) {
    return -1;
  }

  ost.write(reinterpret_cast<const char*>(t), sizeof(T) * n);

  if (!ost.good()) {
    return -1;
  }

  return 0;
}

/// Write 't' into 'ost' in binary form. Return 0 on success.
template <typename T>
inline int writeBytesToStream(const T& t, std::ostream& ost)
{
  return writeBytesToStream(&t, 1, ost);
}

/// Write 'vec' to 'ost' in binary form. Return 0 on success.
template <typename T>
inline int writeVector(const std::vector<T>& vec, std::ostream& ost)
{
  static_assert(EnableBinarySerialize<T>::value);
  int64_t s1 = int64_t(vec.size());
  ost.write(reinterpret_cast<const char*>(&s1), sizeof(s1));

  if (!ost.good()) {
    return -1;
  }

  return writeBytesToStream(vec.data(), vec.size(), ost);
}

inline void serializeMesh(const GEO::Mesh& mesh, std::ostream& oss)
{
  GEO::index_t numDimensions = 3;

  auto numVertices = mesh.vertices.nb();
  auto numEdges    = mesh.edges.nb();
  auto numFaces    = mesh.facets.nb();
  auto numCells    = mesh.cells.nb();

  writeBytesToStream(numDimensions, oss);
  writeBytesToStream(numVertices, oss);
  writeBytesToStream(numEdges, oss);
  writeBytesToStream(numFaces, oss);
  writeBytesToStream(numCells, oss);

  // Write vertices
  writeToStream(oss,
                std::string_view(reinterpret_cast<const char*>(mesh.vertices.point_ptr(0)),
                                 size_t(numVertices) * size_t(numDimensions) * sizeof(double)));

  // Write edges
  if (numEdges != 0) {
    writeToStream(oss,
                  std::string_view(reinterpret_cast<const char*>(mesh.edges.vertex_index_ptr(0)),
                                   2 * size_t(numEdges) * sizeof(GEO::index_t)));
  }

  // Write facet distinguisher
  std::vector<std::array<GEO::index_t, 2>> faceVertices;
  for (GEO::index_t f = 0; f != numFaces; ++f) {
    auto numFaceVertices = GEO::index_t(mesh.facets.nb_vertices(f));
    if (faceVertices.empty() || faceVertices.back()[1] != numFaceVertices) {
      faceVertices.push_back({1, numFaceVertices});
    }
    else {
      faceVertices.back()[0]++;
    }
  }
  writeVector(faceVertices, oss);

  // Write facets
  for (GEO::index_t f = 0; f != numFaces; ++f) {
    GEO::index_t fb = mesh.facets.corners_begin(f);
    GEO::index_t nv = mesh.facets.nb_corners(f);
    writeBytesToStream(fb, oss);
    writeBytesToStream(nv, oss);
    writeToStream(
      oss, std::string_view(reinterpret_cast<const char*>(mesh.facet_corners.vertex_index_ptr(fb)), nv * sizeof(GEO::index_t)));
  }

  // Write cell distinguisher
  std::vector<std::array<int, 2>> cellTypes;
  for (GEO::index_t c = 0; c != numCells; ++c) {
    auto t = mesh.cells.type(c);
    if (cellTypes.empty() || cellTypes.back()[1] != t) {
      cellTypes.push_back({1, t});
    }
    else {
      cellTypes.back()[0]++;
    }
  }
  writeVector(cellTypes, oss);

  // Write cells
  for (GEO::index_t c = 0; c != numCells; ++c) {
    auto cb = mesh.cells.corners_begin(c);
    auto nc = mesh.cells.nb_corners(c);
    writeBytesToStream(cb, oss);
    writeBytesToStream(nc, oss);
    writeToStream(
      oss, std::string_view(reinterpret_cast<const char*>(mesh.cell_corners.vertex_index_ptr(cb)), nc * sizeof(GEO::index_t)));
  }
}

inline int deserializeMesh(GEO::Mesh& mesh, std::istream& iss)
{
  GEO::index_t numDimensions;
  GEO::index_t numVertices;
  GEO::index_t numEdges;
  GEO::index_t numFaces;
  GEO::index_t numCells;

  std::vector<std::array<GEO::index_t, 2>> faceVertices;
  std::vector<std::array<GEO::index_t, 2>> cellTypes;

  if (readBytesFromStream(numDimensions, iss) != 0 || readBytesFromStream(numVertices, iss) != 0 ||
      readBytesFromStream(numEdges, iss) != 0 || readBytesFromStream(numFaces, iss) != 0 ||
      readBytesFromStream(numCells, iss) != 0) {
    return -1;
  }

  mesh.clear();

  // Setup vertices
  mesh.vertices.create_vertices(GEO::index_t(numVertices));
  if (readFromStream(iss, (char*)mesh.vertices.point_ptr(0), numVertices * size_t(numDimensions) * sizeof(double)) != 0) {
    return -1;
  }

  // Setup edges
  mesh.edges.create_edges(numEdges);
  if (numEdges != 0) {
    if (readFromStream(iss, (char*)mesh.edges.vertex_index_ptr(0), 2 * size_t(numEdges) * sizeof(GEO::index_t)) != 0) {
      return -1;
    }
  }

  // Setup face distinguisher
  if (readVector(faceVertices, iss) != 0) {
    return -1;
  }

  // Setup faces
  GEO::index_t faceId  = 0;
  GEO::index_t listCtr = 0;
  GEO::index_t numFaceVertices {};
  for (GEO::index_t f = 0; f != numFaces; ++f) {
    if (f == faceId) {
      numFaceVertices = faceVertices[listCtr][1];
      faceId += faceVertices[listCtr][0];
      ++listCtr;
    }
    GEO::index_t fb;
    GEO::index_t nv;
    if (readBytesFromStream(fb, iss) != 0 || readBytesFromStream(nv, iss) != 0) {
      return -1;
    }
    mesh.facets.create_polygon((GEO::index_t)numFaceVertices);
    if (readFromStream(iss, (char*)mesh.facet_corners.vertex_index_ptr(fb), nv * sizeof(GEO::index_t)) != 0) {
      return -1;
    }
  }

  // Setup cell distinguisher
  if (readVector(cellTypes, iss) != 0) {
    return -1;
  }

  // Setup cells
  for (GEO::index_t c = 0; c < cellTypes.size(); c++) {
    mesh.cells.create_cells((GEO::index_t)cellTypes[c][0], (GEO::MeshCellType)cellTypes[c][1]);
  }

  for (GEO::index_t c = 0; c < numCells; c++) {
    GEO::index_t cb;
    GEO::index_t nc;
    if (readBytesFromStream(cb, iss) != 0 || readBytesFromStream(nc, iss) != 0 ||
        readFromStream(iss, (char*)mesh.cell_corners.vertex_index_ptr(cb), nc * sizeof(GEO::index_t)) != 0) {
      return -1;
    }
  }
  return 0;
}
}  // namespace GEO::stream