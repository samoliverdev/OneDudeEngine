#include "Animations.h"

namespace OD{

Bone::Bone(const std::string& name, int ID, const aiNodeAnim* channel): _name(name), _id(ID), _localTransform(1.0f){
    _numPositions = channel->mNumPositionKeys;

    for (int positionIndex = 0; positionIndex < _numPositions; ++positionIndex){
        aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
        float timeStamp = channel->mPositionKeys[positionIndex].mTime;
        KeyPosition data;
        data.position = AssimpGLMHelpers::GetGLMVec(aiPosition);
        data.timeStamp = timeStamp;
        _positions.push_back(data);
    }

    _numRotations = channel->mNumRotationKeys;
    for (int rotationIndex = 0; rotationIndex < _numRotations; ++rotationIndex){
        aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
        float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
        KeyRotation data;
        data.orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
        data.timeStamp = timeStamp;
        _rotations.push_back(data);
    }

    _numScalings = channel->mNumScalingKeys;
    for (int keyIndex = 0; keyIndex < _numScalings; ++keyIndex){
        aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
        float timeStamp = channel->mScalingKeys[keyIndex].mTime;
        KeyScale data;
        data.scale = AssimpGLMHelpers::GetGLMVec(scale);
        data.timeStamp = timeStamp;
        _scales.push_back(data);
    }
}

void Bone::Update(float animationTime){
    Matrix4 translation = InterpolatePosition(animationTime);
    //Matrix4 translation = Matrix4::identity;
    Matrix4 rotation = InterpolateRotation(animationTime);
    //Matrix4 rotation = Matrix4::identity;
    Matrix4 scale = InterpolateScaling(animationTime);
    //Matrix4 scale = Matrix4::identity;
    _localTransform = translation * rotation * scale;
}

int Bone::GetPositionIndex(float animationTime){
    for(int index = 0; index < _numPositions - 1; ++index){
        if(animationTime < _positions[index + 1].timeStamp) return index;
    }
    assert(0);
    return 0;
}

int Bone::GetRotationIndex(float animationTime){
    for(int index = 0; index < _numRotations - 1; ++index){
        if(animationTime < _rotations[index + 1].timeStamp) return index;
    }
    assert(0);
    return 0;
}

int Bone::GetScaleIndex(float animationTime){
    for(int index = 0; index < _numScalings - 1; ++index){
        if(animationTime < _scales[index + 1].timeStamp) return index;
    }
    assert(0);
    return 0;
}

float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime){
    float scaleFactor = 0.0f;
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}

Matrix4 Bone::InterpolatePosition(float animationTime){
    if(1 == _numPositions) return Matrix4::Translate(_positions[0].position);

    int p0Index = GetPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(_positions[p0Index].timeStamp, _positions[p1Index].timeStamp, animationTime);
    Vector3 finalPosition = Vector3::Lerp(_positions[p0Index].position, _positions[p1Index].position, scaleFactor);

    return Matrix4::Translate(finalPosition);
}

Matrix4 Bone::InterpolateRotation(float animationTime){
    if(1 == _numRotations){
        auto rotation = _rotations[0].orientation.normalized();
        return Matrix4::ToMatrix(rotation);
    }

    int p0Index = GetRotationIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(_rotations[p0Index].timeStamp, _rotations[p1Index].timeStamp, animationTime);
    Quaternion finalRotation = Quaternion::Slerp(_rotations[p0Index].orientation, _rotations[p1Index].orientation, scaleFactor);
    finalRotation = finalRotation.normalized();
    return Matrix4::ToMatrix(finalRotation);
}

Matrix4 Bone::InterpolateScaling(float animationTime){
    if(1 == _numScalings)
        return Matrix4::Scale(_scales[0].scale);

    int p0Index = GetScaleIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(_scales[p0Index].timeStamp, _scales[p1Index].timeStamp, animationTime);
    Vector3 finalScale = Vector3::Lerp(_scales[p0Index].scale, _scales[p1Index].scale, scaleFactor);
    return Matrix4::Scale(finalScale);
}

//////////////////////////////////////////////

Animation::Animation(const std::string& animationPath, Model* model){
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
    assert(scene && scene->mRootNode);
    auto animation = scene->mAnimations[0];
    
    _duration = animation->mDuration;
    _ticksPerSecond = animation->mTicksPerSecond;
    ReadHeirarchyData(_rootNode, scene->mRootNode);
    ReadMissingBones(animation, *model);
}

Animation::Animation(const aiScene* scene, aiAnimation* animation, Model* model){
    assert(scene && scene->mRootNode);

    _duration = animation->mDuration;
    _ticksPerSecond = animation->mTicksPerSecond;
    ReadHeirarchyData(_rootNode, scene->mRootNode);
    ReadMissingBones(animation, *model);
}

Animation::~Animation(){}

Bone* Animation::FindBone(const std::string& name){
    auto iter = std::find_if(_bones.begin(), _bones.end(),
        [&](const Bone& Bone){
            return Bone.GetBoneName() == name;
        }
    );
    if (iter == _bones.end()) return nullptr;
    else return &(*iter);
}

void Animation::ReadMissingBones(const aiAnimation* animation, Model& model){
    int size = animation->mNumChannels;

    auto& boneInfoMap = model.boneInfoMap();//getting m_BoneInfoMap from Model class
    int& boneCount = model.boneCounter(); //getting the m_BoneCounter from Model class

    LogInfo("%zd %d", boneInfoMap.size(), boneCount);
    Assert(boneInfoMap.size() == boneCount);

    //reading channels(bones engaged in an animation and their keyframes)
    for (int i = 0; i < size; i++){
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (boneInfoMap.find(boneName) == boneInfoMap.end()){
            //Assert(false);
            boneInfoMap[boneName].id = boneCount;
            boneCount++;
        }
        _bones.push_back(Bone(channel->mNodeName.data,
            boneInfoMap[channel->mNodeName.data].id, channel));
    }

    _boneInfoMap = boneInfoMap;
}

void Animation::ReadHeirarchyData(AnimNodeData& dest, const aiNode* src){
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++){
        AnimNodeData newData;
        ReadHeirarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}

//////////////////////////////////////////////

Animator::Animator(Animation* Animation){
    _currentTime = 0.0;
    _currentAnimation = Animation;

    _finalBoneMatrices.reserve(100);

    for (int i = 0; i < 100; i++)
        _finalBoneMatrices.push_back(Matrix4(1.0f));
}

void Animator::UpdateAnimation(float dt){
    _deltaTime = dt;
    if (_currentAnimation){
        _currentTime += _currentAnimation->ticksPerSecond() * dt;
        _currentTime = fmod(_currentTime, _currentAnimation->duration());
        CalculateBoneTransform(&_currentAnimation->rootNode(), Matrix4(1.0f));
    }
}

void Animator::PlayAnimation(Animation* pAnimation){
    _currentAnimation = pAnimation;
    _currentTime = 0.0f;
}

void Animator::CalculateBoneTransform(const AnimNodeData* node, Matrix4 parentTransform){
    std::string nodeName = node->name;
    Matrix4 nodeTransform = node->transformation;

    Bone* Bone = _currentAnimation->FindBone(nodeName);

    if(Bone){
        Bone->Update(_currentTime);
        nodeTransform = Bone->GetLocalTransform();
    }

    Matrix4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = _currentAnimation->boneInfoMap();
    if (boneInfoMap.find(nodeName) != boneInfoMap.end()){
        int index = boneInfoMap[nodeName].id;
        Matrix4 offset = boneInfoMap[nodeName].offset;
        _finalBoneMatrices[index] = globalTransformation * offset;
    }

    for (int i = 0; i < node->childrenCount; i++)
        CalculateBoneTransform(&node->children[i], globalTransformation);
}

}