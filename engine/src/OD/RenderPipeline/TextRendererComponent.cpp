#include "TextRendererComponent.h"

namespace OD{

void TextRendererComponent::OnGui(Entity& e){
    TextRendererComponent& text = e.GetComponent<TextRendererComponent>();

    cereal::ImGuiArchive uiArchive;
    uiArchive(text);
}

}