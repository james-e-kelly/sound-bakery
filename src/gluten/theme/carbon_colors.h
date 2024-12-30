#pragma once

#include "imgui.h"
#include "implot.h"

namespace gluten::theme
{
    consteval ImVec4 adjust_alpha(const ImVec4& color, const float& alpha)
    {
        return ImVec4(color.x, color.y, color.z, alpha);
    }

    consteval ImVec4 hex_to_imgui_imvec4(const unsigned long& hex)
    {
        const float s = 1.0f / 255.0f;

        // The RGB order is backwards here -> BGR
        // Also, set alpha to 1 as the carbon hex values don't hold alpha and therefore
        // everything would get set to 0
        return ImVec4(((hex >> IM_COL32_B_SHIFT) & 0xFF) * s, ((hex >> IM_COL32_G_SHIFT) & 0xFF) * s,
                      ((hex >> IM_COL32_R_SHIFT) & 0xFF) * s, 1.0f);
    }

    constexpr ImVec4 black      = hex_to_imgui_imvec4(0x000000);
    constexpr ImVec4 black100   = black;
    constexpr ImVec4 blackHover = hex_to_imgui_imvec4(0x212121);

    constexpr ImVec4 white      = hex_to_imgui_imvec4(0xffffff);
    constexpr ImVec4 white0     = white;
    constexpr ImVec4 whiteHover = hex_to_imgui_imvec4(0xe8e8e8);

    constexpr ImVec4 yellow10  = hex_to_imgui_imvec4(0xfcf4d6);
    constexpr ImVec4 yellow20  = hex_to_imgui_imvec4(0xfddc69);
    constexpr ImVec4 yellow30  = hex_to_imgui_imvec4(0xf1c21b);
    constexpr ImVec4 yellow40  = hex_to_imgui_imvec4(0xd2a106);
    constexpr ImVec4 yellow50  = hex_to_imgui_imvec4(0xb28600);
    constexpr ImVec4 yellow60  = hex_to_imgui_imvec4(0x8e6a00);
    constexpr ImVec4 yellow70  = hex_to_imgui_imvec4(0x684e00);
    constexpr ImVec4 yellow80  = hex_to_imgui_imvec4(0x483700);
    constexpr ImVec4 yellow90  = hex_to_imgui_imvec4(0x302400);
    constexpr ImVec4 yellow100 = hex_to_imgui_imvec4(0x1c1500);

    constexpr ImVec4 yellow10Hover  = hex_to_imgui_imvec4(0xf8e6a0);
    constexpr ImVec4 yellow20Hover  = hex_to_imgui_imvec4(0xfccd27);
    constexpr ImVec4 yellow30Hover  = hex_to_imgui_imvec4(0xddb00e);
    constexpr ImVec4 yellow40Hover  = hex_to_imgui_imvec4(0xbc9005);
    constexpr ImVec4 yellow50Hover  = hex_to_imgui_imvec4(0x9e7700);
    constexpr ImVec4 yellow60Hover  = hex_to_imgui_imvec4(0x755800);
    constexpr ImVec4 yellow70Hover  = hex_to_imgui_imvec4(0x806000);
    constexpr ImVec4 yellow80Hover  = hex_to_imgui_imvec4(0x5c4600);
    constexpr ImVec4 yellow90Hover  = hex_to_imgui_imvec4(0x3d2e00);
    constexpr ImVec4 yellow100Hover = hex_to_imgui_imvec4(0x332600);

    constexpr ImVec4 orange10  = hex_to_imgui_imvec4(0xfff2e8);
    constexpr ImVec4 orange20  = hex_to_imgui_imvec4(0xffd9be);
    constexpr ImVec4 orange30  = hex_to_imgui_imvec4(0xffb784);
    constexpr ImVec4 orange40  = hex_to_imgui_imvec4(0xff832b);
    constexpr ImVec4 orange50  = hex_to_imgui_imvec4(0xeb6200);
    constexpr ImVec4 orange60  = hex_to_imgui_imvec4(0xba4e00);
    constexpr ImVec4 orange70  = hex_to_imgui_imvec4(0x8a3800);
    constexpr ImVec4 orange80  = hex_to_imgui_imvec4(0x5e2900);
    constexpr ImVec4 orange90  = hex_to_imgui_imvec4(0x3e1a00);
    constexpr ImVec4 orange100 = hex_to_imgui_imvec4(0x231000);

    constexpr ImVec4 orange10Hover  = hex_to_imgui_imvec4(0xffe2cc);
    constexpr ImVec4 orange20Hover  = hex_to_imgui_imvec4(0xffc69e);
    constexpr ImVec4 orange30Hover  = hex_to_imgui_imvec4(0xff9d57);
    constexpr ImVec4 orange40Hover  = hex_to_imgui_imvec4(0xfa6800);
    constexpr ImVec4 orange50Hover  = hex_to_imgui_imvec4(0xcc5500);
    constexpr ImVec4 orange60Hover  = hex_to_imgui_imvec4(0x9e4200);
    constexpr ImVec4 orange70Hover  = hex_to_imgui_imvec4(0xa84400);
    constexpr ImVec4 orange80Hover  = hex_to_imgui_imvec4(0x753300);
    constexpr ImVec4 orange90Hover  = hex_to_imgui_imvec4(0x522200);
    constexpr ImVec4 orange100Hover = hex_to_imgui_imvec4(0x421e00);

    constexpr ImVec4 red10  = hex_to_imgui_imvec4(0xfff1f1);
    constexpr ImVec4 red20  = hex_to_imgui_imvec4(0xffd7d9);
    constexpr ImVec4 red30  = hex_to_imgui_imvec4(0xffb3b8);
    constexpr ImVec4 red40  = hex_to_imgui_imvec4(0xff8389);
    constexpr ImVec4 red50  = hex_to_imgui_imvec4(0xfa4d56);
    constexpr ImVec4 red60  = hex_to_imgui_imvec4(0xda1e28);
    constexpr ImVec4 red70  = hex_to_imgui_imvec4(0xa2191f);
    constexpr ImVec4 red80  = hex_to_imgui_imvec4(0x750e13);
    constexpr ImVec4 red90  = hex_to_imgui_imvec4(0x520408);
    constexpr ImVec4 red100 = hex_to_imgui_imvec4(0x2d0709);

    constexpr ImVec4 red100Hover = hex_to_imgui_imvec4(0x540d11);
    constexpr ImVec4 red90Hover  = hex_to_imgui_imvec4(0x66050a);
    constexpr ImVec4 red80Hover  = hex_to_imgui_imvec4(0x921118);
    constexpr ImVec4 red70Hover  = hex_to_imgui_imvec4(0xc21e25);
    constexpr ImVec4 red60Hover  = hex_to_imgui_imvec4(0xb81922);
    constexpr ImVec4 red50Hover  = hex_to_imgui_imvec4(0xee0713);
    constexpr ImVec4 red40Hover  = hex_to_imgui_imvec4(0xff6168);
    constexpr ImVec4 red30Hover  = hex_to_imgui_imvec4(0xff99a0);
    constexpr ImVec4 red20Hover  = hex_to_imgui_imvec4(0xffc2c5);
    constexpr ImVec4 red10Hover  = hex_to_imgui_imvec4(0xffe0e0);

    constexpr ImVec4 magenta10  = hex_to_imgui_imvec4(0xfff0f7);
    constexpr ImVec4 magenta20  = hex_to_imgui_imvec4(0xffd6e8);
    constexpr ImVec4 magenta30  = hex_to_imgui_imvec4(0xffafd2);
    constexpr ImVec4 magenta40  = hex_to_imgui_imvec4(0xff7eb6);
    constexpr ImVec4 magenta50  = hex_to_imgui_imvec4(0xee5396);
    constexpr ImVec4 magenta60  = hex_to_imgui_imvec4(0xd02670);
    constexpr ImVec4 magenta70  = hex_to_imgui_imvec4(0x9f1853);
    constexpr ImVec4 magenta80  = hex_to_imgui_imvec4(0x740937);
    constexpr ImVec4 magenta90  = hex_to_imgui_imvec4(0x510224);
    constexpr ImVec4 magenta100 = hex_to_imgui_imvec4(0x2a0a18);

    constexpr ImVec4 magenta100Hover = hex_to_imgui_imvec4(0x53142f);
    constexpr ImVec4 magenta90Hover  = hex_to_imgui_imvec4(0x68032e);
    constexpr ImVec4 magenta80Hover  = hex_to_imgui_imvec4(0x8e0b43);
    constexpr ImVec4 magenta70Hover  = hex_to_imgui_imvec4(0xbf1d63);
    constexpr ImVec4 magenta60Hover  = hex_to_imgui_imvec4(0xb0215f);
    constexpr ImVec4 magenta50Hover  = hex_to_imgui_imvec4(0xe3176f);
    constexpr ImVec4 magenta40Hover  = hex_to_imgui_imvec4(0xff57a0);
    constexpr ImVec4 magenta30Hover  = hex_to_imgui_imvec4(0xff94c3);
    constexpr ImVec4 magenta20Hover  = hex_to_imgui_imvec4(0xffbdda);
    constexpr ImVec4 magenta10Hover  = hex_to_imgui_imvec4(0xffe0ef);

    constexpr ImVec4 purple10  = hex_to_imgui_imvec4(0xf6f2ff);
    constexpr ImVec4 purple20  = hex_to_imgui_imvec4(0xe8daff);
    constexpr ImVec4 purple30  = hex_to_imgui_imvec4(0xd4bbff);
    constexpr ImVec4 purple40  = hex_to_imgui_imvec4(0xbe95ff);
    constexpr ImVec4 purple50  = hex_to_imgui_imvec4(0xa56eff);
    constexpr ImVec4 purple60  = hex_to_imgui_imvec4(0x8a3ffc);
    constexpr ImVec4 purple70  = hex_to_imgui_imvec4(0x6929c4);
    constexpr ImVec4 purple80  = hex_to_imgui_imvec4(0x491d8b);
    constexpr ImVec4 purple90  = hex_to_imgui_imvec4(0x31135e);
    constexpr ImVec4 purple100 = hex_to_imgui_imvec4(0x1c0f30);

    constexpr ImVec4 purple100Hover = hex_to_imgui_imvec4(0x341c59);
    constexpr ImVec4 purple90Hover  = hex_to_imgui_imvec4(0x40197b);
    constexpr ImVec4 purple80Hover  = hex_to_imgui_imvec4(0x5b24ad);
    constexpr ImVec4 purple70Hover  = hex_to_imgui_imvec4(0x7c3dd6);
    constexpr ImVec4 purple60Hover  = hex_to_imgui_imvec4(0x7822fb);
    constexpr ImVec4 purple50Hover  = hex_to_imgui_imvec4(0x9352ff);
    constexpr ImVec4 purple40Hover  = hex_to_imgui_imvec4(0xae7aff);
    constexpr ImVec4 purple30Hover  = hex_to_imgui_imvec4(0xc5a3ff);
    constexpr ImVec4 purple20Hover  = hex_to_imgui_imvec4(0xdcc7ff);
    constexpr ImVec4 purple10Hover  = hex_to_imgui_imvec4(0xede5ff);

    constexpr ImVec4 blue10  = hex_to_imgui_imvec4(0xedf5ff);
    constexpr ImVec4 blue20  = hex_to_imgui_imvec4(0xd0e2ff);
    constexpr ImVec4 blue30  = hex_to_imgui_imvec4(0xa6c8ff);
    constexpr ImVec4 blue40  = hex_to_imgui_imvec4(0x78a9ff);
    constexpr ImVec4 blue50  = hex_to_imgui_imvec4(0x4589ff);
    constexpr ImVec4 blue60  = hex_to_imgui_imvec4(0x0f62fe);
    constexpr ImVec4 blue70  = hex_to_imgui_imvec4(0x0043ce);
    constexpr ImVec4 blue80  = hex_to_imgui_imvec4(0x002d9c);
    constexpr ImVec4 blue90  = hex_to_imgui_imvec4(0x001d6c);
    constexpr ImVec4 blue100 = hex_to_imgui_imvec4(0x001141);

    constexpr ImVec4 blue100Hover = hex_to_imgui_imvec4(0x001f75);
    constexpr ImVec4 blue90Hover  = hex_to_imgui_imvec4(0x00258a);
    constexpr ImVec4 blue80Hover  = hex_to_imgui_imvec4(0x0039c7);
    constexpr ImVec4 blue70Hover  = hex_to_imgui_imvec4(0x0053ff);
    constexpr ImVec4 blue60Hover  = hex_to_imgui_imvec4(0x0050e6);
    constexpr ImVec4 blue50Hover  = hex_to_imgui_imvec4(0x1f70ff);
    constexpr ImVec4 blue40Hover  = hex_to_imgui_imvec4(0x5c97ff);
    constexpr ImVec4 blue30Hover  = hex_to_imgui_imvec4(0x8ab6ff);
    constexpr ImVec4 blue20Hover  = hex_to_imgui_imvec4(0xb8d3ff);
    constexpr ImVec4 blue10Hover  = hex_to_imgui_imvec4(0xdbebff);

    constexpr ImVec4 cyan10  = hex_to_imgui_imvec4(0xe5f6ff);
    constexpr ImVec4 cyan20  = hex_to_imgui_imvec4(0xbae6ff);
    constexpr ImVec4 cyan30  = hex_to_imgui_imvec4(0x82cfff);
    constexpr ImVec4 cyan40  = hex_to_imgui_imvec4(0x33b1ff);
    constexpr ImVec4 cyan50  = hex_to_imgui_imvec4(0x1192e8);
    constexpr ImVec4 cyan60  = hex_to_imgui_imvec4(0x0072c3);
    constexpr ImVec4 cyan70  = hex_to_imgui_imvec4(0x00539a);
    constexpr ImVec4 cyan80  = hex_to_imgui_imvec4(0x003a6d);
    constexpr ImVec4 cyan90  = hex_to_imgui_imvec4(0x012749);
    constexpr ImVec4 cyan100 = hex_to_imgui_imvec4(0x061727);

    constexpr ImVec4 cyan10Hover  = hex_to_imgui_imvec4(0xcceeff);
    constexpr ImVec4 cyan20Hover  = hex_to_imgui_imvec4(0x99daff);
    constexpr ImVec4 cyan30Hover  = hex_to_imgui_imvec4(0x57beff);
    constexpr ImVec4 cyan40Hover  = hex_to_imgui_imvec4(0x059fff);
    constexpr ImVec4 cyan50Hover  = hex_to_imgui_imvec4(0x0f7ec8);
    constexpr ImVec4 cyan60Hover  = hex_to_imgui_imvec4(0x005fa3);
    constexpr ImVec4 cyan70Hover  = hex_to_imgui_imvec4(0x0066bd);
    constexpr ImVec4 cyan80Hover  = hex_to_imgui_imvec4(0x00498a);
    constexpr ImVec4 cyan90Hover  = hex_to_imgui_imvec4(0x013360);
    constexpr ImVec4 cyan100Hover = hex_to_imgui_imvec4(0x0b2947);

    constexpr ImVec4 teal10  = hex_to_imgui_imvec4(0xd9fbfb);
    constexpr ImVec4 teal20  = hex_to_imgui_imvec4(0x9ef0f0);
    constexpr ImVec4 teal30  = hex_to_imgui_imvec4(0x3ddbd9);
    constexpr ImVec4 teal40  = hex_to_imgui_imvec4(0x08bdba);
    constexpr ImVec4 teal50  = hex_to_imgui_imvec4(0x009d9a);
    constexpr ImVec4 teal60  = hex_to_imgui_imvec4(0x007d79);
    constexpr ImVec4 teal70  = hex_to_imgui_imvec4(0x005d5d);
    constexpr ImVec4 teal80  = hex_to_imgui_imvec4(0x004144);
    constexpr ImVec4 teal90  = hex_to_imgui_imvec4(0x022b30);
    constexpr ImVec4 teal100 = hex_to_imgui_imvec4(0x081a1c);

    constexpr ImVec4 teal10Hover  = hex_to_imgui_imvec4(0xacf6f6);
    constexpr ImVec4 teal20Hover  = hex_to_imgui_imvec4(0x57e5e5);
    constexpr ImVec4 teal30Hover  = hex_to_imgui_imvec4(0x25cac8);
    constexpr ImVec4 teal40Hover  = hex_to_imgui_imvec4(0x07aba9);
    constexpr ImVec4 teal50Hover  = hex_to_imgui_imvec4(0x008a87);
    constexpr ImVec4 teal60Hover  = hex_to_imgui_imvec4(0x006b68);
    constexpr ImVec4 teal70Hover  = hex_to_imgui_imvec4(0x007070);
    constexpr ImVec4 teal80Hover  = hex_to_imgui_imvec4(0x005357);
    constexpr ImVec4 teal90Hover  = hex_to_imgui_imvec4(0x033940);
    constexpr ImVec4 teal100Hover = hex_to_imgui_imvec4(0x0f3034);

    constexpr ImVec4 green10  = hex_to_imgui_imvec4(0xdefbe6);
    constexpr ImVec4 green20  = hex_to_imgui_imvec4(0xa7f0ba);
    constexpr ImVec4 green30  = hex_to_imgui_imvec4(0x6fdc8c);
    constexpr ImVec4 green40  = hex_to_imgui_imvec4(0x42be65);
    constexpr ImVec4 green50  = hex_to_imgui_imvec4(0x24a148);
    constexpr ImVec4 green60  = hex_to_imgui_imvec4(0x198038);
    constexpr ImVec4 green70  = hex_to_imgui_imvec4(0x0e6027);
    constexpr ImVec4 green80  = hex_to_imgui_imvec4(0x044317);
    constexpr ImVec4 green90  = hex_to_imgui_imvec4(0x022d0d);
    constexpr ImVec4 green100 = hex_to_imgui_imvec4(0x071908);

    constexpr ImVec4 green10Hover  = hex_to_imgui_imvec4(0xb6f6c8);
    constexpr ImVec4 green20Hover  = hex_to_imgui_imvec4(0x74e792);
    constexpr ImVec4 green30Hover  = hex_to_imgui_imvec4(0x36ce5e);
    constexpr ImVec4 green40Hover  = hex_to_imgui_imvec4(0x3bab5a);
    constexpr ImVec4 green50Hover  = hex_to_imgui_imvec4(0x208e3f);
    constexpr ImVec4 green60Hover  = hex_to_imgui_imvec4(0x166f31);
    constexpr ImVec4 green70Hover  = hex_to_imgui_imvec4(0x11742f);
    constexpr ImVec4 green80Hover  = hex_to_imgui_imvec4(0x05521c);
    constexpr ImVec4 green90Hover  = hex_to_imgui_imvec4(0x033b11);
    constexpr ImVec4 green100Hover = hex_to_imgui_imvec4(0x0d300f);

    constexpr ImVec4 coolGray10  = hex_to_imgui_imvec4(0xf2f4f8);
    constexpr ImVec4 coolGray20  = hex_to_imgui_imvec4(0xdde1e6);
    constexpr ImVec4 coolGray30  = hex_to_imgui_imvec4(0xc1c7cd);
    constexpr ImVec4 coolGray40  = hex_to_imgui_imvec4(0xa2a9b0);
    constexpr ImVec4 coolGray50  = hex_to_imgui_imvec4(0x878d96);
    constexpr ImVec4 coolGray60  = hex_to_imgui_imvec4(0x697077);
    constexpr ImVec4 coolGray70  = hex_to_imgui_imvec4(0x4d5358);
    constexpr ImVec4 coolGray80  = hex_to_imgui_imvec4(0x343a3f);
    constexpr ImVec4 coolGray90  = hex_to_imgui_imvec4(0x21272a);
    constexpr ImVec4 coolGray100 = hex_to_imgui_imvec4(0x121619);

    constexpr ImVec4 coolGray10Hover  = hex_to_imgui_imvec4(0xe4e9f1);
    constexpr ImVec4 coolGray20Hover  = hex_to_imgui_imvec4(0xcdd3da);
    constexpr ImVec4 coolGray30Hover  = hex_to_imgui_imvec4(0xadb5bd);
    constexpr ImVec4 coolGray40Hover  = hex_to_imgui_imvec4(0x9199a1);
    constexpr ImVec4 coolGray50Hover  = hex_to_imgui_imvec4(0x757b85);
    constexpr ImVec4 coolGray60Hover  = hex_to_imgui_imvec4(0x585e64);
    constexpr ImVec4 coolGray70Hover  = hex_to_imgui_imvec4(0x5d646a);
    constexpr ImVec4 coolGray80Hover  = hex_to_imgui_imvec4(0x434a51);
    constexpr ImVec4 coolGray90Hover  = hex_to_imgui_imvec4(0x2b3236);
    constexpr ImVec4 coolGray100Hover = hex_to_imgui_imvec4(0x222a2f);

    constexpr ImVec4 gray10  = hex_to_imgui_imvec4(0xf4f4f4);
    constexpr ImVec4 gray20  = hex_to_imgui_imvec4(0xe0e0e0);
    constexpr ImVec4 gray30  = hex_to_imgui_imvec4(0xc6c6c6);
    constexpr ImVec4 gray40  = hex_to_imgui_imvec4(0xa8a8a8);
    constexpr ImVec4 gray50  = hex_to_imgui_imvec4(0x8d8d8d);
    constexpr ImVec4 gray60  = hex_to_imgui_imvec4(0x6f6f6f);
    constexpr ImVec4 gray70  = hex_to_imgui_imvec4(0x525252);
    constexpr ImVec4 gray80  = hex_to_imgui_imvec4(0x393939);
    constexpr ImVec4 gray90  = hex_to_imgui_imvec4(0x262626);
    constexpr ImVec4 gray100 = hex_to_imgui_imvec4(0x161616);

    constexpr ImVec4 gray10Hover  = hex_to_imgui_imvec4(0xe8e8e8);
    constexpr ImVec4 gray20Hover  = hex_to_imgui_imvec4(0xd1d1d1);
    constexpr ImVec4 gray30Hover  = hex_to_imgui_imvec4(0xb5b5b5);
    constexpr ImVec4 gray40Hover  = hex_to_imgui_imvec4(0x999999);
    constexpr ImVec4 gray50Hover  = hex_to_imgui_imvec4(0x7a7a7a);
    constexpr ImVec4 gray60Hover  = hex_to_imgui_imvec4(0x5e5e5e);
    constexpr ImVec4 gray70Hover  = hex_to_imgui_imvec4(0x636363);
    constexpr ImVec4 gray80Hover  = hex_to_imgui_imvec4(0x474747);
    constexpr ImVec4 gray90Hover  = hex_to_imgui_imvec4(0x333333);
    constexpr ImVec4 gray100Hover = hex_to_imgui_imvec4(0x292929);

    constexpr ImVec4 warmGray10  = hex_to_imgui_imvec4(0xf7f3f2);
    constexpr ImVec4 warmGray20  = hex_to_imgui_imvec4(0xe5e0df);
    constexpr ImVec4 warmGray30  = hex_to_imgui_imvec4(0xcac5c4);
    constexpr ImVec4 warmGray40  = hex_to_imgui_imvec4(0xada8a8);
    constexpr ImVec4 warmGray50  = hex_to_imgui_imvec4(0x8f8b8b);
    constexpr ImVec4 warmGray60  = hex_to_imgui_imvec4(0x726e6e);
    constexpr ImVec4 warmGray70  = hex_to_imgui_imvec4(0x565151);
    constexpr ImVec4 warmGray80  = hex_to_imgui_imvec4(0x3c3838);
    constexpr ImVec4 warmGray90  = hex_to_imgui_imvec4(0x272525);
    constexpr ImVec4 warmGray100 = hex_to_imgui_imvec4(0x171414);

    constexpr ImVec4 warmGray10Hover  = hex_to_imgui_imvec4(0xf0e8e6);
    constexpr ImVec4 warmGray20Hover  = hex_to_imgui_imvec4(0xd8d0cf);
    constexpr ImVec4 warmGray30Hover  = hex_to_imgui_imvec4(0xb9b3b1);
    constexpr ImVec4 warmGray40Hover  = hex_to_imgui_imvec4(0x9c9696);
    constexpr ImVec4 warmGray50Hover  = hex_to_imgui_imvec4(0x7f7b7b);
    constexpr ImVec4 warmGray60Hover  = hex_to_imgui_imvec4(0x605d5d);
    constexpr ImVec4 warmGray70Hover  = hex_to_imgui_imvec4(0x696363);
    constexpr ImVec4 warmGray80Hover  = hex_to_imgui_imvec4(0x4c4848);
    constexpr ImVec4 warmGray90Hover  = hex_to_imgui_imvec4(0x343232);
    constexpr ImVec4 warmGray100Hover = hex_to_imgui_imvec4(0x2c2626);
}  // namespace gluten::theme