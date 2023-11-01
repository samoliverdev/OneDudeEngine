#include "GLTFLoader.h"
#include <iostream>
#include "OD/Core/Transform.h"

namespace OD{

namespace GLTFHelpers {
	Transform GetLocalTransform(cgltf_node& node){
		Transform result;

		if(node.has_matrix){
			Matrix4 mat = math::make_mat4(&node.matrix[0]);
			result = Matrix4(mat);
		}

		if(node.has_translation){
			result.localPosition(Vector3(node.translation[0], node.translation[1], node.translation[2]));
		}

		if(node.has_rotation){
			result.localRotation(Quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
		}

		if(node.has_scale){
			result.localScale(Vector3(node.scale[0], node.scale[1], node.scale[2]));
		}

		return result;
	}

	int GetNodeIndex(cgltf_node* target, cgltf_node* allNodes, unsigned int numNodes){
		if(target == 0) return -1;
		
		for(unsigned int i = 0; i < numNodes; ++i){
			if(target == &allNodes[i]) return (int)i;
		}

		return -1;
	}

	void GetScalarValues(std::vector<float>& outScalars, unsigned int inComponentCount, const cgltf_accessor& inAccessor){
		outScalars.resize(inAccessor.count * inComponentCount);
		for(cgltf_size i = 0; i < inAccessor.count; ++i){
			cgltf_accessor_read_float(&inAccessor, i, &outScalars[i * inComponentCount], inComponentCount);
		}
	}

	template<typename T, int N>
	void TrackFromChannel(Track<T, N>& inOutTrack, const cgltf_animation_channel& inChannel){
		cgltf_animation_sampler& sampler = *inChannel.sampler;

		Interpolation interpolation = Interpolation::Constant;
		if(inChannel.sampler->interpolation == cgltf_interpolation_type_linear){
			interpolation = Interpolation::Linear;
		} else if(inChannel.sampler->interpolation == cgltf_interpolation_type_cubic_spline){
			interpolation = Interpolation::Cubic;
		}
		bool isSamplerCubic = interpolation == Interpolation::Cubic;
		inOutTrack.SetInterpolation(interpolation);

		std::vector<float> timelineFloats;
		GetScalarValues(timelineFloats, 1, *sampler.input);

		std::vector<float> valueFloats;
		GetScalarValues(valueFloats, N, *sampler.output);

		unsigned int numFrames = (unsigned int)sampler.input->count;
		unsigned int numberOfValuesPerFrame = valueFloats.size() / timelineFloats.size();
		inOutTrack.Resize(numFrames);
		for(unsigned int i = 0; i < numFrames; ++i){
			int baseIndex = i * numberOfValuesPerFrame;
			Frame<N>& frame = inOutTrack[i];
			int offset = 0;

			frame.time = timelineFloats[i];

			for(int component = 0; component < N; ++component){
				frame.in[component] = isSamplerCubic ? valueFloats[baseIndex + offset++] : 0.0f;
			}

			for(int component = 0; component < N; ++component){
				frame.value[component] = valueFloats[baseIndex + offset++];
			}

			for(int component = 0; component < N; ++component){
				frame.out[component] = isSamplerCubic ? valueFloats[baseIndex + offset++] : 0.0f;
			}
		}
	} 


} // End of GLTFHelpers

cgltf_data* LoadGLTFFile(const char* path){
	cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, path, &data);
	if(result != cgltf_result_success){
		LogError("Could not load input file: %s", path);
		return 0;
	}
	result = cgltf_load_buffers(&options, data, path);
	if(result != cgltf_result_success){
		cgltf_free(data);
		LogError("Could not load buffers for: %s", path);
		return 0;
	}
	result = cgltf_validate(data);
	if(result != cgltf_result_success){
		cgltf_free(data);
		LogError("Invalid gltf file: %s", path);
		return 0;
	}
	return data;
}

void FreeGLTFFile(cgltf_data* data){
	if(data == 0){
		LogWarning("Can't free null data");
	} else {
		cgltf_free(data);
	}
}

Pose LoadRestPose(cgltf_data* data){
	unsigned int boneCount = (unsigned int)data->nodes_count;
	Pose result(boneCount);

	for(unsigned int i = 0; i < boneCount; ++i){
		cgltf_node* node = &(data->nodes[i]);

		Transform transform = GLTFHelpers::GetLocalTransform(data->nodes[i]);
		result.SetLocalTransform(i, transform);

		int parent = GLTFHelpers::GetNodeIndex(node->parent, data->nodes, boneCount);
		result.SetParent(i, parent);
	}

	return result;
}

std::vector<std::string> LoadJointNames(cgltf_data* data){
	unsigned int boneCount = (unsigned int)data->nodes_count;
	std::vector<std::string> result(boneCount, "Not Set");

	for(unsigned int i = 0; i < boneCount; ++i){
		cgltf_node* node = &(data->nodes[i]);

		if(node->name == 0){
			result[i] = "EMPTY NODE";
		} else {
			result[i] = node->name;
		}
	}

	return result;
}

std::vector<Clip> LoadAnimationClips(cgltf_data* data){
	unsigned int numClips = (unsigned int)data->animations_count;
	unsigned int numNodes = (unsigned int)data->nodes_count;

	std::vector<Clip> result;
	result.resize(numClips);

	for(unsigned int i = 0; i < numClips; ++i){
		result[i].SetName(data->animations[i].name);

		unsigned int numChannels = (unsigned int)data->animations[i].channels_count;
		for(unsigned int j = 0; j < numChannels; ++j){
			cgltf_animation_channel& channel = data->animations[i].channels[j];
			cgltf_node* target = channel.target_node;
			int nodeId = GLTFHelpers::GetNodeIndex(target, data->nodes, numNodes);
			if(channel.target_path == cgltf_animation_path_type_translation){
				VectorTrack& track = result[i][nodeId].GetPositionTrack();
				GLTFHelpers::TrackFromChannel<Vector3, 3>(track, channel);
			} else if (channel.target_path == cgltf_animation_path_type_scale){
				VectorTrack& track = result[i][nodeId].GetScaleTrack();
				GLTFHelpers::TrackFromChannel<Vector3, 3>(track, channel);
			} else if (channel.target_path == cgltf_animation_path_type_rotation){
				QuaternionTrack& track = result[i][nodeId].GetRotationTrack();
				GLTFHelpers::TrackFromChannel<Quaternion, 4>(track, channel);
			}
		}
		result[i].RecalculateDuration();
	} 

	return result;
}

}