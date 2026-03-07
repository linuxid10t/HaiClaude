#pragma once

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <RadioButton.h>
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

    BTextControl*       fWorkDirField;
    BButton*            fBrowseBtn;
    BFilePanel*         fFilePanel;
    BRadioButton*       fCloudRadio;
    BRadioButton*       fApiRadio;
    BBox*               fModelBox;
    BRadioButton*       fCloudOpusRadio;
    BRadioButton*       fCloudSonnetRadio;
    BRadioButton*       fCloudHaikuRadio;
    BBox*               fApiBox;
    BTextControl*       fApiUrlField;
    BTextControl*       fApiKeyField;
    BCheckBox*          fApiCurrentModelCheck;
    BTextControl*       fApiCurrentModelField;
    BCheckBox*          fApiOpusModelCheck;
    BTextControl*       fApiOpusModelField;
    BCheckBox*          fApiSonnetModelCheck;
    BTextControl*       fApiSonnetModelField;
    BCheckBox*          fApiHaikuModelCheck;
    BTextControl*       fApiHaikuModelField;
    BButton*            fLaunchBtn;
    bool                fIsApiMode;

    BLayoutItem*        fModelBoxItem;
    BLayoutItem*        fApiBoxItem;
    BLayoutItem*        fApiCurrentModelFieldItem;
    BLayoutItem*        fApiOpusModelFieldItem;
    BLayoutItem*        fApiSonnetModelFieldItem;
    BLayoutItem*        fApiHaikuModelFieldItem;
};
