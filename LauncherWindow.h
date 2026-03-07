#pragma once

#include <Box.h>
#include <Button.h>
#include <FilePanel.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

extern BString gPendingExec;

static const uint32 MSG_BROWSE_DIR   = 'mBRW';
static const uint32 MSG_MODE_CLOUD   = 'mCLD';
static const uint32 MSG_MODE_LOCAL   = 'mLCL';
static const uint32 MSG_LAUNCH       = 'mLNC';
static const uint32 MSG_FETCH_MODELS = 'mFET';
static const uint32 MSG_MODELS_READY   = 'mMRD';
static const uint32 MSG_MODEL_SELECTED = 'mMSL';

class LauncherWindow : public BWindow {
public:
                        LauncherWindow();
                        ~LauncherWindow();
    void                MessageReceived(BMessage* msg) override;

private:
    void                _UpdateLocalVisibility();
    void                _Launch();
    void                _FetchModels();
    void                _PopulateModels(BMessage* msg);
    void                _LoadSettings();
    void                _SaveSettings();

    BTextControl*       fWorkDirField;
    BButton*            fBrowseBtn;
    BFilePanel*         fFilePanel;
    BRadioButton*       fCloudRadio;
    BRadioButton*       fLocalRadio;
    BBox*               fLocalBox;
    BTextControl*       fBaseUrlField;
    BTextControl*       fTokensField;
    BTextControl*       fModelField;
    BMenuField*         fModelMenu;
    BPopUpMenu*         fModelPopup;
    BButton*            fRefreshBtn;
    BStringView*        fStatusView;
    BButton*            fLaunchBtn;
    thread_id           fFetchThread;
};
