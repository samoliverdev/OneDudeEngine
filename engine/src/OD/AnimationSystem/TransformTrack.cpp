#include "TransfomTrack.h"

namespace OD{

/*
TransformTrack::TransformTrack(){
    _id = 0;
}

unsigned int TransformTrack::GetId(){
    return _id;
}

void TransformTrack::SetId(unsigned int id){
    _id = id;
}

VectorTrack& TransformTrack::GetPositionTrack(){
    return _position;
}

QuaternionTrack& TransformTrack::GetRotationTrack(){
    return _rotation;
}

VectorTrack& TransformTrack::GetScaleTrack(){
    return _scale;
}

bool TransformTrack::IsValid(){
    return _position.Size() > 1 || _rotation.Size() > 1 || _scale.Size() > 1;
}

float TransformTrack::GetStartTime(){
	float result = 0.0f;
	bool isSet = false;

	if(_position.Size() > 1){
		result = _position.GetStartTime();
		isSet = true;
	}
	if(_rotation.Size() > 1){
		float rotationStart = _rotation.GetStartTime();
		if(rotationStart < result || !isSet){
			result = rotationStart;
			isSet = true;
		}
	}
	if(_scale.Size() > 1){
		float scaleStart = _scale.GetStartTime();
		if(scaleStart < result || !isSet){
			result = scaleStart;
			isSet = true;
		}
	}

	return result;
}

float TransformTrack::GetEndTime() {
	float result = 0.0f;
	bool isSet = false;

	if(_position.Size() > 1){
		result = _position.GetEndTime();
		isSet = true;
	}
	if (_rotation.Size() > 1){
		float rotationEnd = _rotation.GetEndTime();
		if (rotationEnd > result || !isSet) {
			result = rotationEnd;
			isSet = true;
		}
	}
	if(_scale.Size() > 1){
		float scaleEnd = _scale.GetEndTime();
		if (scaleEnd > result || !isSet) {
			result = scaleEnd;
			isSet = true;
		}
	}

	return result;
}

Transform TransformTrack::Sample(const Transform& ref,
	float time, bool looping) {
	Transform result = ref; // Assign default values
	if(_position.Size() > 1){ // Only assign if animated
		result.localPosition(_position.Sample(time, looping));
	}
	if(_rotation.Size() > 1){ // Only assign if animated
		result.localRotation(_rotation.Sample(time, looping));
	}
	if(_scale.Size() > 1){ // Only assign if animated
		result.localScale(_scale.Sample(time, looping));
	}
	return result;
}
*/

FastTransformTrack OptimizeTransformTrack(TransformTrack& input) {
	FastTransformTrack result;

	result.SetId(input.GetId());
	result.GetPositionTrack() = OptimizeTrack<Vector3, 3>(input.GetPositionTrack());
	result.GetRotationTrack() = OptimizeTrack<Quaternion, 4>(input.GetRotationTrack());
	result.GetScaleTrack() = OptimizeTrack<Vector3, 3>(input.GetScaleTrack());

	return result;
}

}