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

#ifndef SCIDE_WIDGETS_DOC_LIST_HPP_INCLUDED
#define SCIDE_WIDGETS_DOC_LIST_HPP_INCLUDED

#include "../core/doc_manager.hpp"

#include <QListWidget>
#include <QDockWidget>
#include <QSignalMapper>

namespace ScIDE {

class DocumentManager;
class Document;

class DocumentList : public QListWidget
{
    Q_OBJECT

public:

    DocumentList(DocumentManager *, QWidget * parent = 0);

public Q_SLOTS:

    void setCurrent( Document * );

Q_SIGNALS:

    void clicked( Document * );

private Q_SLOTS:

    void onOpen( Document *, int );
    void onClose( Document * );
    void onSaved( Document * );
    void onModificationChanged(QObject*);
    void onItemClicked(QListWidgetItem*);

protected:

    virtual QSize sizeHint() const { return QSize(170,170); }

private:
    struct Item : public QListWidgetItem
    {
        Item( Document * doc, QListWidget * parent = 0 ):
            QListWidgetItem(parent, QListWidgetItem::UserType),
            mDoc(doc)
        {
            setText(doc->title());
        }

        Document *mDoc;
    };

    Item *addItemFor( Document * );
    Item *itemFor( Document * );
    Item *itemFor( QListWidgetItem * );
    QSignalMapper mModificationMapper;
    QIcon mDocModifiedIcon;
};

class DocumentsDock : public QDockWidget
{
public:
    DocumentsDock(DocumentManager *manager, QWidget* parent = 0):
        QDockWidget(tr("Documents"), parent),
        mDocList(new DocumentList(manager))
    {
        setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        setFeatures(DockWidgetFloatable | DockWidgetMovable | DockWidgetClosable);
        setWidget(mDocList);
    }

    DocumentList *list() { return mDocList; }

private:

    DocumentList *mDocList;
};

} // namespace ScIDE

#endif // SCIDE_WIDGETS_DOC_LIST_HPP_INCLUDED
