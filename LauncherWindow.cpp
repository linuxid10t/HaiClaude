/*  LauncherWindow.cpp – HaiClaude */

#include "LauncherWindow.h"

#include <Application.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Rect.h>
#include <Roster.h>
#include <String.h>
#include <Url.h>

#include <netservices2/HttpRequest.h>
#include <netservices2/HttpResult.h>
#include <netservices2/HttpSession.h>

#include <Entry.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>

#include <cstring>
#include <unistd.h>

using namespace BPrivate::Network;

static const char* kClaudeBin   = "/boot/home/.npm-global/bin/claude";
static const char* kTerminalSig = "application/x-vnd.Haiku-Terminal";

// ---------------------------------------------------------------------------
// Background fetch thread
// ---------------------------------------------------------------------------

struct FetchArg {
    BMessenger  window;
    BString     url;
};

static status_t
FetchThread(void* arg)
{
    FetchArg* fa = (FetchArg*)arg;
    BMessage reply(MSG_MODELS_READY);
    try {
        BHttpSession session;
        BHttpRequest req(BUrl(fa->url.String(), false));
        req.SetTimeout(5'000'000);  // 5 s
        BHttpResult result = session.Execute(std::move(req));
        const BHttpBody& body = result.Body();
        if (body.text.has_value())
            reply.AddString("json", body.text.value());
    } catch (...) {
        // empty reply signals error
    }
    fa->window.SendMessage(&reply);
    delete fa;
    return B_OK;
}

// ---------------------------------------------------------------------------
// LauncherWindow
// ---------------------------------------------------------------------------

LauncherWindow::LauncherWindow()
    : BWindow(BRect(100, 100, 500, 300),
              "Claude Code Launcher",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
    , fFetchThread(-1)
{
    fCloudRadio = new BRadioButton("cloudRadio", "Cloud  (Anthropic API)",
                                  new BMessage(MSG_MODE_CLOUD));
    fLocalRadio = new BRadioButton("localRadio", "Local  (Ollama / local LLM)",
                                  new BMessage(MSG_MODE_LOCAL));
    fCloudRadio->SetValue(B_CONTROL_ON);

    fBaseUrlField = new BTextControl("baseUrl", "Base URL:",
                                    "http://localhost:11434/v1", nullptr);
    fBaseUrlField->SetDivider(70);

    fModelField  = new BTextControl("model", "Model:", "", nullptr);
    fModelField->SetDivider(70);

    fModelPopup  = new BPopUpMenu("(none)");
    fModelMenu   = new BMenuField("modelMenu", "", fModelPopup);
    fRefreshBtn  = new BButton("refresh", "Refresh", new BMessage(MSG_FETCH_MODELS));
    fStatusView  = new BStringView("status", "");

    fWorkDirField = new BTextControl("workDir", "Directory:", "/boot/home", nullptr);
    fWorkDirField->SetDivider(70);
    fBrowseBtn = new BButton("browse", "Browse\xe2\x80\xa6", new BMessage(MSG_BROWSE_DIR));

    fFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), nullptr,
                                B_DIRECTORY_NODE, false);

    fLocalBox = new BBox("localBox");
    fLocalBox->SetLabel("Local Settings");

    BLayoutBuilder::Group<>(fLocalBox, B_VERTICAL, B_USE_SMALL_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
                   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fBaseUrlField)
        .Add(fModelField)
        .AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
            .Add(fModelMenu, 1.0f)
            .Add(fRefreshBtn, 0.0f)
            .End()
        .Add(fStatusView)
        .End();

    fLaunchBtn = new BButton("launchBtn", "Launch Claude Code", new BMessage(MSG_LAUNCH));
    fLaunchBtn->MakeDefault(true);

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(new BStringView("modeLabel", "Launch mode:"))
        .AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
            .SetInsets(B_USE_DEFAULT_SPACING, 0, 0, 0)
            .Add(fCloudRadio)
            .Add(fLocalRadio)
            .End()
        .AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
            .Add(fWorkDirField)
            .Add(fBrowseBtn, 0.0f)
            .End()
        .Add(fLocalBox)
        .AddGlue()
        .Add(fLaunchBtn)
        .End();

    fLocalBox->Hide();

    InvalidateLayout(true);
    BSize preferred = GetLayout()->PreferredSize();
    ResizeTo(preferred.width, preferred.height);
    CenterOnScreen();

    _LoadSettings();
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
        case MSG_MODE_LOCAL:
            _UpdateLocalVisibility();
            break;
        case MSG_LAUNCH:
            _Launch();
            break;
        case MSG_FETCH_MODELS:
            _FetchModels();
            break;
        case MSG_MODELS_READY:
            _PopulateModels(msg);
            break;
        case MSG_MODEL_SELECTED: {
            BMenuItem* marked = fModelPopup->FindMarked();
            if (marked != nullptr)
                fModelField->SetText(marked->Label());
            break;
        }
        default:
            BWindow::MessageReceived(msg);
    }
}

void
LauncherWindow::_UpdateLocalVisibility()
{
    if (fLocalRadio->Value() == B_CONTROL_ON)
        fLocalBox->Show();
    else
        fLocalBox->Hide();

    InvalidateLayout(true);
    BSize preferred = GetLayout()->PreferredSize();
    SetSizeLimits(preferred.width, preferred.width, preferred.height, preferred.height);
    ResizeTo(preferred.width, preferred.height);
}

void
LauncherWindow::_FetchModels()
{
    if (fFetchThread >= 0)
        return;  // already in progress

    fStatusView->SetText("Fetching\xe2\x80\xa6");  // "Fetching…" UTF-8
    fRefreshBtn->SetEnabled(false);

    FetchArg* fa = new FetchArg();
    fa->window = BMessenger(this);
    fa->url    = fBaseUrlField->Text();
    fa->url   << "/models";

    fFetchThread = spawn_thread(FetchThread, "model-fetch",
                                B_NORMAL_PRIORITY, fa);
    if (fFetchThread < 0) {
        delete fa;
        fStatusView->SetText("Error: could not spawn thread");
        fRefreshBtn->SetEnabled(true);
        return;
    }
    resume_thread(fFetchThread);
}

void
LauncherWindow::_PopulateModels(BMessage* msg)
{
    fFetchThread = -1;
    fRefreshBtn->SetEnabled(true);

    const char* json = msg->GetString("json", nullptr);
    if (json == nullptr) {
        fStatusView->SetText("Error: no response");
        return;
    }

    // Clear old items
    while (fModelPopup->CountItems() > 0)
        delete fModelPopup->RemoveItem((int32)0);

    // Parse: scan for "id":" ... "
    int count = 0;
    const char* p = json;
    while ((p = strstr(p, "\"id\":\"")) != nullptr) {
        p += 6;
        const char* end = strchr(p, '"');
        if (end == nullptr) break;
        BString id(p, end - p);
        fModelPopup->AddItem(new BMenuItem(id, new BMessage(MSG_MODEL_SELECTED)));
        p = end + 1;
        count++;
    }

    if (count == 0) {
        fStatusView->SetText("No models found");
    } else {
        fModelPopup->ItemAt(0)->SetMarked(true);
        fModelField->SetText(fModelPopup->ItemAt(0)->Label());
        BString s;
        s << count << (count == 1 ? " model" : " models") << " found";
        fStatusView->SetText(s);
    }
}

void
LauncherWindow::_Launch()
{
    // Build the shell command
    BString workDir = fWorkDirField->Text();
    BString cmd;
    if (strlen(workDir) > 0)
        cmd << "cd '" << workDir << "' && ";

    if (fCloudRadio->Value() == B_CONTROL_ON) {
        cmd << kClaudeBin;
    } else {
        BString model = fModelField->Text();
        BString baseUrl = fBaseUrlField->Text();
        cmd << "CLAUDE_CONFIG_DIR=/boot/home/.claude-local"
            << " ANTHROPIC_BASE_URL=" << baseUrl
            << " ANTHROPIC_API_KEY=ollama"
            << " " << kClaudeBin
            << " --model " << model;
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

    bool localMode = false;
    settings.FindBool("localMode", &localMode);
    if (localMode) {
        fLocalRadio->SetValue(B_CONTROL_ON);
        fCloudRadio->SetValue(B_CONTROL_OFF);
        _UpdateLocalVisibility();
    }

    const char* url = nullptr;
    if (settings.FindString("baseUrl", &url) == B_OK)
        fBaseUrlField->SetText(url);

    const char* model = nullptr;
    if (settings.FindString("model", &model) == B_OK)
        fModelField->SetText(model);

    const char* workDir = nullptr;
    if (settings.FindString("workDir", &workDir) == B_OK)
        fWorkDirField->SetText(workDir);
}

void
LauncherWindow::_SaveSettings()
{
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) return;
    path.Append("HaiClaude");

    BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) return;

    BMessage settings;
    settings.AddBool("localMode", fLocalRadio->Value() == B_CONTROL_ON);
    settings.AddString("baseUrl", fBaseUrlField->Text());
    settings.AddString("model", fModelField->Text());
    settings.AddString("workDir", fWorkDirField->Text());
    settings.Flatten(&file);
}
