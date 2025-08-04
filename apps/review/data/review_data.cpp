#include "review_data.h"

auto get_review_phase_string(review_phase phase) -> std::string
{
    std::string result;

    switch (phase)
    {
        case review_phase::temp:
            result = "Temp";
            break;
        case review_phase::first_pass:
            result = "First Pass";
            break;
        case review_phase::iteration_pass:
            result = "Iteration Pass";
            break;
        case review_phase::final_pass:
            result = "Final Pass";
            break;
        case review_phase::num:
            break;
        default:
            break;
    }
    return result;
}

auto get_review_quality_string(review_quality quality) -> std::string
{
    std::string result;

    switch (quality)
    {
        case review_quality::c:
            result = "C";
            break;
        case review_quality::b:
            result = "B";
            break;
        case review_quality::a:
            result = "A";
            break;
        case review_quality::a_plus:
            result = "A+";
            break;
        case review_quality::a_plus_plus:
            result = "A++";
            break;
        case review_quality::num:
            break;
        default:
            break;
    }

    return result;
}

auto get_review_status_string(review_status status) -> std::string
{
    std::string result;

    switch (status)
    {
        case review_status::open:
            result = "Open";
            break;
        case review_status::closed:
            result = "Closed";
            break;
        case review_status::archived:
            result = "Archived";
            break;
        default:
            break;
    }

    return result;
}

new_transit_review_data::new_transit_review_data(const new_frontend_review_data& frontendData) : new_review_data_base(frontendData) 
{
    for (const auto& contextFile : frontendData.m_absoluteContextFiles)
    {
        std::ifstream stream(contextFile.string(), std::ios::binary | std::ios::ate);
        const std::streamsize fileSize = stream.tellg();
        std::vector<uint8_t> fileData(fileSize);
        stream.read(reinterpret_cast<char*>(fileData.data()), fileSize);

        const std::string fileName = contextFile.filename().string();

        review_file_data reviewFileData{.m_fileName = fileName, .m_fileData = fileData};
        m_contextFiles.push_back(reviewFileData);
    }

    for (const auto& reviewFile : frontendData.m_absoluteContextFiles)
    {
        std::ifstream stream(reviewFile.string(), std::ios::binary | std::ios::ate);
        const std::streamsize fileSize = stream.tellg();
        std::vector<uint8_t> fileData(fileSize);
        stream.read(reinterpret_cast<char*>(fileData.data()), fileSize);

        const std::string fileName = reviewFile.filename().string();

        review_file_data reviewFileData{.m_fileName = fileName, .m_fileData = fileData};
        m_reviewFiles.push_back(reviewFileData);
    }
}