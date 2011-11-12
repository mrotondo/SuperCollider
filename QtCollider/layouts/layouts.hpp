/************************************************************************
*
* Copyright 2010 Jakob Leben (jakob.leben@gmail.com)
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

#ifndef QC_LAYOUTS_H
#define QC_LAYOUTS_H

#include "../QcObjectFactory.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

template<class BOXLAYOUT> class QcBoxLayoutFactory : public QcObjectFactory<BOXLAYOUT>
{
protected:
  virtual void initialize( QObjectProxy *, BOXLAYOUT *l, QList<QVariant> & items ) {
    Q_FOREACH( QVariant v, items ) {
      VariantList item = v.value<VariantList>();
      l->addItem( item );
    }
  }
};

template <class LAYOUT> struct QcLayout : public LAYOUT
{
public:
  VariantList margins() const { return VariantList(); }

  void setMargins( const VariantList &list ) {
    if( list.data.size() < 4 ) return;
    int m[4];
    for( int i=0; i<4; ++i ) {
      m[i] = list.data[i].value<int>();
    }
    LAYOUT::setContentsMargins( m[0], m[1], m[2], m[3] );
  }
};

template <class BOXLAYOUT> class QcBoxLayout : public QcLayout<BOXLAYOUT>
{
public:
  void addItem( const VariantList & item ) {
    if( item.data.size() < 3 ) return;

    int stretch = item.data[1].toInt();
    Qt::Alignment alignment = (Qt::Alignment) item.data[2].toInt();

    QVariant varObject = item.data[0];

    if( !varObject.isValid() ) {
      BOXLAYOUT::addStretch( stretch );
      return;
    }

    if( varObject.canConvert<int>() ) {
      int size = varObject.toInt();
      BOXLAYOUT::addSpacing( size );
      return;
    }

    QObjectProxy *p = varObject.value<QObjectProxy*>();
    if( !p || !p->object() ) return;

    QWidget *w = qobject_cast<QWidget*>( p->object() );
    if( w ) {
      BOXLAYOUT::addWidget( w, stretch, alignment );
      return;
    }

    QLayout *l2 = qobject_cast<QLayout*>( p->object() );
    if(l2) {
      BOXLAYOUT::addLayout( l2, stretch );
      return;
    }
  }

  void setStretch( QObjectProxy *p, int stretch ) {
    QWidget *w = qobject_cast<QWidget*>( p->object() );
    if( w ) {
      BOXLAYOUT::setStretchFactor( w, stretch );
      return;
    }

    QLayout *l = qobject_cast<QLayout*>( p->object() );
    if(l) {
      BOXLAYOUT::setStretchFactor( l, stretch );
      return;
    }
  }

  void setAlignment( QObjectProxy *p, Qt::Alignment alignment ) {
    QWidget *w = qobject_cast<QWidget*>( p->object() );
    if( w ) {
      BOXLAYOUT::setAlignment( w, alignment );
      return;
    }

    QLayout *l = qobject_cast<QLayout*>( p->object() );
    if(l) {
      BOXLAYOUT::setAlignment( l, alignment );
      return;
    }
  }
};

class QcHBoxLayout : public QcBoxLayout<QHBoxLayout>
{
  Q_OBJECT
  Q_PROPERTY( VariantList margins READ margins WRITE setMargins )
public:
  Q_INVOKABLE void addItem( const VariantList &data ) { QcBoxLayout<QHBoxLayout>::addItem(data); }
  Q_INVOKABLE void setStretch( int index, int stretch ) {
    QBoxLayout::setStretch( index, stretch );
  }
  Q_INVOKABLE void setStretch( QObjectProxy *p, int stretch ) {
    QcBoxLayout<QHBoxLayout>::setStretch( p, stretch );
  }
  Q_INVOKABLE void setAlignment( int i, int a ) {
    itemAt(i)->setAlignment( (Qt::Alignment) a );
    update();
  }
  Q_INVOKABLE void setAlignment( QObjectProxy *p, int a ) {
    QcBoxLayout<QHBoxLayout>::setAlignment( p, (Qt::Alignment) a );
  }
};

class QcVBoxLayout : public QcBoxLayout<QVBoxLayout>
{
  Q_OBJECT
  Q_PROPERTY( VariantList margins READ margins WRITE setMargins )
public:
  Q_INVOKABLE void addItem( const VariantList &data ) { QcBoxLayout<QVBoxLayout>::addItem(data); }
  Q_INVOKABLE void setStretch( int index, int stretch ) {
    QBoxLayout::setStretch( index, stretch );
  }
  Q_INVOKABLE void setStretch( QObjectProxy *p, int stretch ) {
    QcBoxLayout<QVBoxLayout>::setStretch( p, stretch );
  }
  Q_INVOKABLE void setAlignment( int i, int a ) {
    itemAt(i)->setAlignment( (Qt::Alignment) a );
    update();
  }
  Q_INVOKABLE void setAlignment( QObjectProxy *p, int a ) {
    QcBoxLayout<QVBoxLayout>::setAlignment( p, (Qt::Alignment) a );
  }
};

class QcGridLayout : public QcLayout<QGridLayout>
{
  Q_OBJECT
  Q_PROPERTY( VariantList margins READ margins WRITE setMargins )
  Q_PROPERTY( int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing )
  Q_PROPERTY( int horizontalSpacing READ horizontalSpacing WRITE setHorizontalSpacing )
public:
  Q_INVOKABLE void addItem( const VariantList &dataList );
  Q_INVOKABLE void setRowStretch( int row, int factor ) {
    QcLayout<QGridLayout>::setRowStretch( row, factor );
  }
  Q_INVOKABLE void setColumnStretch( int column, int factor ) {
    QcLayout<QGridLayout>::setColumnStretch( column, factor );
  }
  Q_INVOKABLE void setAlignment( int r, int c, int a ) {
    itemAtPosition(r,c)->setAlignment( (Qt::Alignment) a );
    update();
  }
  Q_INVOKABLE void setAlignment( QObjectProxy *p, int a ) {
    QWidget *w = qobject_cast<QWidget*>( p->object() );
    if( w ) {
      QLayout::setAlignment( w, (Qt::Alignment) a );
      return;
    }

    QLayout *l = qobject_cast<QLayout*>( p->object() );
    if(l) {
      QLayout::setAlignment( l, (Qt::Alignment) a );
      return;
    }
  }
  Q_INVOKABLE int minRowHeight( int row ) {
    return ( row >= 0 && row < rowCount() ) ? rowMinimumHeight( row ) : 0;
  }
  Q_INVOKABLE int minColumnWidth( int col ) {
    return ( col >= 0 && col < columnCount() ) ? columnMinimumWidth( col ) : 0;
  }
  Q_INVOKABLE void setMinRowHeight( int row, int h ) {
    setRowMinimumHeight( row, h );
  }
  Q_INVOKABLE void setMinColumnWidth( int col, int w ) {
    setColumnMinimumWidth( col, w );
  }
};

#endif
