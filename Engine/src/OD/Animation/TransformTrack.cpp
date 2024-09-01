#include "TransfomTrack.h"

namespace OD{

FastTransformTrack OptimizeTransformTrack(TransformTrack& input) {
	FastTransformTrack result;

	result.SetId(input.GetId());
	result.GetPositionTrack() = OptimizeTrack<Vector3, 3>(input.GetPositionTrack());
	result.GetRotationTrack() = OptimizeTrack<Quaternion, 4>(input.GetRotationTrack());
	result.GetScaleTrack() = OptimizeTrack<Vector3, 3>(input.GetScaleTrack());

	return result;
}

}