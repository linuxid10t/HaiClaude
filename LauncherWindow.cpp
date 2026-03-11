/*  LauncherWindow.cpp – HaiClaude */

#include "LauncherWindow.h"

#include <Application.h>
#include <Layout.h>
#include <LayoutBuilder.h>
#include <LayoutItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Rect.h>
#include <Roster.h>
#include <String.h>

#include <Entry.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <Alert.h>

#include <cstring>
#include <unistd.h>

static const char* kClaudeBin   = "/boot/home/.npm-global/bin/claude";
static const char* kTerminalSig = "application/x-vnd.Haiku-Terminal";

// ---------------------------------------------------------------------------
// Helper: Shell Escape - prevents shell injection by escaping single quotes
// ---------------------------------------------------------------------------

static BString shellEscape(const BString& s)
{
    BString result = "'";
    BString escaped = s;
    escaped.ReplaceAll("'", "'\\''");
    result << escaped << "'";
    return result;
}

// ---------------------------------------------------------------------------
// LauncherWindow
// ---------------------------------------------------------------------------

LauncherWindow::LauncherWindow()
    : BWindow(BRect(100, 100, 500, 300),
              "Claude Code Launcher",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
    fIsApiMode = false;
    fCloudRadio = new BRadioButton("cloudRadio", "Cloud",
                                  new BMessage(MSG_MODE_CLOUD));
    fApiRadio   = new BRadioButton("apiRadio", "API  (API key)",
                                  new BMessage(MSG_MODE_API));
    fCloudRadio->SetValue(B_CONTROL_ON);

    // Model selection radio buttons in their own BBox (separate radio group)
    fCloudOpusRadio = new BRadioButton("cloudOpus", "Opus", nullptr);
    fCloudSonnetRadio = new BRadioButton("cloudSonnet", "Sonnet", nullptr);
    fCloudHaikuRadio = new BRadioButton("cloudHaiku", "Haiku", nullptr);
    fCloudOpusRadio->SetValue(B_CONTROL_ON);

    fModelBox = new BBox("modelBox");
    fModelBox->SetLabel("Model");
    BLayoutBuilder::Group<>(fModelBox, B_HORIZONTAL, B_USE_SMALL_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
                   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fCloudOpusRadio)
        .Add(fCloudSonnetRadio)
        .Add(fCloudHaikuRadio)
        .End();

    // API mode controls
    fApiUrlField = new BTextControl("apiUrl", "API URL:",
                                    "https://api.anthropic.com/v1", nullptr);
    fApiUrlField->SetDivider(70);

    fApiKeyField = new BTextControl("apiKey", "API Key:", "", nullptr);
    fApiKeyField->SetDivider(70);
    fApiKeyField->TextView()->HideTyping(true);

    fSaveApiKeyCheck = new BCheckBox("saveApiKeyCheck", "Remember key", nullptr);

    fApiCurrentModelCheck = new BCheckBox("apiCurrentModelCheck", "Override current model",
                                          new BMessage(MSG_API_CURRENT_MODEL));
    fApiCurrentModelField = new BTextControl("apiCurrentModel", "", "claude-sonnet-4-6", nullptr);
    fApiCurrentModelField->SetDivider(0);

    fApiOpusModelCheck = new BCheckBox("apiOpusModelCheck", "Override ANTHROPIC_DEFAULT_OPUS_MODEL",
                                       new BMessage(MSG_API_OPUS_MODEL));
    fApiOpusModelField = new BTextControl("apiOpusModel", "", "claude-opus-4-20250514", nullptr);
    fApiOpusModelField->SetDivider(0);

    fApiSonnetModelCheck = new BCheckBox("apiSonnetModelCheck", "Override ANTHROPIC_DEFAULT_SONNET_MODEL",
                                         new BMessage(MSG_API_SONNET_MODEL));
    fApiSonnetModelField = new BTextControl("apiSonnetModel", "", "claude-sonnet-4-6", nullptr);
    fApiSonnetModelField->SetDivider(0);

    fApiHaikuModelCheck = new BCheckBox("apiHaikuModelCheck", "Override ANTHROPIC_DEFAULT_HAIKU_MODEL",
                                        new BMessage(MSG_API_HAIKU_MODEL));
    fApiHaikuModelField = new BTextControl("apiHaikuModel", "", "claude-haiku-4-20250514", nullptr);
    fApiHaikuModelField->SetDivider(0);

    fApiBox = new BBox("apiBox");
    fApiBox->SetLabel("API Settings");

    BLayoutBuilder::Group<>(fApiBox, B_VERTICAL, B_USE_SMALL_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
                   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fApiUrlField)
        .Add(fApiKeyField)
        .Add(fSaveApiKeyCheck)
        .Add(fApiCurrentModelCheck)
        .Add(fApiCurrentModelField)
        .Add(fApiOpusModelCheck)
        .Add(fApiOpusModelField)
        .Add(fApiSonnetModelCheck)
        .Add(fApiSonnetModelField)
        .Add(fApiHaikuModelCheck)
        .Add(fApiHaikuModelField)
        .End();

    fWorkDirField = new BTextControl("workDir", "Directory:", "/boot/home", nullptr);
    fWorkDirField->SetDivider(70);
    fBrowseBtn = new BButton("browse", "Browse\xe2\x80\xa6", new BMessage(MSG_BROWSE_DIR));

    fFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), nullptr,
                                B_DIRECTORY_NODE, false);

    fLaunchBtn = new BButton("launchBtn", "Launch Claude Code", new BMessage(MSG_LAUNCH));
    fLaunchBtn->MakeDefault(true);

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(new BStringView("modeLabel", "Launch mode:"))
        .AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
            .SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
            .Add(fCloudRadio)
            .End()
        .Add(fModelBox)
        .AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
            .SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
            .Add(fApiRadio)
            .End()
        .AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
            .Add(fWorkDirField)
            .Add(fBrowseBtn, 0.0f)
            .End()
        .Add(fApiBox)
        .AddGlue()
        .Add(fLaunchBtn)
        .End();

    // Find layout items for dynamic visibility control via SetVisible()
    fModelBoxItem = nullptr;
    fApiBoxItem = nullptr;
    BLayout* rootLayout = GetLayout();
    for (int32 i = 0; i < rootLayout->CountItems(); i++) {
        BLayoutItem* item = rootLayout->ItemAt(i);
        if (item->View() == fModelBox)
            fModelBoxItem = item;
        else if (item->View() == fApiBox)
            fApiBoxItem = item;
    }

    fApiCurrentModelFieldItem = nullptr;
    fApiOpusModelFieldItem = nullptr;
    fApiSonnetModelFieldItem = nullptr;
    fApiHaikuModelFieldItem = nullptr;
    BLayout* apiLayout = fApiBox->GetLayout();
    for (int32 i = 0; i < apiLayout->CountItems(); i++) {
        BLayoutItem* item = apiLayout->ItemAt(i);
        BView* view = item->View();
        if (view == fApiCurrentModelField)
            fApiCurrentModelFieldItem = item;
        else if (view == fApiOpusModelField)
            fApiOpusModelFieldItem = item;
        else if (view == fApiSonnetModelField)
            fApiSonnetModelFieldItem = item;
        else if (view == fApiHaikuModelField)
            fApiHaikuModelFieldItem = item;
    }

    // Set initial visibility via layout items (not BView::Hide)
    if (fApiCurrentModelFieldItem)
        fApiCurrentModelFieldItem->SetVisible(false);
    if (fApiOpusModelFieldItem)
        fApiOpusModelFieldItem->SetVisible(false);
    if (fApiSonnetModelFieldItem)
        fApiSonnetModelFieldItem->SetVisible(false);
    if (fApiHaikuModelFieldItem)
        fApiHaikuModelFieldItem->SetVisible(false);

    InvalidateLayout(true);
    BSize preferred = GetLayout()->PreferredSize();
    ResizeTo(preferred.width, preferred.height);
    CenterOnScreen();

    _LoadSettings();
    _UpdateModeVisibility();
}

LauncherWindow::~LauncherWindow()
{
    delete fFilePanel;
}

void
LauncherWindow::MessageReceived(BMessage* msg)
{
    switch (msg->what) {
        case MSG_BROWSE_DIR:
            fFilePanel->Show();
            break;
        case B_REFS_RECEIVED: {
            entry_ref ref;
            if (msg->FindRef("refs", &ref) == B_OK) {
                BEntry entry(&ref);
                BPath path;
                if (entry.GetPath(&path) == B_OK)
                    fWorkDirField->SetText(path.Path());
            }
            break;
        }
        case MSG_MODE_CLOUD:
            fIsApiMode = false;
            fCloudRadio->SetValue(B_CONTROL_ON);
            fApiRadio->SetValue(B_CONTROL_OFF);
            _UpdateModeVisibility();
            break;
        case MSG_MODE_API:
            fIsApiMode = true;
            fApiRadio->SetValue(B_CONTROL_ON);
            fCloudRadio->SetValue(B_CONTROL_OFF);
            _UpdateModeVisibility();
            break;
        case MSG_LAUNCH:
            _Launch();
            break;
        case MSG_API_CURRENT_MODEL:
            if (fApiCurrentModelFieldItem)
                fApiCurrentModelFieldItem->SetVisible(
                    fApiCurrentModelCheck->Value() == B_CONTROL_ON);
            _ResizeToFit();
            break;
        case MSG_API_OPUS_MODEL:
            if (fApiOpusModelFieldItem)
                fApiOpusModelFieldItem->SetVisible(
                    fApiOpusModelCheck->Value() == B_CONTROL_ON);
            _ResizeToFit();
            break;
        case MSG_API_SONNET_MODEL:
            if (fApiSonnetModelFieldItem)
                fApiSonnetModelFieldItem->SetVisible(
                    fApiSonnetModelCheck->Value() == B_CONTROL_ON);
            _ResizeToFit();
            break;
        case MSG_API_HAIKU_MODEL:
            if (fApiHaikuModelFieldItem)
                fApiHaikuModelFieldItem->SetVisible(
                    fApiHaikuModelCheck->Value() == B_CONTROL_ON);
            _ResizeToFit();
            break;
        default:
            BWindow::MessageReceived(msg);
    }
}

void
LauncherWindow::_ResizeToFit()
{
    InvalidateLayout(true);
    BSize preferred = GetLayout()->PreferredSize();
    SetSizeLimits(preferred.width, preferred.width, preferred.height, preferred.height);
    ResizeTo(preferred.width, preferred.height);
}

void
LauncherWindow::_UpdateModeVisibility()
{
    if (fModelBoxItem)
        fModelBoxItem->SetVisible(!fIsApiMode);
    if (fApiBoxItem)
        fApiBoxItem->SetVisible(fIsApiMode);

    _ResizeToFit();
}

void
LauncherWindow::_Launch()
{
    // Validate working directory
    BString workDir = fWorkDirField->Text();
    if (workDir.Length() > 0) {
        BEntry entry(workDir);
        if (!entry.Exists() || !entry.IsDirectory()) {
            BAlert* alert = new BAlert("Invalid Directory",
                "The specified working directory does not exist.", "OK");
            alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
            alert->Go();
            return;
        }
    }

    // Validate API fields if in API mode
    if (fApiRadio->Value() == B_CONTROL_ON) {
        BString apiUrl = fApiUrlField->Text();
        if (apiUrl.Length() == 0) {
            BAlert* alert = new BAlert("Missing API URL",
                "API URL cannot be empty.", "OK");
            alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
            alert->Go();
            return;
        }
        BString apiKey = fApiKeyField->Text();
        if (apiKey.Length() == 0) {
            BAlert* alert = new BAlert("Missing API Key",
                "API Key cannot be empty.", "OK");
            alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
            alert->Go();
            return;
        }
    }

    // Build the shell command
    BString cmd;
    if (workDir.Length() > 0)
        cmd << "cd " << shellEscape(workDir) << " && ";

    if (fCloudRadio->Value() == B_CONTROL_ON) {
        // Unset API key to avoid conflict with claude.ai OAuth token
        cmd << "unset ANTHROPIC_API_KEY && " << kClaudeBin;

        // Add model selection
        if (fCloudSonnetRadio->Value() == B_CONTROL_ON)
            cmd << " --model sonnet";
        else if (fCloudHaikuRadio->Value() == B_CONTROL_ON)
            cmd << " --model haiku";
        else
            cmd << " --model opus";
    } else if (fApiRadio->Value() == B_CONTROL_ON) {
        BString apiUrl = fApiUrlField->Text();
        BString apiKey = fApiKeyField->Text();
        // Use trap to guarantee credentials are restored on exit (even if claude crashes)
        cmd << "trap 'mv \"$HOME/.claude/.credentials.json.bak\" \"$HOME/.claude/.credentials.json\" 2>/dev/null' EXIT; "
            << "mv \"$HOME/.claude/.credentials.json\" \"$HOME/.claude/.credentials.json.bak\" 2>/dev/null; "
            << "ANTHROPIC_BASE_URL=" << shellEscape(apiUrl)
            << " ANTHROPIC_API_KEY=" << shellEscape(apiKey);

        if (fApiOpusModelCheck->Value() == B_CONTROL_ON) {
            BString opusModel = fApiOpusModelField->Text();
            if (opusModel.Length() > 0)
                cmd << " ANTHROPIC_DEFAULT_OPUS_MODEL=" << shellEscape(opusModel);
        }
        if (fApiSonnetModelCheck->Value() == B_CONTROL_ON) {
            BString sonnetModel = fApiSonnetModelField->Text();
            if (sonnetModel.Length() > 0)
                cmd << " ANTHROPIC_DEFAULT_SONNET_MODEL=" << shellEscape(sonnetModel);
        }
        if (fApiHaikuModelCheck->Value() == B_CONTROL_ON) {
            BString haikuModel = fApiHaikuModelField->Text();
            if (haikuModel.Length() > 0)
                cmd << " ANTHROPIC_DEFAULT_HAIKU_MODEL=" << shellEscape(haikuModel);
        }

        cmd << " " << kClaudeBin;

        if (fApiCurrentModelCheck->Value() == B_CONTROL_ON) {
            BString currentModel = fApiCurrentModelField->Text();
            if (currentModel.Length() > 0)
                cmd << " --model " << shellEscape(currentModel);
        }
    }

    _SaveSettings();

    if (isatty(STDIN_FILENO)) {
        // Launched from a terminal — exec claude there after the GUI exits
        // rather than opening a second terminal window.
        gPendingExec = cmd;
    } else {
        // Launched from Tracker/Deskbar — spawn a fresh Terminal process.
        // Using fork+exec (not be_roster->Launch) avoids Terminal opening
        // a second default window via B_READY_TO_RUN.
        entry_ref termRef;
        if (be_roster->FindApp(kTerminalSig, &termRef) == B_OK) {
            BEntry entry(&termRef);
            BPath termPath;
            if (entry.GetPath(&termPath) == B_OK) {
                pid_t pid = fork();
                if (pid == 0) {
                    execl(termPath.Path(), "Terminal",
                          "/bin/sh", "-c", cmd.String(), (char*)nullptr);
                    _exit(1);
                }
            }
        }
    }

    be_app->PostMessage(B_QUIT_REQUESTED);
}

void
LauncherWindow::_LoadSettings()
{
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) return;
    path.Append("HaiClaude");

    BFile file(path.Path(), B_READ_ONLY);
    if (file.InitCheck() != B_OK) return;

    BMessage settings;
    if (settings.Unflatten(&file) != B_OK) return;

    int32 mode = 0;  // 0=cloud, 1=api
    settings.FindInt32("mode", &mode);
    if (mode == 1) {
        fIsApiMode = true;
        fApiRadio->SetValue(B_CONTROL_ON);
        fCloudRadio->SetValue(B_CONTROL_OFF);
    }

    int32 cloudModel = 0;  // 0=opus, 1=sonnet, 2=haiku
    settings.FindInt32("cloudModel", &cloudModel);
    if (cloudModel == 1) {
        fCloudSonnetRadio->SetValue(B_CONTROL_ON);
    } else if (cloudModel == 2) {
        fCloudHaikuRadio->SetValue(B_CONTROL_ON);
    } else {
        fCloudOpusRadio->SetValue(B_CONTROL_ON);
    }

    const char* workDir = nullptr;
    if (settings.FindString("workDir", &workDir) == B_OK)
        fWorkDirField->SetText(workDir);

    const char* apiUrl = nullptr;
    if (settings.FindString("apiUrl", &apiUrl) == B_OK)
        fApiUrlField->SetText(apiUrl);

    // Only load API key if saveApiKey is true
    bool saveApiKey = false;
    if (settings.FindBool("saveApiKey", &saveApiKey) == B_OK)
        fSaveApiKeyCheck->SetValue(saveApiKey ? B_CONTROL_ON : B_CONTROL_OFF);
    if (saveApiKey) {
        const char* apiKey = nullptr;
        if (settings.FindString("apiKey", &apiKey) == B_OK)
            fApiKeyField->SetText(apiKey);
    }

    bool currentModelCheck = false;
    if (settings.FindBool("apiCurrentModelCheck", &currentModelCheck) == B_OK) {
        fApiCurrentModelCheck->SetValue(currentModelCheck ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiCurrentModelFieldItem)
            fApiCurrentModelFieldItem->SetVisible(currentModelCheck);
    }

    const char* apiCurrentModel = nullptr;
    if (settings.FindString("apiCurrentModel", &apiCurrentModel) == B_OK)
        fApiCurrentModelField->SetText(apiCurrentModel);

    bool opusModelCheck = false;
    if (settings.FindBool("apiOpusModelCheck", &opusModelCheck) == B_OK) {
        fApiOpusModelCheck->SetValue(opusModelCheck ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiOpusModelFieldItem)
            fApiOpusModelFieldItem->SetVisible(opusModelCheck);
    }

    const char* apiOpusModel = nullptr;
    if (settings.FindString("apiOpusModel", &apiOpusModel) == B_OK)
        fApiOpusModelField->SetText(apiOpusModel);

    bool sonnetModelCheck = false;
    if (settings.FindBool("apiSonnetModelCheck", &sonnetModelCheck) == B_OK) {
        fApiSonnetModelCheck->SetValue(sonnetModelCheck ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiSonnetModelFieldItem)
            fApiSonnetModelFieldItem->SetVisible(sonnetModelCheck);
    }

    const char* apiSonnetModel = nullptr;
    if (settings.FindString("apiSonnetModel", &apiSonnetModel) == B_OK)
        fApiSonnetModelField->SetText(apiSonnetModel);

    bool haikuModelCheck = false;
    if (settings.FindBool("apiHaikuModelCheck", &haikuModelCheck) == B_OK) {
        fApiHaikuModelCheck->SetValue(haikuModelCheck ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiHaikuModelFieldItem)
            fApiHaikuModelFieldItem->SetVisible(haikuModelCheck);
    }

    const char* apiHaikuModel = nullptr;
    if (settings.FindString("apiHaikuModel", &apiHaikuModel) == B_OK)
        fApiHaikuModelField->SetText(apiHaikuModel);
}

void
LauncherWindow::_SaveSettings()
{
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) return;
    path.Append("HaiClaude");

    BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) return;

    int32 mode = 0;  // 0=cloud, 1=api
    if (fApiRadio->Value() == B_CONTROL_ON)
        mode = 1;

    int32 cloudModel = 0;  // 0=opus, 1=sonnet, 2=haiku
    if (fCloudSonnetRadio->Value() == B_CONTROL_ON)
        cloudModel = 1;
    else if (fCloudHaikuRadio->Value() == B_CONTROL_ON)
        cloudModel = 2;

    BMessage settings;
    settings.AddInt32("mode", mode);
    settings.AddInt32("cloudModel", cloudModel);
    settings.AddString("workDir", fWorkDirField->Text());
    settings.AddString("apiUrl", fApiUrlField->Text());

    // Only save API key if checkbox is checked
    bool saveApiKey = fSaveApiKeyCheck->Value() == B_CONTROL_ON;
    settings.AddBool("saveApiKey", saveApiKey);
    if (saveApiKey)
        settings.AddString("apiKey", fApiKeyField->Text());

    settings.AddBool("apiCurrentModelCheck", fApiCurrentModelCheck->Value() == B_CONTROL_ON);
    settings.AddString("apiCurrentModel", fApiCurrentModelField->Text());
    settings.AddBool("apiOpusModelCheck", fApiOpusModelCheck->Value() == B_CONTROL_ON);
    settings.AddString("apiOpusModel", fApiOpusModelField->Text());
    settings.AddBool("apiSonnetModelCheck", fApiSonnetModelCheck->Value() == B_CONTROL_ON);
    settings.AddString("apiSonnetModel", fApiSonnetModelField->Text());
    settings.AddBool("apiHaikuModelCheck", fApiHaikuModelCheck->Value() == B_CONTROL_ON);
    settings.AddString("apiHaikuModel", fApiHaikuModelField->Text());
    settings.Flatten(&file);
}
