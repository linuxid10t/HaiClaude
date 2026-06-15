#pragma once

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <ListView.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <StringList.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

class BLayoutItem;

extern BString gPendingExec;

static const uint32 MSG_BROWSE_DIR           = 'mBRW';
static const uint32 MSG_MODE_CLOUD           = 'mCLD';
static const uint32 MSG_MODE_API             = 'mAPI';
static const uint32 MSG_LAUNCH               = 'mLNC';
static const uint32 MSG_API_CURRENT_MODEL    = 'mACM';
static const uint32 MSG_API_OPUS_MODEL       = 'mAOM';
static const uint32 MSG_API_SONNET_MODEL     = 'mASM';
static const uint32 MSG_API_HAIKU_MODEL      = 'mAHM';
static const uint32 MSG_SAVE_PROFILE         = 'mSPR';
static const uint32 MSG_DELETE_PROFILE       = 'mDPR';
static const uint32 MSG_PROFILE_LIST_SEL     = 'mPLS';
static const uint32 MSG_PROFILE_SELECTED     = 'mPSL';

class LauncherWindow : public BWindow {
public:
                        LauncherWindow();
                        ~LauncherWindow();
    void                MessageReceived(BMessage* msg) override;

private:
    void                _UpdateModeVisibility();
    void                _ResizeToFit();
    void                _Launch();
    void                _LoadSettings();
    void                _SaveSettings();
    void                _LoadProfiles();
    void                _PopulateProfileMenu();
    void                _SaveProfile();
    void                _DeleteProfile();
    void                _LoadProfileIntoUI(const char* name);
    void                _ApplyAttributionHeaderFix();

    // Mode
    BRadioButton*       fCloudRadio;
    BRadioButton*       fApiRadio;
    bool                fIsApiMode;

    // Cloud model box
    BBox*               fModelBox;
    BRadioButton*       fCloudOpusRadio;
    BRadioButton*       fCloudSonnetRadio;
    BRadioButton*       fCloudHaikuRadio;

    // Working directory
    BTextControl*       fWorkDirField;
    BButton*            fBrowseBtn;
    BFilePanel*         fFilePanel;

    // YOLO mode (always visible)
    BCheckBox*          fYoloCheck;

    // API settings box
    BBox*               fApiBox;
    BTextControl*       fApiUrlField;
    BTextControl*       fApiKeyField;
    BCheckBox*          fSaveApiKeyCheck;
    BCheckBox*          fApiCurrentModelCheck;
    BTextControl*       fApiCurrentModelField;
    BCheckBox*          fApiOpusModelCheck;
    BTextControl*       fApiOpusModelField;
    BCheckBox*          fApiSonnetModelCheck;
    BTextControl*       fApiSonnetModelField;
    BCheckBox*          fApiHaikuModelCheck;
    BTextControl*       fApiHaikuModelField;
    BCheckBox*          fFixAttributionCheck;

    // Profile management box (API mode only)
    BBox*               fProfileBox;
    BPopUpMenu*         fProfileMenu;
    BMenuField*         fProfileCombo;
    BTextControl*       fProfileNameField;
    BButton*            fSaveProfileBtn;
    BListView*          fProfileListView;
    BScrollView*        fProfileListScroll;
    BButton*            fDeleteProfileBtn;

    // Launch button
    BButton*            fLaunchBtn;

    // Layout items for dynamic visibility control
    BLayoutItem*        fModelBoxItem;
    BLayoutItem*        fApiBoxItem;
    BLayoutItem*        fProfileBoxItem;
    BLayoutItem*        fApiCurrentModelFieldItem;
    BLayoutItem*        fApiOpusModelFieldItem;
    BLayoutItem*        fApiSonnetModelFieldItem;
    BLayoutItem*        fApiHaikuModelFieldItem;

    // Profile data (in-memory store)
    BStringList         fProfileNames;
    BMessage            fProfileData;
    BString             fActiveProfile;
};
