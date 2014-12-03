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

struct ServoTabEventHandler
{
    virtual void LoadStateChanged(bool loading, bool canGoForward, bool canGoBack) = 0;
};

struct ServoTab : public CefClient,
                  public CefLoadHandler,
                  public CefRenderHandler
{
    explicit ServoTab() {}

    void init(ServoTabEventHandler *pEventHandler, GtkGrid *pParent)
    {
        mpEventHandler = pEventHandler;
        mWidth = 800;
        mHeight = 600;
        mpWidget = te_gtkgl_new();
        mHiDpiScale = gdk_screen_get_monitor_scale_factor(GDK_SCREEN(gdk_screen_get_default()), 0);

        gtk_widget_set_size_request(mpWidget, mWidth, mHeight);
        gtk_grid_attach(pParent, mpWidget, 0, 2, 4, 1);

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
        mpEventHandler->LoadStateChanged(isLoading, canGoBack, canGoForward);
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

    GtkWidget *mpWidget;
    CefRefPtr<CefBrowser> mpBrowser;
    int mWidth, mHeight;
    int mHiDpiScale;
    
private:
    IMPLEMENT_REFCOUNTING(ServoTab);
    DISALLOW_COPY_AND_ASSIGN(ServoTab);

    ServoTabEventHandler *mpEventHandler;

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

};

struct MiniServo
{
    struct TabEventHandler : public ServoTabEventHandler
    {
        TabEventHandler(MiniServo *pApp) : mpApp(pApp) {}

        virtual void LoadStateChanged(bool loading, bool canGoForward, bool canGoBack)
        {
            gtk_widget_set_sensitive(GTK_WIDGET(mpApp->mWidgets.back_button), canGoBack);
            gtk_widget_set_sensitive(GTK_WIDGET(mpApp->mWidgets.forward_button), canGoForward);
            if (loading) {
                gtk_spinner_start(mpApp->mWidgets.progress_spinner);
            } else {
                gtk_spinner_stop(mpApp->mWidgets.progress_spinner);
            }
        }

        MiniServo *mpApp;
    };

    MiniServo(int argc, char **argv) : mQuit(false), mEventHandler(this)
    {
        XInitThreads();
        gtk_init (&argc, &argv);

        //CefScopedArgArray scoped_arg_array(argc, argv);
        //char** argv_copy = scoped_arg_array.array();
    
        CefMainArgs main_args(argc, argv);
        CefSettings settings;
        CefInitialize(main_args, settings, NULL, NULL);

        GtkBuilder *pBuilder = gtk_builder_new();
        GError *pError = NULL;
        if (!gtk_builder_add_from_file(pBuilder, "../miniservo.glade", &pError)) {
            g_warning("%s", pError->message);
            exit(EXIT_FAILURE);
        }

        get_widgets(pBuilder);
        gtk_builder_connect_signals(pBuilder, &mWidgets);
        g_object_unref(G_OBJECT(pBuilder));
        gtk_widget_show_all(mWidgets.main_window);

        mpCurrentTab = new ServoTab;
        mpCurrentTab->init(&mEventHandler, mWidgets.main_grid);

        gtk_widget_show_all(mWidgets.main_window);
    }

    ~MiniServo()
    {
        // todo!
    }

    void run()
    {
        while (true) {
            gtk_main_iteration_do(false);
            CefDoMessageLoopWork();

            if (mQuit) {
                break;
            }
        }
    }

    struct Widgets
    {
        GtkWidget *main_window;
        GtkEntry *url_bar;
        GtkStatusbar *status_bar;
        GtkGrid *main_grid;
        GtkSpinner *progress_spinner;
        GtkButton *back_button;
        GtkButton *forward_button;
    };

    ServoTab *mpCurrentTab;
    Widgets mWidgets;
    bool mQuit;
    TabEventHandler mEventHandler;

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
        mWidgets.main_window = GTK_WIDGET(gtk_builder_get_object(pBuilder, "window1"));
        mWidgets.status_bar = GTK_STATUSBAR(gtk_builder_get_object(pBuilder, "status_bar"));
        mWidgets.url_bar = GTK_ENTRY(gtk_builder_get_object(pBuilder, "entry1"));
        mWidgets.main_grid = GTK_GRID(gtk_builder_get_object(pBuilder, "grid_main"));
        mWidgets.progress_spinner = GTK_SPINNER(gtk_builder_get_object(pBuilder, "spinner1"));
        mWidgets.back_button = GTK_BUTTON(gtk_builder_get_object(pBuilder, "button_back"));
        mWidgets.forward_button = GTK_BUTTON(gtk_builder_get_object(pBuilder, "button_forward"));

        assert(mWidgets.main_window);
        assert(mWidgets.status_bar);
        assert(mWidgets.url_bar);
        assert(mWidgets.main_grid);
        assert(mWidgets.progress_spinner);  
        assert(mWidgets.back_button);
        assert(mWidgets.forward_button);  

        g_signal_connect(mWidgets.main_window, "destroy", G_CALLBACK(cb_quit), this);
        g_signal_connect(mWidgets.url_bar, "activate", G_CALLBACK(cb_url_changed), this);
        g_signal_connect(mWidgets.back_button, "clicked", G_CALLBACK(cb_back_clicked), this);
        g_signal_connect(mWidgets.forward_button, "clicked", G_CALLBACK(cb_forward_clicked), this);
    }

    static void cb_back_clicked(GtkButton *button, gpointer user_data)
    {
        MiniServo *ms = (MiniServo *) user_data;
        ms->mpCurrentTab->mpBrowser->GoBack();
    }

    static void cb_forward_clicked(GtkButton *button, gpointer user_data)
    {
        MiniServo *ms = (MiniServo *) user_data;
        ms->mpCurrentTab->mpBrowser->GoForward();
    }

    static void cb_quit(GtkWidget *, gpointer user_data) {
        MiniServo *ms = (MiniServo *) user_data;
        ms->mQuit = true;
    }

    static void cb_url_changed(GtkEntry *e, gpointer user_data) {
        MiniServo *ms = (MiniServo *) user_data;
        std::string url(gtk_entry_get_text(e));

        if (!starts_with(url, "http://") && !starts_with(url, "https://")) {
            url = "http://" + url;
        }

        CefString cefString(url);
        if (ms->mpCurrentTab->mpBrowser == nullptr)
          return;
        if (ms->mpCurrentTab->mpBrowser->GetMainFrame() == nullptr)
          return;
        ms->mpCurrentTab->mpBrowser->GetMainFrame()->LoadURL(cefString);
    }

    static void on_load_state_changed(void *user_data)
    {
        MiniServo *ms = (MiniServo *) user_data;
        assert(false);
    }
};

int main(int argc, char **argv)
{
    MiniServo app(argc, argv);
    app.run();
}
