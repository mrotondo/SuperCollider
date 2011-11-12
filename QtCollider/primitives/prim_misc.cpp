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

#include "primitives.h"

#include "../Common.h"
#include "../Slot.h"
#include "../QcApplication.h"
#include "../QObjectProxy.h"
#include "../style/ProxyStyle.hpp"
#include "QtCollider.h"

#include <PyrKernel.h>

#include <QFontMetrics>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QWebSettings>

using namespace QtCollider;

QC_LANG_PRIMITIVE( QtGUI_SetDebugLevel, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  QtCollider::setDebugLevel( Slot::toInt(a) );
  return errNone;
}

QC_LANG_PRIMITIVE( QtGUI_DebugLevel, 0, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  SetInt( r, QtCollider::debugLevel() );
  return errNone;
}

QC_LANG_PRIMITIVE( QWindow_ScreenBounds, 1, PyrSlot *r, PyrSlot *rectSlot, VMGlobals *g )
{
  if( !QcApplication::compareThread() ) return QtCollider::wrongThreadError();

  QRect screenGeometry = QApplication::desktop()->screenGeometry();

  int err = Slot::setRect( rectSlot, screenGeometry );
  if( err ) return err;

  slotCopy( r, rectSlot );

  return errNone;
}

QC_LANG_PRIMITIVE( QWindow_AvailableGeometry, 1, PyrSlot *r, PyrSlot *rectSlot, VMGlobals *g )
{
  if( !QcApplication::compareThread() ) return QtCollider::wrongThreadError();

  QRect rect = QApplication::desktop()->availableGeometry();

  int err = Slot::setRect( rectSlot, rect );
  if( err ) return err;

  slotCopy( r, rectSlot );

  return errNone;
}

QC_LANG_PRIMITIVE( Qt_StringBounds, 3, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  QString str = Slot::toString( a );

  QFont f = Slot::toFont( a+1 );

  QFontMetrics fm( f );
  QRect bounds = fm.boundingRect( str );

  // we keep the font height even on empty string;
  if( str.isEmpty() ) bounds.setHeight( fm.height() );

  Slot::setRect( a+2, bounds );
  slotCopy( r, a+2 );

  return errNone;
}

QC_LANG_PRIMITIVE( Qt_AvailableFonts, 0, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  QFontDatabase database;
  VariantList l;
  Q_FOREACH( QString family, database.families() ) {
    l.data << QVariant(family);
  }
  Slot::setVariantList( r, l );
  return errNone;
}

QC_LANG_PRIMITIVE( Qt_GlobalPalette, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  if( !QcApplication::compareThread() ) return QtCollider::wrongThreadError();

  QPalette p( QApplication::palette() );

  if( Slot::setPalette( a, p ) ) return errFailed;
  slotCopy( r, a );

  return errNone;
}

QC_LANG_PRIMITIVE( Qt_SetGlobalPalette, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  if( !QcApplication::compareThread() ) return QtCollider::wrongThreadError();

  QPalette p = Slot::toPalette( a );
  QApplication::setPalette( p );

  return errNone;
}

QC_LANG_PRIMITIVE( Qt_FocusWidget, 0,  PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  if( !QcApplication::compareThread() ) return QtCollider::wrongThreadError();

  QWidget *w = QApplication::focusWidget();

  if( w ) {
    QObjectProxy *proxy = QObjectProxy::fromObject(w);
    if( proxy && proxy->scObject() ) {
      SetObject( r, proxy->scObject() );
      return errNone;
    }
  }

  SetNil(r);
  return errNone;
}

QC_LANG_PRIMITIVE( Qt_SetStyle, 1, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  if( !QcApplication::compareThread() ) return QtCollider::wrongThreadError();

  QString str = Slot::toString( a );
  if( str.isEmpty() ) return errFailed;

  QStyle *style = QStyleFactory::create( str );
  if( !style ) return errFailed;

  QApplication::setStyle( new QtCollider::ProxyStyle( style ) );
  return errNone;
}

QC_LANG_PRIMITIVE( Qt_AvailableStyles, 0, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  if( !QcApplication::compareThread() ) return QtCollider::wrongThreadError();

  VariantList list;
  Q_FOREACH( QString key, QStyleFactory::keys() ) {
    list.data << QVariant(key);
  }

  Slot::setVariantList( r, list );
  return errNone;
}

QC_LANG_PRIMITIVE( QWebView_ClearMemoryCaches, 0, PyrSlot *r, PyrSlot *a, VMGlobals *g )
{
  if( !QcApplication::compareThread() ) return QtCollider::wrongThreadError();

  QWebSettings::clearMemoryCaches();

  return errNone;
}
