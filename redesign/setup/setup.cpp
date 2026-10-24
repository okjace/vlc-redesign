/*
 * VLC Redesign setup wizard for the Linux AppImage.
 *
 * Modelled on Jace Launcher's self-installer: the first time the AppImage
 * runs it offers to install itself (copy to ~/Applications, applications-menu
 * entry, desktop shortcut, terminal command, app-center metadata), so users
 * never have to chmod/move files or rely on AppImageLauncher, which aborts on
 * Wayland desktops such as niri, Sway and Hyprland.
 *
 * Builds against Qt 5 (VLC 3 AppImage) and Qt 6 (VLC 4 AppImage).
 *
 *   vlc-redesign-setup                 wizard
 *   vlc-redesign-setup --install --yes install with the defaults, no questions
 *   vlc-redesign-setup --uninstall     remove (asks first unless --yes)
 *
 * Exit codes: 0 installed (AppRun then starts the installed copy),
 *             10 run without installing, 1 cancelled / failed.
 */

#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QStyleFactory>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>

#include <cstdio>

namespace {

const char *APP_ID = "io.github.okjace.vlc_redesign";
const char *APP_NAME = "VLC Redesign";
const char *FILE_NAME = "VLC-Redesign.AppImage";
const int EXIT_RUN_UNINSTALLED = 10;

QString env(const char *name) { return QString::fromLocal8Bit(qgetenv(name)); }

QString home() { return QDir::homePath(); }

QString dataHome()
{
    const QString d = env("XDG_DATA_HOME");
    return d.isEmpty() ? home() + "/.local/share" : d;
}

QString appDir() { return env("APPDIR"); }        // set by the AppImage runtime
QString runningAppImage() { return env("APPIMAGE"); }

QString menuFile() { return dataHome() + "/applications/" + APP_ID + ".desktop"; }
QString iconFile(int size)
{
    return dataHome() + QString("/icons/hicolor/%1x%1/apps/").arg(size) + APP_ID + ".png";
}
QString metainfoFile() { return dataHome() + "/metainfo/" + APP_ID + ".metainfo.xml"; }
QString binFile() { return home() + "/.local/bin/vlc-redesign"; }
QString recordFile() { return dataHome() + "/vlc-redesign/install.ini"; }

QString desktopDir()
{
    QProcess p;
    p.start("xdg-user-dir", {"DESKTOP"});
    if (p.waitForFinished(3000)) {
        const QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
        if (!out.isEmpty() && out != home())
            return out;
    }
    return home() + "/Desktop";
}
QString desktopShortcut() { return desktopDir() + "/" + APP_ID + ".desktop"; }

QString version()
{
    QFile f(appDir() + "/usr/share/vlc-redesign/VERSION");
    if (f.open(QIODevice::ReadOnly))
        return QString::fromUtf8(f.readAll()).trimmed();
    return QString();
}

QString installedPath()
{
    QSettings s(recordFile(), QSettings::IniFormat);
    const QString p = s.value("path").toString();
    return (!p.isEmpty() && QFileInfo::exists(p)) ? p : QString();
}

bool writeFile(const QString &path, const QByteArray &data, bool executable = false)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(data);
    f.close();
    if (executable)
        f.setPermissions(f.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
    return true;
}

bool copyFile(const QString &from, const QString &to)
{
    QDir().mkpath(QFileInfo(to).absolutePath());
    QFile::remove(to);
    return QFile::copy(from, to);
}

QString quoted(const QString &path) { return "\"" + path + "\""; }

/* The launcher line for every entry we write: skip AppImageLauncher, which
 * aborts on Wayland desktops without qt5-wayland before VLC can start. */
QString execLine(const QString &exe, const QString &args)
{
    return "env APPIMAGELAUNCHER_DISABLE=1 " + quoted(exe) + (args.isEmpty() ? "" : " " + args);
}

/* Our desktop entry, based on the one bundled in the AppImage (MimeType etc.) */
QByteArray desktopEntry(const QString &exe)
{
    QStringList out;
    QFile bundled(appDir() + "/usr/share/applications/" + APP_ID + ".desktop");
    if (bundled.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&bundled);
        bool mainGroup = false;
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (line.startsWith('['))
                mainGroup = (line == "[Desktop Entry]");
            if (!mainGroup || line.startsWith('#'))
                continue;
            if (line.startsWith("Exec=") || line.startsWith("TryExec=") || line.startsWith("Icon=")
                || line.startsWith("Actions=") || line.startsWith("X-AppImage-"))
                continue;
            out << line;
        }
    }
    if (out.isEmpty())
        out << "[Desktop Entry]" << "Type=Application" << QString("Name=%1").arg(APP_NAME)
            << "Categories=AudioVideo;Player;";
    out << "Exec=" + execLine(exe, "--started-from-file %U")
        << QString("Icon=%1").arg(APP_ID)
        << "Actions=uninstall;"
        << "X-AppImage-Integrate=false"
        << ""
        << "[Desktop Action uninstall]"
        << QString("Name=Uninstall %1").arg(APP_NAME)
        << "Exec=" + execLine(exe, "--uninstall");
    return (out.join('\n') + '\n').toUtf8();
}

void refreshMenus()
{
    QProcess::execute("update-desktop-database", {dataHome() + "/applications"});
    QProcess::execute("gtk-update-icon-cache", {"-q", "-t", dataHome() + "/icons/hicolor"});
}

struct Options {
    QString dir = home() + "/Applications";
    bool menu = true, desktop = true, terminal = true, appstream = true, defaultPlayer = false;
};

/* Copy the AppImage and create the shortcuts. `log` receives progress lines. */
template <typename Log>
bool install(const Options &o, Log log, QString *installed)
{
    const QString src = runningAppImage();
    if (src.isEmpty() || !QFileInfo::exists(src)) {
        log("Setup must be started from the VLC Redesign AppImage.");
        return false;
    }
    const QString dst = QDir(o.dir).absoluteFilePath(FILE_NAME);
    QStringList parts;

    log(QString("Copying VLC Redesign to %1").arg(dst));
    if (QFileInfo(src).canonicalFilePath() != QFileInfo(dst).canonicalFilePath()) {
        QDir().mkpath(o.dir);
        const QString tmp = dst + ".part";
        if (!copyFile(src, tmp)) {
            log("Could not copy the AppImage there. Is the folder writable?");
            return false;
        }
        QFile::remove(dst);
        if (!QFile::rename(tmp, dst)) {
            log("Could not replace the existing copy.");
            return false;
        }
    }
    QFile(dst).setPermissions(QFile(dst).permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser
                              | QFileDevice::ExeGroup | QFileDevice::ExeOther);

    log("Installing the icon");
    for (int size : {256, 512}) {
        const QString icon = appDir() + QString("/usr/share/icons/hicolor/%1x%1/apps/vlc-redesign.png").arg(size);
        if (QFileInfo::exists(icon))
            copyFile(icon, iconFile(size));
    }

    const QByteArray entry = desktopEntry(dst);
    if (o.menu) {
        log("Adding VLC Redesign to the applications menu");
        if (writeFile(menuFile(), entry, true))
            parts << "menu";
    }
    if (o.desktop) {
        log("Creating the desktop shortcut");
        if (writeFile(desktopShortcut(), entry, true)) {
            parts << "desktop";
            // GNOME: mark the launcher as trusted so it isn't shown as "untrusted"
            QProcess::execute("gio", {"set", desktopShortcut(), "metadata::trusted", "true"});
        }
    }
    if (o.terminal) {
        log("Adding the  vlc-redesign  terminal command");
        const QByteArray script = QString("#!/bin/sh\n# VLC Redesign\nexec %1 \"$@\"\n")
                                      .arg(execLine(dst, QString())).toUtf8();
        if (writeFile(binFile(), script, true))
            parts << "terminal";
    }
    if (o.appstream) {
        log("Registering with software centers");
        const QString meta = appDir() + "/usr/share/metainfo/" + APP_ID + ".metainfo.xml";
        if (QFileInfo::exists(meta) && copyFile(meta, metainfoFile()))
            parts << "appstream";
    }
    if (o.defaultPlayer && o.menu) {
        log("Making VLC Redesign the default media player");
        QStringList mimes;
        for (const QString &line : QString::fromUtf8(entry).split('\n'))
            if (line.startsWith("MimeType="))
                mimes = line.mid(9).split(';', Qt::SkipEmptyParts);
        for (const QString &m : mimes)
            if (m.startsWith("video/") || m.startsWith("audio/"))
                QProcess::execute("xdg-mime", {"default", QString(APP_ID) + ".desktop", m});
        parts << "default";
    }
    refreshMenus();

    QSettings s(recordFile(), QSettings::IniFormat);
    s.setValue("path", dst);
    s.setValue("version", version());
    s.setValue("parts", parts);
    s.setValue("installed", QDateTime::currentDateTime().toString(Qt::ISODate));
    s.sync();

    *installed = dst;
    log("Done.");
    return true;
}

void uninstall()
{
    QSettings s(recordFile(), QSettings::IniFormat);
    const QString path = s.value("path").toString();
    for (const QString &f : {menuFile(), desktopShortcut(), metainfoFile(), iconFile(256), iconFile(512)})
        QFile::remove(f);
    QFile bin(binFile());
    if (bin.open(QIODevice::ReadOnly) && bin.readAll().contains("# VLC Redesign")) {
        bin.close();
        bin.remove();
    }
    if (!path.isEmpty())
        QFile::remove(path);
    QFile::remove(recordFile());
    QDir().rmdir(QFileInfo(recordFile()).absolutePath());
    refreshMenus();
}

/* ----- look & feel: the redesign palette ------------------------------------------------ */

void applyTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    const QColor bg("#111111"), base("#0A0A0A"), panel("#1E1E1E"), text("#F2F2F2"),
                 muted("#8A8A8A"), accent("#FF8800");
    p.setColor(QPalette::Window, bg);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, panel);
    p.setColor(QPalette::Button, panel);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::ToolTipBase, panel);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, Qt::black);
    p.setColor(QPalette::Link, accent);
    p.setColor(QPalette::PlaceholderText, muted);
    p.setColor(QPalette::Disabled, QPalette::Text, muted);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
    p.setColor(QPalette::Disabled, QPalette::WindowText, muted);
    app.setPalette(p);
    app.setStyleSheet(
        "QWizard, QWizardPage { background: #111111; }"
        "QLabel#muted { color: #8A8A8A; }"
        "QLabel#title { font-size: 22px; font-weight: 700; }"
        "QPushButton { background: #1E1E1E; border: 1px solid #2F2F2F; border-radius: 8px;"
        "  padding: 7px 18px; min-width: 70px; }"
        "QPushButton:hover { background: #262626; }"
        "QPushButton:default, QPushButton#primary { background: #FF8800; color: black;"
        "  border: none; font-weight: 600; }"
        "QPushButton:default:hover, QPushButton#primary:hover { background: #FF9A26; }"
        "QPushButton:disabled { color: #5A5A5A; background: #171717; }"
        "QLineEdit, QPlainTextEdit { background: #0A0A0A; border: 1px solid #2F2F2F; border-radius: 8px;"
        "  padding: 6px; selection-background-color: #FF8800; selection-color: black; }"
        "QLineEdit:focus { border-color: #FF8800; }"
        "QCheckBox { spacing: 10px; padding: 3px 0; }"
        "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px;"
        "  border: 1px solid #3A3A3A; background: #0A0A0A; }"
        "QCheckBox::indicator:checked { background: #FF8800; border-color: #FF8800; }");
}

QLabel *muted(const QString &text)
{
    auto *l = new QLabel(text);
    l->setObjectName("muted");
    l->setWordWrap(true);
    return l;
}

QPixmap sideArt()
{
    return QPixmap(appDir() + "/usr/share/vlc-redesign/setup-side.png");
}

/* ----- wizard pages ---------------------------------------------------------------------- */

class WelcomePage : public QWizardPage {
public:
    explicit WelcomePage(bool upgrade)
    {
        auto *lay = new QVBoxLayout(this);
        auto *title = new QLabel(upgrade ? "Update VLC Redesign" : "Welcome to VLC Redesign");
        title->setObjectName("title");
        lay->addWidget(title);
        lay->addSpacing(8);
        auto *text = new QLabel(
            QString("<p style='font-size:14px'>This will set up <b>VLC Redesign %1</b> on your computer.</p>"
                    "<p>VLC media player with the interface from "
                    "<a href='https://www.youtube.com/watch?v=ERFeOgqk324'>“I Redesigned VLC After 25 Years”</a> "
                    "by Juxtopposed.</p>"
                    "<p>The next steps will:</p>"
                    "<ul><li>choose where to install VLC Redesign</li>"
                    "<li>add it to your applications menu and desktop</li>"
                    "<li>register it with GNOME Software and other app centers</li></ul>")
                .arg(version()));
        text->setWordWrap(true);
        text->setOpenExternalLinks(true);
        lay->addWidget(text);
        if (upgrade)
            lay->addWidget(muted("A version is already installed. It will be replaced; your VLC settings and "
                                 "media library are kept."));
        lay->addStretch();
        lay->addWidget(muted("VLC media player is made by VideoLAN and its contributors. "
                             "This is an unofficial build, not affiliated with VideoLAN."));
    }
};

class LocationPage : public QWizardPage {
public:
    QLineEdit *path;
    QCheckBox *menu, *desktop, *terminal, *appstream, *defaultPlayer;
    QLabel *space;

    LocationPage()
    {
        setTitle("Install location and shortcuts");
        auto *lay = new QVBoxLayout(this);
        lay->addWidget(new QLabel("Install VLC Redesign to:"));
        auto *row = new QHBoxLayout;
        const QString current = installedPath();
        path = new QLineEdit(current.isEmpty() ? home() + "/Applications" : QFileInfo(current).absolutePath());
        auto *browse = new QPushButton("Browse…");
        row->addWidget(path, 1);
        row->addWidget(browse);
        lay->addLayout(row);
        space = muted(QString());
        lay->addWidget(space);
        lay->addSpacing(10);
        menu = new QCheckBox("Add to the applications menu");
        desktop = new QCheckBox(QString("Create a desktop shortcut  (%1)").arg(desktopDir()));
        terminal = new QCheckBox("Add the  vlc-redesign  terminal command");
        appstream = new QCheckBox("Show in GNOME Software, KDE Discover and other app centers");
        defaultPlayer = new QCheckBox("Make VLC Redesign the default player for audio and video files");
        for (QCheckBox *c : {menu, desktop, terminal, appstream})
            c->setChecked(true);
        for (QCheckBox *c : {menu, desktop, terminal, appstream, defaultPlayer})
            lay->addWidget(c);
        lay->addStretch();
        lay->addWidget(muted("Shortcuts start VLC Redesign directly, so AppImageLauncher (which crashes on "
                             "some Wayland desktops) is never involved."));

        QObject::connect(browse, &QPushButton::clicked, this, [this] {
            const QString d = QFileDialog::getExistingDirectory(this, "Install location", path->text());
            if (!d.isEmpty())
                path->setText(d);
        });
        QObject::connect(path, &QLineEdit::textChanged, this, [this] { updateSpace(); });
        updateSpace();
    }

    void updateSpace()
    {
        QDir d(path->text());
        while (!d.exists() && !d.isRoot())
            d.setPath(QFileInfo(d.absolutePath()).absolutePath());
        const QStorageInfo info(d.absolutePath());
        const qint64 size = QFileInfo(runningAppImage()).size();
        space->setText(QString("%1 GB free there. VLC Redesign needs about %2 MB.")
                           .arg(info.bytesAvailable() / 1073741824.0, 0, 'f', 1)
                           .arg(qMax<qint64>(1, size / 1048576)));
    }

    Options options() const
    {
        Options o;
        QString dir = path->text().trimmed();
        if (dir.startsWith('~'))
            dir = home() + dir.mid(1);
        o.dir = QDir(dir).absolutePath();
        o.menu = menu->isChecked();
        o.desktop = desktop->isChecked();
        o.terminal = terminal->isChecked();
        o.appstream = appstream->isChecked();
        o.defaultPlayer = defaultPlayer->isChecked();
        return o;
    }

    bool validatePage() override
    {
        const QString dir = options().dir;
        QDir().mkpath(dir);
        QFile probe(dir + "/.vlc-redesign-write-test");
        if (!probe.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "Can't install there", "That folder isn't writable:\n" + dir);
            return false;
        }
        probe.close();
        probe.remove();
        return true;
    }

};

class InstallPage : public QWizardPage {
public:
    LocationPage *loc;
    QPlainTextEdit *log;
    bool ok = false;
    QString installed;

    explicit InstallPage(LocationPage *l) : loc(l)
    {
        setTitle("Installing");
        auto *lay = new QVBoxLayout(this);
        log = new QPlainTextEdit;
        log->setReadOnly(true);
        lay->addWidget(log);
    }

    void initializePage() override
    {
        ok = false;
        log->clear();
        wizard()->button(QWizard::BackButton)->setEnabled(false);
        auto line = [this](const QString &s) {
            log->appendPlainText(s);
            QApplication::processEvents();
        };
        ok = install(loc->options(), line, &installed);
        if (!ok)
            wizard()->button(QWizard::BackButton)->setEnabled(true);
        emit completeChanged();
        if (ok)
            wizard()->next();
    }

    bool isComplete() const override { return ok; }
};

class FinishPage : public QWizardPage {
public:
    InstallPage *inst;
    QLabel *summary;
    QCheckBox *launch;

    explicit FinishPage(InstallPage *i) : inst(i)
    {
        auto *lay = new QVBoxLayout(this);
        auto *title = new QLabel("All set!");
        title->setObjectName("title");
        lay->addWidget(title);
        summary = new QLabel;
        summary->setWordWrap(true);
        lay->addWidget(summary);
        lay->addStretch();
        launch = new QCheckBox("Start VLC Redesign now");
        launch->setChecked(true);
        lay->addWidget(launch);
    }

    void initializePage() override
    {
        QSettings s(recordFile(), QSettings::IniFormat);
        const QStringList parts = s.value("parts").toStringList();
        QStringList lines{QString("Installed to <b>%1</b>").arg(inst->installed.toHtmlEscaped())};
        const QList<QPair<QString, QString>> desc{
            {"menu", "Added to your applications menu"},
            {"desktop", "Desktop shortcut created"},
            {"terminal", "Run  <b>vlc-redesign</b>  from a terminal"},
            {"appstream", "App center info registered"},
            {"default", "Set as the default media player"}};
        for (const auto &d : desc)
            if (parts.contains(d.first))
                lines << "✓ " + d.second;
        if (parts.contains("desktop"))
            lines << "(on GNOME you may need the Desktop Icons extension to see desktop shortcuts)";
        if (QFileInfo(runningAppImage()).canonicalFilePath() != QFileInfo(inst->installed).canonicalFilePath())
            lines << QString("<br>You can delete the downloaded file now:<br><i>%1</i>")
                         .arg(runningAppImage().toHtmlEscaped());
        lines << "<br>To uninstall later, right-click VLC Redesign in the applications menu → "
                 "<i>Uninstall VLC Redesign</i>, or run  <b>vlc-redesign --uninstall</b>.";
        summary->setText(lines.join("<br>"));
    }
};

class SetupWizard : public QWizard {
public:
    LocationPage *loc;
    InstallPage *inst;
    FinishPage *finish;
    bool runWithoutInstall = false;

    SetupWizard()
    {
        setWindowTitle("VLC Redesign Setup");
        setWizardStyle(QWizard::ClassicStyle);
        setPixmap(QWizard::WatermarkPixmap, sideArt().scaledToHeight(440, Qt::SmoothTransformation));
        setOption(QWizard::NoBackButtonOnLastPage);
        setOption(QWizard::HaveCustomButton1);
        setButtonText(QWizard::CustomButton1, "Run without installing");
        resize(780, 500);
        loc = new LocationPage;
        inst = new InstallPage(loc);
        finish = new FinishPage(inst);
        addPage(new WelcomePage(!installedPath().isEmpty()));
        addPage(loc);
        addPage(inst);
        addPage(finish);
        loc->setCommitPage(true);
        setButtonText(QWizard::CommitButton, "Install");
        connect(this, &QWizard::customButtonClicked, this, [this] {
            runWithoutInstall = true;
            reject();
        });
        connect(this, &QWizard::currentIdChanged, this, [this] {
            button(QWizard::CustomButton1)->setVisible(currentPage() != finish && currentPage() != inst);
        });
    }
};

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(APP_NAME);
    app.setDesktopFileName(APP_ID);
    const QString icon = appDir() + "/usr/share/icons/hicolor/256x256/apps/vlc-redesign.png";
    app.setWindowIcon(QIcon(icon));
    applyTheme(app);

    const QStringList args = app.arguments();
    const bool yes = args.contains("--yes");

    if (args.contains("--uninstall")) {
        if (!yes && QMessageBox::question(nullptr, "Uninstall VLC Redesign",
                "<b>Uninstall VLC Redesign?</b><br><br>This removes the app, its menu entry, desktop "
                "shortcut and terminal command. Your VLC settings and media library are kept.")
                != QMessageBox::Yes)
            return 1;
        uninstall();
        if (!yes)
            QMessageBox::information(nullptr, "Uninstalled", "VLC Redesign has been uninstalled.");
        return 0;
    }

    if (args.contains("--install") && yes) {
        QString installed;
        const bool ok = install(Options(), [](const QString &s) {
            std::fprintf(stderr, "%s\n", qPrintable(s));
        }, &installed);
        return ok ? 0 : 1;
    }

    SetupWizard w;
    const int r = w.exec();
    if (w.runWithoutInstall)
        return EXIT_RUN_UNINSTALLED;
    if (r == QDialog::Accepted && w.inst->ok) {
        std::printf("%s\n", qPrintable(w.inst->installed));
        return w.finish->launch->isChecked() ? 0 : 2;
    }
    return 1;
}
