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


#ifndef K3BDOC_H
#define K3BDOC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

// include files for QT
#include <qobject.h>
#include <qstring.h>
#include <qptrlist.h>


// include files for KDE
#include <kurl.h>



// forward declaration of the K3b classes
class K3bView;
class QTimer;
class KTempFile;
class K3bDevice;
class K3bMainWindow;
class K3bBurnJob;
class QDomDocument;
class QDomElement;



/**	K3bDoc provides a document object for a document-view model.
  *
  * The K3bDoc class provides a document object that can be used in conjunction with the classes
  * K3bMainWindow and K3bView to create a document-view model for MDI (Multiple Document Interface)
  * KDE 2 applications based on KApplication and KTMainWindow as main classes and QWorkspace as MDI manager widget.
  * Thereby, the document object is created by the K3bMainWindow instance (and kept in a document list) and contains
  * the document structure with the according methods for manipulating the document
  * data by K3bView objects. Also, K3bDoc contains the methods for serialization of the document data
  * from and to files.
  * @author Source Framework Automatically Generated by KDevelop, (c) The KDevelop Team. 	
  * @version KDevelop version 1.3 code generation
  */
class K3bDoc : public QObject
{
  Q_OBJECT

    friend class K3bView;

 public:
  /** Constructor for the fileclass of the application */
  K3bDoc( QObject* );
  /** Destructor for the fileclass of the application */
  virtual ~K3bDoc();

  enum DocType { AUDIO = 1, DATA, BACKUP, MIXED, VCD, MOVIX };


  virtual void disable();
  virtual void enable();


  virtual int docType() const { return m_docType; }
	
  /** adds a view to the document which represents the document contents. Usually this is your main view. */
  virtual void addView(K3bView *view);
  /** removes a view from the list of currently connected views */
  void removeView(K3bView *view);
  /** gets called if a view is removed or added */
  void changedViewList();
  /** returns the first view instance */
  K3bView* firstView(){ return pViewList->first(); };
  /** returns true, if the requested view is the last view of the document */
  bool isLastView();

  /** sets the modified flag for the document after a modifying action on the view connected to the document.*/
  void setModified(bool _m=true){ modified=_m; };
  /** returns if the document is modified or not. Use this to determine if your document needs saving by the user on closing.*/
  bool isModified(){ return modified; };

  /** this virtual version only sets the modified flag */
  virtual bool newDocument();

  static K3bDoc* openDocument(const KURL &url);

  /**
   * saves the document under filename and format.
   */	
  bool saveDocument(const KURL &url);

  /** returns the KURL of the document */
  const KURL& URL() const;
  /** sets the URL of the document */
  void setURL(const KURL& url);
	
  /** Create a new view */
  virtual K3bView* newView( QWidget* parent ) = 0;

  virtual const QString& projectName() const { return m_projectName; }
  bool dao() const { return m_dao; }
  bool dummy() const { return m_dummy; }
  bool onTheFly() const { return m_onTheFly; }
  bool burnproof() const { return m_burnproof; }
  bool overburn() const { return m_overburn; }
  int speed() const { return m_speed; }
  K3bDevice* burner() const { return m_burner; }
  virtual unsigned long long size() const = 0;
  virtual unsigned long long length() const = 0;

  const QString& tempDir() const { return m_tempDir; }

  virtual int numOfTracks() const { return 1; }
	
  virtual K3bBurnJob* newBurnJob() = 0;
  
  int writingApp() const { return m_writingApp; }
  void setWritingApp( int a ) { m_writingApp = a; }

  void setSaved( bool b ) { m_saved = b; }
  bool saved() const { return m_saved; }

  /**
   * should return the name of the document type
   * for saving the contents in a XML file
   */
  virtual QString documentType() const = 0;

 public slots:
  void updateAllViews();
  void setDummy( bool d );
  void setDao( bool d );
  void setOnTheFly( bool b ) { m_onTheFly = b; }
  void setOverburn( bool b ) { m_overburn = b; }
  void setSpeed( int speed );
  void setBurner( K3bDevice* dev );
  void setBurnproof( bool b ) { m_burnproof = b; }
  void setTempDir( const QString& dir ) { m_tempDir = dir; }

  virtual void addUrl( const KURL& url ) = 0;
  virtual void addUrls( const KURL::List& urls ) = 0;
	
 signals:
  void errorMessage( const QString& );
  void warningMessage( const QString& );
  void infoMessage( const QString& );
  void result();
  void percent( int percent );
	 	
 protected:
  /** 
   * when deriving from K3bDoc this method really opens the document since
   * openDocument only opens a tempfile and calls this method. 
   */
  virtual bool loadDocumentData( QDomElement* root ) = 0;
	
  /** 
   * when deriving from K3bDoc this method really saves the document since
   * saveDocument only opens the file and calls this method. 
   * Append all child elements to docElem.
   * XML header was already created
   */
  virtual bool saveDocumentData( QDomElement* docElem ) = 0;

  bool saveGeneralDocumentData( QDomElement* );
  bool readGeneralDocumentData( const QDomElement& );

  /**
   * load the default project settings from the app configuration
   */
  virtual void loadDefaultSettings() = 0;

  int m_docType;

 private:
  /** the modified flag of the current document */
  bool modified;
  KURL doc_url;
  /** the list of the views currently connected to the document */
  QList<K3bView> *pViewList;	
  QString m_projectName;
  QString m_tempDir;
  K3bDevice* m_burner;
  bool m_dao;
  bool m_dummy;
  bool m_onTheFly;
  bool m_burnproof;
  bool m_overburn;
  int  m_speed;

  /** see k3bglobals.h */
  int m_writingApp;

  bool m_saved;
};

#endif // K3BDOC_H
