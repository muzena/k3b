/***************************************************************************
                          k3bdatajob.cpp  -  description
                             -------------------
    begin                : Tue May 15 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3biso9660imagewritingjob.h"

#include <device/k3bdevice.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>
#include <tools/k3bglobals.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klocale.h>
#include <ktempfile.h>

#include <qstring.h>
#include <qtextstream.h>
#include <qfile.h>


K3bIso9660ImageWritingJob::K3bIso9660ImageWritingJob()
  : K3bBurnJob(),
    m_dao(true),
    m_simulate(false),
    m_burnproof(false),
    m_device(0),
    m_noFix(false),
    m_speed(2),
    m_writer(0),
    m_tocFile(0)
{
}

K3bIso9660ImageWritingJob::~K3bIso9660ImageWritingJob()
{
  if( m_tocFile )
    delete m_tocFile;
}


void K3bIso9660ImageWritingJob::start()
{
  emit started();

  if( !QFile::exists( m_imagePath ) ) {
    emit infoMessage( i18n("Could not find image %1").arg(m_imagePath), K3bJob::ERROR );
    emit finished( false );
    return;
  }

  if( prepareWriter() )
    m_writer->start();
}


void K3bIso9660ImageWritingJob::slotWriterJobFinished( bool success )
{
  if( m_tocFile ) {
    delete m_tocFile;
    m_tocFile = 0;
  }

  if( success ) {
    // allright
    // the writerJob should have emited the "simulation/writing successful" signal
    emit finished(true);
  }
  else {
    emit finished(false);
  }
}


void K3bIso9660ImageWritingJob::cancel()
{
  if( m_writer ) {
    emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
    emit canceled();
    
    m_writer->cancel();
  }
}


bool K3bIso9660ImageWritingJob::prepareWriter()
{
  if( m_writer )
    delete m_writer;

  if( writingApp() == K3b::CDRECORD || writingApp() == K3b::DEFAULT ) {
    K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_device, this );

    writer->setDao( m_dao );
    writer->setSimulate( m_simulate );
    writer->setBurnproof( m_burnproof );
    writer->setBurnSpeed( m_speed );
    writer->prepareArgumentList();

    if( m_noFix )
      writer->addArgument("-multi");

    writer->addArgument( m_imagePath );

    m_writer = writer;
  }
  else {
    // create cdrdao job
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_device, this );
    writer->setSimulate( m_simulate );
    writer->setBurnSpeed( m_speed );
    // multisession
    writer->setMulti( m_noFix );

    // now write the tocfile
    if( m_tocFile ) delete m_tocFile;
    m_tocFile = new KTempFile( QString::null, "toc" );
    m_tocFile->setAutoDelete(true);

    if( QTextStream* s = m_tocFile->textStream() ) {
      *s << "CD_ROM" << "\n";
      *s << "\n";
      *s << "TRACK MODE1" << "\n";
      *s << "DATAFILE \"" << m_imagePath << "\" 0 \n";

      m_tocFile->close();
    }
    else {
      kdDebug() << "(K3bDataJob) could not write tocfile." << endl;
      emit infoMessage( i18n("IO Error"), ERROR );
      emit finished(false);
      return false;
    }

    writer->setTocFile( m_tocFile->name() );

    m_writer = writer;
  }
    
  connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writer, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
  connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterJobFinished(bool)) );
  connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


#include "k3biso9660imagewritingjob.moc"
