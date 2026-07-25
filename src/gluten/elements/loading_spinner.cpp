#include "loading_spinner.h"

#include "gluten/theme/theme.h"
#include "imspinner.h"

auto gluten::loading_spinner::render_element(const element_render_info& renderInfo) -> bool
{
    ImSpinner::SpinnerAngEclipse("##Loading", ImGui::GetFontSize() / 2.0f, 2.0f, gluten::theme::white, 8.0f);

    return false;
}