#pragma once
#include "pad_state.h"

// Shared by arcade_play and EasyArcade3.
inline void setDefaultPadConfig(PadTranslator &translator)
{
    using T = PadConfig::Type;
    using AP = PadConfig::AnalogPos;
    using HP = PadConfig::HatPos;
    using B = PadStateButton;
    PadConfig defaultConfig{0, 0, 0, {
                                         {T::ANALOG, 0, AP::L, AP::MID, {}, static_cast<int>(B::LEFT)},
                                         {T::ANALOG, 0, AP::H, AP::MID, {}, static_cast<int>(B::RIGHT)},
                                         {T::ANALOG, 1, AP::L, AP::MID, {}, static_cast<int>(B::UP)},
                                         {T::ANALOG, 1, AP::H, AP::MID, {}, static_cast<int>(B::DOWN)},
                                         {T::HAT, 0, {}, {}, HP::LEFT, static_cast<int>(B::LEFT)},
                                         {T::HAT, 0, {}, {}, HP::RIGHT, static_cast<int>(B::RIGHT)},
                                         {T::HAT, 0, {}, {}, HP::UP, static_cast<int>(B::UP)},
                                         {T::HAT, 0, {}, {}, HP::DOWN, static_cast<int>(B::DOWN)},
                                         {T::BUTTON, 0, {}, {}, {}, static_cast<int>(B::A)},
                                         {T::BUTTON, 1, {}, {}, {}, static_cast<int>(B::B)},
                                         {T::BUTTON, 2, {}, {}, {}, static_cast<int>(B::COIN)},
                                         {T::BUTTON, 3, {}, {}, {}, static_cast<int>(B::START)},
                                         {T::BUTTON, 4, {}, {}, {}, static_cast<int>(B::C)},
                                         {T::BUTTON, 5, {}, {}, {}, static_cast<int>(B::D)},
                                         {T::BUTTON, 6, {}, {}, {}, static_cast<int>(B::E)},
                                         {T::BUTTON, 7, {}, {}, {}, static_cast<int>(B::F)},
                                     },
                            {}};

    translator.setDefaultConfig(std::move(defaultConfig));
}
