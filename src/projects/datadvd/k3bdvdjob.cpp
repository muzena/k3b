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


#include "k3bdvdjob.h"
#include "k3bdvddoc.h"
#include "k3bgrowisofsimager.h"

#include <k3bdvdrecordwriter.h>
#include <k3bisoimager.h>
#include <k3bgrowisofswriter.h>
#include <k3bemptydiscwaiter.h>
#include <k3bglobals.h>
#include <k3bemptydiscwaiter.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicehandler.h>
#include <device/k3bdiskinfo.h>
#include <device/k3bdeviceglobals.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kapplication.h>


K3bDvdJob::K3bDvdJob( K3bDataDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_doc( doc ),
    m_isoImager( 0 ),
    m_growisofsImager( 0 ),
    m_writerJob( 0 )
{
}


K3bDvdJob::~K3bDvdJob()
{
}


K3bDoc* K3bDvdJob::doc() const
{
  return m_doc;
}


K3bCdDevice::CdDevice* K3bDvdJob::writer() const
{
  return m_doc->burner();
}


void K3bDvdJob::start()
{
  //
  // When writing multisession the design of growisofs does not allow creating an image
  // before writing.
  // In this case we use the K3bGrowisofsWriter, in all other cases the K3bGrowisofsImager.
  // 

  emit started();

  m_canceled = false;
  m_writingStarted = false;

  if( !m_doc->onTheFly() || m_doc->onlyCreateImages() ) {
    emit newTask( i18n("Writing data") );
    writeImage();
  }
  else {
    prepareGrowisofsImager();
    
    if( waitForDvd() )
      m_growisofsImager->start();
    else
      emit finished(false);
  }
}


void K3bDvdJob::writeImage()
{
  //
  // disable all multisession since we do only support multisession in on-the-fly mode
  //
  m_doc->setMultiSessionMode( K3bDataDoc::NONE );
  prepareIsoImager();

  // get image file path
  if( m_doc->tempDir().isEmpty() )
    m_doc->setTempDir( K3b::findUniqueFilePrefix( m_doc->isoOptions().volumeID() ) + ".iso" );
    
  // open the file for writing
  m_imageFile.setName( m_doc->tempDir() );
  if( !m_imageFile.open( IO_WriteOnly ) ) {
    emit infoMessage( i18n("Could not open %1 for writing").arg(m_doc->tempDir()), ERROR );
    cleanup();
    emit finished(false);
    return;
  }

  emit infoMessage( i18n("Writing image file to %1").arg(m_doc->tempDir()), INFO );
  emit newSubTask( i18n("Creating image file") );

  m_imageFileStream.setDevice( &m_imageFile );
  m_isoImager->start();
}


void K3bDvdJob::prepareIsoImager()
{
  if( !m_isoImager ) {
    m_isoImager = new K3bIsoImager( m_doc, this );
    connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), 
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_isoImager, SIGNAL(data(const char*, int)), 
	     this, SLOT(slotReceivedIsoImagerData(const char*, int)) );
    connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
    connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
    connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }
}


void K3bDvdJob::prepareGrowisofsImager()
{
  if( !m_growisofsImager ) {
    m_growisofsImager = new K3bGrowisofsImager( m_doc, this );
    connect( m_growisofsImager, SIGNAL(infoMessage(const QString&, int)), 
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_growisofsImager, SIGNAL(percent(int)), this, SLOT(slotGrowisofsImagerPercent(int)) );
    connect( m_growisofsImager, SIGNAL(finished(bool)), this, SLOT(slotWritingFinished(bool)) );
    connect( m_growisofsImager, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( m_growisofsImager, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_growisofsImager, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }
}


void K3bDvdJob::slotReceivedIsoImagerData( const char* data, int len )
{
  m_imageFileStream.writeRawBytes( data, len );
  m_isoImager->resume();
}


void K3bDvdJob::slotIsoImagerPercent( int p )
{
  if( m_doc->onlyCreateImages() ) {
    emit percent( p  );
    emit subPercent( p );
  }
  else {
    emit subPercent( p );
    emit percent( p/2 );
  }
}


void K3bDvdJob::slotGrowisofsImagerPercent( int p )
{
  emit subPercent( p );
  emit percent( p );
  if( !m_writingStarted ) {
    m_writingStarted = true;
    emit newSubTask( i18n("Writing data") );
  }
}


void K3bDvdJob::slotIsoImagerFinished( bool success )
{
  if( m_canceled ) {
    emit canceled();
    emit finished(false);
    return;
  }

  m_imageFile.close();
  if( success ) {
    emit infoMessage( i18n("Image successfully created in %1").arg(m_doc->tempDir()), K3bJob::STATUS );
    
    if( m_doc->onlyCreateImages() ) {
      emit finished( true );
    }
    else {
      if( prepareWriterJob() ) {
	if( waitForDvd() )
	  m_writerJob->start();
	else
	  emit finished(false);
      }
      else {
	emit finished(false);
      }
    }
  }
  
  else {
    emit infoMessage( i18n("Error while creating iso image"), ERROR );
    cleanup();
    emit finished( false );
  }
}


void K3bDvdJob::cancel()
{
  m_canceled = true;

  if( m_isoImager )
    m_isoImager->cancel();
  if( m_growisofsImager )
    m_growisofsImager->cancel();

  cleanup();
}


bool K3bDvdJob::prepareWriterJob()
{
  if( m_writerJob )
    delete m_writerJob;

//   //
//   // determine the writer
//   // default is always Growisofs
//   // only if the user chooses dvdrecord manually we use it
//   // dvdrecord does not support multisession!
//   //
//   // Possibility: Use a K3bDataJob and set it's writer when using dvdrecord
//   //

//   if( m_usedWritingApp == K3b::DVDRECORD )  {
//     K3bDvdrecordWriter* writer = new K3bDvdrecordWriter( m_doc->burner(), this );

//     writer->setSimulate( m_doc->dummy() );
//     writer->setBurnproof( m_doc->burnproof() );
//     writer->setBurnSpeed( m_doc->speed() );

//     // multisession
//     if( m_doc->multiSessionMode() == K3bDataDoc::START ||
// 	m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ) {
//       writer->addArgument("-multi");
//     }

//     if( m_doc->onTheFly() &&
// 	( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
// 	  m_doc->multiSessionMode() == K3bDataDoc::FINISH ) )
//       writer->addArgument("-waiti");

//     if( m_doc->onTheFly() ) {
//       writer->addArgument( QString("-tsize=%1s").arg(m_isoImager->size()) )->addArgument("-");
//       writer->setProvideStdin(true);
//     }
//     else {
//       writer->addArgument( m_doc->tempDir() );
//     }

//     m_writerJob = writer;
//   }
//   else {
    K3bGrowisofsWriter* writer = new K3bGrowisofsWriter( m_doc->burner(), this );

    // these do only make sense with DVD-R(W)
    writer->setSimulate( m_doc->dummy() );
    //    writer->setBurnproof( m_doc->burnproof() );
    writer->setBurnSpeed( m_doc->speed() );

    //
    // for now we only use the K3bGrowisofsWriter for writing images
    // in every other case we use the K3bGrowisofsImager.
    //

    writer->setImageToWrite( m_doc->tempDir() );

    m_writerJob = writer;
    //  }


  connect( m_writerJob, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writerJob, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writerJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  //  connect( m_writerJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  //  connect( m_writerJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  //  connect( m_writerJob, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( m_writerJob, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writerJob, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotWritingFinished(bool)) );
  //  connect( m_writerJob, SIGNAL(dataWritten()), this, SLOT(slotDataWritten()) );
  connect( m_writerJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writerJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


void K3bDvdJob::slotWriterJobPercent( int p )
{
  // we only use the writer when creating an image first
  emit percent( 50 + p/2 );
}


void K3bDvdJob::slotWritingFinished( bool success )
{
  if( m_canceled ) {
    emit canceled();
    emit finished(false);
    return;
  }

  cleanup();
  
  emit finished( success );
}


void K3bDvdJob::cleanup()
{
  if( m_canceled || m_doc->removeImages() ) {
    if( m_imageFile.exists() ) {
      m_imageFile.remove();
      emit infoMessage( i18n("Removed image file %1").arg(m_imageFile.name()), K3bJob::STATUS );
    }
  }
}


bool K3bDvdJob::waitForDvd()
{
  int mt = 0;
  if( m_doc->writingMode() == K3b::WRITING_MODE_INCR_SEQ || m_doc->writingMode() == K3b::DAO )
    mt = K3bCdDevice::MEDIA_DVD_RW_SEQ|K3bCdDevice::MEDIA_DVD_R_SEQ;
  else if( m_doc->writingMode() == K3b::WRITING_MODE_RES_OVWR ) // we treat DVD+R(W) as restricted overwrite media
    mt = K3bCdDevice::MEDIA_DVD_RW_OVWR|K3bCdDevice::MEDIA_DVD_PLUS_RW|K3bCdDevice::MEDIA_DVD_PLUS_R;
  else
    mt = K3bCdDevice::MEDIA_WRITABLE_DVD;

  int m = K3bEmptyDiscWaiter::wait( m_doc->burner(), 
				    m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
				    m_doc->multiSessionMode() == K3bDataDoc::FINISH,
				    mt );
  if( m == -1 ) {
    cancel();
    return false;
  }
  
  if( m == 0 ) {
    emit infoMessage( i18n("Forced by user. Growisofs will be called eithout further tests."), INFO );
  }

  else {
    if( m & (K3bCdDevice::MEDIA_DVD_PLUS_RW|K3bCdDevice::MEDIA_DVD_PLUS_R) ) {
      if( m_doc->dummy() ) {
	if( KMessageBox::warningYesNo( qApp->activeWindow(),
				       i18n("K3b does not support simulation with DVD+R(W) media. "
					    "Do you really want to continue? The media will be written "
					    "for real."),
				       i18n("No simulation with DVD+R(W)") ) == KMessageBox::No ) {
	  cancel();
	  return false;
	}
      }
      
      if( m_doc->speed() > 0 ) {
	emit infoMessage( i18n("DVD+R(W) writers do take care of the writing speed themselves."), INFO );
	emit infoMessage( i18n("The K3b writing speed setting is ignored for DVD+R(W) media."), INFO );
      }

      if( m & K3bCdDevice::MEDIA_DVD_PLUS_RW ) {
	  if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
	      m_doc->multiSessionMode() == K3bDataDoc::START )
	    emit infoMessage( i18n("Writing DVD+RW."), INFO );
	  else
	    emit infoMessage( i18n("Growing Iso9660 filesystem on DVD+RW."), INFO );
      }
    }
    else if( m & K3bCdDevice::MEDIA_DVD_RW_OVWR ) {
      if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
	  m_doc->multiSessionMode() == K3bDataDoc::START )
	emit infoMessage( i18n("Writing DVD-RW in restricted overwrite mode."), INFO );
      else
	emit infoMessage( i18n("Growing Iso9660 filesystem on DVD-RW in restricted overwrite mode."), INFO );
    }
    else if( m & (K3bCdDevice::MEDIA_DVD_RW_SEQ|
		  K3bCdDevice::MEDIA_DVD_R_SEQ|
		  K3bCdDevice::MEDIA_DVD_R|
		  K3bCdDevice::MEDIA_DVD_RW) ) {
      emit infoMessage( i18n("Writing DVD-R(W) in sequential mode."), INFO );
    }
  }

  return true;
}


QString K3bDvdJob::jobDescription() const
{
  if( m_doc->onlyCreateImages() ) {
    return i18n("Creating Data Image File");
  }
  else {
    if( m_doc->isoOptions().volumeID().isEmpty() ) {
      if( m_doc->multiSessionMode() == K3bDataDoc::NONE )
	return i18n("Writing Data DVD");
      else
	return i18n("Writing Multisession DVD");
    }
    else {
      if( m_doc->multiSessionMode() == K3bDataDoc::NONE )
	return i18n("Writing Data DVD (%1)").arg(m_doc->isoOptions().volumeID());
      else
	return i18n("Writing Multisession DVD (%1)").arg(m_doc->isoOptions().volumeID());
    }
  }
}


QString K3bDvdJob::jobDetails() const
{
  return i18n("Iso9660 Filesystem (Size: %1)").arg(KIO::convertSize( m_doc->size() ));
}

#include "k3bdvdjob.moc"
