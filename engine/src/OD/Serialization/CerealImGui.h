#pragma once

#include "Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Color.h"
#include <entt/entt.hpp>
#include <map>
#include <string>
#include <magic_enum/magic_enum.hpp>

namespace cereal {
    
/*class ImGuiArchiveInternal: public OutputArchive<ImGuiArchiveInternal>{
public:
    ImGuiArchiveInternal(): OutputArchive<ImGuiArchiveInternal>(this){}
    ~ImGuiArchiveInternal() override = default;

    std::string nextPolymorhphicTypeName{};
};*/

class ImGuiArchive: public InputArchive<ImGuiArchive>{
public:
    class Options{
    public:
        
        Options(){}
        ~Options(){}
        
        Options(float _min, float _max, float _stepAmt  ) : mMin(_min), mMax(_max), mStepAmt(_stepAmt){
            mHasMinMax = true;
        }
        
        bool hasMinMax(){ return mHasMinMax; }
        float getMin() { return mMin; }
        float getMax() { return mMax; }

        Options& setMinMax(float _min, float _max){
            mMin = _min;
            mMax = _max;
            mHasMinMax = true;
            return *this;
        }
        Options& useMinMax(bool b){
            mHasMinMax = false;
            return *this;
        }

        float getStep(){ return mStepAmt; }
        
        Options& setStep( float amt ){
            mStepAmt = amt;
            return *this;
        }
        
        Options& setSpacing(float spacing){
            mSpacing = spacing;
            mHasSpacing = true;
            return *this;
        }
        
        float getSpacing(){ return mSpacing; }
        bool hasSpacing(){ return mHasSpacing; }

        bool isColor = false;
        bool colorHDR = false;
         
    private:

        float mSpacing = 0;
        bool mHasSpacing = false;

        float mStepAmt = 0.1;
        
        float mMin = 0;
        float mMax = 10.0;
        bool mHasMinMax = false;
        
    };
    
    ImGuiArchive(): InputArchive<ImGuiArchive>(this){}
    ~ImGuiArchive(){}
    
    template <class T>
    void draw(std::string name, T& value){
        //elementCount = 0;
        elementCount += 1;
        
        if(name == ""){
            name = std::to_string(elementCount);
        }
        
        auto opt = mElementOptions[name];
        
        if(opt.hasSpacing()){
            ImGui::Dummy({0, opt.getSpacing()});
        }
        
        DrawUI(name.c_str(), value, opt);
    }
    
    void setOption(const std::string& elementName, Options option){
        mElementOptions[elementName] = option;
    }

    inline void CleanOptions(){
        mElementOptions.clear();
    }
private:

    template<class T, std::enable_if_t<!std::is_enum<T>{}> * = nullptr>
    void DrawUI(const char* name, T &value, Options opt = Options()){
        elementCount += 1;

        ImGui::PushID(elementCount);

        if(ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen)){
            //ar(value);
            (*this)(value);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    template <class T>
    void DrawUI(const char* name, std::vector<T>& vector, Options opt = Options()){
        int removeIndex = -1;

        if(ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen)){
            int i = 0;
            for(auto &&value : vector){
                std::string _name = std::to_string(i);

                DrawUI(_name.c_str(), value);

                ImGui::PushID(elementCount++);
                ImGui::SameLine();
                if(ImGui::Button("-")){
                    removeIndex = i;
                }
                ImGui::PopID();

                i++;
            }

            ImGui::TreePop();
        }

        if(removeIndex != -1){
            vector.erase(vector.begin() + removeIndex);
        }

        ImGui::PushID(elementCount++);
        if (ImGui::Button("+")) {
            vector.push_back({});
        }
        ImGui::PopID();
    }

    void DrawUI(const char * name, entt::entity& value, Options opt = Options()){
        //ImGui::DragInt(name, &value, opt.getStep());
        ImGui::DragScalar(name, ImGuiDataType_U32, &value, opt.getStep());
    }
    
    void DrawUI(const char * name, int& value, Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragInt(name, &value, opt.getStep(), opt.getMin(), opt.getMax());
        } else {
            ImGui::DragInt(name, &value, opt.getStep());
        }
    }
    
    void DrawUI(const char * name, float& value, Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragFloat(name, &value, opt.getStep(), opt.getMin(), opt.getMax());
            //ImGui::SliderFloat(name, &value, opt.getMin(), opt.getMax());
        } else {
            ImGui::DragFloat(name, &value, opt.getStep());
        }
        
    }
    
    void DrawUI(const char* name, bool& value, Options opt = Options()){
        ImGui::Checkbox(name, &value);
    }
    
    void DrawUI(const char* name, std::string& value, Options opt = Options()){
        std::vector<char> charData(value.begin(), value.end());
        charData.resize(1000);
        
        if(ImGui::InputText(name, &charData[0], charData.size())){
            value = std::string(charData.data() );
        } 
    }

    void DrawUI(const char* name, glm::vec2& value, Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragFloat2(name, &value[0], opt.getStep(), opt.getMin(), opt.getMax() );
        } else {
            ImGui::DragFloat2(name, &value[0], opt.getStep());
        }
    }
    
    void DrawUI(const char* name, glm::vec3& value, Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragFloat3(name, &value[0], opt.getStep(), opt.getMin(), opt.getMax() );
        } else {
            ImGui::DragFloat3(name, &value[0], opt.getStep());
        }
    }
    
    void DrawUI(const char* name, glm::vec4& value, Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragFloat4(name, &value[0], opt.getStep(), opt.getMin(), opt.getMax() );
        } else {
            if(opt.isColor){
                ImGui::ColorEdit4(name, &value[0]);
            } else {
                ImGui::DragFloat4(name, &value[0], opt.getStep());
            }
        }
    }

    void DrawUI(const char* name, OD::Color& value, Options opt = Options()){
        ImGui::ColorEdit4(name, &value.r, opt.colorHDR ? ImGuiColorEditFlags_HDR : ImGuiColorEditFlags_None);
    }
    
    void DrawUI(const char* name, glm::quat& value, Options opt = Options()){
        glm::vec3 euler = glm::eulerAngles(value);
        if(ImGui::DragFloat3(name, &euler[0], opt.getStep())){
            value = glm::quat(euler);
        }
    }

    template <class E, std::enable_if_t<std::is_enum<E>{}> * = nullptr>
    void DrawUI(std::string name, E &e, Options opt = Options()){
        ImGui::PushID(elementCount++);

        static std::unique_ptr<std::vector<std::string>> enumNames{};
        if (!enumNames) {
            enumNames = std::make_unique<std::vector<std::string>>();
            auto vals = magic_enum::enum_names<E>();
            for (auto &v : vals) {
                enumNames->push_back({v.begin(), v.end()});
            }
        }

        std::string currentItem = std::string(magic_enum::enum_name(e));

        ImGui::Text("%s", name.c_str());
        if(ImGui::BeginCombo("##enum_combo", currentItem.c_str())){
            for (int n = 0; n < enumNames->size(); n++) {
                bool is_selected = (currentItem == enumNames->at(n));
                if (ImGui::Selectable(enumNames->at(n).c_str(), is_selected)) {
                    currentItem = enumNames->at(n);
                    std::optional<E> getter = magic_enum::enum_cast<E>(currentItem);
                    if(getter.has_value()) {
                        e = getter.value();
                    }
                }
                if(is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
    }
    
    int elementCount = 0;
    std::map<std::string, Options> mElementOptions;
};

// ----
/*
template <class T> inline
void prologue(ImGuiArchive &, NameValuePair<T> const &){}

template <class T> inline
void epilogue(ImGuiArchive &, NameValuePair<T> const &){}

template <class T, traits::DisableIf<traits::has_minimal_base_class_serialization<T, traits::has_minimal_input_serialization, ImGuiArchive>::value ||
traits::has_minimal_input_serialization<T, ImGuiArchive>::value> = traits::sfinae> inline
void prologue(ImGuiArchive & ar, T const &){}

template <class T, traits::DisableIf<traits::has_minimal_base_class_serialization<T, traits::has_minimal_input_serialization, ImGuiArchive>::value ||
traits::has_minimal_input_serialization<T, ImGuiArchive>::value> = traits::sfinae> inline
void epilogue(ImGuiArchive & ar, T const &){}
*/

// ----
template <class T> inline
void CEREAL_LOAD_FUNCTION_NAME(ImGuiArchive & ar, NameValuePair<T> & t){
    ar.draw<T>(t.name, t.value);
}

template <class T, traits::EnableIf<std::is_arithmetic<T>::value> = traits::sfinae> inline
void CEREAL_LOAD_FUNCTION_NAME(ImGuiArchive & ar, T & t){
    ar.draw<T>("", t);
}

//    template<class CharT, class Traits, class Alloc> inline
//    void CEREAL_LOAD_FUNCTION_NAME(ImGuiArchive & ar, std::basic_string<CharT, Traits, Alloc> & str)
//    {
//        ar.draw<std::string>("", str);
//    }

}
