#ifndef _COMPSCREEN_H
#define _COMPSCREEN_H

#include <compwindow.h>
#include <compoutput.h>
#include <compsession.h>
#include <compmetadata.h>
#include <compplugin.h>
#include <compmatch.h>
#include <core/privates.h>

class CompScreen;
class PrivateScreen;
typedef std::list<CompWindow *> CompWindowList;

extern char       *backgroundImage;
extern bool       replaceCurrentWm;
extern bool       indirectRendering;
extern bool       strictBinding;
extern bool       noDetection;

extern CompScreen   *screen;
extern CompMetadata *coreMetadata;

extern REGION emptyRegion;
extern REGION infiniteRegion;

extern int lastPointerX;
extern int lastPointerY;
extern int pointerX;
extern int pointerY;

#define NOTIFY_CREATE_MASK (1 << 0)
#define NOTIFY_DELETE_MASK (1 << 1)
#define NOTIFY_MOVE_MASK   (1 << 2)
#define NOTIFY_MODIFY_MASK (1 << 3)

typedef boost::function<void ()> FdWatchCallBack;
typedef boost::function<void (const char *)> FileWatchCallBack;

typedef int CompFileWatchHandle;
typedef int CompWatchFdHandle;

struct CompFileWatch {
    char		*path;
    int			mask;
    FileWatchCallBack   callBack;
    CompFileWatchHandle handle;
};

/* camera distance from screen, 0.5 * tan (FOV) */
#define DEFAULT_Z_CAMERA 0.866025404f

#define OPAQUE 0xffff
#define COLOR  0xffff
#define BRIGHT 0xffff

#define PAINT_SCREEN_REGION_MASK		   (1 << 0)
#define PAINT_SCREEN_FULL_MASK			   (1 << 1)
#define PAINT_SCREEN_TRANSFORMED_MASK		   (1 << 2)
#define PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK (1 << 3)
#define PAINT_SCREEN_CLEAR_MASK			   (1 << 4)
#define PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK   (1 << 5)
#define PAINT_SCREEN_NO_BACKGROUND_MASK            (1 << 6)

struct CompGroup {
    unsigned int      refCnt;
    Window	      id;
};

struct CompStartupSequence {
    SnStartupSequence		*sequence;
    unsigned int		viewportX;
    unsigned int		viewportY;
};


#define SCREEN_EDGE_LEFT	0
#define SCREEN_EDGE_RIGHT	1
#define SCREEN_EDGE_TOP		2
#define SCREEN_EDGE_BOTTOM	3
#define SCREEN_EDGE_TOPLEFT	4
#define SCREEN_EDGE_TOPRIGHT	5
#define SCREEN_EDGE_BOTTOMLEFT	6
#define SCREEN_EDGE_BOTTOMRIGHT 7
#define SCREEN_EDGE_NUM		8

struct CompScreenEdge {
    Window	 id;
    unsigned int count;
};


#define ACTIVE_WINDOW_HISTORY_SIZE 64
#define ACTIVE_WINDOW_HISTORY_NUM  32

struct CompActiveWindowHistory {
    Window id[ACTIVE_WINDOW_HISTORY_SIZE];
    int    x;
    int    y;
    int    activeNum;
};

class ScreenInterface : public WrapableInterface<CompScreen, ScreenInterface> {
    public:
	virtual void fileWatchAdded (CompFileWatch *fw);
	virtual void fileWatchRemoved (CompFileWatch *fw);

	virtual bool initPluginForScreen (CompPlugin *p);
	virtual void finiPluginForScreen (CompPlugin *p);

	virtual bool setOptionForPlugin (const char *plugin,
					 const char *name,
					 CompOption::Value &v);

	virtual void sessionEvent (CompSession::Event event,
				   CompOption::Vector &options);

	virtual void handleEvent (XEvent *event);
        virtual void handleCompizEvent (const char * plugin, const char *event,
					CompOption::Vector &options);

        virtual bool fileToImage (const char *path, const char *name,
				  int *width, int *height,
				  int *stride, void **data);
	virtual bool imageToFile (const char *path, const char *name,
				  const char *format, int width, int height,
				  int stride, void *data);

	virtual CompMatch::Expression *matchInitExp (const CompString value);

	virtual void matchExpHandlerChanged ();
	virtual void matchPropertyChanged (CompWindow *window);

	virtual void logMessage (const char   *componentName,
				 CompLogLevel level,
				 const char   *message);

	virtual void enterShowDesktopMode ();
	virtual void leaveShowDesktopMode (CompWindow *window);

	virtual void outputChangeNotify ();
};


class CompScreen :
    public WrapableHandler<ScreenInterface, 17>,
    public CompPrivateStorage
{

    public:
	typedef void* GrabHandle;

    public:
	CompScreen ();
	~CompScreen ();

	CompString objectName ();

	bool init (const char *name);
	
	void eventLoop ();

	CompFileWatchHandle addFileWatch (const char        *path,
					  int               mask,
					  FileWatchCallBack callBack);

	void removeFileWatch (CompFileWatchHandle handle);
	
	CompWatchFdHandle addWatchFd (int             fd,
				      short int       events,
				      FdWatchCallBack callBack);
	
	void removeWatchFd (CompWatchFdHandle handle);

	void storeValue (CompString key, CompPrivate value);
	bool hasValue (CompString key);
	CompPrivate getValue (CompString key);
	void eraseValue (CompString key);
	
	Display * dpy();

	CompOption::Vector & getOptions ();
	
	CompOption * getOption (const char *);

	bool setOption (const char        *name,
			CompOption::Value &value);

	bool XRandr ();

	int randrEvent ();
	
	bool XShape ();

	int shapeEvent ();

	int syncEvent ();

	SnDisplay * snDisplay ();
	
	Window activeWindow ();
	
	Window autoRaiseWindow ();

	const char * displayString ();

	unsigned int lastPing ();

	void updateModifierMappings ();

	unsigned int virtualToRealModMask (unsigned int modMask);

	unsigned int keycodeToModifiers (int keycode);

	CompWindow * findWindow (Window id);

	CompWindow * findTopLevelWindow (Window id,
					 bool   override_redirect = false);

	bool readImageFromFile (const char *name,
				int        *width,
				int        *height,
				void       **data);

	bool writeImageToFile (const char *path,
			       const char *name,
			       const char *format,
			       int        width,
			       int        height,
			       void       *data);

	Window getActiveWindow (Window root);

	int getWmState (Window id);

	void setWmState (int state, Window id);

	unsigned int windowStateMask (Atom state);


	static unsigned int windowStateFromString (const char *str);

	unsigned int getWindowState (Window id);

	void setWindowState (unsigned int state, Window id);

	unsigned int getWindowType (Window id);

	void getMwmHints (Window       id,
			  unsigned int *func,
			  unsigned int *decor);


	unsigned int getProtocols (Window id);


	unsigned int getWindowProp (Window       id,
				    Atom         property,
				    unsigned int defaultValue);


	void setWindowProp (Window       id,
			    Atom         property,
			    unsigned int value);


	bool readWindowProp32 (Window         id,
			       Atom           property,
			       unsigned short *returnValue);


	unsigned short getWindowProp32 (Window         id,
					Atom           property,
					unsigned short defaultValue);


	void setWindowProp32 (Window         id,
			      Atom           property,
			      unsigned short value);

	void
	addScreenActions (CompScreen *s);
	
	Window
	root ();

	XWindowAttributes
	attrib ();

	int
	screenNum ();

	CompWindowList &
	windows ();

	unsigned int
	showingDesktopMask ();
	
	void
	setCurrentOutput (unsigned int outputNum);

	void
	configure (XConfigureEvent *ce);

	void
	warpPointer (int dx, int dy);

	Time
	getCurrentTime ();

	Atom
	selectionAtom ();

	Window
	selectionWindow ();

	Time
	selectionTimestamp ();

	void
	updateWorkareaForScreen ();

	void
	forEachWindow (CompWindow::ForEach);

	void
	focusDefaultWindow ();

	void
	insertWindow (CompWindow *w, Window aboveId);

	void
	unhookWindow (CompWindow *w);

	void
	eraseWindowFromMap (Window id);

	GrabHandle
	pushGrab (Cursor cursor, const char *name);

	void
	updateGrab (GrabHandle handle, Cursor cursor);

	void
	removeGrab (GrabHandle handle, CompPoint *restorePointer);

	bool
	otherGrabExist (const char *, ...);

	bool
	addAction (CompAction *action);

	void
	removeAction (CompAction *action);

	void
	updateWorkarea ();

	void
	updateClientList ();

	void
	toolkitAction (Atom	  toolkitAction,
		       Time       eventTime,
		       Window	  window,
		       long	  data0,
		       long	  data1,
		       long	  data2);

	void
	runCommand (CompString command);

	void
	moveViewport (int tx, int ty, bool sync);

	CompGroup *
	addGroup (Window id);

	void
	removeGroup (CompGroup *group);

	CompGroup *
	findGroup (Window id);

	void
	applyStartupProperties (CompWindow *window);

	void
	sendWindowActivationRequest (Window id);


	void
	enableEdge (int edge);

	void
	disableEdge (int edge);

	Window
	getTopWindow ();

	int
	outputDeviceForPoint (int x, int y);

	void
	getCurrentOutputExtents (int *x1, int *y1, int *x2, int *y2);

	void
	setNumberOfDesktops (unsigned int nDesktop);

	void
	setCurrentDesktop (unsigned int desktop);

	void
	getWorkareaForOutput (int output, XRectangle *area);


	void
	viewportForGeometry (CompWindow::Geometry gm,
			     int                  *viewportX,
			     int                  *viewportY);

	int
	outputDeviceForGeometry (CompWindow::Geometry gm);

	bool
	updateDefaultIcon ();

	void
	setCurrentActiveWindowHistory (int x, int y);

	void
	addToCurrentActiveWindowHistory (Window id);

	CompPoint vp ();

	CompSize vpSize ();

	CompSize size ();

	unsigned int &
	pendingDestroys ();


	unsigned int &
	mapNum ();

	int &
	desktopWindowCount ();

	CompOutput::vector &
	outputDevs ();

	XRectangle
	workArea ();

	unsigned int
	currentDesktop ();

	unsigned int
	nDesktop ();

	CompActiveWindowHistory *
	currentHistory ();

	CompScreenEdge &
	screenEdge (int);

	unsigned int &
	activeNum ();

	Region region ();

	bool hasOverlappingOutputs ();

	CompOutput & fullscreenOutput ();

	std::vector<XineramaScreenInfo> & screenInfo ();

	static int allocPrivateIndex ();
	static void freePrivateIndex (int index);

	WRAPABLE_HND (0, ScreenInterface, void, fileWatchAdded, CompFileWatch *)
	WRAPABLE_HND (1, ScreenInterface, void, fileWatchRemoved, CompFileWatch *)

	WRAPABLE_HND (2, ScreenInterface, bool, initPluginForScreen,
		      CompPlugin *)
	WRAPABLE_HND (3, ScreenInterface, void, finiPluginForScreen,
		      CompPlugin *)

	WRAPABLE_HND (4, ScreenInterface, bool, setOptionForPlugin,
		      const char *, const char *, CompOption::Value &)

	WRAPABLE_HND (5, ScreenInterface, void, sessionEvent, CompSession::Event,
		      CompOption::Vector &)
	WRAPABLE_HND (6, ScreenInterface, void, handleEvent, XEvent *event)
	WRAPABLE_HND (7, ScreenInterface, void, handleCompizEvent,
		      const char *, const char *, CompOption::Vector &)

	WRAPABLE_HND (8, ScreenInterface, bool, fileToImage, const char *,
		     const char *,  int *, int *, int *, void **data)
	WRAPABLE_HND (9, ScreenInterface, bool, imageToFile, const char *,
		      const char *, const char *, int, int, int, void *)

	
	WRAPABLE_HND (10, ScreenInterface, CompMatch::Expression *,
		      matchInitExp, const CompString);
	WRAPABLE_HND (11, ScreenInterface, void, matchExpHandlerChanged)
	WRAPABLE_HND (12, ScreenInterface, void, matchPropertyChanged,
		      CompWindow *)

	WRAPABLE_HND (13, ScreenInterface, void, logMessage, const char *,
		      CompLogLevel, const char*)
	WRAPABLE_HND (14, ScreenInterface, void, enterShowDesktopMode);
	WRAPABLE_HND (15, ScreenInterface, void, leaveShowDesktopMode,
		      CompWindow *);

	WRAPABLE_HND (16, ScreenInterface, void, outputChangeNotify);

	friend class CompTimer;
	friend class CompWindow;
	friend class PrivateWindow;

    private:
	PrivateScreen *priv;

    public :

	static bool runCommandDispatch (CompAction         *action,
					CompAction::State  state,
					CompOption::Vector &options);

	static bool runCommandScreenshot(CompAction         *action,
					 CompAction::State  state,
					 CompOption::Vector &options);

	static bool runCommandWindowScreenshot(CompAction         *action,
					       CompAction::State  state,
					       CompOption::Vector &options);

	static bool runCommandTerminal (CompAction         *action,
					CompAction::State  state,
					CompOption::Vector &options);

	static bool mainMenu (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector &options);

	static bool runDialog (CompAction         *action,
			       CompAction::State  state,
			       CompOption::Vector &options);

	static bool showDesktop (CompAction         *action,
				 CompAction::State  state,
				 CompOption::Vector &options);

	static bool windowMenu (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector &options);

	static bool closeWin (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector &options);

	static bool unmaximizeWin (CompAction         *action,
				   CompAction::State  state,
				   CompOption::Vector &options);

	static bool minimizeWin (CompAction         *action,
				 CompAction::State  state,
				 CompOption::Vector &options);

	static bool maximizeWin (CompAction         *action,
				 CompAction::State  state,
				 CompOption::Vector &options);

	static bool maximizeWinHorizontally (CompAction         *action,
					     CompAction::State  state,
					     CompOption::Vector &options);

	static bool maximizeWinVertically (CompAction         *action,
					   CompAction::State  state,
					   CompOption::Vector &options);

	static bool raiseWin (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector &options);
	
	static bool lowerWin (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector &options);

	static bool toggleWinMaximized (CompAction         *action,
					CompAction::State  state,
					CompOption::Vector &options);

	static bool toggleWinMaximizedHorizontally (CompAction         *action,
						    CompAction::State  state,
						    CompOption::Vector &options);

	static bool toggleWinMaximizedVertically (CompAction         *action,
					          CompAction::State  state,
					          CompOption::Vector &options);

	static bool shadeWin (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector &options);

	static void
	compScreenSnEvent (SnMonitorEvent *event,
			   void           *userData);

	static int checkForError (Display *dpy);
};

#endif