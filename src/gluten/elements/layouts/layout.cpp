#include "layout.h"

#include "gluten/theme/carbon/carbon_theme_g100.h"

static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static ImVec2 operator/(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static ImVec2 operator*(const ImVec2& lhs, const float& rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }

gluten::layout::layout(const layout_type& layoutType) : m_layoutType(layoutType) {}

gluten::layout::layout(const layout_type& layoutType, const anchor_preset& anchorPreset)
    : element(anchorPreset), m_layoutType(layoutType)
{
}

gluten::layout::layout(const anchor_preset& anchorPreset)
    : element(anchorPreset), m_layoutType(layout_type::left_to_right)
{
}

gluten::layout& gluten::layout::set_layout_type(const layout_type& type) 
{ 
    m_layoutType = type; 
    return *this;
}

gluten::layout& gluten::layout::set_layout_spacing(float spacing) 
{ 
    m_spacing = spacing; 
    return *this;
}

void gluten::layout::render_spacer_pixels(float horizonalPixels, float verticalPixels)
{
    render_layout_element_pixels(nullptr, horizonalPixels, verticalPixels);
}

void gluten::layout::render_spacer_percent(float horizontalPercent, float verticalPercent)
{
    render_layout_element_percent(nullptr, horizontalPercent, verticalPercent);
}

bool gluten::layout::render_layout_element_full(element* element)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x, elementBox.GetSize().y);
}

bool gluten::layout::render_layout_element_remaining(element* element) 
{ 
    if (m_currentLayoutPos.has_value())
    {
        const ImRect elementBox = get_element_rect(); 
        const ImVec2 remainingSize = get_remaining_layout_size();
        return render_layout_element_internal(elementBox, element, remainingSize.x, remainingSize.y);
    }
    else
    {
        // Don't render if there is nothing "remaining"
        return false;
    }
}

bool gluten::layout::render_layout_element_pixels(element* element, float horizontalPixels, float verticalPixels)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, horizontalPixels, verticalPixels);
}

bool gluten::layout::render_layout_element_pixels_horizontal(element* element, float horizontalPixels)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, horizontalPixels, elementBox.GetHeight());
}

bool gluten::layout::render_layout_element_pixels_vertical(element* element, float verticalPixels)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x, verticalPixels);
}

bool gluten::layout::render_layout_element_percent(element* element, float horizontalPercent, float verticalPercent)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x * horizontalPercent,
                                          elementBox.GetSize().y * verticalPercent);
}

bool gluten::layout::render_layout_element_percent_horizontal(element* element, float horizontalPercent)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x * horizontalPercent,
                                          elementBox.GetSize().y);
}

bool gluten::layout::render_layout_element_percent_vertical(element* element, float verticalPercent)
{
    const ImRect elementBox = get_element_rect();

    return render_layout_element_internal(elementBox, element, elementBox.GetSize().x,
                                          elementBox.GetSize().y * verticalPercent);
}

auto gluten::layout::render_vertical_spacer(float verticalPixels) -> void
{
    render_layout_element_pixels(nullptr, 0.0f, verticalPixels);
}

bool gluten::layout::render_layout_element_internal(const ImRect& thisBox,
                                                    element* element,
                                                    float horizontalPixels,
                                                    float verticalPixels)
{
    bool activated = false;

    const ImVec2 requestedElementSize = element ? element->get_element_content_size(ImVec2(horizontalPixels, verticalPixels)) : ImVec2();

    const ImVec2 sizeGivenToElement = ImVec2(std::max<float>(horizontalPixels, requestedElementSize.x), std::max<float>(verticalPixels, requestedElementSize.y));

    const bool firstLayoutRender = m_firstLayout;
    m_firstLayout                = false;

    if (!m_currentLayoutPos.has_value())
    {
        setup_layout_begin(thisBox);
    }

    ImVec2 currentLayoutPos = m_currentLayoutPos.value();

    switch (m_layoutType)
    {
        case gluten::layout::layout_type::right_to_left:
            currentLayoutPos.x -= sizeGivenToElement.x;
            break;
        case gluten::layout::layout_type::bottom_to_top:
            currentLayoutPos.y -= sizeGivenToElement.y;
            break;
        case gluten::layout::layout_type::left_to_right:
        case gluten::layout::layout_type::top_to_bottom:
        default:
            break;
    }

    // If rendering would go outside the element box
    // drop a new row or column
    const ImVec2 thisSize = thisBox.GetSize();
    switch (m_layoutType)
    {
        case layout::layout_type::left_to_right:
            if (thisSize.x > 1.0f)  // if we have no size, allow going outside our box
            {
                if (currentLayoutPos.x + sizeGivenToElement.x > thisBox.GetTR().x)
                {
                    currentLayoutPos.x = thisBox.Min.x;
                    currentLayoutPos.y += sizeGivenToElement.y + m_spacing;
                }
            }
            break;
    }

    if (element)
    {
        if (has_element_scale())
        {
            element->set_element_content_scale(element->get_element_scale() * get_element_scale());
        }

        activated = element->render({currentLayoutPos, currentLayoutPos + sizeGivenToElement});

        if (s_debug)
        {
            ImDrawList* const drawList = ImGui::GetWindowDrawList();
            drawList->AddRect(currentLayoutPos, currentLayoutPos + sizeGivenToElement,
                                        ImGui::ColorConvertFloat4ToU32(gluten::theme::purple50));
        }
    }

    switch (m_layoutType)
    {
        case gluten::layout::layout_type::left_to_right:
            currentLayoutPos.x += sizeGivenToElement.x + m_spacing;
            break;
        case gluten::layout::layout_type::top_to_bottom:
            currentLayoutPos.y += sizeGivenToElement.y + m_spacing;
            break;
        case gluten::layout::layout_type::right_to_left:
            currentLayoutPos.x -= m_spacing;
            break;
        case gluten::layout::layout_type::bottom_to_top:
            break;
        default:
            break;
    }

    m_currentLayoutPos = currentLayoutPos;

    return activated;
}

void gluten::layout::reset_layout(const ImRect& parent)
{
    const std::pair<ImRect, ImRect> elementBox =
        get_element_box_from_parent(parent, m_minSize, get_element_content_size(), m_alignment, m_padding, m_anchor);
    m_currentRect = elementBox.first;
    setup_layout_begin(elementBox.first);
    m_firstLayout = true;
}

void gluten::layout::finish_layout()
{
    if (m_currentRect.has_value())
    {
        ImGui::SetCursorScreenPos(m_currentRect.value().GetBL());
    }
}

ImVec2 gluten::layout::get_current_layout_pos_local() const
{
    ImVec2 layoutPos = get_current_layout_pos();
    return layoutPos - ImGui::GetWindowPos();
}


auto gluten::layout::get_remaining_layout_size() const -> ImVec2 
{
    ImVec2 sizeRemain;

    if (m_currentLayoutPos.has_value())
    {
        const ImRect elementBox = get_element_rect();
        const ImVec2 currentLayoutPos = m_currentLayoutPos.value();


        switch (m_layoutType)
        {
            case gluten::layout::layout_type::left_to_right:
            case gluten::layout::layout_type::top_to_bottom:
                sizeRemain = ImVec2(elementBox.Max.x - currentLayoutPos.x, elementBox.Max.y - currentLayoutPos.y);
                break;
            case gluten::layout::layout_type::bottom_to_top:
                sizeRemain = ImVec2(elementBox.Max.x - currentLayoutPos.x, currentLayoutPos.y - elementBox.Min.y);
                break;
            case gluten::layout::layout_type::right_to_left:
                sizeRemain = ImVec2(currentLayoutPos.x - elementBox.Min.x, elementBox.Max.y - currentLayoutPos.y);
                break;
            default:
                break;
        }
    }
    
    return sizeRemain;
}

auto gluten::layout::pre_render_element() -> void
{
    m_firstLayout = true;
    m_currentRect.reset();
    m_currentLayoutPos.reset();
}

void gluten::layout::setup_layout_begin(const ImRect& thisBox)
{
    if (m_layoutType == layout_type::left_to_right || m_layoutType == layout_type::top_to_bottom)
    {
        m_currentLayoutPos = thisBox.GetTL();
    }
    else if (m_layoutType == layout_type::right_to_left)
    {
        m_currentLayoutPos = thisBox.GetTR();
    }
    else if (m_layoutType == layout_type::bottom_to_top)
    {
        m_currentLayoutPos = thisBox.GetBL();
    }
}