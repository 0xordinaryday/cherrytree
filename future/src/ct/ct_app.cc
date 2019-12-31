/*
 * ct_app.cc
 *
 * Copyright 2017-2020 Giuseppe Penone <giuspen@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <glib/gstdio.h>
#include "ct_app.h"
#include "ct_pref_dlg.h"
#include "config.h"

CtApp::CtApp() : Gtk::Application("com.giuspen.cherrytree", Gio::APPLICATION_HANDLES_OPEN)
{
    Gsv::init();

    _uCtCfg.reset(new CtConfig());
    //std::cout << _uCtCfg->specialChars.size() << "\t" << _uCtCfg->specialChars << std::endl;

    _uCtActions.reset(new CtActions());

    _iconthemeInit();

    _uCtTmp.reset(new CtTmp());
    //std::cout << _uCtTmp->get_root_dirpath() << std::endl;

    _rTextTagTable = Gtk::TextTagTable::create();

    _rLanguageManager = Gsv::LanguageManager::create();

    _rStyleSchemeManager = Gsv::StyleSchemeManager::create();

    _rCssProvider = Gtk::CssProvider::create();

    _uCtMenu.reset(new CtMenu());
    _uCtMenu->init_actions(this, _uCtActions.get());
}

CtApp::~CtApp()
{
    //std::cout << "~CtApp()" << std::endl;
}

Glib::RefPtr<CtApp> CtApp::create()
{
    return Glib::RefPtr<CtApp>(new CtApp());
}

void CtApp::_printHelpMessage()
{
    std::cout << "Usage: " << GETTEXT_PACKAGE << " [filepath.ctd|.ctb|.ctz|.ctx]" << std::endl;
}

void CtApp::_printGresourceIcons()
{
    for (const std::string& str_icon : Gio::Resource::enumerate_children_global("/icons/", Gio::ResourceLookupFlags::RESOURCE_LOOKUP_FLAGS_NONE))
    {
        std::cout << str_icon << std::endl;
    }
}

void CtApp::_iconthemeInit()
{
    _rIcontheme = Gtk::IconTheme::get_default();
    _rIcontheme->add_resource_path("/icons/");
    //_printGresourceIcons();
}

CtMainWin* CtApp::create_appwindow()
{
    CtMainWin* pCtMainWin = new CtMainWin(_uCtCfg.get(),
                                          _uCtActions.get(),
                                          _uCtTmp.get(),
                                          _uCtMenu.get(),
                                          _rIcontheme.get(),
                                          _rTextTagTable.get(),
                                          _rCssProvider.get(),
                                          _rLanguageManager.get(),
                                          _rStyleSchemeManager.get());
    CtApp::_uCtActions->init(pCtMainWin);

    add_window(*pCtMainWin);

    pCtMainWin->signal_hide().connect(sigc::bind<CtMainWin*>(sigc::mem_fun(*this, &CtApp::on_hide_window), pCtMainWin));
    return pCtMainWin;
}

CtMainWin* CtApp::get_main_win()
{
    auto windows_list = get_windows();
    if (windows_list.size() > 0)
        return dynamic_cast<CtMainWin*>(windows_list[0]);
    return create_appwindow();
}

void CtApp::on_activate()
{
    // app run without arguments
    auto pAppWindow = create_appwindow();
    pAppWindow->present();

    if (not CtApp::_uCtCfg->recentDocsFilepaths.empty())
    {
        Glib::RefPtr<Gio::File> r_file = Gio::File::create_for_path(CtApp::_uCtCfg->recentDocsFilepaths.front());
        if (r_file->query_exists())
        {
            if (not pAppWindow->read_nodes_from_gio_file(r_file, false/*isImport*/))
            {
                _printHelpMessage();
            }
        }
        else
        {
            std::cout << "? not found " << CtApp::_uCtCfg->recentDocsFilepaths.front() << std::endl;
            CtApp::_uCtCfg->recentDocsFilepaths.move_or_push_back(CtApp::_uCtCfg->recentDocsFilepaths.front());
            pAppWindow->set_menu_items_recent_documents();
        }
    }
}

void CtApp::on_hide_window(CtMainWin* pCtMainWin)
{
    pCtMainWin->config_update_data_from_curr_status();
    _uCtCfg->write_to_file();
    delete pCtMainWin;
}

void CtApp::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& /*hint*/)
{
    // app run with arguments
    CtMainWin* pAppWindow = get_main_win();

    for (const Glib::RefPtr<Gio::File>& r_file : files)
    {
        if (r_file->query_exists())
        {
            if (not pAppWindow->read_nodes_from_gio_file(r_file, false/*isImport*/))
            {
                _printHelpMessage();
            }
        }
        else
        {
            std::cout << "!! Missing file " << r_file->get_path() << std::endl;
        }
    }

    pAppWindow->present();
}

void CtApp::quit_application()
{
    quit();
}

void CtApp::dialog_preferences()
{
    CtPrefDlg prefDlg(get_main_win());
    prefDlg.show();
    prefDlg.run();
    prefDlg.hide();
}
