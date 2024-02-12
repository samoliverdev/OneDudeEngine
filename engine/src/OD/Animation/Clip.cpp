#include "Clip.h"

namespace OD{

FastClip OptimizeClip(Clip& input) {
	FastClip result;

	result.SetName(input.GetName());
	result.SetLooping(input.GetLooping());
	unsigned int size = input.Size();
	for(unsigned int i = 0; i < size; ++i) {
		unsigned int joint = input.GetIdAtIndex(i);
		result[joint] = OptimizeTransformTrack(input[joint]);
	}
	result.RecalculateDuration();

	return result;
}

}