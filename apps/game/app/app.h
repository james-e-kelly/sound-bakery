#pragma once

#include "pch.h"

class game_app final : public gluten::app
{
public:
    void post_init() override;

    void open_soundbank();
};
