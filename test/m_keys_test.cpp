//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021 Ioan Chera
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "m_keys.h"
#include "testUtils/TempDirContext.hpp"
#include "Instance.h"

#ifdef __APPLE__
#define CMD "CMD"
#else
#define CMD "CTRL"
#endif

void DLG_Notify(EUR_FORMAT_STRING(const char *msg), ...)
{
}

void DLG_ShowError(bool fatal, EUR_FORMAT_STRING(const char* msg), ...)
{
}

static int sUpdates;
namespace menu
{
void updateBindings(Fl_Sys_Menu_Bar *)
{
	++sUpdates;
}
}

void Instance::Status_Set(EUR_FORMAT_STRING(const char *fmt), ...) const
{
}

//========================================================================

//
// Fixture
//
class MKeys : public TempDirContext
{
protected:
    void SetUp() override
    {
        TempDirContext::SetUp();
        static bool loaded;
        if(!loaded)
        {
            static editor_command_t commands[] =
            {
                { "BR_ClearSearch", "Browser", nullptr },
                { "BR_Scroll", "Browser", nullptr },
                { "3D_NAV_Left", NULL, nullptr },
                { "3D_NAV_Right", NULL, nullptr },
                { "LIN_SelectPath", NULL, nullptr },
                { "GivenFile",  "File", nullptr },
                { "Insert",    "Edit", nullptr },
                { "Delete",    "Edit", nullptr },
                { "Mirror",    "General", nullptr },
                { nullptr, nullptr, nullptr },
            };
            M_RegisterCommandList(commands);
            loaded = true;
        }
        global::home_dir = mTempDir;
        global::install_dir = getChildPath("install");
        ASSERT_TRUE(FileMakeDir(global::install_dir));
        mDeleteList.push(global::install_dir);

        writeBindingsFile();
        M_LoadBindings();
        --sUpdates; // don't increment it here
    }

    void TearDown() override
    {
        global::config_file.clear();
        global::install_dir.clear();
        global::home_dir.clear();
        sUpdates = 0;   // reset it to 0
        TempDirContext::TearDown();
    }
private:
    void writeBindingsFile();
};

//
// Write the bindings file
//
void MKeys::writeBindingsFile()
{
    FILE *f = fopen((global::install_dir / "bindings.cfg").u8string().c_str(), "wt");
    ASSERT_NE(f, nullptr);
    mDeleteList.push(global::install_dir / "bindings.cfg");

    fprintf(f, "browser    CMD-k    BR_ClearSearch\n");
    fprintf(f, "browser    PGUP    BR_Scroll    -3\n");
    fprintf(f, "browser    PGDN    BR_Scroll    +3\n");
    fprintf(f, "render    ALT-LEFT    3D_NAV_Left    384\n");
    fprintf(f, "render    ALT-RIGHT    3D_NAV_Right    384\n");
    fprintf(f, "line    E    LIN_SelectPath    /sametex\n");
    fprintf(f, "general    META-n    GivenFile    next\n");
    fprintf(f, "general    CMD-SPACE    Insert    /nofill\n");

    int n = fclose(f);
    ASSERT_EQ(n, 0);

    f = fopen((global::home_dir / "bindings.cfg").u8string().c_str(), "wt");
    ASSERT_NE(f, nullptr);
    mDeleteList.push(global::home_dir / "bindings.cfg");

    fprintf(f, "general SHIFT-DEL    Delete    /keep\n");
    fprintf(f, "general    SHIFT-BS    Delete    /keep\n");
    fprintf(f, "general    CMD-k    Mirror    horiz\n");

    n = fclose(f);
    ASSERT_EQ(n, 0);
}

TEST_F(MKeys, MKeyToString)
{
    ASSERT_EQ(M_KeyToString(EMOD_COMMAND | 'a'), CMD "-a");
    ASSERT_EQ(M_KeyToString(EMOD_SHIFT | 'a'), "A");
    ASSERT_EQ(M_KeyToString(EMOD_SHIFT | FL_Page_Up), "SHIFT-PGUP");
    ASSERT_EQ(M_KeyToString(EMOD_META | FL_Page_Down), "META-PGDN");
}

TEST_F(MKeys, MIsKeyBound)
{
    ASSERT_TRUE(M_IsKeyBound(FL_Page_Up, KeyContext::browser));
    ASSERT_TRUE(M_IsKeyBound(EMOD_COMMAND | 'k', KeyContext::browser));
    ASSERT_TRUE(M_IsKeyBound(EMOD_COMMAND | 'k', KeyContext::general));
    ASSERT_TRUE(M_IsKeyBound(EMOD_SHIFT | 'e', KeyContext::line));
    ASSERT_FALSE(M_IsKeyBound(EMOD_COMMAND | 'k', KeyContext::render));
}

TEST_F(MKeys, MRemoveBindingAndSave)
{
    ASSERT_TRUE(M_IsKeyBound(EMOD_SHIFT | FL_BackSpace, KeyContext::general));
    M_RemoveBinding(EMOD_SHIFT | FL_BackSpace, KeyContext::vertex);
    ASSERT_TRUE(M_IsKeyBound(EMOD_SHIFT | FL_BackSpace, KeyContext::general));
    M_RemoveBinding(EMOD_SHIFT | FL_BackSpace, KeyContext::general);
    ASSERT_FALSE(M_IsKeyBound(EMOD_SHIFT | FL_BackSpace, KeyContext::general));
    M_RemoveBinding(EMOD_ALT | FL_Left, KeyContext::render);
    ASSERT_FALSE(M_IsKeyBound(EMOD_ALT | FL_Left, KeyContext::render));
    M_SaveBindings();

    M_LoadBindings();

    ASSERT_TRUE(M_IsKeyBound(EMOD_COMMAND | 'k', KeyContext::browser));
    ASSERT_TRUE(M_IsKeyBound(FL_Page_Up, KeyContext::browser));
    ASSERT_TRUE(M_IsKeyBound(FL_Page_Down, KeyContext::browser));
    ASSERT_FALSE(M_IsKeyBound(EMOD_ALT | FL_Left, KeyContext::render));
    ASSERT_TRUE(M_IsKeyBound(EMOD_ALT | FL_Right, KeyContext::render));
    ASSERT_TRUE(M_IsKeyBound(EMOD_SHIFT | 'e', KeyContext::line));
    ASSERT_TRUE(M_IsKeyBound(EMOD_META | 'n', KeyContext::general));
    ASSERT_TRUE(M_IsKeyBound(EMOD_COMMAND | ' ', KeyContext::general));
    ASSERT_TRUE(M_IsKeyBound(EMOD_SHIFT | FL_Delete, KeyContext::general));
    ASSERT_FALSE(M_IsKeyBound(EMOD_SHIFT | FL_BackSpace, KeyContext::general));
    ASSERT_TRUE(M_IsKeyBound(EMOD_COMMAND | 'k', KeyContext::general));

}

TEST_F(MKeys, FindKeyCodeForCommandName)
{
    const char *params[MAX_EXEC_PARAM] = {};
    keycode_t code = 0;

    params[0] = "-3";
    ASSERT_TRUE(findKeyCodeForCommandName("BR_Scroll", params, &code));
    ASSERT_EQ(code, FL_Page_Up);

    params[0] = "+3";
    ASSERT_TRUE(findKeyCodeForCommandName("BR_Scroll", params, &code));
    ASSERT_EQ(code, FL_Page_Down);

    params[0] = nullptr;
    ASSERT_FALSE(findKeyCodeForCommandName("Mirror", params, &code));
    params[0] = "horiz";
    ASSERT_TRUE(findKeyCodeForCommandName("Mirror", params, &code));
    ASSERT_EQ(code, EMOD_COMMAND | 'k');
}

TEST_F(MKeys, UpdateMenuBindingsCall)
{
    ASSERT_EQ(sUpdates, 0);
    M_LoadBindings();
    ASSERT_EQ(sUpdates, 1);

    // Restore
    M_CopyBindings();
    ASSERT_EQ(sUpdates, 1);
    M_ChangeBindingKey(0, 'a');
    ASSERT_EQ(sUpdates, 1);

    M_ApplyBindings();
    ASSERT_EQ(sUpdates, 2);

    const char *params[MAX_EXEC_PARAM] = {};
    keycode_t code = 0;
    ASSERT_TRUE(findKeyCodeForCommandName("BR_ClearSearch", params, &code));
    ASSERT_EQ(code, 'a');
    ASSERT_EQ(sUpdates, 2);
}
