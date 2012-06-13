/*
    SuperCollider Qt IDE
    Copyright (c) 2012 Jakob Leben & Tim Blechmann
    http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "main_window.hpp"
#include "../core/main.hpp"
#include "../core/doc_manager.hpp"
#include "code_editor/editor.hpp"
#include "multi_editor.hpp"
#include "cmd_line.hpp"
#include "find_replace_tool.hpp"
#include "tool_box.hpp"
#include "doc_list.hpp"
#include "post_window.hpp"
#include "settings/dialog.hpp"
#include "documents_dialog.hpp"

#include <QAction>
#include <QShortcut>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QApplication>
#include <QMessageBox>
#include <QStatusBar>
#include <QFileDialog>

namespace ScIDE {

MainWindow * MainWindow::mInstance = 0;

MainWindow::MainWindow(Main * main) :
    mMain(main),
    mDocDialog(0)
{
    Q_ASSERT(!mInstance);
    mInstance = this;

    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );

    // Construct status bar:

    mLangStatus = new StatusLabel();
    mLangStatus->setText("Inactive");
    mSynthStatus = new StatusLabel();
    mSynthStatus->setText("Inactive");

    QStatusBar *status = statusBar();
    status->addPermanentWidget( new QLabel("Interpreter:") );
    status->addPermanentWidget( mLangStatus );
    status->addPermanentWidget( new QLabel("Synth:") );
    status->addPermanentWidget( mSynthStatus );

    // Code editor
    mEditors = new MultiEditor(main);

    // Tools

    mCmdLine = new CmdLine("Command Line:");
    connect(mCmdLine, SIGNAL(invoked(QString, bool)),
            main->scProcess(), SLOT(evaluateCode(QString, bool)));

    mFindReplaceTool = new TextFindReplacePanel;

    mToolBox = new ToolBox;
    mToolBox->addWidget(mCmdLine);
    mToolBox->addWidget(mFindReplaceTool);
    mToolBox->hide();

    // Docks
    mDocListDock = new DocumentsDock(main->documentManager(), this);
    mPostDock = new PostDock(this);

    // Layout

    QVBoxLayout *center_box = new QVBoxLayout;
    center_box->setContentsMargins(0,0,0,0);
    center_box->setSpacing(0);
    center_box->addWidget(mEditors);
    center_box->addWidget(mToolBox);

    QWidget *central = new QWidget;
    central->setLayout(center_box);
    setCentralWidget(central);

    addDockWidget(Qt::LeftDockWidgetArea, mDocListDock);
    addDockWidget(Qt::BottomDockWidgetArea, mPostDock);

    // A system for easy evaluation of pre-defined code:
    connect(&mCodeEvalMapper, SIGNAL(mapped(QString)),
            this, SIGNAL(evaluateCode(QString)));
    connect(this, SIGNAL(evaluateCode(QString,bool)),
            main->scProcess(), SLOT(evaluateCode(QString,bool)));
    // Interpreter: post output
    connect(main->scProcess(), SIGNAL( scPost(QString) ),
            mPostDock->mPostWindow, SLOT( post(QString) ) );
    // Interpreter: monitor running state
    connect(main->scProcess(), SIGNAL( stateChanged(QProcess::ProcessState) ),
            this, SLOT( onInterpreterStateChanged(QProcess::ProcessState) ) );
    // Interpreter: forward status messages
    connect(main->scProcess(), SIGNAL(statusMessage(const QString&)),
            status, SLOT(showMessage(const QString&)));
    // Document list interaction
    connect(mDocListDock->list(), SIGNAL(clicked(Document*)),
            mEditors, SLOT(setCurrent(Document*)));
    connect(mEditors, SIGNAL(currentChanged(Document*)),
            mDocListDock->list(), SLOT(setCurrent(Document*)),
            Qt::QueuedConnection);
    // Update actions on document change
    connect(mEditors, SIGNAL(currentChanged(Document*)),
            this, SLOT(onCurrentDocumentChanged(Document*)));
    // Document management
    DocumentManager *docMng = main->documentManager();
    connect(docMng, SIGNAL(changedExternally(Document*)),
            this, SLOT(onDocumentChangedExternally(Document*)));
    connect(docMng, SIGNAL(recentsChanged()),
            this, SLOT(updateRecentDocsMenu()));
    // ToolBox
    connect(mToolBox->closeButton(), SIGNAL(clicked()), this, SLOT(hideToolBox()));

    createActions();
    createMenus();

    // Initialize recent documents menu
    updateRecentDocsMenu();

    QIcon icon;
    icon.addFile(":/icons/sc-cube-128");
    icon.addFile(":/icons/sc-cube-48");
    icon.addFile(":/icons/sc-cube-32");
    icon.addFile(":/icons/sc-cube-16");
    QApplication::setWindowIcon(icon);
}

void MainWindow::createActions()
{
    Settings::Manager *s = mMain->settings();
    s->beginGroup("IDE/shortcuts");

    QAction *act;

    // File

    mActions[Quit] = act = new QAction(
        QIcon::fromTheme("application-exit"), tr("&Quit..."), this);
    act->setShortcut(tr("Ctrl+Q", "Quit application"));
    act->setStatusTip(tr("Quit SuperCollider IDE"));
    QObject::connect( act, SIGNAL(triggered()), this, SLOT(onQuit()) );

    mActions[DocNew] = act = new QAction(
        QIcon::fromTheme("document-new"), tr("&New"), this);
    act->setShortcut(tr("Ctrl+N", "New document"));
    act->setStatusTip(tr("Create a new document"));
    connect(act, SIGNAL(triggered()), this, SLOT(newDocument()));

    mActions[DocOpen] = act = new QAction(
        QIcon::fromTheme("document-open"), tr("&Open..."), this);
    act->setShortcut(tr("Ctrl+O", "Open document"));
    act->setStatusTip(tr("Open an existing file"));
    connect(act, SIGNAL(triggered()), this, SLOT(openDocument()));

    mActions[DocSave] = act = new QAction(
        QIcon::fromTheme("document-save"), tr("&Save"), this);
    act->setShortcut(tr("Ctrl+S", "Save document"));
    act->setStatusTip(tr("Save the current document"));
    connect(act, SIGNAL(triggered()), this, SLOT(saveDocument()));

    mActions[DocSaveAs] = act = new QAction(
        QIcon::fromTheme("document-save-as"), tr("Save &As..."), this);
    act->setStatusTip(tr("Save the current document into a different file"));
    connect(act, SIGNAL(triggered()), this, SLOT(saveDocumentAs()));

    mActions[DocClose] = act = new QAction(
        QIcon::fromTheme("window-close"), tr("&Close"), this);
    act->setShortcut(tr("Ctrl+W", "Close document"));
    act->setStatusTip(tr("Close the current document"));
    connect(act, SIGNAL(triggered()), this, SLOT(closeDocument()));

    mActions[DocReload] = act = new QAction(
        QIcon::fromTheme("view-refresh"), tr("&Reload"), this);
    act->setShortcut(tr("F5", "Reload document"));
    act->setStatusTip(tr("Reload the current document"));
    connect(act, SIGNAL(triggered()), this, SLOT(reloadDocument()));

    mActions[ClearRecentDocs] = act = new QAction(tr("Clear", "Clear recent documents"), this);
    connect(act, SIGNAL(triggered()),
            Main::instance()->documentManager(), SLOT(clearRecents()));

    // Edit

    mActions[Find] = act = new QAction(
        QIcon::fromTheme("edit-find"), tr("&Find..."), this);
    act->setShortcut(tr("Ctrl+F", "Find"));
    act->setStatusTip(tr("Find text in document"));
    connect(act, SIGNAL(triggered()), this, SLOT(showFindTool()));

    mActions[Replace] = act = new QAction(
        QIcon::fromTheme("edit-replace"), tr("&Replace..."), this);
    act->setShortcut(tr("Ctrl+R", "Replace"));
    act->setStatusTip(tr("Find and replace text in document"));
    connect(act, SIGNAL(triggered()), this, SLOT(showReplaceTool()));

    // View

    mActions[ShowDocList] = act = new QAction(tr("&Documents"), this);
    act->setStatusTip(tr("Show/Hide the Documents dock"));
    act->setCheckable(true);
    connect(act, SIGNAL(triggered(bool)), mDocListDock, SLOT(setVisible(bool)));
    connect(mDocListDock, SIGNAL(visibilityChanged(bool)), act, SLOT(setChecked(bool)));

    mActions[ShowCmdLine] = act = new QAction(tr("&Command Line"), this);
    act->setStatusTip(tr("Command line for quick code evaluation"));
    act->setShortcut(tr("Ctrl+E", "Show command line"));
    connect(act, SIGNAL(triggered()), this, SLOT(showCmdLine()));

    mActions[CloseToolBox] = act = new QAction(
        QIcon::fromTheme("window-close"), tr("&Close Tool Panel"), this);
    act->setStatusTip(tr("Close any open tool panel"));
    act->setShortcut(tr("Esc", "Close tool box"));
    connect(act, SIGNAL(triggered()), this, SLOT(hideToolBox()));

    // Language

    mActions[EvaluateCurrentFile] = act = new QAction(
        QIcon::fromTheme("media-playback-start"), tr("Evaluate &File"), this);
    act->setStatusTip(tr("Evaluate current File"));
    connect(act, SIGNAL(triggered()), this, SLOT(evaluateCurrentFile()));

    mActions[EvaluateRegion] = act = new QAction(
    QIcon::fromTheme("media-playback-start"), tr("&Evaluate Region"), this);
    act->setShortcut(tr("Ctrl+Return", "Evaluate region"));
    act->setStatusTip(tr("Evaluate current region"));
    connect(act, SIGNAL(triggered()), this, SLOT(evaluateRegion()));

    // Settings

    mActions[ShowSettings] = act = new QAction(tr("&Configure IDE..."), this);
    act->setStatusTip(tr("Show configuration dialog"));
    connect(act, SIGNAL(triggered()), this, SLOT(showSettings()));

    // Help

    mActions[BrowseHelp] = act = new QAction(
    QIcon::fromTheme("system-help"), tr("&Browse Help"), this);
    act->setStatusTip(tr("Open help browser on the Browse page."));
    //connect(act, SIGNAL(triggered()), this, SLOT(browseHelp()));
    mCodeEvalMapper.setMapping(act, "HelpBrowser.openBrowsePage");
    connect(act, SIGNAL(triggered()), &mCodeEvalMapper, SLOT(map()));

    mActions[SearchHelp] = act = new QAction(
    QIcon::fromTheme("system-help"), tr("&Search Help"), this);
    act->setStatusTip(tr("Open help browser on the Search page."));
    //connect(act, SIGNAL(triggered()), this, SLOT(searchHelp()));
    mCodeEvalMapper.setMapping(act, "HelpBrowser.openSearchPage");
    connect(act, SIGNAL(triggered()), &mCodeEvalMapper, SLOT(map()));

    mActions[HelpForSelection] = act = new QAction(
    QIcon::fromTheme("system-help"), tr("&Help for Selection"), this);
    act->setShortcut(tr("Ctrl+H", "Help for selection"));
    act->setStatusTip(tr("Find help for selected text"));
    connect(act, SIGNAL(triggered()), this, SLOT(helpForSelection()));

    s->endGroup(); // IDE/shortcuts;

    // Add actions to settings
    for (int i = 0; i < ActionCount; ++i)
        s->addAction( mActions[i] );
}

void MainWindow::createMenus()
{
    QMenu *menu;
    QMenu *submenu;

    menu = new QMenu(tr("&File"), this);
    menu->addAction( mActions[DocNew] );
    menu->addAction( mActions[DocOpen] );
    mRecentDocsMenu = menu->addMenu(tr("Open Recent", "Open a recent document"));
    connect(mRecentDocsMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(onRecentDocAction(QAction*)));
    menu->addAction( mActions[DocSave] );
    menu->addAction( mActions[DocSaveAs] );
    menu->addSeparator();
    menu->addAction( mActions[DocReload] );
    menu->addSeparator();
    menu->addAction( mActions[DocClose] );
    menu->addSeparator();
    menu->addAction( mActions[Quit] );

    menuBar()->addMenu(menu);

    menu = new QMenu(tr("&Edit"), this);
    menu->addAction( mEditors->action(MultiEditor::Undo) );
    menu->addAction( mEditors->action(MultiEditor::Redo) );
    menu->addSeparator();
    menu->addAction( mEditors->action(MultiEditor::Cut) );
    menu->addAction( mEditors->action(MultiEditor::Copy) );
    menu->addAction( mEditors->action(MultiEditor::Paste) );
    menu->addSeparator();
    menu->addAction( mActions[Find] );
    menu->addAction( mActions[Replace] );
    menu->addSeparator();
    menu->addAction( mEditors->action(MultiEditor::IndentMore) );
    menu->addAction( mEditors->action(MultiEditor::IndentLess) );
    menu->addSeparator();
    menu->addAction( mEditors->action(MultiEditor::OpenClassDefinition) );

    menuBar()->addMenu(menu);

    menu = new QMenu(tr("&View"), this);
    submenu = new QMenu(tr("&Docks"), this);
    submenu->addAction( mActions[ShowDocList] );
    menu->addMenu(submenu);
    menu->addSeparator();
    submenu = menu->addMenu(tr("&Tool Panels"));
    submenu->addAction( mActions[Find] );
    submenu->addAction( mActions[Replace] );
    submenu->addAction( mActions[ShowCmdLine] );
    submenu->addSeparator();
    submenu->addAction( mActions[CloseToolBox] );
    menu->addSeparator();
    menu->addAction( mEditors->action(MultiEditor::EnlargeFont) );
    menu->addAction( mEditors->action(MultiEditor::ShrinkFont) );
    menu->addSeparator();
    menu->addAction( mEditors->action(MultiEditor::ShowWhitespace) );

    menuBar()->addMenu(menu);

    menu = new QMenu(tr("&Language"), this);
    menu->addAction( mMain->scProcess()->action(SCProcess::StartSCLang) );
    menu->addAction( mMain->scProcess()->action(SCProcess::RecompileClassLibrary) );
    menu->addAction( mMain->scProcess()->action(SCProcess::StopSCLang) );
    menu->addSeparator();
    menu->addAction( mActions[EvaluateCurrentFile] );
    menu->addAction( mActions[EvaluateRegion] );
    menu->addSeparator();
    menu->addAction( mMain->scProcess()->action(ScIDE::SCProcess::RunMain) );
    menu->addAction( mMain->scProcess()->action(ScIDE::SCProcess::StopMain) );

    menuBar()->addMenu(menu);

    menu = new QMenu(tr("&Settings"), this);
    menu->addAction( mActions[ShowSettings] );

    menuBar()->addMenu(menu);

    menu = new QMenu(tr("&Help"), this);
    menu->addAction( mActions[BrowseHelp] );
    menu->addAction( mActions[SearchHelp] );
    menu->addAction( mActions[HelpForSelection] );

    menuBar()->addMenu(menu);
}

QAction *MainWindow::action( ActionRole role )
{
    Q_ASSERT( role < ActionCount );
    return mActions[role];
}

bool MainWindow::quit()
{
    bool ok = true;

    QList<Document*> docs = mMain->documentManager()->documents();
    QList<Document*> unsavedDocs;
    foreach(Document* doc, docs)
        if(doc->textDocument()->isModified())
            unsavedDocs.append(doc);

    if (unsavedDocs.count())
    {
        DocumentsDialog dialog(unsavedDocs, DocumentsDialog::Quit, this);

        if (!dialog.exec())
            return false;
    }

    mMain->storeSettings();

    QApplication::quit();

    return true;
}

void MainWindow::onQuit()
{
    quit();
}

void MainWindow::onCurrentDocumentChanged( Document * doc )
{
    mActions[DocSave]->setEnabled(doc);
    mActions[DocSaveAs]->setEnabled(doc);
    mActions[DocClose]->setEnabled(doc);

    mFindReplaceTool->setEditor( mEditors->currentEditor() );
}

void MainWindow::onDocumentChangedExternally( Document *doc )
{
    if (mDocDialog)
        return;

    mDocDialog = new DocumentsDialog(DocumentsDialog::ExternalChange, this);
    mDocDialog->addDocument(doc);
    connect(mDocDialog, SIGNAL(finished(int)), this, SLOT(onDocDialogFinished()));
    mDocDialog->open();
}

void MainWindow::onDocDialogFinished()
{
    mDocDialog->deleteLater();
    mDocDialog = 0;
}

void MainWindow::updateRecentDocsMenu()
{
    mRecentDocsMenu->clear();

    const QStringList &recent = mMain->documentManager()->recents();

    foreach( const QString & path, recent )
        QAction *action = mRecentDocsMenu->addAction(path);

    if (!recent.isEmpty()) {
        mRecentDocsMenu->addSeparator();
        mRecentDocsMenu->addAction(mActions[ClearRecentDocs]);
    }
}

void MainWindow::onRecentDocAction( QAction *action )
{
    mMain->documentManager()->open(action->text());
}

void MainWindow::onInterpreterStateChanged( QProcess::ProcessState state )
{
    QString text;
    QColor color;

    switch(state) {
    case QProcess::NotRunning:
        text = "Inactive";
        color = Qt::white;
        break;
    case QProcess::Starting:
        text = "Booting";
        color = QColor(255,255,0);
        break;
    case QProcess::Running:
        text = "Active";
        color = Qt::green;
        break;
    }

    mLangStatus->setText(text);
    mLangStatus->setTextColor(color);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(!quit()) event->ignore();
}

bool MainWindow::close( Document *doc )
{
    if (doc->textDocument()->isModified())
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(
            mInstance,
            tr("SuperCollider IDE"),
            tr("There are unsaved changes in document '%1'.\n\n"
                "Do you want to save it?").arg(doc->title()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save // the default
        );
        switch (ret)
        {
        case QMessageBox::Cancel:
            return false;
        case QMessageBox::Save:
            if (!MainWindow::save(doc))
                return false;
            break;
        }
    }

    Main::instance()->documentManager()->close(doc);
    return true;
}

bool MainWindow::reload( Document *doc )
{
    if (doc->filePath().isEmpty())
        return false;

    if (doc->textDocument()->isModified())
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(
            mInstance,
            tr("SuperCollider IDE"),
            tr("There are unsaved changes in document '%1'.\n\n"
                "Do you want to reload it?").arg(doc->title()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No // the default
        );
        if (ret == QMessageBox::No)
            return false;
    }

    return Main::instance()->documentManager()->reload(doc);
}

bool MainWindow::save( Document *doc, bool forceChoose )
{
    DocumentManager *mng = Main::instance()->documentManager();
    if (forceChoose || doc->filePath().isEmpty())
    {
        QFileDialog dialog(mInstance);

        dialog.setAcceptMode( QFileDialog::AcceptSave );

        QStringList filters;
        filters
            << "SuperCollider(*.scd *.sc)"
            << "SCDoc(*.schelp)"
            << "All files(*)";
        dialog.setNameFilters(filters);

        dialog.setDefaultSuffix("scd");

        if (dialog.exec() == QDialog::Accepted)
            return mng->saveAs(doc, dialog.selectedFiles()[0]);
        else
            return false;
    }
    else
        return mng->save(doc);
}

void MainWindow::newDocument()
{
    mMain->documentManager()->create();
}

void MainWindow::openDocument()
{
    QFileDialog dialog (this, Qt::Dialog);
    dialog.setModal(true);
    dialog.setWindowModality(Qt::ApplicationModal);

    dialog.setFileMode( QFileDialog::ExistingFiles );

    QStringList filters;
    filters
        << "All files(*)"
        << "SuperCollider(*.scd *.sc)"
        << "SCDoc(*.schelp)";
    dialog.setNameFilters(filters);

    if (dialog.exec())
    {
        QStringList filenames = dialog.selectedFiles();
        foreach(QString filename, filenames)
            mMain->documentManager()->open(filename);
    }
}

void MainWindow::saveDocument()
{
    CodeEditor *editor = mEditors->currentEditor();
    if(!editor) return;

    Document *doc = editor->document();
    Q_ASSERT(doc);

    MainWindow::save(doc);
}

void MainWindow::saveDocumentAs()
{
    CodeEditor *editor = mEditors->currentEditor();
    if(!editor) return;

    Document *doc = editor->document();
    Q_ASSERT(doc);

    MainWindow::save(doc, true);
}

void MainWindow::reloadDocument()
{
    CodeEditor *editor = mEditors->currentEditor();
    if(!editor) return;

    Q_ASSERT(editor->document());
    MainWindow::reload(editor->document());
}

void MainWindow::closeDocument()
{
    CodeEditor *editor = mEditors->currentEditor();
    if(!editor) return;

    Q_ASSERT(editor->document());
    MainWindow::close( editor->document() );
}

void MainWindow::showCmdLine()
{
    mToolBox->setCurrentWidget( mCmdLine );
    mToolBox->show();

    mCmdLine->setFocus(Qt::OtherFocusReason);
}

void MainWindow::showFindTool()
{
    mFindReplaceTool->setMode( TextFindReplacePanel::Find );
    mFindReplaceTool->initiate();

    mToolBox->setCurrentWidget( mFindReplaceTool );
    mToolBox->show();

    mFindReplaceTool->setFocus(Qt::OtherFocusReason);
}

void MainWindow::showReplaceTool()
{
    mFindReplaceTool->setMode( TextFindReplacePanel::Replace );
    mFindReplaceTool->initiate();

    mToolBox->setCurrentWidget( mFindReplaceTool );
    mToolBox->show();

    mFindReplaceTool->setFocus(Qt::OtherFocusReason);
}

void MainWindow::hideToolBox()
{
    mToolBox->hide();
    CodeEditor *editor = mEditors->currentEditor();
    if (editor) {
        // This slot is mapped to Escape, so also clear highlighting
        // whenever invoked:
        editor->clearSearchHighlighting();
        if (!editor->hasFocus())
            editor->setFocus(Qt::OtherFocusReason);
    }
}

void MainWindow::toggleComandLineFocus()
{
    QWidget *cmd = cmdLine();
    if(cmd->hasFocus()) {
        QWidget *editor = mEditors->currentEditor();
        if(editor) editor->setFocus(Qt::OtherFocusReason);
    }
    else
        cmd->setFocus(Qt::OtherFocusReason);
}

void MainWindow::showSettings()
{
    Settings::Dialog dialog(mMain->settings());
    dialog.resize(700,400);
    int result = dialog.exec();
    if( result == QDialog::Accepted )
        mMain->applySettings();
}

QWidget *MainWindow::cmdLine()
{
    static QWidget *widget = 0;
    if(!widget) {
        CmdLine *w = new CmdLine("Command line:");
        connect(w, SIGNAL(invoked(QString, bool)),
                mMain->scProcess(), SLOT(evaluateCode(QString, bool)));
        widget = w;
    }
    return widget;
}

void MainWindow::evaluateRegion()
{
    CodeEditor *editor = mEditors->currentEditor();
    if (!editor)
        return;

    QString text;

    // Try current selection
    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection())
        text = cursor.selectedText();
    else
    {
        // If no selection, try current region
        cursor = editor->currentRegion();
        if (!cursor.isNull())
            text = cursor.selectedText();
        else
        {
            //If no current region, try current line
            cursor = editor->textCursor();
            text = cursor.block().text();
            if( mEditors->stepForwardEvaluation() ) {
                QTextCursor newCursor = cursor;
                newCursor.movePosition(QTextCursor::NextBlock);
                newCursor.movePosition(QTextCursor::EndOfBlock);
                editor->setTextCursor(newCursor);
            }
            // Adjust cursor for code blinking:
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        }
    }

    if (text.isEmpty())
        return;

    text.replace( QChar( 0x2029 ), QChar( '\n' ) );

    Main::instance()->scProcess()->evaluateCode(text);

    editor->blinkCode(cursor);
}

void MainWindow::evaluateCurrentFile()
{
    CodeEditor *editor = mEditors->currentEditor();
    if (!editor)
        return;

    QString documentText = editor->document()->textDocument()->toPlainText();
    Main::instance()->scProcess()->evaluateCode(documentText);
}

void MainWindow::helpForSelection()
{
    CodeEditor *editor = mEditors->currentEditor();
    if (editor)
    {
        QString code = QString("\"%1\".help").arg(editor->textCursor().selectedText());
        Main::instance()->scProcess()->evaluateCode(code, true);
    }
}

//////////////////////////// StatusLabel /////////////////////////////////

StatusLabel::StatusLabel(QWidget *parent) : QLabel(parent)
{
    setAutoFillBackground(true);
    setMargin(3);
    setAlignment(Qt::AlignCenter);
    setBackground(Qt::black);
    setTextColor(Qt::white);
}

void StatusLabel::setBackground(const QBrush & brush)
{
    QPalette plt(palette());
    plt.setBrush(QPalette::Window, brush);
    setPalette(plt);
}

void StatusLabel::setTextColor(const QColor & color)
{
    QPalette plt(palette());
    plt.setColor(QPalette::WindowText, color);
    setPalette(plt);
}

} // namespace ScIDE
