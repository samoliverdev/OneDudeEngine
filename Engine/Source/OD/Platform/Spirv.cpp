#include "Spirv.h"
#include <spirv_reflect.h>
#include "OD/Base.h"

namespace OD{

bool SpirvReflect(int set, int bind, void* data, size_t size, UniformBufferDef& out){
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

    LogWarning("%s", sets[set]->bindings[bind]->name);
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
    }

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