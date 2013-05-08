/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidmanifesteditorwidget.h"
#include "androidmanifesteditor.h"
#include "androidconstants.h"
#include "androidmanifestdocument.h"

#include <coreplugin/infobar.h>
#include <texteditor/plaintexteditor.h>
#include <projectexplorer/projectwindow.h>
#include <projectexplorer/iprojectproperties.h>
#include <texteditor/texteditoractionhandler.h>

#include <QLineEdit>
#include <QFileInfo>
#include <QDomDocument>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QDebug>
#include <QToolButton>
#include <utils/fileutils.h>
#include <QListView>
#include <QPushButton>
#include <QFileDialog>
#include <QTimer>

namespace {
const QLatin1String packageNameRegExp("^([a-z_]{1}[a-z0-9_]+(\\.[a-zA-Z_]{1}[a-zA-Z0-9_]*)*)$");
const char infoBarId[] = "Android.AndroidManifestEditor.InfoBar";

bool checkPackageName(const QString &packageName)
{
    return QRegExp(packageNameRegExp).exactMatch(packageName);
}
} // anonymous namespace


using namespace Android;
using namespace Android::Internal;

AndroidManifestEditorWidget::AndroidManifestEditorWidget(QWidget *parent, TextEditor::TextEditorActionHandler *ah)
    : TextEditor::PlainTextEditorWidget(parent),
      m_dirty(false),
      m_stayClean(false),
      m_setAppName(false),
      m_ah(ah)
{
    QSharedPointer<AndroidManifestDocument> doc(new AndroidManifestDocument(this));
    doc->setMimeType(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));
    setBaseTextDocument(doc);

    ah->setupActions(this);
    configure(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));

    initializePage();

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_timerParseCheck.setInterval(800);
    m_timerParseCheck.setSingleShot(true);

    connect(&m_timerParseCheck, SIGNAL(timeout()),
            this, SLOT(delayedParseCheck()));

    connect(document(), SIGNAL(contentsChanged()),
            this, SLOT(startParseCheck()));
}

TextEditor::BaseTextEditor *AndroidManifestEditorWidget::createEditor()
{
    return new AndroidManifestEditor(this);
}

void AndroidManifestEditorWidget::initializePage()
{
    ProjectExplorer::PanelsWidget *generalPanel = new ProjectExplorer::PanelsWidget(this);
    generalPanel->widget()->setMinimumWidth(0);
    generalPanel->widget()->setMaximumWidth(900);

    // Package
    ProjectExplorer::PropertiesPanel *manifestPanel = new ProjectExplorer::PropertiesPanel;
    manifestPanel->setDisplayName(tr("Package"));
    {
        QWidget *mainWidget = new QWidget();

        QFormLayout *formLayout = new QFormLayout(mainWidget);

        m_packageNameLineEdit = new QLineEdit(mainWidget);
        formLayout->addRow(tr("Package name:"), m_packageNameLineEdit);

        m_packageNameWarning = new QLabel;
        m_packageNameWarning->setText(tr("The package name is not valid."));
        m_packageNameWarning->setVisible(false);

        m_packageNameWarningIcon = new QLabel;
        m_packageNameWarningIcon->setPixmap(QPixmap(QString::fromUtf8(":/projectexplorer/images/compile_warning.png")));
        m_packageNameWarningIcon->setVisible(false);
        m_packageNameWarningIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        QHBoxLayout *warningRow = new QHBoxLayout;
        warningRow->setMargin(0);
        warningRow->addWidget(m_packageNameWarningIcon);
        warningRow->addWidget(m_packageNameWarning);

        formLayout->addRow(QString(), warningRow);


        m_versionCode = new QSpinBox(mainWidget);
        m_versionCode->setMaximum(99);
        m_versionCode->setValue(1);
        formLayout->addRow(tr("Version code:"), m_versionCode);

        m_versionNameLinedit = new QLineEdit(mainWidget);
        formLayout->addRow(tr("Version name:"), m_versionNameLinedit);

        manifestPanel->setWidget(mainWidget);

        connect(m_packageNameLineEdit, SIGNAL(textEdited(QString)),
                this, SLOT(setPackageName()));
        connect(m_versionCode, SIGNAL(valueChanged(int)),
                this, SLOT(setDirty()));
        connect(m_versionNameLinedit, SIGNAL(textEdited(QString)),
                this, SLOT(setDirty()));

    }
    generalPanel->addPropertiesPanel(manifestPanel);

    // Application
    ProjectExplorer::PropertiesPanel *applicationPanel = new ProjectExplorer::PropertiesPanel;
    applicationPanel->setDisplayName(tr("Application"));
    {
        QWidget *mainWidget = new QWidget();
        QFormLayout *formLayout = new QFormLayout(mainWidget);

        m_appNameLineEdit = new QLineEdit(mainWidget);
        formLayout->addRow(tr("Application name:"), m_appNameLineEdit);

        m_targetLineEdit = new QLineEdit(mainWidget);
        formLayout->addRow(tr("Run:"), m_targetLineEdit);

        QHBoxLayout *iconLayout = new QHBoxLayout();
        m_lIconButton = new QToolButton(mainWidget);
        m_lIconButton->setMinimumSize(QSize(48, 48));
        m_lIconButton->setMaximumSize(QSize(48, 48));
        iconLayout->addWidget(m_lIconButton);

        iconLayout->addItem(new QSpacerItem(28, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

        m_mIconButton = new QToolButton(mainWidget);
        m_mIconButton->setMinimumSize(QSize(48, 48));
        m_mIconButton->setMaximumSize(QSize(48, 48));
        iconLayout->addWidget(m_mIconButton);

        iconLayout->addItem(new QSpacerItem(28, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

        m_hIconButton = new QToolButton(mainWidget);
        m_hIconButton->setMinimumSize(QSize(48, 48));
        m_hIconButton->setMaximumSize(QSize(48, 48));
        iconLayout->addWidget(m_hIconButton);

        formLayout->addRow(tr("Application icon:"), iconLayout);

        applicationPanel->setWidget(mainWidget);

        connect(m_appNameLineEdit, SIGNAL(textEdited(QString)),
                this, SLOT(setAppName()));
        connect(m_targetLineEdit, SIGNAL(textEdited(QString)),
                this, SLOT(setDirty()));

        connect(m_lIconButton, SIGNAL(clicked()), SLOT(setLDPIIcon()));
        connect(m_mIconButton, SIGNAL(clicked()), SLOT(setMDPIIcon()));
        connect(m_hIconButton, SIGNAL(clicked()), SLOT(setHDPIIcon()));
    }
    generalPanel->addPropertiesPanel(applicationPanel);


    // Permissions
    ProjectExplorer::PropertiesPanel *permissionsPanel = new ProjectExplorer::PropertiesPanel;
    permissionsPanel->setDisplayName(tr("Permissions"));
    {
        QWidget *mainWidget = new QWidget();
        QGridLayout *layout = new QGridLayout(mainWidget);

        m_permissionsModel = new PermissionsModel(this);

        m_permissionsListView = new QListView(mainWidget);
        m_permissionsListView->setModel(m_permissionsModel);
        m_permissionsListView->setMinimumSize(QSize(0, 200));
        layout->addWidget(m_permissionsListView, 0, 0, 3, 1);

        m_removePermissionButton = new QPushButton(mainWidget);
        m_removePermissionButton->setText(tr("Remove"));
        layout->addWidget(m_removePermissionButton, 0, 1);

        m_permissionsComboBox = new QComboBox(mainWidget);
        m_permissionsComboBox->insertItems(0, QStringList()
         << QStringLiteral("android.permission.ACCESS_CHECKIN_PROPERTIES")
         << QStringLiteral("android.permission.ACCESS_COARSE_LOCATION")
         << QStringLiteral("android.permission.ACCESS_FINE_LOCATION")
         << QStringLiteral("android.permission.ACCESS_LOCATION_EXTRA_COMMANDS")
         << QStringLiteral("android.permission.ACCESS_MOCK_LOCATION")
         << QStringLiteral("android.permission.ACCESS_NETWORK_STATE")
         << QStringLiteral("android.permission.ACCESS_SURFACE_FLINGER")
         << QStringLiteral("android.permission.ACCESS_WIFI_STATE")
         << QStringLiteral("android.permission.ACCOUNT_MANAGER")
         << QStringLiteral("android.permission.AUTHENTICATE_ACCOUNTS")
         << QStringLiteral("android.permission.BATTERY_STATS")
         << QStringLiteral("android.permission.BIND_APPWIDGET")
         << QStringLiteral("android.permission.BIND_DEVICE_ADMIN")
         << QStringLiteral("android.permission.BIND_INPUT_METHOD")
         << QStringLiteral("android.permission.BIND_REMOTEVIEWS")
         << QStringLiteral("android.permission.BIND_WALLPAPER")
         << QStringLiteral("android.permission.BLUETOOTH")
         << QStringLiteral("android.permission.BLUETOOTH_ADMIN")
         << QStringLiteral("android.permission.BRICK")
         << QStringLiteral("android.permission.BROADCAST_PACKAGE_REMOVED")
         << QStringLiteral("android.permission.BROADCAST_SMS")
         << QStringLiteral("android.permission.BROADCAST_STICKY")
         << QStringLiteral("android.permission.BROADCAST_WAP_PUSH")
         << QStringLiteral("android.permission.CALL_PHONE")
         << QStringLiteral("android.permission.CALL_PRIVILEGED")
         << QStringLiteral("android.permission.CAMERA")
         << QStringLiteral("android.permission.CHANGE_COMPONENT_ENABLED_STATE")
         << QStringLiteral("android.permission.CHANGE_CONFIGURATION")
         << QStringLiteral("android.permission.CHANGE_NETWORK_STATE")
         << QStringLiteral("android.permission.CHANGE_WIFI_MULTICAST_STATE")
         << QStringLiteral("android.permission.CHANGE_WIFI_STATE")
         << QStringLiteral("android.permission.CLEAR_APP_CACHE")
         << QStringLiteral("android.permission.CLEAR_APP_USER_DATA")
         << QStringLiteral("android.permission.CONTROL_LOCATION_UPDATES")
         << QStringLiteral("android.permission.DELETE_CACHE_FILES")
         << QStringLiteral("android.permission.DELETE_PACKAGES")
         << QStringLiteral("android.permission.DEVICE_POWER")
         << QStringLiteral("android.permission.DIAGNOSTIC")
         << QStringLiteral("android.permission.DISABLE_KEYGUARD")
         << QStringLiteral("android.permission.DUMP")
         << QStringLiteral("android.permission.EXPAND_STATUS_BAR")
         << QStringLiteral("android.permission.FACTORY_TEST")
         << QStringLiteral("android.permission.FLASHLIGHT")
         << QStringLiteral("android.permission.FORCE_BACK")
         << QStringLiteral("android.permission.GET_ACCOUNTS")
         << QStringLiteral("android.permission.GET_PACKAGE_SIZE")
         << QStringLiteral("android.permission.GET_TASKS")
         << QStringLiteral("android.permission.GLOBAL_SEARCH")
         << QStringLiteral("android.permission.HARDWARE_TEST")
         << QStringLiteral("android.permission.INJECT_EVENTS")
         << QStringLiteral("android.permission.INSTALL_LOCATION_PROVIDER")
         << QStringLiteral("android.permission.INSTALL_PACKAGES")
         << QStringLiteral("android.permission.INTERNAL_SYSTEM_WINDOW")
         << QStringLiteral("android.permission.INTERNET")
         << QStringLiteral("android.permission.KILL_BACKGROUND_PROCESSES")
         << QStringLiteral("android.permission.MANAGE_ACCOUNTS")
         << QStringLiteral("android.permission.MANAGE_APP_TOKENS")
         << QStringLiteral("android.permission.MASTER_CLEAR")
         << QStringLiteral("android.permission.MODIFY_AUDIO_SETTINGS")
         << QStringLiteral("android.permission.MODIFY_PHONE_STATE")
         << QStringLiteral("android.permission.MOUNT_FORMAT_FILESYSTEMS")
         << QStringLiteral("android.permission.MOUNT_UNMOUNT_FILESYSTEMS")
         << QStringLiteral("android.permission.NFC")
         << QStringLiteral("android.permission.PERSISTENT_ACTIVITY")
         << QStringLiteral("android.permission.PROCESS_OUTGOING_CALLS")
         << QStringLiteral("android.permission.READ_CALENDAR")
         << QStringLiteral("android.permission.READ_CONTACTS")
         << QStringLiteral("android.permission.READ_FRAME_BUFFER")
         << QStringLiteral("com.android.browser.permission.READ_HISTORY_BOOKMARKS")
         << QStringLiteral("android.permission.READ_INPUT_STATE")
         << QStringLiteral("android.permission.READ_LOGS")
         << QStringLiteral("android.permission.READ_OWNER_DATA")
         << QStringLiteral("android.permission.READ_PHONE_STATE")
         << QStringLiteral("android.permission.READ_SMS")
         << QStringLiteral("android.permission.READ_SYNC_SETTINGS")
         << QStringLiteral("android.permission.READ_SYNC_STATS")
         << QStringLiteral("android.permission.REBOOT")
         << QStringLiteral("android.permission.RECEIVE_BOOT_COMPLETED")
         << QStringLiteral("android.permission.RECEIVE_MMS")
         << QStringLiteral("android.permission.RECEIVE_SMS")
         << QStringLiteral("android.permission.RECEIVE_WAP_PUSH")
         << QStringLiteral("android.permission.RECORD_AUDIO")
         << QStringLiteral("android.permission.REORDER_TASKS")
         << QStringLiteral("android.permission.RESTART_PACKAGES")
         << QStringLiteral("android.permission.SEND_SMS")
         << QStringLiteral("android.permission.SET_ACTIVITY_WATCHER")
         << QStringLiteral("com.android.alarm.permission.SET_ALARM")
         << QStringLiteral("android.permission.SET_ALWAYS_FINISH")
         << QStringLiteral("android.permission.SET_ANIMATION_SCALE")
         << QStringLiteral("android.permission.SET_DEBUG_APP")
         << QStringLiteral("android.permission.SET_ORIENTATION")
         << QStringLiteral("android.permission.SET_PREFERRED_APPLICATIONS")
         << QStringLiteral("android.permission.SET_PROCESS_LIMIT")
         << QStringLiteral("android.permission.SET_TIME")
         << QStringLiteral("android.permission.SET_TIME_ZONE")
         << QStringLiteral("android.permission.SET_WALLPAPER")
         << QStringLiteral("android.permission.SET_WALLPAPER_HINTS")
         << QStringLiteral("android.permission.SIGNAL_PERSISTENT_PROCESSES")
         << QStringLiteral("android.permission.STATUS_BAR")
         << QStringLiteral("android.permission.SUBSCRIBED_FEEDS_READ")
         << QStringLiteral("android.permission.SUBSCRIBED_FEEDS_WRITE")
         << QStringLiteral("android.permission.SYSTEM_ALERT_WINDOW")
         << QStringLiteral("android.permission.UPDATE_DEVICE_STATS")
         << QStringLiteral("android.permission.USE_CREDENTIALS")
         << QStringLiteral("android.permission.USE_SIP")
         << QStringLiteral("android.permission.VIBRATE")
         << QStringLiteral("android.permission.WAKE_LOCK")
         << QStringLiteral("android.permission.WRITE_APN_SETTINGS")
         << QStringLiteral("android.permission.WRITE_CALENDAR")
         << QStringLiteral("android.permission.WRITE_CONTACTS")
         << QStringLiteral("android.permission.WRITE_EXTERNAL_STORAGE")
         << QStringLiteral("android.permission.WRITE_GSERVICES")
         << QStringLiteral("com.android.browser.permission.WRITE_HISTORY_BOOKMARKS")
         << QStringLiteral("android.permission.WRITE_OWNER_DATA")
         << QStringLiteral("android.permission.WRITE_SECURE_SETTINGS")
         << QStringLiteral("android.permission.WRITE_SETTINGS")
         << QStringLiteral("android.permission.WRITE_SMS")
         << QStringLiteral("android.permission.WRITE_SYNC_SETTINGS")
        );
        m_permissionsComboBox->setEditable(true);
        layout->addWidget(m_permissionsComboBox, 4, 0);

        m_addPermissionButton = new QPushButton(mainWidget);
        m_addPermissionButton->setText(tr("Add"));
        layout->addWidget(m_addPermissionButton, 4, 1);

        permissionsPanel->setWidget(mainWidget);

        connect(m_addPermissionButton, SIGNAL(clicked()),
                this, SLOT(addPermission()));
        connect(m_removePermissionButton, SIGNAL(clicked()),
                this, SLOT(removePermission()));
        connect(m_permissionsComboBox, SIGNAL(currentTextChanged(QString)),
                this, SLOT(updateAddRemovePermissionButtons()));
    }
    generalPanel->addPropertiesPanel(permissionsPanel);

    m_overlayWidget = generalPanel;
}

void AndroidManifestEditorWidget::resizeEvent(QResizeEvent *event)
{
    PlainTextEditorWidget::resizeEvent(event);
    QSize s = QSize(rect().width(), rect().height());
    m_overlayWidget->resize(s);
}

bool AndroidManifestEditorWidget::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    bool result = PlainTextEditorWidget::open(errorString, fileName, realFileName);
    if (!result)
        return result;

    Q_UNUSED(errorString);
    QString error;
    int errorLine;
    int errorColumn;
    QDomDocument doc;
    if (doc.setContent(toPlainText(), &error, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &error, &errorLine, &errorColumn)) {
            if (activePage() != Source)
                syncToWidgets(doc);
            return true;
        }
    }
    // some error occured
    updateInfoBar(error, errorLine, errorColumn);
    setActivePage(Source);

    return true;
}

void AndroidManifestEditorWidget::setDirty(bool dirty)
{
    if (m_stayClean)
        return;
    m_dirty = dirty;
    emit changed();
}

bool AndroidManifestEditorWidget::isModified() const
{
    return m_dirty
            || !m_hIconPath.isEmpty()
            || !m_mIconPath.isEmpty()
            || !m_lIconPath.isEmpty()
            || m_setAppName;
}

AndroidManifestEditorWidget::EditorPage AndroidManifestEditorWidget::activePage() const
{
    return m_overlayWidget->isVisibleTo(this) ? General : Source;
}

bool AndroidManifestEditorWidget::setActivePage(EditorPage page)
{
    EditorPage prevPage = activePage();

    if (prevPage == page)
        return true;

    if (page == Source) {
        syncToEditor();
    } else if (prevPage == Source) {
        if (!syncToWidgets()) {
            return false;
        }
    }

    m_overlayWidget->setVisible(page == General);
    if (page == General) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    } else {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    return true;
}

void AndroidManifestEditorWidget::preSave()
{
    if (activePage() != Source)
        syncToEditor();

    if (m_setAppName) {
        QString baseDir = QFileInfo(static_cast<AndroidManifestDocument *>(editor()->document())->fileName()).absolutePath();
        QString fileName = baseDir + QLatin1String("/res/values/strings.xml");
        QFile f(fileName);
        if (f.open(QIODevice::ReadOnly)) {
            QDomDocument doc;
            if (doc.setContent(f.readAll())) {
                QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("string"));
                while (!metadataElem.isNull()) {
                    if (metadataElem.attribute(QLatin1String("name")) == QLatin1String("app_name")) {
                        metadataElem.removeChild(metadataElem.firstChild());
                        metadataElem.appendChild(doc.createTextNode(m_appNameLineEdit->text()));
                        break;
                    }
                    metadataElem = metadataElem.nextSiblingElement(QLatin1String("string"));
                }

                f.close();
                f.open(QIODevice::WriteOnly);
                f.write(doc.toByteArray((4)));
            }
        }
        m_setAppName = false;
    }

    QString baseDir = QFileInfo(static_cast<AndroidManifestDocument *>(editor()->document())->fileName()).absolutePath();
    if (!m_lIconPath.isEmpty()) {
        copyIcon(LowDPI, baseDir, m_lIconPath);
        m_lIconPath.clear();
    }
    if (!m_mIconPath.isEmpty()) {
        copyIcon(MediumDPI, baseDir, m_mIconPath);
        m_mIconPath.clear();
    }
    if (!m_hIconPath.isEmpty()) {
        copyIcon(HighDPI, baseDir, m_hIconPath);
        m_hIconPath.clear();
    }
    // no need to emit changed() since this is called as part of saving

    updateInfoBar();
}

bool AndroidManifestEditorWidget::syncToWidgets()
{
    QDomDocument doc;
    QString errorMessage;
    int errorLine, errorColumn;
    if (doc.setContent(toPlainText(), &errorMessage, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &errorMessage, &errorLine, &errorColumn)) {
            hideInfoBar();
            syncToWidgets(doc);
            return true;
        }
    }

    updateInfoBar(errorMessage, errorLine, errorColumn);
    return false;
}

bool AndroidManifestEditorWidget::checkDocument(QDomDocument doc, QString *errorMessage, int *errorLine, int *errorColumn)
{
    QDomElement manifest = doc.documentElement();
    if (manifest.tagName() != QLatin1String("manifest")) {
        *errorMessage = tr("The structure of the android manifest file is corrupt. Expected a top level 'manifest' node.");
        *errorLine = -1;
        *errorColumn = -1;
        return false;
    } else if (manifest.firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).isNull()) {
        // missing either application or activity element
        *errorMessage = tr("The structure of the android manifest file is corrupt. Expected a 'application' and 'activity' sub node.");
        *errorLine = -1;
        *errorColumn = -1;
        return false;
    }
    return true;
}

void AndroidManifestEditorWidget::startParseCheck()
{
    m_timerParseCheck.start();
}

void AndroidManifestEditorWidget::delayedParseCheck()
{
    updateInfoBar();
}

void AndroidManifestEditorWidget::updateInfoBar()
{
    if (activePage() != Source) {
        m_timerParseCheck.stop();
        return;
    }
    QDomDocument doc;
    int errorLine, errorColumn;
    QString errorMessage;
    if (doc.setContent(toPlainText(), &errorMessage, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &errorMessage, &errorLine, &errorColumn)) {
            hideInfoBar();
            return;
        }
    }

    updateInfoBar(errorMessage, errorLine, errorColumn);
}

void AndroidManifestEditorWidget::updateInfoBar(const QString &errorMessage, int line, int column)
{
    Core::InfoBar *infoBar = editorDocument()->infoBar();
    QString text;
    if (line < 0)
        text = tr("Could not parse file: '%1'").arg(errorMessage);
    else
        text = tr("%2: Could not parse file: '%1'").arg(errorMessage).arg(line);
    Core::InfoBarEntry infoBarEntry(infoBarId, text);
    infoBarEntry.setCustomButtonInfo(tr("Goto error"), this, SLOT(gotoError()));
    infoBar->removeInfo(infoBarId);
    infoBar->addInfo(infoBarEntry);

    m_errorLine = line;
    m_errorColumn = column;
    m_timerParseCheck.stop();
}

void AndroidManifestEditorWidget::hideInfoBar()
{
    Core::InfoBar *infoBar = editorDocument()->infoBar();
    infoBar->removeInfo(infoBarId);
    m_timerParseCheck.stop();
}

void AndroidManifestEditorWidget::gotoError()
{
    gotoLine(m_errorLine, m_errorColumn);
}

void AndroidManifestEditorWidget::syncToWidgets(const QDomDocument &doc)
{
    m_stayClean = true;
    QDomElement manifest = doc.documentElement();
    m_packageNameLineEdit->setText(manifest.attribute(QLatin1String("package")));
    m_versionCode->setValue(manifest.attribute(QLatin1String("android:versionCode")).toInt());
    m_versionNameLinedit->setText(manifest.attribute(QLatin1String("android:versionName")));

    QString baseDir = QFileInfo(static_cast<AndroidManifestDocument *>(editor()->document())->fileName()).absolutePath();
    QString fileName = baseDir + QLatin1String("/res/values/strings.xml");

    QFile f(fileName);
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        QDomDocument doc;
        if (doc.setContent(&f)) {
            QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("string"));
            while (!metadataElem.isNull()) {
                if (metadataElem.attribute(QLatin1String("name")) == QLatin1String("app_name")) {
                    m_appNameLineEdit->setText(metadataElem.text());
                    break;
                }
                metadataElem = metadataElem.nextSiblingElement(QLatin1String("string"));
            }
        }
    }

    QDomElement metadataElem = manifest.firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).firstChildElement(QLatin1String("meta-data"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name")) {
            m_targetLineEdit->setText(metadataElem.attribute(QLatin1String("android:value")));
            break;
        }
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }

    m_lIconButton->setIcon(icon(baseDir, LowDPI));
    m_mIconButton->setIcon(icon(baseDir, MediumDPI));
    m_hIconButton->setIcon(icon(baseDir, HighDPI));
    m_lIconPath.clear();
    m_mIconPath.clear();
    m_hIconPath.clear();

    QStringList permissions;
    QDomElement permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        permissions << permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement(QLatin1String("uses-permission"));
    }

    m_permissionsModel->setPermissions(permissions);
    updateAddRemovePermissionButtons();

    m_stayClean = false;
    m_dirty = false;
}

void AndroidManifestEditorWidget::syncToEditor()
{
    QDomDocument doc;
    if (!doc.setContent(toPlainText())) {
        // This should not happen
        updateInfoBar();
        return;
    }

    QDomElement manifest = doc.documentElement();
    manifest.setAttribute(QLatin1String("package"), m_packageNameLineEdit->text());
    manifest.setAttribute(QLatin1String("android:versionCode"), m_versionCode->value());
    manifest.setAttribute(QLatin1String("android:versionName"), m_versionNameLinedit->text());

    setAndroidAppLibName(doc, manifest.firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")), m_targetLineEdit->text());

    // permissions
    QDomElement permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        manifest.removeChild(permissionElem);
        permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    }

    foreach (const QString &permission, m_permissionsModel->permissions()) {
        permissionElem = doc.createElement(QLatin1String("uses-permission"));
        permissionElem.setAttribute(QLatin1String("android:name"), permission);
        manifest.appendChild(permissionElem);
    }

    bool ensureIconAttribute =  !m_lIconPath.isEmpty()
            || !m_mIconPath.isEmpty()
            || !m_hIconPath.isEmpty();

    if (ensureIconAttribute) {
        QDomElement applicationElem = manifest.firstChildElement(QLatin1String("application"));
        applicationElem.setAttribute(QLatin1String("android:icon"), QLatin1String("@drawable/icon"));
    }


    QString newText = doc.toString();
    if (newText == toPlainText())
        return;

    setPlainText(newText);
    document()->setModified(true); // Why is this necessary?

    m_dirty = false;
}

bool AndroidManifestEditorWidget::setAndroidAppLibName(QDomDocument document, QDomElement activity, const QString &name)
{
    QDomElement metadataElem = activity.firstChildElement(QLatin1String("meta-data"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name")) {
            metadataElem.setAttribute(QLatin1String("android:value"), name);
            return true;
        }
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }
    QDomElement elem = document.createElement(QLatin1String("meta-data"));
    elem.setAttribute(QLatin1String("android:name"), QLatin1String("android.app.lib_name"));
    elem.setAttribute(QLatin1String("android:value"), name);
    activity.appendChild(elem);
    return true;
}

QString AndroidManifestEditorWidget::iconPath(const QString &baseDir, IconDPI dpi)
{
    Utils::FileName fileName = Utils::FileName::fromString(baseDir);
    switch (dpi) {
    case HighDPI:
        fileName.appendPath(QLatin1String("res/drawable-hdpi/icon.png"));
        break;
    case MediumDPI:
        fileName.appendPath(QLatin1String("res/drawable-mdpi/icon.png"));
        break;
    case LowDPI:
        fileName.appendPath(QLatin1String("res/drawable-ldpi/icon.png"));
        break;
    default:
        return QString();
    }
    return fileName.toString();
}

QIcon AndroidManifestEditorWidget::icon(const QString &baseDir, IconDPI dpi)
{

    if (dpi == HighDPI && !m_hIconPath.isEmpty())
        return QIcon(m_hIconPath);

    if (dpi == MediumDPI && !m_mIconPath.isEmpty())
        return QIcon(m_mIconPath);

    if (dpi == LowDPI && !m_lIconPath.isEmpty())
        return QIcon(m_lIconPath);

    QString fileName = iconPath(baseDir, dpi);
    if (fileName.isEmpty())
        return QIcon();
    return QIcon(fileName);
}

void AndroidManifestEditorWidget::copyIcon(IconDPI dpi, const QString &baseDir, const QString &filePath)
{
    if (!QFileInfo(filePath).exists())
        return;

    const QString targetPath = iconPath(baseDir, dpi);
    QFile::remove(targetPath);
    QDir dir;
    dir.mkpath(QFileInfo(targetPath).absolutePath());
    QFile::copy(filePath, targetPath);
}

void AndroidManifestEditorWidget::setLDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose Low DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (file.isEmpty())
        return;
    m_lIconPath = file;
    m_lIconButton->setIcon(QIcon(file));
    setDirty(true);
}

void AndroidManifestEditorWidget::setMDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose Medium DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (file.isEmpty())
        return;
    m_mIconPath = file;
    m_mIconButton->setIcon(QIcon(file));
    setDirty(true);
}

void AndroidManifestEditorWidget::setHDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose High DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (file.isEmpty())
        return;
    m_hIconPath = file;
    m_hIconButton->setIcon(QIcon(file));
    setDirty(true);
}

void AndroidManifestEditorWidget::updateAddRemovePermissionButtons()
{
    QStringList permissions = m_permissionsModel->permissions();
    m_removePermissionButton->setEnabled(!permissions.isEmpty());

    m_addPermissionButton->setEnabled(!permissions.contains(m_permissionsComboBox->currentText()));
}

void AndroidManifestEditorWidget::addPermission()
{
    m_permissionsModel->addPermission(m_permissionsComboBox->currentText());
    updateAddRemovePermissionButtons();
    setDirty(true);
}

void AndroidManifestEditorWidget::removePermission()
{
    QModelIndex idx = m_permissionsListView->currentIndex();
    if (idx.isValid())
        m_permissionsModel->removePermission(idx.row());
    updateAddRemovePermissionButtons();
    setDirty(true);
}

void AndroidManifestEditorWidget::setAppName()
{
    m_setAppName = true;
    emit changed();
}

void AndroidManifestEditorWidget::setPackageName()
{
    const QString packageName= m_packageNameLineEdit->text();

    bool valid = checkPackageName(packageName);
    m_packageNameWarning->setVisible(!valid);
    m_packageNameWarningIcon->setVisible(!valid);
    setDirty(true);
}


///////////////////////////// PermissionsModel /////////////////////////////

PermissionsModel::PermissionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void PermissionsModel::setPermissions(const QStringList &permissions)
{
    beginResetModel();
    m_permissions = permissions;
    qSort(m_permissions);
    endResetModel();
}

const QStringList &PermissionsModel::permissions()
{
    return m_permissions;
}

QModelIndex PermissionsModel::addPermission(const QString &permission)
{
    const int idx = qLowerBound(m_permissions, permission) - m_permissions.constBegin();
    beginInsertRows(QModelIndex(), idx, idx);
    m_permissions.insert(idx, permission);
    endInsertRows();
    return index(idx);
}

bool PermissionsModel::updatePermission(QModelIndex index, const QString &permission)
{
    if (!index.isValid())
        return false;
    if (m_permissions[index.row()] == permission)
        return false;

    int newIndex = qLowerBound(m_permissions.constBegin(), m_permissions.constEnd(), permission) - m_permissions.constBegin();
    if (newIndex == index.row() || newIndex == index.row() + 1) {
        m_permissions[index.row()] = permission;
        emit dataChanged(index, index);
        return true;
    }

    beginMoveRows(QModelIndex(), index.row(), index.row(), QModelIndex(), newIndex);

    if (newIndex > index.row()) {
        m_permissions.insert(newIndex, permission);
        m_permissions.removeAt(index.row());
    } else {
        m_permissions.removeAt(index.row());
        m_permissions.insert(newIndex, permission);
    }
    endMoveRows();

    return true;
}

void PermissionsModel::removePermission(int index)
{
    if (index >= m_permissions.size())
        return;
    beginRemoveRows(QModelIndex(), index, index);
    m_permissions.removeAt(index);
    endRemoveRows();
}

QVariant PermissionsModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();
    return m_permissions[index.row()];
}

int PermissionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_permissions.count();
}
