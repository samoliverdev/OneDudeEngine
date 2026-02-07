#if defined(WEBGPU_SUPPORT)
#include "OD/pch.h"
#include "Spirv.h"
#include "OD/Base.h"
#include <spirv_reflect.h>

namespace OD{

bool SpirvReflectMainSet(void* data, size_t size, MaterialMainSetDef& out){
    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(size, data, &module);
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);

    // Go through each enumerate to examine it
    uint32_t count = 0;

    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);

    Assert(sets.size() >= 1);
    Assert(sets[0]->binding_count >= 1);

    //MainBind
    out.bufferName = std::string(sets[0]->bindings[0]->name);
    out.bufferSize = sets[0]->bindings[0]->block.size;
    for(int i = 0; i < sets[0]->bindings[0]->block.member_count; i++){
        LogWarning("{}", sets[0]->bindings[0]->block.members[i].name);
        //LogWarning("%d", sets[0]->bindings[0]->block.members[i].offset);
        
        MaterialMainSetDef::Member member{};
        member.pos = sets[0]->bindings[0]->block.members[i].offset;
        member.size = sets[0]->bindings[0]->block.members[i].size;
        out.bufferMembers[sets[0]->bindings[0]->block.members[i].name] = member;
    }

    //TexSlots
    for(int i = 1; i < sets[0]->binding_count; i++){
        LogWarning("{} {}", sets[0]->bindings[i]->name, i);
        //LogWarning("%d", sets[0]->bindings[i]->resource_type);
        if(sets[0]->bindings[i]->resource_type == SPV_REFLECT_RESOURCE_FLAG_SRV){
            out.textureBindings[std::string(sets[0]->bindings[i]->name)] = i;
        }
    }

    /*LogWarning("%s", sets[set]->bindings[bind]->name);
    out.name = std::string(sets[set]->bindings[bind]->name);
    out.size = sets[set]->bindings[bind]->block.size;
    for(int i = 0; i < sets[set]->bindings[bind]->block.member_count; i++){
        LogWarning("%s", sets[set]->bindings[bind]->block.members[i].name);
        LogWarning("%d", sets[set]->bindings[bind]->block.members[i].offset);
        
        UniformBufferDef::Member member{};
        member.pos = sets[set]->bindings[bind]->block.members[i].offset;
        member.size = sets[set]->bindings[bind]->block.members[i].size;
        //member.name = sets[set]->bindings[bind]->block.members[i].name;
        out.members[sets[set]->bindings[bind]->block.members[i].name] = member;
        //out.members.emplace_back(member);
    }*/

    /*result = spvReflectEnumerateDescriptorBindings(&module, &count, NULL);
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectDescriptorBinding*> bindings(count);
    result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumerateInterfaceVariables(&module, &count, NULL);
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable*> interface_variables(count);
    result = spvReflectEnumerateInterfaceVariables(&module, &count, interface_variables.data());
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumerateInputVariables(&module, &count, NULL);
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable*> input_variables(count);
    result = spvReflectEnumerateInputVariables(&module, &count, input_variables.data());
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);
    result = spvReflectEnumerateOutputVariables(&module, &count, NULL);
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable*> output_variables(count);
    result = spvReflectEnumerateOutputVariables(&module, &count, output_variables.data());
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumeratePushConstantBlocks(&module, &count, NULL);
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectBlockVariable*> push_constant(count);
    result = spvReflectEnumeratePushConstantBlocks(&module, &count, push_constant.data());
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);*/

    // Can set a breakpoint here and explorer the various variables enumerated.
    spvReflectDestroyShaderModule(&module);

    return true;
}

}
#endif