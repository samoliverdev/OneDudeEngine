#pragma once

#include "OD/Core/Math.h"

namespace OD{

class AssimpGLMHelpers{
public:

	static inline Matrix4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from){
		Matrix4 to;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		return to;
	}

	static inline Vector3 GetGLMVec(const aiVector3D& vec){ 
		return Vector3(vec.x, vec.y, vec.z); 
	}

	static inline Quaternion GetGLMQuat(const aiQuaternion& pOrientation){
		return Quaternion(pOrientation.x, pOrientation.y, pOrientation.z, pOrientation.w);
	}
};

}