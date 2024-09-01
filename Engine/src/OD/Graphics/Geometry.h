// The MIT License (MIT)
// Copyright (c) 2018 cmf028
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Various ways to transform an AABB with a matrix and compute the result AABB in the new space
// Useful for various culling tasks in rendering like tiled/clustered shading
// Language used is GLSL
// This is a relatively minor extension of the excellent post on frustum AABB culling by Fabian “ryg” Giesen
// https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/

#pragma once
#include "Culling.h"

namespace OD{

// First Optimized implementation
// The vertex transforms have the following form:
// x = m[0][0]*{min.x, max.x} + m[1][0]*{min.y, max.y} + m[2][0]*{min.z, max.z} + m[3][0];
// y = m[0][1]*{min.x, max.x} + m[1][1]*{min.y, max.y} + m[2][1]*{min.z, max.z} + m[3][1];
// z = m[0][2]*{min.x, max.x} + m[1][2]*{min.y, max.y} + m[2][2]*{min.z, max.z} + m[3][2];
// For each component of the output (x,y,z), the simple version will compute both the minimum and the maximum of the 8 possible dot products
// To minimize the 8 dot products, it is sufficient to minimize the set of intermediate multiplies before adding
// The same is true for maximizing the 8 dot products
// On AMD GCN, gives a series of instructions taking 48 cycles for both mat3 and mat4, and slightly reduces register pressure.
AABB transform_aabb_optimized_min_max(AABB box, Matrix4 m){
  // compute column multiplies for the AABB min
  Vector3 min_c1 = m[0] * box.GetMin().x;
  Vector3 min_c2 = m[1] * box.GetMin().y;
  Vector3 min_c3 = m[2] * box.GetMin().z + m[3]; // place 4th column add here for free add (MAD)
	
  // compute column multiplies for the AABB max
  Vector3 max_c1 = m[0] * box.GetMax().x;
  Vector3 max_c2 = m[1] * box.GetMax().y;
  Vector3 max_c3 = m[2] * box.GetMax().z + m[3]; // place 4th column add here for free add (MAD)

  // minimize and maximize the resulting transforms
  Vector3 tmin = min(min_c1,max_c1) + min(min_c2, max_c2) + min(min_c3, max_c3);
  Vector3 tmax = max(min_c1,max_c1) + max(min_c2, max_c2) + max(min_c3, max_c3);
  
  AABB rbox(tmin, tmax);
  
  return rbox;
}

// Second Optimized implementation
// An AABB can be defined using center and extents vectors instead of min and max vectors
// center = 0.5*(max - min)
// extents = max - center
// max = center + extents
// min = center - extents
// If the AABB is in this form, the vertex transforms have the following form:
// x = m[0][0]*{center.x - extents.x, center.x + extents.x} + m[1][0]*{center.y - extents.y, center.y + extents.y} + m[2][0]*{center.z - extents.z, center.z + extents.z} + m[3][0];
// y = m[0][1]*{center.x - extents.x, center.x + extents.x} + m[1][1]*{center.y - extents.y, center.y + extents.y} + m[2][1]*{center.z - extents.z, center.z + extents.z} + m[3][1];
// z = m[0][2]*{center.x - extents.x, center.x + extents.x} + m[1][2]*{center.y - extents.y, center.y + extents.y} + m[2][2]*{center.z - extents.z, center.z + extents.z} + m[3][2];
// This can be factored:
// vec3 t_center = (m * vec4(center,1.0)).xyz
// x = t_center.x + (m[0][0]*{+- extents.x} + m[1][0]*{+- extents.y} + m[2][0]*{+-extents.z});
// y = t_center.y + (m[0][1]*{+- extents.x} + m[1][1]*{+- extents.y} + m[2][1]*{+-extents.z});
// z = t_center.z + (m[0][2]*{+- extents.x} + m[1][2]*{+- extents.y} + m[2][2]*{+-extents.z});
// To maximize the 8 dot products, it is sufficient to take the intermediate multiplies with a positive result before adding
// To minimize the 8 dot products, it is sufficient to take the intermediate multiplies with a negative result before adding
// Or equivalently:
// To maximize the 8 dot products, it is sufficient to take the dot product of the extents vector with the absolute value of the matrix elements
// To minimize the 8 dot products, it is sufficient to negate the maximum dot product
// On AMD GCN, it is free to take the absolute value of the input to an operation
// On AMD GCN, this gives a series of instructions taking 30 cycles for both mat3 and mat4 and significantly reduces register pressure (about 10-12VGPR).
AABB transform_aabb_optimized_abs_center_extents(AABB box, Matrix4 m){
  // transform to center/extents box representation
  Vector3 center = (box.GetMax() + box.GetMin()) * 0.5f;
  Vector3 extents = box.GetMax() - center;

  // transform center
  Vector3 t_center = (m * Vector4(center,1.0));

  // transform extents (take maximum)
  glm::mat3 abs_mat = glm::mat3(abs(m[0]), abs(m[1]), abs(m[2]));
  Vector3 t_extents = abs_mat * extents;

  // transform to min/max box representation
  Vector3 tmin = t_center - t_extents;
  Vector3 tmax = t_center + t_extents;
  
  return AABB(tmin, tmax);;
}

}