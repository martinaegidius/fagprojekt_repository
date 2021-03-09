#include	"epwin.h"	/* our abridged version of windows.h	*/
#include	"stdio.h"
#include	"math.h"
#include	"resource.h"
#include	"api.h"

#define	PLOT_TIMER	1
#define	INIT_GR		1
#define	DEG_TO_RAD	0.017453293

#define decade(a)	(int)(pow(10.0,(double)((int)log10((double)a)))+0.1)

/* The Programming-Interface messages for EasyPlot...		*/
#define	API_READY		70
#define	API_REMOVE_FILE		71
#define	API_GET_XTICD		74
#define	API_GET_XMAJS		75
#define	API_SHFT_X		76
#define	API_SHFT_X_NOUPDATE	77
#define	API_LOCK_YAXIS		78
#define	API_RUN_FILE		80
#define	API_CLOSE		81
#define	API_ADD_DATA		82
#define	API_SET_CURVE		83
#define	API_NEW_DATA		84
/**/

	HANDLE	hInst;		/* current instance              */
	HANDLE	hWnd;		/* Main window handle.           */
	HANDLE	hDlg;		/* Main window handle.           */
	HANDLE	hTerm;		/* terminal win handle           */
	HANDLE	par_wh;
	HANDLE	ccurs;
 
	FILE	*tmp;
	char	simp_wc_nm[] = "simp";

extern  HANDLE	tfont;
	RECT	desk_rect;

	char	run_cap[]  = "EasyPlot - Plotting Real-Time Data sent by API DEMO...";
	char	stop_cap[] = "EasyPlot - API DEMO stopped";

	char	epmod[200];	/* path to EasyPlot epw32.exe		*/
	char	runfile[200];   /* file used to send data to EP		*/

	char	binary  = 1;	/* send data to EP in binary format	*/
	int	miss_ct = 0;	/* xferrs missed due to system overload	*/
long	int	ptct;		/* running point count (X-axis=Pt #)	*/
long	int	ptn0;		/* 1st point # in current data set	*/
long	int	xmin, xmax;	/* min & max for X-axis			*/

	int	segs = 50;	/* fit N xferrs on X-axis (def=50)	*/
	int	adct = 100;	/* points sent per xferr (def=100)	*/
	int	msec = 100;	/* msecs between xfers to EP (def=100)	*/

	UINT	timer_id;	/* timer tells demo to send data to EP	*/
	HANDLE	epwh;		/* handle of EasyPlot window		*/
	PROCESS_INFORMATION	pi;

LRESULT CALLBACK MainWndProc(	HWND, UINT, WPARAM, LPARAM);
BOOL	CALLBACK dbox_fcn(	HWND, UINT, WPARAM, LPARAM);
BOOL	CALLBACK enum_epwh(	HWND, LPARAM);	
 
/***********************************************************************/
/*  procedure:  WinMain                                                */
/***********************************************************************/
int PASCAL WinMain( HANDLE hInstance, HANDLE hPrevInst, LPSTR lpCmdln, int nCmdShow)
{
  char		*cp;
  MSG		msg;
  FARPROC	pri;
 
  #if (_DEBUG)
  putenv( "DBG=1");
  if (tmp = fopen( cp="e:\\tmp", "w")) {
    fprintf( tmp, " \n");
    pp( "TMP=%s\n", cp);
    }
  #endif

  if (!hPrevInst)
    if (!InitApplication(hInstance))
      return(0);

  hInst = hInstance;
  GetWindowRect( GetDesktopWindow(), &desk_rect);
  hWnd = CreateWindowEx( 0L, simp_wc_nm, "EasyPlot API Demo", WS_OVERLAPPEDWINDOW, 
			10, 10, 320, 300, NULL, (HMENU)NULL, hInst, NULL );
  if (!hWnd)
    return(0);

  pri = MakeProcInstance( dbox_fcn, hInst);	/* use pri for compatibility w/ 16-bit world	*/
  DialogBox( hInst, "MAINDB", hWnd, pri);
  FreeProcInstance( pri);

  term_appl();  
  while (GetMessage(&msg, NULL, NULL, NULL)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
  DestroyWindow( hWnd);
  return(msg.wParam);
}
 
/***********************************************************************/
/*  procedure:  initapplication                                        */
/***********************************************************************/
BOOL InitApplication( HANDLE hInst)
{
  int       ct, ret;
  char	    *cp;
  WNDCLASS  wc;
 
  for (ct=0, cp=(char *)&wc; ct < sizeof(WNDCLASS); cp[ct]=0, ct++);
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc   = MainWndProc;
  wc.hInstance     = hInst;
  wc.lpszClassName = simp_wc_nm;
  wc.hIcon	   = LoadIcon( hInst, "epicon");
  return( RegisterClass(&wc));
}
 
/***********************************************************************/
/*  procedure:  term_appl                                              */
/***********************************************************************/
term_appl()
{
  term_kid();
  PostQuitMessage(0);
  if (tmp) {
    fclose( tmp);
    tmp = 0;
    }
  if (hTerm) {
    DeleteObject( tfont);
    DestroyWindow( hTerm);
    hTerm = 0;
    }
}

/************************************************************************/
/*  procedure:  term_kid						*/
/************************************************************************/
term_kid()
{
  if (pi.hProcess) {
    if (epwh)			/* if we can terminate gracefully, ...	*/
      SendMessage( epwh, WM_COMMAND, API_CLOSE, 0);
    else			/* otherwise just kill the process	*/
      TerminateProcess( pi.hProcess, 0);
    pi.hProcess = epwh = 0;
    }
}

/************************************************************************/
/*  procedure:  stop_timer						*/
/************************************************************************/
stop_timer()
{
  int	htid;

  if (htid = timer_id) {
    miss_ct = 0;
    timer_id = 0;
    KillTimer( hWnd, htid);
    SetDlgItemText( hDlg, START_STOP, "Start");
    }
  return( htid);
}

/************************************************************************/
/*  procedure:  gen_data_for_ep						*/
/************************************************************************/
gen_data_for_ep( int init, char * fname)
{
  int     ct;
  float   xsc, pt[2];
  FILE    *fp;

  if (fp = fopen( fname, "w")) {
    if (init) {
      fprintf( fp, "/fl\n");			/* close all graphs	*/
      fprintf( fp, "/sm OFF\n");		/* data markers off	*/
      fprintf( fp, "/sd OFF\n");		/* dashed lines off	*/
      fprintf( fp, "/sc ON\n");			/* connecting lines on	*/
      fprintf( fp, "/or y -2 2\n");		/* set Y-range		*/
      xmin = 0;
      xmax = adct * segs;
      fprintf( fp, "/or x %d %d\n", xmin, xmax);/* set X-range		*/
      }
    if (binary) {
      fclose( fp);
      fp = fopen( fname, "a+b");		/* reopen the file in binary mode */
      fprintf( fp, "//binary 2 %d\n", adct);
      }
    xsc = (adct > 100) ? 1.0 / (1.0 + (adct/100-1)) : 1;
    for (ct = 0; ct < adct; ct++, ptct++) {
      pt[0] = ptct;
      pt[1] = sin( ptct * DEG_TO_RAD * xsc);
      if (binary)
	fwrite( pt, sizeof(float), 2, fp);
      else
	fprintf( fp, "%ld %g\n", ptct, pt[1]);
      }
    fprintf( fp, "\n");
    fclose( fp);
    }
}

/************************************************************************/
/*  procedure:  init_graph						*/
/************************************************************************/
init_graph()
{
  int	ct;

  if (epwh) {
    for (ct=0; _access( runfile, 0)!=-1 && ct < 100; ct++)
      Sleep( 20);			/* if runfile exists, give ep time to finish last RUN/REM */
    ptct = ptn0 = 0;
    gen_data_for_ep( INIT_GR, runfile);
    for (ct=0; !SendMessage(epwh, WM_COMMAND, API_READY, 0) && ct < 100; ct++)
      Sleep( 20);			/* give ep time to initialize if needed */
    PostMessage( epwh, WM_COMMAND, API_RUN_FILE, 0);
    PostMessage( epwh, WM_COMMAND, API_SET_CURVE, 1);
    PostMessage( epwh, WM_COMMAND, API_REMOVE_FILE, 0);
    }
}

/***********************************************************************/
/*  procedure:  mainwndproc                                            */
/***********************************************************************/
long FAR PASCAL MainWndProc( HWND hw, UINT message, WPARAM wParam, LPARAM lParam)
{
  int		ct, ret, do_def, replace, add_op, dx, pct, majs;
  long  int	wret, lsh;
  RECT		cr;
  HDC		dc;
  PAINTSTRUCT	ps;
  float		xshft, ticd;
 
  wret   = 0;
  do_def = 0;
  switch (message) {
    case WM_NCLBUTTONDBLCLK:
      if (hw == hTerm)
        clear_term();
      break;
    case WM_MOUSEMOVE:
      SetCursor( LoadCursor( 0, IDC_ARROW));
      break;
    case WM_TIMER:
      if (epwh && timer_id && wParam == PLOT_TIMER) {
	if (_access( runfile, 0)==-1) {
	  if (miss_ct)
	    pp( "Missed %d timer%s at X=%2.1f%s\n", miss_ct, (miss_ct>1)?"s":"  ", (float)ptct/(1+ (ptct>1000)*999), (ptct>1000)?"K":"");
	  gen_data_for_ep( 0, runfile);
	  replace = miss_ct = 0;
	  add_op  = API_ADD_DATA;
	  if (ptct > xmax) {	//if data goes off graph, shift X-axis range
	    if (ptct > 5000000) {
	      pp( "Reinit; keep X < 5 Million\n", ptct);
	      remove( runfile);
	      init_graph();
	      break;
	      }
	    majs       = SendMessage(epwh, WM_COMMAND, API_GET_XMAJS, 0);
	    (long)ticd = SendMessage(epwh, WM_COMMAND, API_GET_XTICD, 0);
	    pct = ptct - ptn0;
	    dx  = xmax - xmin;
	    xshft = (majs > 1)? majs * ticd : dx * 0.8;
	    if (replace = (pct > dx*(5 << (pct<1000)))) { //if > 5/10 graphs widths of data, clear old data
	      pp( "Removing %2.1fK old pts\n", (float)(ptct-ptn0)/1000.0);
	      ptn0 = ptct;
	      add_op  = API_NEW_DATA;
	      PostMessage( epwh, WM_COMMAND, API_LOCK_YAXIS, 1);
	      }
	    (float)lsh = xshft;		// '(LPARAM)(*(long *)&xshft)' also works for lParam
	    PostMessage( epwh, WM_COMMAND, API_SHFT_X+replace, (LPARAM)((float)lsh));
	    xmin += xshft;
	    xmax += xshft;
	    }
	  PostMessage( epwh, WM_COMMAND, add_op, 0);
	  PostMessage( epwh, WM_COMMAND, API_REMOVE_FILE, 0);
	  if (replace)
	    PostMessage( epwh, WM_COMMAND, API_LOCK_YAXIS, 0);
	  }
	else
	  miss_ct++;
	}
      break;
    case WM_QUIT:
    case WM_CLOSE:
      term_appl();
      break;
    case WM_PAINT:
      if (hw == hTerm) {
	GetClientRect( hw, (LPRECT)&cr);
	InvalidateRect( hw, (LPRECT)&cr, 0);
	dc = BeginPaint( hw, &ps);
	term_paint( dc);
	EndPaint( hw, &ps);
	}
      break;
    default:                          /* Passes it on if unproccessed    */
      do_def = 1;
    }
  if (do_def)
    wret = DefWindowProc(hw, message, wParam, lParam);
  return( wret);
}
 
/***********************************************************************/
/*  procedure:	enum_get_epwinh					       */
/***********************************************************************/
BOOL CALLBACK enum_epwh( HWND hwnd, LPARAM lParam)
{
  epwh = hwnd;
  return( 1);
}

/***********************************************************************/
/*  procedure:	spawn_epw32					       */
/***********************************************************************/
spawn_epw32()
{
  int		ct, ret, ok;
  char		dir[202], *cp;
  FILE		*fp;
  STARTUPINFO	si;

  term_kid();
  for (ct=0, cp=(char *)&pi; ct < sizeof(PROCESS_INFORMATION); cp[ct]=0, ct++);
  for (ct=0, cp=(char *)&si; ct < sizeof(STARTUPINFO);	       cp[ct]=0, ct++);
  si.cb = sizeof( STARTUPINFO);
  si.dwX = 150;
  si.dwY = 300;
  si.dwXSize = 600;
  si.dwYSize = 400;
  si.dwFlags = STARTF_USEPOSITION | STARTF_USESIZE;

  ct = GetModuleFileName( hInst, dir, 200);
  for (ct-=2; ct>0 && dir[ct]!=':' && dir[ct]!='\\'; dir[ct]=0, ct--);
  #if (_DEBUG)
    copy_str( "e:\\vcp\\", dir);
  #endif
  sprintf( epmod,   "%sepw32.exe", dir);
  sprintf( runfile, "%sep_data",   dir);
  pp( "runfile:  %s\n", runfile);

  epwh = 0;
  sprintf( dir, "%s -run=%s -ret=%u '-cap=EasyPlot Running API DEMO'", epmod, runfile, hWnd);
  if (ok = CreateProcess( epmod, dir, 0, 0, 0, DETACHED_PROCESS, 0, 0, &si, &pi)) {
    for (ct=0; !epwh && ct < 100; ct++) {
      Sleep( 20);			// give ep time to create its window
      EnumThreadWindows( pi.dwThreadId, enum_epwh, 0);
      }
    if (epwh) {
      AttachThreadInput( pi.dwThreadId, GetCurrentThreadId(), 1);
      EnableWindow( epwh, 0);
      }
    else {
      term_kid();
      pp( "Failed to get EP window handle.\n");
      }
    }
  else
    pp( "Couldn't find <%s>\n", epmod);
  return( ok);
}

/***********************************************************************/
/*  procedure:	dbox_fcn 					       */
/***********************************************************************/
BOOL CALLBACK dbox_fcn( HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  int		ct, act, sta, *avp, restart;
  char		str[20];
  DWORD		exitc;
  BOOL		ret;
  UINT		ncod, ctlid;
  static char	tmr_only;

  ret = 1;
  switch (message) {
    case WM_COMMAND:
      #if (WIN32)
        ncod  = HIWORD(wParam);
        ctlid = LOWORD(wParam);
      #else
        ncod  = HIWORD(lParam);
        ctlid = wParam;
      #endif
      switch (ctlid) {
	case CLOSE_EPW:
	  if (pi.hProcess) {
	    stop_timer();
	    term_kid();
	    }
	  break;
	case START_STOP:
	  if (!stop_timer()) {
	    if (!GetExitCodeProcess( pi.hProcess, &exitc) || exitc!= STILL_ACTIVE) {
	      TerminateProcess( pi.hProcess, 0);
	      pi.hProcess = epwh = 0;
	      }
	    if (!pi.hProcess)
	      spawn_epw32();
	    if (!tmr_only)
	      init_graph();
	    tmr_only = 0;
	    pp( "\n");
	    if (pi.hProcess && epwh) {
	      pp( "Plotting %1.1fKpts/sec  (%d/%dms)\n", (adct * 1000.0/msec)/1000.0, adct, msec);
	      timer_id = SetTimer( hWnd, PLOT_TIMER, msec, NULL);
	      }
	    }
	  EnableWindow( epwh, (timer_id == 0));	/* if running, don't let user click on EP	*/
	  SetDlgItemText( hDlg, START_STOP, (timer_id) ? "Stop" : "Start");
	  SendMessage( epwh, WM_SETTEXT, 0, (LPARAM)((timer_id) ? run_cap : stop_cap));
	  break;
        case ADDCT:
        case MSECS:
        case SEGCT:
	  if (ncod == CBN_SELCHANGE) {
	    ct = SendDlgItemMessage( hdlg, ctlid, CB_GETCURSEL, 0, 0);
	    SendDlgItemMessage( hdlg, ctlid, CB_GETLBTEXT, ct, (LPARAM)str);
	    ct = atoi( str);
	    restart  = stop_timer();
	    tmr_only = (ctlid == MSECS);
	    if (ctlid == MSECS) msec = ct;
	    if (ctlid == ADDCT) adct = ct;
	    if (ctlid == SEGCT) segs = ct;
	    if (restart)
	      SendMessage( hdlg, WM_COMMAND, START_STOP, 0);
	    }
	  break;
        case ASCII:
	  binary = 0;
	  pp( "ASCII\n");
	  break;
        case BINARY:
	  binary = 1;
	  pp( "Binary\n");
	  break;
	case IDCANCEL:		/* from <esc> key	*/
	  if (timer_id) {
	    SendMessage( hdlg, WM_COMMAND, START_STOP, 0);
	    break;
	    }			/* otherwise exit ...	*/
	case IDEXIT:
	  stop_timer();
	  EndDialog( hdlg, NULL);
	  break;
	default:
	  ret = 0;
	  break;
	}
      break;
    case WM_CLOSE:
      EndDialog( hdlg, NULL);
      break;
    case WM_INITDIALOG:
      hDlg = hdlg;
      remove( runfile);
      init_term( hdlg);

      for (ct = 2; ct < 10001; ct += decade(ct)) {
        sprintf( str, "%d", ct);
	if (ct >=10)		SendDlgItemMessage( hdlg, ADDCT, CB_ADDSTRING, 0, (LPARAM)str);
	if (ct < 101)		SendDlgItemMessage( hdlg, SEGCT, CB_ADDSTRING, 0, (LPARAM)str);
	if (ct >=10 && ct<1001)	SendDlgItemMessage( hdlg, MSECS, CB_ADDSTRING, 0, (LPARAM)str);
	if (msec == ct) SendDlgItemMessage( hdlg, MSECS, CB_SELECTSTRING, -1, (LPARAM)str);
	if (adct == ct) SendDlgItemMessage( hdlg, ADDCT, CB_SELECTSTRING, -1, (LPARAM)str);
	if (segs == ct) SendDlgItemMessage( hdlg, SEGCT, CB_SELECTSTRING, -1, (LPARAM)str);
	}
      SendDlgItemMessage( hdlg, ASCII,  BM_SETCHECK, (!binary), 0);
      SendDlgItemMessage( hdlg, BINARY, BM_SETCHECK,   binary, 0);

      if (getenv( "DBG"))
	PostMessage( hdlg, WM_COMMAND, START_STOP, 0);
      break;
    default:
      ret = 0;
      break;
    }
  return( ret);
}
