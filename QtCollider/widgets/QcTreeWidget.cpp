/************************************************************************
*
* Copyright 2011 Jakob Leben (jakob.leben@gmail.com)
*
* This file is part of SuperCollider Qt GUI.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
************************************************************************/

#include "QcTreeWidget.h"
#include "../QcWidgetFactory.h"

#include <PyrKernel.h>

class QcTreeWidgetFactory : public QcWidgetFactory<QcTreeWidget>
{
  void initialize( QWidgetProxy *p, QcTreeWidget *w, QList<QVariant> & ) {
    p->setMouseEventWidget( w->viewport() );
  }
};

static QcTreeWidgetFactory treeWidgetFactory;

QcTreeWidget::QcTreeWidget()
: _itemOnPress(0), _emitAction(true)
{
  connect( this, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ),
           this, SLOT( onCurrentItemChanged() ) );
  viewport()->installEventFilter( this );
}

VariantList QcTreeWidget::columns() const
{
  VariantList varList;
  QTreeWidgetItem *header = headerItem();
  if( header ) {
    for( int i = 0; i < header->columnCount(); ++i ) {
      varList.data << header->text(i);
    }
  }
  return varList;
}

void QcTreeWidget::setColumns( const VariantList & varList )
{
  int count = varList.data.size();
  setColumnCount( count );
  QStringList labels;
  Q_FOREACH( QVariant var, varList.data )
    labels << var.toString();
  setHeaderLabels( labels );
}

QcTreeWidget::ItemPtr QcTreeWidget::currentItem() const
{
  QTreeWidgetItem *item = QTreeWidget::currentItem();
  return QcTreeWidget::Item::safePtr( item );
}

void QcTreeWidget::setCurrentItem( const ItemPtr &item )
{
  _emitAction = false;
  QTreeWidget::setCurrentItem( item );
  _emitAction = true;
}

QcTreeWidget::ItemPtr QcTreeWidget::item( const QcTreeWidget::ItemPtr & parent, int index  )
{
  QTreeWidgetItem *item =
    parent ? parent->child(index) : QTreeWidget::topLevelItem(index);

  return QcTreeWidget::Item::safePtr( item );
}

QcTreeWidget::ItemPtr QcTreeWidget::parentItem( const QcTreeWidget::ItemPtr & item )
{
  if( !item ) return QcTreeWidget::ItemPtr();
  return QcTreeWidget::Item::safePtr( item->parent() );
}

int QcTreeWidget::indexOfItem( const QcTreeWidget::ItemPtr & item )
{
  if( !item ) return -1;
  QTreeWidgetItem *parent = item->parent();
  if( parent )
    return parent->indexOfChild( item );
  else
    return indexOfTopLevelItem( item );
}

QcTreeWidget::ItemPtr QcTreeWidget::addItem (
  const QcTreeWidget::ItemPtr & parent, const VariantList & varList )
{
  QStringList strings;
  for( int i = 0; i < varList.data.count(); ++i )
    strings << varList.data[i].toString();

  Item *item = new Item( strings );
  if( !parent ) addTopLevelItem( item );
  else parent->addChild( item );

  return item->safePtr();
}

QcTreeWidget::ItemPtr QcTreeWidget::insertItem (
  const QcTreeWidget::ItemPtr & parent, int index, const VariantList & varList )
{
  int itemCount = topLevelItemCount();
  if( index < 0 || index > itemCount ) return ItemPtr();

  QStringList strings;
  for( int i = 0; i < varList.data.count(); ++i )
    strings << varList.data[i].toString();

  Item *item = new Item( strings );
  if( !parent ) insertTopLevelItem( index, item );
  else parent->insertChild( index, item );

  if( !item->treeWidget() ) {
    delete item;
    return ItemPtr();
  }

  return item->safePtr();
}

void QcTreeWidget::removeItem( const QcTreeWidget::ItemPtr & item )
{
  delete item.ptr();
}

VariantList QcTreeWidget::strings( const QcTreeWidget::ItemPtr & item )
{
  VariantList varList;
  if( !item ) return varList;
  for( int i = 0; i < item->columnCount(); ++i ) {
    varList.data << item->text(i);
  }
  return varList;
}

void QcTreeWidget::setText( const QcTreeWidget::ItemPtr &item, int column, const QString & text )
{
  if( item ) item->setText( column, text );
}

void QcTreeWidget::setColor( const QcTreeWidget::ItemPtr &item, int column, const QColor & color )
{
  if( item ) item->setBackground( column, color );
}

void QcTreeWidget::setTextColor( const QcTreeWidget::ItemPtr & item, int column, const QColor & color )
{
  if( item ) item->setData( column, Qt::ForegroundRole, color );
}

QWidget * QcTreeWidget::itemWidget( const QcTreeWidget::ItemPtr &item, int column )
{
  return item ? QTreeWidget::itemWidget( item, column ) : 0;
}

void QcTreeWidget::setItemWidget( const QcTreeWidget::ItemPtr &item, int column, QObjectProxy *o )
{
  if( !item ) return;

  QWidget *w = qobject_cast<QWidget*>(o->object());
  if( !w ) return;

  QTreeWidget::setItemWidget( item, column, w );
}

void QcTreeWidget::removeItemWidget( const QcTreeWidget::ItemPtr &item, int column )
{
  if( item ) QTreeWidget::removeItemWidget( item, column );
}

void QcTreeWidget::sort( int column, bool descending )
{
  sortItems( column, descending ? Qt::DescendingOrder : Qt::AscendingOrder );
}

void QcTreeWidget::onCurrentItemChanged()
{
  if( _emitAction ) Q_EMIT( action() );
}

bool QcTreeWidget::eventFilter( QObject *o, QEvent *e )
{
  if( o == viewport() ) {
    if( e->type() == QEvent::MouseButtonPress ) {
      _emitAction = false;
      _itemOnPress = QTreeWidget::currentItem();
    }
    else if( e->type() == QEvent::MouseButtonRelease ) {
      _emitAction = true;
      if( QTreeWidget::currentItem() != _itemOnPress ) Q_EMIT( action() );
    }
  }
  return QTreeWidget::eventFilter( o, e );
}

QcTreeWidget::ItemPtr QcTreeWidget::Item::safePtr( QTreeWidgetItem * item )
{
  if( item && item->type() == Item::Type )
    return static_cast<Item*>(item)->safePtr();
  else
    return ItemPtr();
}

void QcTreeWidget::Item::initialize (
  VMGlobals *g, PyrObject *obj, const QcTreeWidget::ItemPtr &ptr )
{
  Q_ASSERT( isKindOf( obj, class_QTreeViewItem ) );
  if( ptr.id() ) {
    // store the SafePtr
    QcTreeWidget::ItemPtr *newPtr = new QcTreeWidget::ItemPtr( ptr );
    SetPtr( obj->slots+0, newPtr );
    // store the id for equality comparison
    SetPtr( obj->slots+1, ptr.id() );
    // install finalizer
  }
  else {
    SetNil( obj->slots+0 );
    SetNil( obj->slots+1 );
  }
  InstallFinalizer( g, obj, 2, &QcTreeWidget::Item::finalize );
}

int QcTreeWidget::Item::finalize( VMGlobals *g, PyrObject *obj )
{
  qcDebugMsg(1,"finalizing QTreeViewItem!");
  if( IsPtr( obj->slots+0 ) ) {
    QcTreeWidget::ItemPtr *ptr = static_cast<QcTreeWidget::ItemPtr*>( slotRawPtr(obj->slots+0) );
    delete ptr;
  }
  return errNone;
}
