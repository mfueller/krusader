/***************************************************************************
                       diskusagegui.cpp  -  description
                             -------------------
    copyright            : (C) 2004 by Csaba Karai
    e-mail               : krusader@users.sourceforge.net
    web site             : http://krusader.sourceforge.net
 ---------------------------------------------------------------------------
  Description
 ***************************************************************************

  A

     db   dD d8888b. db    db .d8888.  .d8b.  d8888b. d88888b d8888b.
     88 ,8P' 88  `8D 88    88 88'  YP d8' `8b 88  `8D 88'     88  `8D
     88,8P   88oobY' 88    88 `8bo.   88ooo88 88   88 88ooooo 88oobY'
     88`8b   88`8b   88    88   `Y8b. 88~~~88 88   88 88~~~~~ 88`8b
     88 `88. 88 `88. 88b  d88 db   8D 88   88 88  .8D 88.     88 `88.
     YP   YD 88   YD ~Y8888P' `8888Y' YP   YP Y8888D' Y88888P 88   YD

                                                     S o u r c e    F i l e

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "diskusagegui.h"
#include "../kicons.h"
#include "../krusader.h"
#include "../VFS/vfs.h"
#include "../Dialogs/krdialogs.h"

#include <qtimer.h>
#include <qhbox.h>
#include <klocale.h>

DiskUsageGUI::DiskUsageGUI( QString openDir, QWidget* parent, char *name ) 
  : QDialog( parent, name, false, 0 )
{
  setCaption( i18n("Krusader::Disk Usage") );
  
  baseDirectory = vfs::fromPathOrURL( openDir );
  if( !newSearch() )
    return;
  
  QGridLayout *duGrid = new QGridLayout( this );
  duGrid->setSpacing( 6 );
  duGrid->setMargin( 11 );
  
  QHBox *duTools = new QHBox( this, "duTools" );
  duTools->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    
  QToolButton * btnNewSearch = new QToolButton( duTools, "btnNewSearch" );
  btnNewSearch->setIconSet( QIconSet(krLoader->loadIcon("fileopen",KIcon::Desktop)) );
  
  QToolButton * btnRefresh = new QToolButton( duTools, "btnRefresh" );
  btnRefresh->setIconSet( QIconSet(krLoader->loadIcon("reload",KIcon::Desktop)) );

  QToolButton * btnDirUp = new QToolButton( duTools, "btnDirUp" );
  btnDirUp->setIconSet( QIconSet(krLoader->loadIcon("up",KIcon::Desktop)) );
  
  QWidget * separatorWidget = new QWidget( duTools, "separatorWidget" );
  separatorWidget->setMinimumWidth( 10 );
  
  btnLines = new QToolButton( duTools, "btnLines" );
  btnLines->setIconSet( QIconSet(krLoader->loadIcon("leftjust",KIcon::Desktop)) );
  btnLines->setToggleButton( true );

  btnDetailed = new QToolButton( duTools, "btnDetailed" );
  btnDetailed->setIconSet( QIconSet(krLoader->loadIcon("view_detailed",KIcon::Desktop)) );
  btnDetailed->setToggleButton( true );

  btnFilelight = new QToolButton( duTools, "btnFilelight" );
  btnFilelight->setIconSet( QIconSet(krLoader->loadIcon("none",KIcon::Desktop)) );
  btnFilelight->setToggleButton( true );
    
  QWidget *spacerWidget = new QWidget( duTools, "spacerWidget" );
  QHBoxLayout *hboxlayout = new QHBoxLayout( spacerWidget );
  QSpacerItem* spacer = new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed );
  hboxlayout->addItem( spacer );
  
  duGrid->addWidget( duTools, 0, 0 );
  
  diskUsage = new DiskUsage( "DiskUsage", this );
  duGrid->addWidget( diskUsage, 1, 0 );
  
  status = new KSqueezedTextLabel( this );
  status->setFrameShape( QLabel::StyledPanel );
  status->setFrameShadow( QLabel::Sunken );  
  duGrid->addWidget( status, 2, 0 );
  
  connect( diskUsage, SIGNAL( status( QString ) ), this, SLOT( setStatus( QString ) ) );
  connect( diskUsage, SIGNAL( viewChanged( int ) ), this, SLOT( slotViewChanged( int ) ) );
  connect( btnNewSearch, SIGNAL( clicked() ), this, SLOT( newSearch() ) );
  connect( btnRefresh, SIGNAL( clicked() ), this, SLOT( loadUsageInfo() ) );
  connect( btnDirUp, SIGNAL( clicked() ), diskUsage, SLOT( dirUp() ) );
  connect( btnLines, SIGNAL( clicked() ), this, SLOT( selectLinesView() ) );
  connect( btnDetailed, SIGNAL( clicked() ), this, SLOT( selectListView() ) );
  connect( btnFilelight, SIGNAL( clicked() ), this, SLOT( selectFilelightView() ) );  
  
  krConfig->setGroup( "DiskUsage" ); 
  
  diskUsage->setView( krConfig->readNumEntry( "View",  VIEW_LINES ) );
  
  sizeX = krConfig->readNumEntry( "Window Width",  QFontMetrics(font()).width("W") * 70 );
  sizeY = krConfig->readNumEntry( "Window Height", QFontMetrics(font()).height() * 25 );    
  resize( sizeX, sizeY );
  
  if( krConfig->readBoolEntry( "Window Maximized",  false ) )
    showMaximized();
  else  
    show();
  
  exec();
}

DiskUsageGUI::~DiskUsageGUI()
{
}

void DiskUsageGUI::resizeEvent( QResizeEvent *e )
{   
  if( !isMaximized() )
  {
    sizeX = e->size().width();
    sizeY = e->size().height();
  }
  QDialog::resizeEvent( e );
}

void DiskUsageGUI::reject()
{
  krConfig->setGroup( "DiskUsage" ); 
  krConfig->writeEntry("Window Width", sizeX );
  krConfig->writeEntry("Window Height", sizeY );
  krConfig->writeEntry("Window Maximized", isMaximized() );
  krConfig->writeEntry("View", diskUsage->getActiveView() );
  
  QDialog::reject();
}

void DiskUsageGUI::loadUsageInfo()
{
  if( !diskUsage->load( baseDirectory, this ) )
    reject();
}

void DiskUsageGUI::setStatus( QString stat )
{
  status->setText( stat );
}

void DiskUsageGUI::slotViewChanged( int view )
{
  btnLines->setOn( false );
  btnDetailed->setOn( false );
  btnFilelight->setOn( false );
  
  switch( view )
  {
  case VIEW_LINES:
    btnLines->setOn( true );
    break;
  case VIEW_DETAILED:
    btnDetailed->setOn( true );
    break;
  case VIEW_FILELIGHT:
    btnFilelight->setOn( true );
    break;
  }
}

bool DiskUsageGUI::newSearch()
{
  
  // ask the user for the copy dest
  KChooseDir *chooser = new KChooseDir( 0, i18n( "Viewing the usage of directory:" ),
                                        baseDirectory.prettyURL(1,KURL::StripFileProtocol) );
  QString dest = chooser->dest;
  if ( dest == QString::null )
    return false; // the usr canceled    
    
  baseDirectory = vfs::fromPathOrURL( dest );
  
  QTimer::singleShot( 0, this, SLOT( loadUsageInfo() ) );
  return true;
}

#include "diskusagegui.moc"
