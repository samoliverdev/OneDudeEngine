#include <gtest/gtest.h>
#include <OD/Gfx/GfxReflection.h>

using namespace OD;

TEST(GfxReflection, ReflectsShaderVariableTypes)
{
    const char* shader = R"(
        struct Nested {
            float value;
        };

        layout(std140, set = 0, binding = 0) uniform Main {
            int intValue;
            float floatValue;
            vec2 vector2Value;
            vec3 vector3Value;
            vec4 vector4Value;
            mat4 matrix4Value;
            float floatListValue[2];
            vec4 vector4ListValue[2];
            mat4 matrix4ListValue[2];
            Nested bufferValue;
        };

        #ifdef VERTEX
        void main(){
        }
        #endif

        #ifdef FRAGMENT
        void main(){
        }
        #endif
    )";

    Gfx::ShaderReflection reflection;
    Gfx::Reflect(shader, reflection);

    ASSERT_EQ(reflection.bindings.size(), 1u);
    const auto& variables = reflection.bindings.front().variables;
    ASSERT_EQ(variables.size(), 10u);

    EXPECT_EQ(variables[0].type, Gfx::ShaderVariable::Type::Int);
    EXPECT_EQ(variables[1].type, Gfx::ShaderVariable::Type::Float);
    EXPECT_EQ(variables[2].type, Gfx::ShaderVariable::Type::Vector2);
    EXPECT_EQ(variables[3].type, Gfx::ShaderVariable::Type::Vector3);
    EXPECT_EQ(variables[4].type, Gfx::ShaderVariable::Type::Vector4);
    EXPECT_EQ(variables[5].type, Gfx::ShaderVariable::Type::Matrix4);
    EXPECT_EQ(variables[6].type, Gfx::ShaderVariable::Type::FloatList);
    EXPECT_EQ(variables[7].type, Gfx::ShaderVariable::Type::Vector4List);
    EXPECT_EQ(variables[8].type, Gfx::ShaderVariable::Type::Matrix4List);
    EXPECT_EQ(variables[9].type, Gfx::ShaderVariable::Type::Buffer);
}
