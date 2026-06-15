/*  LauncherWindow.cpp – HaiClaude */

#include "LauncherWindow.h"

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Layout.h>
#include <LayoutBuilder.h>
#include <LayoutItem.h>
#include <Message.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <Path.h>
#include <Rect.h>
#include <Roster.h>
#include <String.h>
#include <StringItem.h>
#include <Alert.h>

#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

static const char* kClaudeBin   = "/boot/home/.npm-global/bin/claude";
static const char* kTerminalSig = "application/x-vnd.Haiku-Terminal";

// Build a profile settings key: "profile_<name>_<suffix>"
static BString pkey(const BString& pfx, const char* suffix)
{
    BString k(pfx);
    k.Append(suffix);
    return k;
}

// Shell escaping: wrap in single quotes, escape internal single quotes
static BString shellEscape(const BString& s)
{
    BString result = "'";
    BString escaped = s;
    escaped.ReplaceAll("'", "'\\''");
    result << escaped << "'";
    return result;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

LauncherWindow::LauncherWindow()
    : BWindow(BRect(100, 100, 500, 300),
              "Claude Code Launcher",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
    fIsApiMode = false;

    // Mode selection
    fCloudRadio = new BRadioButton("cloudRadio", "Cloud", new BMessage(MSG_MODE_CLOUD));
    fApiRadio   = new BRadioButton("apiRadio", "API  (API key)", new BMessage(MSG_MODE_API));
    fCloudRadio->SetValue(B_CONTROL_ON);

    // Cloud model box (visible in Cloud mode only)
    fCloudOpusRadio   = new BRadioButton("cloudOpus",   "Opus",   nullptr);
    fCloudSonnetRadio = new BRadioButton("cloudSonnet", "Sonnet", nullptr);
    fCloudHaikuRadio  = new BRadioButton("cloudHaiku",  "Haiku",  nullptr);
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

    // Working directory
    fWorkDirField = new BTextControl("workDir", "Directory:", "/boot/home", nullptr);
    fWorkDirField->SetDivider(70);
    fBrowseBtn = new BButton("browse", "Browse\xe2\x80\xa6", new BMessage(MSG_BROWSE_DIR));
    fFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), nullptr,
                                B_DIRECTORY_NODE, false);

    // YOLO mode (visible in both Cloud and API modes)
    fYoloCheck = new BCheckBox("yoloCheck",
        "YOLO mode \xe2\x80\x94 skip all permissions (Claude executes without asking)",
        nullptr);

    // API settings controls
    fApiUrlField = new BTextControl("apiUrl", "API URL:",
                                    "https://api.anthropic.com/v1", nullptr);
    fApiUrlField->SetDivider(70);

    fApiKeyField = new BTextControl("apiKey", "API Key:", "", nullptr);
    fApiKeyField->SetDivider(70);
    fApiKeyField->TextView()->HideTyping(true);

    fSaveApiKeyCheck = new BCheckBox("saveApiKeyCheck", "Remember key", nullptr);

    fApiCurrentModelCheck = new BCheckBox("apiCurrentModelCheck",
        "Override current model", new BMessage(MSG_API_CURRENT_MODEL));
    fApiCurrentModelField = new BTextControl("apiCurrentModel", "",
        "claude-sonnet-4-6", nullptr);
    fApiCurrentModelField->SetDivider(0);

    fApiOpusModelCheck = new BCheckBox("apiOpusModelCheck",
        "Override ANTHROPIC_DEFAULT_OPUS_MODEL", new BMessage(MSG_API_OPUS_MODEL));
    fApiOpusModelField = new BTextControl("apiOpusModel", "",
        "claude-opus-4-20250514", nullptr);
    fApiOpusModelField->SetDivider(0);

    fApiSonnetModelCheck = new BCheckBox("apiSonnetModelCheck",
        "Override ANTHROPIC_DEFAULT_SONNET_MODEL", new BMessage(MSG_API_SONNET_MODEL));
    fApiSonnetModelField = new BTextControl("apiSonnetModel", "",
        "claude-sonnet-4-6", nullptr);
    fApiSonnetModelField->SetDivider(0);

    fApiHaikuModelCheck = new BCheckBox("apiHaikuModelCheck",
        "Override ANTHROPIC_DEFAULT_HAIKU_MODEL", new BMessage(MSG_API_HAIKU_MODEL));
    fApiHaikuModelField = new BTextControl("apiHaikuModel", "",
        "claude-haiku-4-20250514", nullptr);
    fApiHaikuModelField->SetDivider(0);

    // Attribution fix: disables header that slows local LLMs by ~90%
    fFixAttributionCheck = new BCheckBox("fixAttributionCheck",
        "Fix attribution header (for local LLMs)", nullptr);

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
        .Add(fFixAttributionCheck)
        .End();

    // Profile management box (visible in API mode only)
    fProfileMenu = new BPopUpMenu("Select profile...");
    fProfileCombo = new BMenuField("profileCombo", "Profile:", fProfileMenu);
    fProfileCombo->SetDivider(55);

    fProfileNameField = new BTextControl("profileName", "New profile:", "", nullptr);
    fProfileNameField->SetDivider(80);
    fProfileNameField->TextView()->SetMaxBytes(64);

    fSaveProfileBtn = new BButton("saveProfile", "Save Profile",
                                  new BMessage(MSG_SAVE_PROFILE));

    fProfileListView = new BListView("profileList");
    fProfileListView->SetSelectionMessage(new BMessage(MSG_PROFILE_LIST_SEL));
    fProfileListScroll = new BScrollView("profileScroll", fProfileListView,
                                         0, false, true);
    fProfileListScroll->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 130));

    fDeleteProfileBtn = new BButton("deleteProfile", "Delete Profile",
                                    new BMessage(MSG_DELETE_PROFILE));
    fDeleteProfileBtn->SetEnabled(false);

    fProfileBox = new BBox("profileBox");
    fProfileBox->SetLabel("Profiles");
    BLayoutBuilder::Group<>(fProfileBox, B_VERTICAL, B_USE_SMALL_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
                   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fProfileCombo)
        .AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
            .Add(fProfileNameField)
            .Add(fSaveProfileBtn, 0.0f)
            .End()
        .Add(fProfileListScroll)
        .AddGroup(B_HORIZONTAL, 0)
            .AddGlue()
            .Add(fDeleteProfileBtn, 0.0f)
            .End()
        .End();

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
        .Add(fYoloCheck)
        .Add(fApiBox)
        .Add(fProfileBox)
        .AddGlue()
        .Add(fLaunchBtn)
        .End();

    // Find root-level layout items for dynamic visibility
    fModelBoxItem   = nullptr;
    fApiBoxItem     = nullptr;
    fProfileBoxItem = nullptr;
    BLayout* rootLayout = GetLayout();
    for (int32 i = 0; i < rootLayout->CountItems(); i++) {
        BLayoutItem* item = rootLayout->ItemAt(i);
        if (item->View() == fModelBox)
            fModelBoxItem = item;
        else if (item->View() == fApiBox)
            fApiBoxItem = item;
        else if (item->View() == fProfileBox)
            fProfileBoxItem = item;
    }

    // Find API box child items for override field visibility
    fApiCurrentModelFieldItem = nullptr;
    fApiOpusModelFieldItem    = nullptr;
    fApiSonnetModelFieldItem  = nullptr;
    fApiHaikuModelFieldItem   = nullptr;
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

    // Hide model override fields initially
    if (fApiCurrentModelFieldItem) fApiCurrentModelFieldItem->SetVisible(false);
    if (fApiOpusModelFieldItem)    fApiOpusModelFieldItem->SetVisible(false);
    if (fApiSonnetModelFieldItem)  fApiSonnetModelFieldItem->SetVisible(false);
    if (fApiHaikuModelFieldItem)   fApiHaikuModelFieldItem->SetVisible(false);

    InvalidateLayout(true);
    BSize preferred = GetLayout()->PreferredSize();
    ResizeTo(preferred.width, preferred.height);
    CenterOnScreen();

    _LoadSettings();
    _LoadProfiles();
    _UpdateModeVisibility();
}

LauncherWindow::~LauncherWindow()
{
    delete fFilePanel;
}

// ---------------------------------------------------------------------------
// MessageReceived
// ---------------------------------------------------------------------------

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
        case MSG_PROFILE_SELECTED: {
            const char* name;
            if (msg->FindString("name", &name) == B_OK) {
                fActiveProfile = name;
                _LoadProfileIntoUI(name);
                _ResizeToFit();
            }
            break;
        }
        case MSG_PROFILE_LIST_SEL:
            fDeleteProfileBtn->SetEnabled(
                fProfileListView->CurrentSelection() >= 0);
            break;
        case MSG_SAVE_PROFILE:
            _SaveProfile();
            break;
        case MSG_DELETE_PROFILE:
            _DeleteProfile();
            break;
        default:
            BWindow::MessageReceived(msg);
    }
}

// ---------------------------------------------------------------------------
// Layout helpers
// ---------------------------------------------------------------------------

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
    if (fProfileBoxItem)
        fProfileBoxItem->SetVisible(fIsApiMode);

    _ResizeToFit();
}

// ---------------------------------------------------------------------------
// Launch logic
// ---------------------------------------------------------------------------

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
        if (BString(fApiUrlField->Text()).Length() == 0) {
            BAlert* alert = new BAlert("Missing API URL",
                "API URL cannot be empty.", "OK");
            alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
            alert->Go();
            return;
        }
        if (BString(fApiKeyField->Text()).Length() == 0) {
            BAlert* alert = new BAlert("Missing API Key",
                "API Key cannot be empty.", "OK");
            alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
            alert->Go();
            return;
        }
        // Apply attribution header fix before launching
        if (fFixAttributionCheck->Value() == B_CONTROL_ON)
            _ApplyAttributionHeaderFix();
    }

    // Build shell command
    BString cmd;
    if (workDir.Length() > 0)
        cmd << "cd " << shellEscape(workDir) << " && ";

    if (fCloudRadio->Value() == B_CONTROL_ON) {
        cmd << "unset ANTHROPIC_API_KEY && " << kClaudeBin;

        if (fCloudSonnetRadio->Value() == B_CONTROL_ON)
            cmd << " --model sonnet";
        else if (fCloudHaikuRadio->Value() == B_CONTROL_ON)
            cmd << " --model haiku";
        else
            cmd << " --model opus";
    } else if (fApiRadio->Value() == B_CONTROL_ON) {
        BString apiUrl = fApiUrlField->Text();
        BString apiKey = fApiKeyField->Text();
        // trap ensures credentials are restored on exit even if claude crashes
        cmd << "trap 'mv \"$HOME/.claude/.credentials.json.bak\" "
               "\"$HOME/.claude/.credentials.json\" 2>/dev/null' EXIT; "
            << "mv \"$HOME/.claude/.credentials.json\" "
               "\"$HOME/.claude/.credentials.json.bak\" 2>/dev/null; "
            << "ANTHROPIC_BASE_URL=" << shellEscape(apiUrl)
            << " ANTHROPIC_API_KEY=" << shellEscape(apiKey);

        if (fApiOpusModelCheck->Value() == B_CONTROL_ON) {
            BString m = fApiOpusModelField->Text();
            if (m.Length() > 0)
                cmd << " ANTHROPIC_DEFAULT_OPUS_MODEL=" << shellEscape(m);
        }
        if (fApiSonnetModelCheck->Value() == B_CONTROL_ON) {
            BString m = fApiSonnetModelField->Text();
            if (m.Length() > 0)
                cmd << " ANTHROPIC_DEFAULT_SONNET_MODEL=" << shellEscape(m);
        }
        if (fApiHaikuModelCheck->Value() == B_CONTROL_ON) {
            BString m = fApiHaikuModelField->Text();
            if (m.Length() > 0)
                cmd << " ANTHROPIC_DEFAULT_HAIKU_MODEL=" << shellEscape(m);
        }

        cmd << " " << kClaudeBin;

        if (fApiCurrentModelCheck->Value() == B_CONTROL_ON) {
            BString m = fApiCurrentModelField->Text();
            if (m.Length() > 0)
                cmd << " --model " << shellEscape(m);
        }
    }

    if (fYoloCheck->Value() == B_CONTROL_ON)
        cmd << " --dangerously-skip-permissions";

    // Disable claude's auto-updater — it would replace the pinned 2.1.112
    // pure-JS install with a native binary variant that has no Haiku build.
    cmd.Prepend("DISABLE_AUTOUPDATER=1 ");

    _SaveSettings();

    if (isatty(STDIN_FILENO)) {
        // Launched from a terminal — exec claude there after the GUI exits
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
                    // Write a temp script and have Terminal exec it directly.
                    // This avoids any Terminal argv-parsing ambiguity and lets
                    // us pause on failure so startup errors are visible.
                    BPath scriptPath;
                    if (find_directory(B_USER_CACHE_DIRECTORY, &scriptPath) == B_OK) {
                        scriptPath.Append("haiclaude-launch.sh");
                        BFile script(scriptPath.Path(),
                            B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
                        if (script.InitCheck() == B_OK) {
                            BString s = "#!/bin/sh\n";
                            s << ". /etc/profile.d/npm-global.sh 2>/dev/null\n";
                            s << "export PATH=\"$HOME/.npm-global/bin:$PATH\"\n";
                            // Disable claude's auto-updater — it replaces the
                            // pinned 2.1.112 pure-JS install with a native
                            // binary variant that has no Haiku build.
                            s << "export DISABLE_AUTOUPDATER=1\n";
                            s << cmd << "\n";
                            s << "rc=$?\n";
                            s << "if [ $rc -ne 0 ]; then\n";
                            s << "  echo\n";
                            s << "  echo \"Claude Code exited with status $rc. "
                                 "Press Enter to close.\"\n";
                            s << "  read _\n";
                            s << "fi\n";
                            script.Write(s.String(), s.Length());
                        }
                        script.Unset();
                        chmod(scriptPath.Path(), 0755);
                        execl(termPath.Path(), "Terminal",
                              scriptPath.Path(), (char*)nullptr);
                    }
                    _exit(1);
                }
            }
        }
    }

    be_app->PostMessage(B_QUIT_REQUESTED);
}

// ---------------------------------------------------------------------------
// Attribution header fix — sets CLAUDE_CODE_ATTRIBUTION_HEADER=0 in
// ~/.claude/settings.json to avoid ~90% inference slowdown with local LLMs
// ---------------------------------------------------------------------------

void
LauncherWindow::_ApplyAttributionHeaderFix()
{
    BPath homePath;
    if (find_directory(B_USER_DIRECTORY, &homePath) != B_OK) return;

    BPath dirPath(homePath);
    dirPath.Append(".claude");
    create_directory(dirPath.Path(), 0755);

    BPath settingsPath(dirPath);
    settingsPath.Append("settings.json");

    BString json;
    {
        BFile file(settingsPath.Path(), B_READ_ONLY);
        if (file.InitCheck() == B_OK) {
            off_t size;
            file.GetSize(&size);
            if (size > 0 && size < 1024 * 1024) {
                char* buf = json.LockBuffer(size);
                file.Read(buf, size);
                json.UnlockBuffer(size);
            }
        }
    }

    if (json.IsEmpty()) json = "{}";

    const char* kKey = "\"CLAUDE_CODE_ATTRIBUTION_HEADER\"";
    const char* kEnv = "\"env\"";

    int32 keyPos = json.FindFirst(kKey);
    if (keyPos >= 0) {
        // Key exists — update its value to "0"
        int32 colonPos = json.FindFirst(':', keyPos + (int32)strlen(kKey));
        if (colonPos >= 0) {
            int32 p = colonPos + 1;
            while (p < json.Length() && (json[p] == ' ' || json[p] == '\t')) p++;
            if (p < json.Length() && json[p] == '"') {
                int32 end = json.FindFirst('"', p + 1);
                if (end >= 0) {
                    json.Remove(p, end - p + 1);
                    json.Insert("\"0\"", p);
                }
            }
        }
    } else {
        int32 envPos = json.FindFirst(kEnv);
        if (envPos >= 0) {
            // "env" exists — add key inside it
            int32 brace = json.FindFirst('{', envPos + (int32)strlen(kEnv));
            if (brace >= 0) {
                int32 p = brace + 1;
                while (p < json.Length() &&
                       (json[p]==' '||json[p]=='\n'||json[p]=='\r'||json[p]=='\t'))
                    p++;
                BString ins;
                if (json[p] != '}')
                    ins = "\n    \"CLAUDE_CODE_ATTRIBUTION_HEADER\": \"0\",";
                else
                    ins = "\n    \"CLAUDE_CODE_ATTRIBUTION_HEADER\": \"0\"\n  ";
                json.Insert(ins, brace + 1);
            }
        } else {
            // No "env" key — add before closing brace of root object
            int32 closePos = json.FindLast('}');
            if (closePos >= 0) {
                int32 p = closePos - 1;
                while (p >= 0 &&
                       (json[p]==' '||json[p]=='\n'||json[p]=='\r'||json[p]=='\t')) p--;
                bool isEmpty = (p >= 0 && json[p] == '{');
                BString ins;
                if (!isEmpty) ins = ",";
                ins << "\n  \"env\": {\n    \"CLAUDE_CODE_ATTRIBUTION_HEADER\": \"0\"\n  }";
                json.Insert(ins, closePos);
            }
        }
    }

    BFile outFile(settingsPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (outFile.InitCheck() != B_OK) {
        BAlert* alert = new BAlert("Error",
            "Could not write to ~/.claude/settings.json.", "OK");
        alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
        alert->Go();
        return;
    }
    outFile.Write(json.String(), json.Length());
}

// ---------------------------------------------------------------------------
// Profile management
// ---------------------------------------------------------------------------

void
LauncherWindow::_PopulateProfileMenu()
{
    while (fProfileMenu->CountItems() > 0)
        delete fProfileMenu->RemoveItem((int32)0);

    for (int32 i = 0; i < fProfileNames.CountStrings(); i++) {
        const BString& name = fProfileNames.StringAt(i);
        BMessage* msg = new BMessage(MSG_PROFILE_SELECTED);
        msg->AddString("name", name.String());
        fProfileMenu->AddItem(new BMenuItem(name.String(), msg));
    }
    fProfileMenu->SetTargetForItems(this);
}

void
LauncherWindow::_LoadProfiles()
{
    fProfileListView->MakeEmpty();
    _PopulateProfileMenu();

    for (int32 i = 0; i < fProfileNames.CountStrings(); i++)
        fProfileListView->AddItem(new BStringItem(fProfileNames.StringAt(i).String()));

    // Restore active profile selection and fields
    if (!fActiveProfile.IsEmpty()) {
        for (int32 i = 0; i < fProfileMenu->CountItems(); i++) {
            BMenuItem* item = fProfileMenu->ItemAt(i);
            if (item && fActiveProfile == item->Label()) {
                item->SetMarked(true);
                break;
            }
        }
        _LoadProfileIntoUI(fActiveProfile.String());
    }
}

void
LauncherWindow::_LoadProfileIntoUI(const char* name)
{
    BString pfx = "profile_";
    pfx << name << "_";

    const char* val;
    bool bval;

    if (fProfileData.FindString(pkey(pfx, "url").String(), &val) == B_OK)
        fApiUrlField->SetText(val);

    bval = false;
    if (fProfileData.FindBool(pkey(pfx, "saveKey").String(), &bval) == B_OK)
        fSaveApiKeyCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
    if (bval) {
        if (fProfileData.FindString(pkey(pfx, "key").String(), &val) == B_OK)
            fApiKeyField->SetText(val);
    }

    bval = false;
    if (fProfileData.FindBool(pkey(pfx, "currentModelCheck").String(), &bval) == B_OK) {
        fApiCurrentModelCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiCurrentModelFieldItem)
            fApiCurrentModelFieldItem->SetVisible(bval);
    }
    if (fProfileData.FindString(pkey(pfx, "currentModel").String(), &val) == B_OK)
        fApiCurrentModelField->SetText(val);

    bval = false;
    if (fProfileData.FindBool(pkey(pfx, "opusModelCheck").String(), &bval) == B_OK) {
        fApiOpusModelCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiOpusModelFieldItem)
            fApiOpusModelFieldItem->SetVisible(bval);
    }
    if (fProfileData.FindString(pkey(pfx, "opusModel").String(), &val) == B_OK)
        fApiOpusModelField->SetText(val);

    bval = false;
    if (fProfileData.FindBool(pkey(pfx, "sonnetModelCheck").String(), &bval) == B_OK) {
        fApiSonnetModelCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiSonnetModelFieldItem)
            fApiSonnetModelFieldItem->SetVisible(bval);
    }
    if (fProfileData.FindString(pkey(pfx, "sonnetModel").String(), &val) == B_OK)
        fApiSonnetModelField->SetText(val);

    bval = false;
    if (fProfileData.FindBool(pkey(pfx, "haikuModelCheck").String(), &bval) == B_OK) {
        fApiHaikuModelCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiHaikuModelFieldItem)
            fApiHaikuModelFieldItem->SetVisible(bval);
    }
    if (fProfileData.FindString(pkey(pfx, "haikuModel").String(), &val) == B_OK)
        fApiHaikuModelField->SetText(val);

    bval = false;
    if (fProfileData.FindBool(pkey(pfx, "fixAttribution").String(), &bval) == B_OK)
        fFixAttributionCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
}

void
LauncherWindow::_SaveProfile()
{
    BString name = fProfileNameField->Text();
    name.Trim();

    if (name.IsEmpty()) {
        // No name typed — treat as update of currently-selected profile
        BMenuItem* marked = fProfileMenu->FindMarked();
        if (marked == nullptr) {
            BAlert* alert = new BAlert("No Profile Name",
                "Enter a name for the new profile, or select an existing one to update.",
                "OK");
            alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
            alert->Go();
            return;
        }
        name = marked->Label();
    }

    bool isNew = (fProfileNames.IndexOf(name) < 0);

    BString pfx = "profile_";
    pfx << name << "_";

    // Update profile data in-memory store
    fProfileData.RemoveName(pkey(pfx, "url").String());
    fProfileData.AddString(pkey(pfx, "url").String(), fApiUrlField->Text());

    bool saveKey = fSaveApiKeyCheck->Value() == B_CONTROL_ON;
    fProfileData.RemoveName(pkey(pfx, "saveKey").String());
    fProfileData.AddBool(pkey(pfx, "saveKey").String(), saveKey);
    fProfileData.RemoveName(pkey(pfx, "key").String());
    if (saveKey)
        fProfileData.AddString(pkey(pfx, "key").String(), fApiKeyField->Text());

    fProfileData.RemoveName(pkey(pfx, "currentModelCheck").String());
    fProfileData.AddBool(pkey(pfx, "currentModelCheck").String(),
        fApiCurrentModelCheck->Value() == B_CONTROL_ON);
    fProfileData.RemoveName(pkey(pfx, "currentModel").String());
    fProfileData.AddString(pkey(pfx, "currentModel").String(), fApiCurrentModelField->Text());

    fProfileData.RemoveName(pkey(pfx, "opusModelCheck").String());
    fProfileData.AddBool(pkey(pfx, "opusModelCheck").String(),
        fApiOpusModelCheck->Value() == B_CONTROL_ON);
    fProfileData.RemoveName(pkey(pfx, "opusModel").String());
    fProfileData.AddString(pkey(pfx, "opusModel").String(), fApiOpusModelField->Text());

    fProfileData.RemoveName(pkey(pfx, "sonnetModelCheck").String());
    fProfileData.AddBool(pkey(pfx, "sonnetModelCheck").String(),
        fApiSonnetModelCheck->Value() == B_CONTROL_ON);
    fProfileData.RemoveName(pkey(pfx, "sonnetModel").String());
    fProfileData.AddString(pkey(pfx, "sonnetModel").String(), fApiSonnetModelField->Text());

    fProfileData.RemoveName(pkey(pfx, "haikuModelCheck").String());
    fProfileData.AddBool(pkey(pfx, "haikuModelCheck").String(),
        fApiHaikuModelCheck->Value() == B_CONTROL_ON);
    fProfileData.RemoveName(pkey(pfx, "haikuModel").String());
    fProfileData.AddString(pkey(pfx, "haikuModel").String(), fApiHaikuModelField->Text());

    fProfileData.RemoveName(pkey(pfx, "fixAttribution").String());
    fProfileData.AddBool(pkey(pfx, "fixAttribution").String(),
        fFixAttributionCheck->Value() == B_CONTROL_ON);

    if (isNew) {
        fProfileNames.Add(name);
        fProfileListView->AddItem(new BStringItem(name.String()));
        _PopulateProfileMenu();
    }

    // Mark as active and update menu
    fActiveProfile = name;
    fProfileNameField->SetText("");
    for (int32 i = 0; i < fProfileMenu->CountItems(); i++) {
        BMenuItem* item = fProfileMenu->ItemAt(i);
        if (item && name == item->Label()) {
            item->SetMarked(true);
            break;
        }
    }

    _SaveSettings();

    BAlert* alert = new BAlert("Profile Saved",
        isNew ? "Profile created." : "Profile updated.", "OK");
    alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
    alert->Go();
}

void
LauncherWindow::_DeleteProfile()
{
    int32 sel = fProfileListView->CurrentSelection();
    if (sel < 0) return;

    BStringItem* item = (BStringItem*)fProfileListView->ItemAt(sel);
    if (item == nullptr) return;

    BString name = item->Text();

    fProfileListView->RemoveItem(sel);
    delete item;

    int32 idx = fProfileNames.IndexOf(name);
    if (idx >= 0) fProfileNames.Remove(idx);

    // Remove all per-profile fields
    BString pfx = "profile_";
    pfx << name << "_";
    const char* kSuffixes[] = {
        "url", "key", "saveKey",
        "currentModelCheck", "currentModel",
        "opusModelCheck",    "opusModel",
        "sonnetModelCheck",  "sonnetModel",
        "haikuModelCheck",   "haikuModel",
        "fixAttribution",    nullptr
    };
    for (int i = 0; kSuffixes[i]; i++)
        fProfileData.RemoveName(pkey(pfx, kSuffixes[i]).String());

    if (fActiveProfile == name) fActiveProfile = "";

    _PopulateProfileMenu();
    fDeleteProfileBtn->SetEnabled(false);
    _SaveSettings();
}

// ---------------------------------------------------------------------------
// Settings persistence
// ---------------------------------------------------------------------------

void
LauncherWindow::_LoadSettings()
{
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) return;
    path.Append("HaiClaude");

    BFile file(path.Path(), B_READ_ONLY);
    if (file.InitCheck() != B_OK) return;

    BMessage s;
    if (s.Unflatten(&file) != B_OK) return;

    // Mode
    int32 mode = 0;
    s.FindInt32("mode", &mode);
    if (mode == 1) {
        fIsApiMode = true;
        fApiRadio->SetValue(B_CONTROL_ON);
        fCloudRadio->SetValue(B_CONTROL_OFF);
    }

    // Cloud model
    int32 cloudModel = 0;
    s.FindInt32("cloudModel", &cloudModel);
    if (cloudModel == 1)      fCloudSonnetRadio->SetValue(B_CONTROL_ON);
    else if (cloudModel == 2) fCloudHaikuRadio->SetValue(B_CONTROL_ON);
    else                      fCloudOpusRadio->SetValue(B_CONTROL_ON);

    // Working directory
    const char* str;
    if (s.FindString("workDir", &str) == B_OK) fWorkDirField->SetText(str);

    // API URL
    if (s.FindString("apiUrl", &str) == B_OK) fApiUrlField->SetText(str);

    // API key
    bool saveKey = false;
    if (s.FindBool("saveApiKey", &saveKey) == B_OK)
        fSaveApiKeyCheck->SetValue(saveKey ? B_CONTROL_ON : B_CONTROL_OFF);
    if (saveKey) {
        if (s.FindString("apiKey", &str) == B_OK) fApiKeyField->SetText(str);
    }

    // Model overrides
    bool bval;
    if (s.FindBool("apiCurrentModelCheck", &bval) == B_OK) {
        fApiCurrentModelCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiCurrentModelFieldItem) fApiCurrentModelFieldItem->SetVisible(bval);
    }
    if (s.FindString("apiCurrentModel", &str) == B_OK)
        fApiCurrentModelField->SetText(str);

    if (s.FindBool("apiOpusModelCheck", &bval) == B_OK) {
        fApiOpusModelCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiOpusModelFieldItem) fApiOpusModelFieldItem->SetVisible(bval);
    }
    if (s.FindString("apiOpusModel", &str) == B_OK)
        fApiOpusModelField->SetText(str);

    if (s.FindBool("apiSonnetModelCheck", &bval) == B_OK) {
        fApiSonnetModelCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiSonnetModelFieldItem) fApiSonnetModelFieldItem->SetVisible(bval);
    }
    if (s.FindString("apiSonnetModel", &str) == B_OK)
        fApiSonnetModelField->SetText(str);

    if (s.FindBool("apiHaikuModelCheck", &bval) == B_OK) {
        fApiHaikuModelCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);
        if (fApiHaikuModelFieldItem) fApiHaikuModelFieldItem->SetVisible(bval);
    }
    if (s.FindString("apiHaikuModel", &str) == B_OK)
        fApiHaikuModelField->SetText(str);

    // YOLO mode
    if (s.FindBool("yoloMode", &bval) == B_OK)
        fYoloCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);

    // Attribution header fix
    if (s.FindBool("fixAttribution", &bval) == B_OK)
        fFixAttributionCheck->SetValue(bval ? B_CONTROL_ON : B_CONTROL_OFF);

    // Active profile
    if (s.FindString("activeProfile", &str) == B_OK)
        fActiveProfile = str;

    // Profile names list
    fProfileNames.MakeEmpty();
    const char* pname;
    for (int32 i = 0; s.FindString("profileNames", i, &pname) == B_OK; i++)
        fProfileNames.Add(pname);

    // Copy all profile_* fields into fProfileData
    fProfileData.MakeEmpty();
    int32 n = s.CountNames(B_ANY_TYPE);
    for (int32 i = 0; i < n; i++) {
        char* fname = nullptr;
        type_code ftype;
        s.GetInfo(B_ANY_TYPE, i, &fname, &ftype);
        if (fname == nullptr || strncmp(fname, "profile_", 8) != 0) continue;
        if (ftype == B_STRING_TYPE) {
            const char* sv;
            if (s.FindString(fname, &sv) == B_OK)
                fProfileData.AddString(fname, sv);
        } else if (ftype == B_BOOL_TYPE) {
            bool bv;
            if (s.FindBool(fname, &bv) == B_OK)
                fProfileData.AddBool(fname, bv);
        }
    }
}

void
LauncherWindow::_SaveSettings()
{
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) return;
    path.Append("HaiClaude");

    BMessage s;

    int32 mode = (fApiRadio->Value() == B_CONTROL_ON) ? 1 : 0;
    s.AddInt32("mode", mode);

    int32 cloudModel = 0;
    if (fCloudSonnetRadio->Value() == B_CONTROL_ON)      cloudModel = 1;
    else if (fCloudHaikuRadio->Value() == B_CONTROL_ON) cloudModel = 2;
    s.AddInt32("cloudModel", cloudModel);

    s.AddString("workDir", fWorkDirField->Text());
    s.AddString("apiUrl",  fApiUrlField->Text());

    bool saveKey = fSaveApiKeyCheck->Value() == B_CONTROL_ON;
    s.AddBool("saveApiKey", saveKey);
    if (saveKey) s.AddString("apiKey", fApiKeyField->Text());

    s.AddBool("apiCurrentModelCheck", fApiCurrentModelCheck->Value() == B_CONTROL_ON);
    s.AddString("apiCurrentModel",    fApiCurrentModelField->Text());
    s.AddBool("apiOpusModelCheck",    fApiOpusModelCheck->Value() == B_CONTROL_ON);
    s.AddString("apiOpusModel",       fApiOpusModelField->Text());
    s.AddBool("apiSonnetModelCheck",  fApiSonnetModelCheck->Value() == B_CONTROL_ON);
    s.AddString("apiSonnetModel",     fApiSonnetModelField->Text());
    s.AddBool("apiHaikuModelCheck",   fApiHaikuModelCheck->Value() == B_CONTROL_ON);
    s.AddString("apiHaikuModel",      fApiHaikuModelField->Text());

    s.AddBool("yoloMode",      fYoloCheck->Value() == B_CONTROL_ON);
    s.AddBool("fixAttribution", fFixAttributionCheck->Value() == B_CONTROL_ON);

    s.AddString("activeProfile", fActiveProfile);

    for (int32 i = 0; i < fProfileNames.CountStrings(); i++)
        s.AddString("profileNames", fProfileNames.StringAt(i));

    // Copy all profile_* fields from in-memory store
    int32 n = fProfileData.CountNames(B_ANY_TYPE);
    for (int32 i = 0; i < n; i++) {
        char* fname = nullptr;
        type_code ftype;
        fProfileData.GetInfo(B_ANY_TYPE, i, &fname, &ftype);
        if (fname == nullptr) continue;
        if (ftype == B_STRING_TYPE) {
            const char* sv;
            if (fProfileData.FindString(fname, &sv) == B_OK)
                s.AddString(fname, sv);
        } else if (ftype == B_BOOL_TYPE) {
            bool bv;
            if (fProfileData.FindBool(fname, &bv) == B_OK)
                s.AddBool(fname, bv);
        }
    }

    BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) return;
    s.Flatten(&file);
}
