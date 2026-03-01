#include "loading_spinner.h"

#include "gluten/theme/carbon/carbon_theme_g100.h"

#include "imspinner.h"

auto gluten::loading_spinner::render_element(const ImRect& elementBox) -> bool
{
    ImSpinner::SpinnerAngEclipse("##Loading", ImGui::GetFontSize() / 2.0f, 2.0f, gluten::theme::white, 8.0f);

    return false;
}