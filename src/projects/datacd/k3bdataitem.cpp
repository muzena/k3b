/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bdatadoc.h"
#include <kdebug.h>


K3bDataItem::K3bDataItem( K3bDataDoc* doc, K3bDataItem* parent )
  : m_bHideOnRockRidge(true),
    m_bHideOnJoliet(true),
    m_bRemoveable(true),
    m_bRenameable(true),
    m_bMovable(true),
    m_bHideable(true),
    m_bWriteToCd(true),
    m_sortWeigth(0)
{
  m_doc = doc;
  m_bHideOnRockRidge = m_bHideOnJoliet = false;

  if( parent )
    m_parentDir = parent->getDirItem();
  else
    m_parentDir = 0;
}

K3bDataItem::~K3bDataItem()
{
}


void K3bDataItem::setK3bName( const QString& name ) {
  // test for not-allowed characters
  // TODO: use QRegExp
  if( name.contains('/') || name.contains('?') || name.contains('*') ) {
    kdDebug() << "(K3bDataItem) name contained invalid characters!" << endl;
    return;
  }
//   if( parent() ) {
//     QPtrList<K3bDataItem>* _itemsInDir = parent()->children();
//     for( K3bDataItem* _it = _itemsInDir->first(); _it; _it = _itemsInDir->next() ) {
//       if( _it != this && _it->k3bName() == name ) {
// 	kdDebug() << "(K3bDataItem) already a file with that name in directory: " << _it->k3bName() << endl;
// 	return;
//       }
//     }
//   }

  m_k3bName = name;
  if( parent() )
    parent()->revalidate();
}


void K3bDataItem::setJolietName( const QString& name )
{
  m_jolietName = name;
}


const QString& K3bDataItem::k3bName()
{
  return m_k3bName;
}


const QString& K3bDataItem::jolietName()
{
  return m_jolietName;
}


QString K3bDataItem::k3bPath()
{
  if( !m_parentDir )
    return k3bName();
  else
    return m_parentDir->k3bPath() + k3bName();
}


QString K3bDataItem::jolietPath()
{
  if( !m_parentDir )
    return jolietName();
  else
    return m_parentDir->jolietPath() + jolietName();
}


K3bDataItem* K3bDataItem::nextSibling()
{
  K3bDataItem* _item = this;
  K3bDirItem* _parentItem = parent();
	
  while( _parentItem ) {
    if( K3bDataItem* i = _parentItem->nextChild( _item ) )
      return i;
		
    _item = _parentItem;
    _parentItem = _item->parent();
  }

  return 0;
}


void K3bDataItem::reparent( K3bDirItem* newParent )
{
  if( m_parentDir ) {
    m_parentDir->takeDataItem( this );
  }

  m_parentDir = newParent->addDataItem( this );
}


bool K3bDataItem::hideOnRockRidge() const
{ 
  if( !isHideable() )
    return false;
  if( parent() )
    return m_bHideOnRockRidge || parent()->hideOnRockRidge();
  else 
    return m_bHideOnRockRidge;
}


bool K3bDataItem::hideOnJoliet() const
{
  if( !isHideable() )
    return false;
  if( parent() ) 
    return m_bHideOnJoliet || parent()->hideOnJoliet();
  else
    return m_bHideOnJoliet;
}


void K3bDataItem::setHideOnRockRidge( bool b )
{
  // there is no use in changing the value if 
  // it is already set by the parent
  if( !parent() || !parent()->hideOnRockRidge() )
    m_bHideOnRockRidge = b;
}


void K3bDataItem::setHideOnJoliet( bool b ) 
{ 
  // there is no use in changing the value if 
  // it is already set by the parent
  if( !parent() || !parent()->hideOnJoliet() )
    m_bHideOnJoliet = b;
}


int K3bDataItem::depth() const
{
  if( parent() )
    return parent()->depth() + 1;
  else
    return 0;
}
