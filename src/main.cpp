#include "include/base/cef_logging.h"
#include "include/base/cef_scoped_ptr.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_base.h"
#include "include/cef_client.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <GL/gl.h>

#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <assert.h>

#include "tegtkgl.h"

static XDisplay *g_x_display = NULL;

CEF_EXPORT XDisplay* cef_get_xdisplay()
{
    assert(g_x_display);
    return g_x_display;
}

static bool starts_with(const std::string &haystack, const std::string &needle)
{
    return needle.length() <= haystack.length()
        && equal(needle.begin(), needle.end(), haystack.begin());
}

struct TabEventHandler
{
    virtual void OnClosed(struct ServoTab *pTab) = 0;
    virtual void OnTitleChanged(struct ServoTab *pTab, const std::string &title) = 0;
};

struct ServoTabLabel
{
    typedef void CloseClickedCallback(ServoTab *pTab);

    struct Widgets
    {
        GtkBox *box;
        GtkLabel *label;
        GtkButton *close_button;
    };

    ServoTabLabel(CloseClickedCallback *pCallback, ServoTab *pOwner)
    {
        mpCallback = pCallback;
        mpOwner = pOwner;
        GtkBuilder *pBuilder = gtk_builder_new();
        GError *pError = NULL;
        if (!gtk_builder_add_from_file(pBuilder, "../miniservo-tab-label.glade", &pError)) {
            g_warning("%s", pError->message);
            exit(EXIT_FAILURE);
        }
        get_widgets(pBuilder);
        g_signal_connect(mWidgets.close_button, "clicked", G_CALLBACK(close_callback), this);

        // TODO: Cleanup
        //g_object_unref(G_OBJECT(pBuilder));
    }

    void get_widgets(GtkBuilder *pBuilder)
    {
        mWidgets.box = GTK_BOX(gtk_builder_get_object(pBuilder, "box"));
        mWidgets.label = GTK_LABEL(gtk_builder_get_object(pBuilder, "label"));
        mWidgets.close_button = GTK_BUTTON(gtk_builder_get_object(pBuilder, "button"));

        assert(mWidgets.box);
        assert(mWidgets.label);
        assert(mWidgets.close_button);
    }

    GtkWidget *get_widget()
    {
        return GTK_WIDGET(mWidgets.box);
    }

    void set_text(const char *pText)
    {
        gtk_label_set_text(mWidgets.label, pText);
    }

    static void close_callback(GtkWidget *, gpointer user_data)
    {
        ServoTabLabel *pThis = (ServoTabLabel *) user_data;
        pThis->mpCallback(pThis->mpOwner);
    }

    Widgets mWidgets;
    ServoTab *mpOwner;
    CloseClickedCallback *mpCallback;
};

struct ServoTab : public CefClient,
                  public CefLoadHandler,
                  public CefRenderHandler,
                  public CefStringVisitor
{
    explicit ServoTab() {}

    struct Widgets
    {
        ServoTabLabel *page_title;

        GtkGrid *grid;
        GtkEntry *url_bar;
        GtkButton *back_button;
        GtkButton *forward_button;
    };

    void get_widgets(GtkBuilder *pBuilder)
    {
        mWidgets.grid = GTK_GRID(gtk_builder_get_object(pBuilder, "grid"));
        mWidgets.url_bar = GTK_ENTRY(gtk_builder_get_object(pBuilder, "url_bar"));
        mWidgets.back_button = GTK_BUTTON(gtk_builder_get_object(pBuilder, "back_button"));
        mWidgets.forward_button = GTK_BUTTON(gtk_builder_get_object(pBuilder, "forward_button"));

        assert(mWidgets.grid);
        assert(mWidgets.url_bar);
        assert(mWidgets.back_button);
        assert(mWidgets.forward_button);

        g_signal_connect(mWidgets.url_bar, "activate", G_CALLBACK(cb_url_changed), this);
        g_signal_connect(mWidgets.back_button, "clicked", G_CALLBACK(cb_back_clicked), this);
        g_signal_connect(mWidgets.forward_button, "clicked", G_CALLBACK(cb_forward_clicked), this);
    }

    GtkWidget *get_widget()
    {
        return GTK_WIDGET(mWidgets.grid);
    }

    void init(TabEventHandler *pEventHandler, GtkNotebook *pParent)
    {
        mTitle = "New Tab";
        mpEventHandler = pEventHandler;
        mTitleUpdatePending = true;
        mWidth = 800;
        mHeight = 600;
        mpWidget = te_gtkgl_new();
        mHiDpiScale = gdk_screen_get_monitor_scale_factor(GDK_SCREEN(gdk_screen_get_default()), 0);

        GtkBuilder *pTabBuilder = gtk_builder_new();
        GError *pError = NULL;
        if (!gtk_builder_add_from_file(pTabBuilder, "../miniservo-tab.glade", &pError)) {
            g_warning("%s", pError->message);
            exit(EXIT_FAILURE);
        }

        mWidgets.page_title = new ServoTabLabel(on_tab_closed, this);

        get_widgets(pTabBuilder);
        gtk_notebook_append_page(pParent, get_widget(), mWidgets.page_title->get_widget());
        g_object_unref(G_OBJECT(pTabBuilder));

        gtk_widget_set_can_focus(mpWidget, TRUE);

        gtk_widget_add_events(mpWidget, GDK_CONFIGURE);
        gtk_widget_add_events(mpWidget, GDK_BUTTON_PRESS_MASK);
        gtk_widget_add_events(mpWidget, GDK_BUTTON_RELEASE_MASK);
        gtk_widget_add_events(mpWidget, GDK_SCROLL_MASK);
        gtk_widget_add_events(mpWidget, GDK_KEY_PRESS_MASK);

        g_signal_connect(mpWidget, "button-press-event", G_CALLBACK(on_button_event), this);
        g_signal_connect(mpWidget, "configure-event", G_CALLBACK(on_configure_event), this);
        g_signal_connect(mpWidget, "scroll-event", G_CALLBACK(on_scroll_event), this);
        g_signal_connect(mpWidget, "key-press-event", G_CALLBACK(on_key_event), this);

        gtk_widget_set_size_request(mpWidget, mWidth, mHeight);
        gtk_grid_attach(mWidgets.grid, mpWidget, 0, 1, 4, 1);

        g_x_display = (XDisplay *) te_gtkgl_get_x11_display(mpWidget);
        CefWindowInfo windowInfo;
        windowInfo.SetAsWindowless(0, false);

        CefBrowserSettings browserSettings;
        std::string INITIAL_URL = "https://duckduckgo.com";
        mpBrowser = CefBrowserHost::CreateBrowserSync(windowInfo, this, INITIAL_URL, browserSettings, NULL);

        te_gtkgl_make_current(TE_GTKGL(mpWidget));
        te_gtkgl_swap(TE_GTKGL(mpWidget));

        mpBrowser->GetHost()->InitializeCompositing();
        mpBrowser->GetHost()->WasResized();
    }

    void on_focus_lost()
    {
        mpBrowser->GetHost()->SendFocusEvent(false);
    }

    void on_focus_gained()
    {
        mpBrowser->GetHost()->SendFocusEvent(true);
    }

    std::string get_title()
    {
        return mTitle;
    }

    static void on_tab_closed(ServoTab *pTab)
    {
        pTab->mpBrowser->GetHost()->CloseBrowser(true);
        pTab->mpEventHandler->OnClosed(pTab);
    }

    // CefClient implementation
    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override {
        return this;
    }
    virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override {
        return this;
    }

    // CefLoadHandler implementation
    virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                      bool isLoading,
                                      bool canGoBack,
                                      bool canGoForward) {
        mTitleUpdatePending = true;

        gtk_widget_set_sensitive(GTK_WIDGET(mWidgets.back_button), canGoBack);
        gtk_widget_set_sensitive(GTK_WIDGET(mWidgets.forward_button), canGoForward);
    }

    // CefRenderHandler implementation
    virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override {
        rect.x = rect.y = 0;
        rect.width = mWidth * mHiDpiScale;
        rect.height = mHeight * mHiDpiScale;
        return true;
    }
    virtual bool GetBackingRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override {
        return GetViewRect(browser, rect);
    }
    virtual void OnPaint(CefRefPtr<CefBrowser> browser,
                         CefRenderHandler::PaintElementType type,
                         const CefRenderHandler::RectList& dirtyRects,
                         const void* buffer,
                         int width,
                         int height) override {
        te_gtkgl_make_current(TE_GTKGL(mpWidget));
        glClear(GL_COLOR_BUFFER_BIT);
    }
    virtual void OnPresent(CefRefPtr<CefBrowser> browser) override {
        te_gtkgl_swap(TE_GTKGL(mpWidget));
    }

    // CefStringVisitor implementation
    virtual void Visit(const CefString &s) {
        mTitle = s.ToString();
        if (mTitle.length() == 0) {
            mTitle = mpBrowser->GetMainFrame()->GetURL().ToString();
        }
        mWidgets.page_title->set_text(mTitle.c_str());
        mpEventHandler->OnTitleChanged(this, mTitle.c_str());
    }

    void update() {
        if (mTitleUpdatePending) {
            mpBrowser->GetMainFrame()->GetText(this);
            mTitleUpdatePending = false;
        }
    }

    Widgets mWidgets;

private:
    IMPLEMENT_REFCOUNTING(ServoTab);
    DISALLOW_COPY_AND_ASSIGN(ServoTab);

    TabEventHandler *mpEventHandler;
    bool mTitleUpdatePending;
    GtkWidget *mpWidget;
    CefRefPtr<CefBrowser> mpBrowser;
    int mWidth, mHeight;
    int mHiDpiScale;
    std::string mTitle;

    static gboolean on_configure_event(GtkWindow *window, GdkEvent *event, gpointer user_data)
    {
        ServoTab *self = (ServoTab *) user_data;

        self->mWidth = event->configure.width;
        self->mHeight = event->configure.height;

        if (self->mpBrowser != nullptr)
        {
            self->mpBrowser->GetHost()->WasResized();
            self->mpBrowser->GetHost()->Invalidate(PET_VIEW);
        }

        return FALSE;
    }

    static gboolean on_key_event(GtkWidget *widget, GdkEvent  *event, gpointer user_data)
    {
        ServoTab *self = (ServoTab *) user_data;

        cef_key_event_t cCefKeyEvent;
        cCefKeyEvent.type = KEYEVENT_RAWKEYDOWN;
        cCefKeyEvent.character = event->key.keyval;
        cCefKeyEvent.modifiers = 0;
        cCefKeyEvent.windows_key_code = event->key.hardware_keycode;    // FIXME(pcwalton)
        cCefKeyEvent.native_key_code = event->key.hardware_keycode;
        cCefKeyEvent.is_system_key = false;
        cCefKeyEvent.focus_on_editable_field = false;
        CefKeyEvent keyEvent(cCefKeyEvent);
        self->mpBrowser->GetHost()->SendKeyEvent(keyEvent);
        return FALSE;
    }

    static gboolean on_scroll_event(GtkWidget *widget, GdkEvent* ev, gpointer user_data)
    {
        ServoTab *self = (ServoTab *) user_data;

        cef_mouse_event_t cCefMouseEvent;
        cCefMouseEvent.x = ev->scroll.x;
        cCefMouseEvent.y = ev->scroll.y;
        CefMouseEvent cefMouseEvent(cCefMouseEvent);
        self->mpBrowser->GetHost()->SendMouseWheelEvent(cefMouseEvent, 30 * ev->scroll.delta_x, -30 * ev->scroll.delta_y);
        return FALSE;
    }

    static gboolean on_button_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
    {
        ServoTab *self = (ServoTab *) user_data;

        if (event->type == GDK_BUTTON_PRESS) {
            if (!gtk_widget_has_focus(widget)) {
                gtk_widget_grab_focus(widget);
            }

            gdouble x = self->mHiDpiScale * ((GdkEventButton*)event)->x;
            gdouble y = self->mHiDpiScale * ((GdkEventButton*)event)->y;

            cef_mouse_event_t cCefMouseEvent;
            cCefMouseEvent.x = x;
            cCefMouseEvent.y = y;
            CefMouseEvent cefMouseEvent(cCefMouseEvent);
            self->mpBrowser->GetHost()->SendMouseClickEvent(cefMouseEvent, MBT_LEFT, false, 1);
            self->mpBrowser->GetHost()->SendMouseClickEvent(cefMouseEvent, MBT_LEFT, true, 1);
        }
        return FALSE;
    }

    static void cb_back_clicked(GtkButton *button, gpointer user_data)
    {
        ServoTab *pTab = (ServoTab *) user_data;
        if (pTab && pTab->mpBrowser) {
            pTab->mpBrowser->GoBack();
        }
    }

    static void cb_forward_clicked(GtkButton *button, gpointer user_data)
    {
        ServoTab *pTab = (ServoTab *) user_data;
        if (pTab && pTab->mpBrowser) {
            pTab->mpBrowser->GoForward();
        }
    }

    static void cb_url_changed(GtkEntry *e, gpointer user_data) {
        std::string url(gtk_entry_get_text(e));
        if (!starts_with(url, "http://") && !starts_with(url, "https://")) {
            url = "http://" + url;
        }
        CefString cefString(url);

        ServoTab *pTab = (ServoTab *) user_data;
        if (pTab->mpBrowser == nullptr)
            return;
        if (pTab->mpBrowser->GetMainFrame() == nullptr)
            return;
        pTab->mpBrowser->GetMainFrame()->LoadURL(cefString);
    }

};

struct MiniServo
{
    struct EventHandler : public TabEventHandler {
        EventHandler(MiniServo *pApp) : mpApp(pApp) {}

        virtual void OnTitleChanged(struct ServoTab *pTab, const std::string &title)
        {
            if (pTab == mpApp->mpCurrentTab)
            {
                mpApp->set_title(title);
            }
        }
        virtual void OnClosed(struct ServoTab *pTab)
        {
            pTab->on_focus_lost();
            mpApp->mTabs.erase(std::remove(mpApp->mTabs.begin(), mpApp->mTabs.end(), pTab), mpApp->mTabs.end());
            gint page_index = gtk_notebook_page_num(mpApp->mWidgets.notebook, pTab->get_widget());
            assert(page_index != -1);
            gtk_notebook_remove_page(mpApp->mWidgets.notebook, page_index);
            delete pTab;
            if (mpApp->mpCurrentTab == pTab)
            {
                if (gtk_notebook_get_n_pages(mpApp->mWidgets.notebook) == 0)
                {
                    mpApp->mQuit = true;
                }
                else
                {
                    gint current_page_index = gtk_notebook_get_current_page(mpApp->mWidgets.notebook);
                    assert(current_page_index != -1);
                    GtkGrid *pCurrentPage = GTK_GRID(gtk_notebook_get_nth_page(mpApp->mWidgets.notebook, current_page_index));
                    for (std::vector<ServoTab *>::iterator it = mpApp->mTabs.begin() ; it != mpApp->mTabs.end() ; ++it)
                    {
                        if ((*it)->mWidgets.grid == pCurrentPage)
                        {
                            (*it)->on_focus_gained();
                        }
                    }
                }
            }
            // TODO: This leaks like a sieve.
        }

    private:
        MiniServo *mpApp;
    };

    MiniServo(int argc, char **argv) : mQuit(false), mEventHandler(this), mpCurrentTab(NULL)
    {
        XInitThreads();
        gtk_init (&argc, &argv);

        CefMainArgs main_args(argc, argv);
        CefSettings settings;
        CefInitialize(main_args, settings, NULL, NULL);

        GtkBuilder *pWindowBuilder = gtk_builder_new();
        GError *pError = NULL;
        if (!gtk_builder_add_from_file(pWindowBuilder, "../miniservo.glade", &pError)) {
            g_warning("%s", pError->message);
            exit(EXIT_FAILURE);
        }

        get_widgets(pWindowBuilder);
        gtk_builder_connect_signals(pWindowBuilder, &mWidgets);
        g_object_unref(G_OBJECT(pWindowBuilder));
        gtk_widget_show_all(GTK_WIDGET(mWidgets.main_window));

        add_tab();

        g_signal_connect(mWidgets.notebook, "switch-page", G_CALLBACK(on_tab_changed), this);
    }

    ~MiniServo()
    {
        // todo!
    }

    ServoTab *add_tab()
    {
        ServoTab *pTab = new ServoTab;
        pTab->init(&mEventHandler, mWidgets.notebook);
        mTabs.push_back(pTab);
        if (mpCurrentTab == NULL)
        {
            mpCurrentTab = pTab;
        }
        gtk_widget_show_all(GTK_WIDGET(mWidgets.main_window));
        return pTab;
    }

    void set_current_tab(ServoTab *pTab)
    {
        gint pg = gtk_notebook_page_num(mWidgets.notebook, pTab->get_widget());
        gtk_notebook_set_current_page(mWidgets.notebook, pg);
    }

    void run()
    {
        while (true) {
            gtk_main_iteration_do(false);
            CefDoMessageLoopWork();
            for (std::vector<ServoTab *>::iterator it = mTabs.begin() ; it != mTabs.end() ; ++it) {
                (*it)->update();
            }

            if (mQuit) {
                break;
            }
        }
    }

    struct Widgets
    {
        GtkWindow *main_window;
        GtkNotebook *notebook;
        GtkStatusbar *status_bar;
        GtkMenuItem *mi_new_tab;
        GtkMenuItem *mi_quit;
    };

    ServoTab *mpCurrentTab;
    std::vector<ServoTab *> mTabs;
    Widgets mWidgets;
    bool mQuit;
    EventHandler mEventHandler;

    void push_status(const char *pStatus)
    {
        guint context_id = gtk_statusbar_get_context_id(mWidgets.status_bar, "Default");
        gtk_statusbar_push(mWidgets.status_bar, context_id, pStatus);
    }

    void pop_status()
    {
        assert(false);      // gwtodo
    }

    void get_widgets(GtkBuilder *pBuilder)
    {
        mWidgets.main_window = GTK_WINDOW(gtk_builder_get_object(pBuilder, "window1"));
        mWidgets.status_bar = GTK_STATUSBAR(gtk_builder_get_object(pBuilder, "status_bar"));
        mWidgets.notebook = GTK_NOTEBOOK(gtk_builder_get_object(pBuilder, "notebook1"));
        mWidgets.mi_new_tab = GTK_MENU_ITEM(gtk_builder_get_object(pBuilder, "menuitem_new_tab"));
        mWidgets.mi_quit = GTK_MENU_ITEM(gtk_builder_get_object(pBuilder, "menuitem_quit"));

        assert(mWidgets.main_window);
        assert(mWidgets.notebook);
        assert(mWidgets.status_bar);
        assert(mWidgets.mi_new_tab);
        assert(mWidgets.mi_quit);

        g_signal_connect(GTK_WIDGET(mWidgets.main_window), "destroy", G_CALLBACK(cb_quit), this);
        g_signal_connect(GTK_WIDGET(mWidgets.mi_new_tab), "activate", G_CALLBACK(cb_new_tab), this);
        g_signal_connect(GTK_WIDGET(mWidgets.mi_quit), "activate", G_CALLBACK(cb_quit), this);
    }

    void set_title(const std::string &title) {
        std::string window_title = title;
        if (window_title.length() > 0) {
            window_title.append(" - ");
        }
        window_title.append("MiniServo (GTK)");
        gtk_window_set_title(mWidgets.main_window, window_title.c_str());
    }

    static void cb_quit(GtkWidget *, gpointer user_data) {
        MiniServo *ms = (MiniServo *) user_data;
        ms->mQuit = true;
    }

    static void cb_new_tab(GtkWidget *, gpointer user_data)
    {
        MiniServo *ms = (MiniServo *) user_data;
        ServoTab *pTab = ms->add_tab();
        ms->set_current_tab(pTab);
    }

    static void on_load_state_changed(void *user_data)
    {
        MiniServo *ms = (MiniServo *) user_data;
        assert(false);
    }

    static void on_tab_changed(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
    {
        MiniServo *ms = (MiniServo *) user_data;
        ms->mpCurrentTab->on_focus_lost();
        ms->mpCurrentTab = ms->mTabs[page_num];
        ms->mpCurrentTab->on_focus_gained();
        ms->set_title(ms->mpCurrentTab->get_title());
    }
};

int main(int argc, char **argv)
{
    MiniServo app(argc, argv);
    app.run();
}
