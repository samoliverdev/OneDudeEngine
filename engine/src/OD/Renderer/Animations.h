#pragma once

#include "Model.h"
#include "AssimpGLMHelpers.h"

namespace OD{

struct KeyPosition{
    Vector3 position;
    float timeStamp;
};

struct KeyRotation{
    Quaternion orientation;
    float timeStamp;
};

struct KeyScale{
    Vector3 scale;
    float timeStamp;
};

class Bone{
public:

/*reads keyframes from aiNodeAnim*/
    Bone(const std::string& name, int ID, const aiNodeAnim* channel);
	
    /*interpolates  b/w positions,rotations & scaling keys based on the curren time of 
    the animation and prepares the local transformation matrix by combining all keys 
    tranformations*/
    void Update(float animationTime);

    inline Matrix4 GetLocalTransform() { return _localTransform; }
    inline std::string GetBoneName() const { return _name; }
    inline int GetBoneID(){ return _id; }
	

    /* Gets the current index on mKeyPositions to interpolate to based on 
    the current animation time*/
    int GetPositionIndex(float animationTime);

    /* Gets the current index on mKeyRotations to interpolate to based on the 
    current animation time*/
    int GetRotationIndex(float animationTime);

    /* Gets the current index on mKeyScalings to interpolate to based on the 
    current animation time */
    int GetScaleIndex(float animationTime);

private:
    std::vector<KeyPosition> _positions;
    std::vector<KeyRotation> _rotations;
    std::vector<KeyScale> _scales;
    int _numPositions;
    int _numRotations;
    int _numScalings;
	
    Matrix4 _localTransform;
    std::string _name;
    int _id;

    /* Gets normalized value for Lerp & Slerp*/
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);

    /*figures out which position keys to interpolate b/w and performs the interpolation 
    and returns the translation matrix*/
    Matrix4 InterpolatePosition(float animationTime);

    /*figures out which rotations keys to interpolate b/w and performs the interpolation 
    and returns the rotation matrix*/
    Matrix4 InterpolateRotation(float animationTime);

    /*figures out which scaling keys to interpolate b/w and performs the interpolation 
    and returns the scale matrix*/
    Matrix4 Bone::InterpolateScaling(float animationTime);
	
};

/////////////////////////////

struct AnimNodeData{
    Matrix4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AnimNodeData> children;
};

class Animation{
public:
    Animation() = default;
    Animation(const std::string& animationPath, Model* model);
    Animation(const aiScene* scene, aiAnimation* animation, Model* model);
    ~Animation();

    Bone* FindBone(const std::string& name);

    inline float ticksPerSecond() { return _ticksPerSecond; }
    inline float duration() { return _duration;}
    inline const AnimNodeData& rootNode() { return _rootNode; }
    inline const std::map<std::string,BoneInfo>& boneInfoMap(){ return _boneInfoMap;}

private:
    float _duration;
    int _ticksPerSecond;
    std::vector<Bone> _bones;
    AnimNodeData _rootNode;
    std::map<std::string, BoneInfo> _boneInfoMap;

    void ReadMissingBones(const aiAnimation* animation, Model& model);
    void ReadHeirarchyData(AnimNodeData& dest, const aiNode* src);
};

/////////////////////////////////

class Animator{	
public:
    Animator(Animation* Animation);
	
    void UpdateAnimation(float dt);
    void PlayAnimation(Animation* pAnimation);
    void CalculateBoneTransform(const AnimNodeData* node, Matrix4 parentTransform);
	
    inline std::vector<Matrix4> finalBoneMatrices(){ return _finalBoneMatrices; }
		
private:
    std::vector<Matrix4> _finalBoneMatrices;
    Animation* _currentAnimation;
    float _currentTime;
    float _deltaTime;	
};

}