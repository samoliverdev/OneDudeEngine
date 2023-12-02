
#pragma once

#include "Serialization.h"
#include "OD/Core/ImGui.h"

#include <map>
#include <string>

namespace cereal {
    
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
    
    template <class T> inline
    void draw(std::string name, T & value){
        elementCount += 1;
        
        if(name == ""){
            name = std::to_string(elementCount);
        }
        
        auto opt = mElementOptions[name];
        
        if(opt.hasSpacing()){
            ImGui::Dummy({0, opt.getSpacing()});
        }
        
        draw(name.c_str(), value, opt);
    }
    
    void setOption(const std::string& elementName, Options option){
        mElementOptions[elementName] = option;
    }
private:
    
    void draw(const char * name, int& value, Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragInt(name, &value, opt.getStep(), opt.getMin(), opt.getMax());
        } else {
            ImGui::DragInt(name, &value, opt.getStep());
        }
    }
    
    void draw(const char * name, float& value, Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragFloat(name, &value, opt.getStep(), opt.getMin(), opt.getMax());
        } else {
            ImGui::DragFloat(name, &value, opt.getStep());
        }
        
    }
    
    void draw(const char* name, bool& value, Options opt = Options()){
        ImGui::Checkbox(name, &value);
    }
    
    void draw(const char* name, std::string& value, Options opt = Options()){
        std::vector<char> charData(value.begin(), value.end());
        charData.resize(1000);
        
        if(ImGui::InputText(name, &charData[0], charData.size())){
            value = std::string(charData.data() );
        } 
    }

    void draw(const char* name, glm::vec2& value, Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragFloat2(name, &value[0], opt.getStep(), opt.getMin(), opt.getMax() );
        } else {
            ImGui::DragFloat2(name, &value[0], opt.getStep());
        }
    }
    
    void draw(const char* name, glm::vec3& value,Options opt = Options()){
        if(opt.hasMinMax()){
            ImGui::DragFloat3(name, &value[0], opt.getStep(), opt.getMin(), opt.getMax() );
        } else {
            ImGui::DragFloat3(name, &value[0], opt.getStep());
        }
    }
    
    void draw(const char* name, glm::quat& value, Options opt = Options()){
        glm::vec3 euler = glm::eulerAngles(value);
        if(ImGui::DragFloat3(name, &euler[0], opt.getStep())){
            value = glm::quat(euler);
        }
    }
    
    int elementCount = 0;
    std::map<std::string, Options> mElementOptions;
};

// ----
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

// ----
template <class T> inline
void CEREAL_LOAD_FUNCTION_NAME( ImGuiArchive & ar, NameValuePair<T> & t){
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
