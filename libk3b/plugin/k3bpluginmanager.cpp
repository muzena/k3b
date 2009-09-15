/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpluginmanager.h"
#include "k3bplugin.h"
#include "k3bpluginconfigwidget.h"
#include "k3bversion.h"

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <klibloader.h>
#include <kdialog.h>
#include <kservice.h>
#include <kcmoduleinfo.h>

#include <KServiceTypeTrader>
#include <KService>
#include <KPluginInfo>
#include <KCModuleProxy>

#include <qlist.h>
#include <qmap.h>
#include <qdir.h>
#include <QtGui/QVBoxLayout>



class K3b::PluginManager::Private
{
public:
    Private( K3b::PluginManager* parent )
        : m_parent( parent ) {
    }

    QList<K3b::Plugin*> plugins;

    void loadPlugin( const KService::Ptr &service );

private:
    K3b::PluginManager* m_parent;
};




K3b::PluginManager::PluginManager( QObject* parent )
    : QObject( parent ),
      d( new Private( this ) )
{
}


K3b::PluginManager::~PluginManager()
{
    delete d;
}



QStringList K3b::PluginManager::categories() const
{
    QStringList grps;

    QList<K3b::Plugin*> fl;
    Q_FOREACH( K3b::Plugin* plugin, d->plugins ) {
        if( !grps.contains( plugin->category() ) )
            grps.append( plugin->category() );
    }

    return grps;
}


QList<K3b::Plugin*> K3b::PluginManager::plugins( const QString& group ) const
{
    QList<K3b::Plugin*> fl;
    Q_FOREACH( K3b::Plugin* plugin, d->plugins ) {
        if( plugin->category() == group || group.isEmpty() )
            fl.append( plugin );
    }
    return fl;
}


void K3b::PluginManager::Private::loadPlugin( const KService::Ptr &service )
{
    kDebug() << service->name() << service->library();
    K3b::Plugin* plugin = service->createInstance<K3b::Plugin>( m_parent );
    if ( plugin ) {
        kDebug() << "Loaded plugin" << service->name();
        // FIXME: improve this versioning stuff
        if( plugin->pluginSystemVersion() != K3B_PLUGIN_SYSTEM_VERSION ) {
            delete plugin;
            kDebug() << "plugin system does not fit";
        }
        else {
            KPluginInfo pluginInfo( service );
            plugin->m_pluginInfo = pluginInfo;
            plugins.append( plugin );
        }
    }


// 	// make sure to only use the latest version of one plugin
// 	bool addPlugin = true;
// 	for( Q3PtrListIterator<K3b::Plugin> it( d->plugins ); *it; ++it ) {
// 	  if( it.current()->pluginInfo().name() == plugin->pluginInfo().name() ) {
// 	    if( K3b::Version(it.current()->pluginInfo().version()) < K3b::Version(plugin->pluginInfo().version()) ) {
// 	      K3b::Plugin* p = it.current();
// 	      d->plugins.removeRef( p );
// 	      delete p;
// 	    }
// 	    else {
// 	      addPlugin = false;
// 	    }
// 	    break;
// 	  }
}


void K3b::PluginManager::loadAll()
{
    kDebug();
    KService::List services = KServiceTypeTrader::self()->query( "K3b/Plugin" );
    Q_FOREACH( const KService::Ptr &service, services ) {
        d->loadPlugin( service );
    }
}

int K3b::PluginManager::pluginSystemVersion() const
{
    return K3B_PLUGIN_SYSTEM_VERSION;
}


int K3b::PluginManager::execPluginDialog( K3b::Plugin* plugin, QWidget* parent )
{
    QList<KService::Ptr> kcmServices = plugin->pluginInfo().kcmServices();
    if ( !kcmServices.isEmpty() ) {
        KDialog dlg( parent );
        dlg.setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Default);
        dlg.setCaption( i18n("Configure plugin %1", plugin->pluginInfo().name() ) );

        // In K3b we only have at most one KCM for each plugin
        KCModuleInfo kcmModuleInfo( kcmServices.first() );
        KCModuleProxy* currentModuleProxy = new KCModuleProxy( kcmModuleInfo, dlg.mainWidget() );
        QVBoxLayout* layout = new QVBoxLayout( dlg.mainWidget() );
        layout->setMargin( 0 );
        layout->addWidget( currentModuleProxy );
        connect( &dlg, SIGNAL(defaultClicked()), currentModuleProxy, SLOT(defaults()) );
        
        int ret = dlg.exec();
        if( ret == QDialog::Accepted )
        {
            currentModuleProxy->save();
        }
        return ret;
    }
    else {
        KMessageBox::sorry( parent, i18n("No settings available for plugin %1.", plugin->pluginInfo().name() ) );
        return 0;
    }
}

#include "k3bpluginmanager.moc"
