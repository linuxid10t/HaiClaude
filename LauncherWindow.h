#pragma once

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

static const uint32 MSG_MODE_CLOUD   = 'mCLD';
static const uint32 MSG_MODE_LOCAL   = 'mLCL';
static const uint32 MSG_LAUNCH       = 'mLNC';
static const uint32 MSG_FETCH_MODELS = 'mFET';
static const uint32 MSG_MODELS_READY = 'mMRD';

class LauncherWindow : public BWindow {
public:
                        LauncherWindow();
    void                MessageReceived(BMessage* msg) override;

private:
    void                _UpdateLocalVisibility();
    void                _Launch();
    void                _FetchModels();
    void                _PopulateModels(BMessage* msg);

    BRadioButton*       fCloudRadio;
    BRadioButton*       fLocalRadio;
    BBox*               fLocalBox;
    BTextControl*       fBaseUrlField;
    BMenuField*         fModelMenu;
    BPopUpMenu*         fModelPopup;
    BButton*            fRefreshBtn;
    BStringView*        fStatusView;
    BButton*            fLaunchBtn;
    thread_id           fFetchThread;
};
