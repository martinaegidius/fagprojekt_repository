#include	"platform.h"
#include	"epwin.h"
#include	"stdio.h"

#define	TLINES	20

extern	HANDLE	hInst;
extern	HWND	hTerm;
extern	char	simp_wc_nm[];
extern	FILE	*tmp;
extern	RECT	desk_rect;

static	char	tscrn[TLINES][80];
static	int	cln = 10;
	HFONT	tfont;
	char	pp_fonly = 0;
	char	pp_off   = 0;

/***********************************************************************/
/*  procedure:	ltr_sch 					       */
/***********************************************************************/
ltr_sch( str, ltr)
  char	  str[];
  char	  ltr;
{
  int	  ct, ret;

  for (ct = 0; str[ct] && str[ct] != ltr; ct++);
  ret = (str[ct] == ltr) ? ct : -1;
  return( ret);
}

/***********************************************************************/
/*  procedure:	lcase_ltr					       */
/***********************************************************************/
lcase_ltr( ltr)  char  ltr;  {
  if (ltr >= 0x41 && ltr <= 0x5a)
    ltr += 0x20;
  return( ltr);
}

/***********************************************************************/
/*  procedure:	str_len 					       */
/***********************************************************************/
str_len( str) char str[];
{ int	  ct;
  for (ct = 0; str[ct]; ct++);
  return( ct+1);
}

/***********************************************************************/
/*  procedure:	str_lenm1					       */
/***********************************************************************/
str_lenm1( str) char str[];
{ int	  ct;
  for (ct = 0; str[ct]; ct++);
  return( ct);
}

/***********************************************************************/
/*  procedure:	str_cmp 					       */
/***********************************************************************/
str_cmp( s1, s2)
  char	  s1[];
  char	  s2[];
{
  int	  ct;
  int	  ret;

  if (!s1 && !s2)
    ret = 1;
  else if (!s1 || !s2)
    ret = 0;
  else {
    for (ct = 0; s1[ct] && s2[ct] && s1[ct] == s2[ct]; ct++) {}
    ret = (!s1[ct] && !s2[ct]);
    }
  return( ret);
}

/***********************************************************************/
/*  procedure:	copy_str					       */
/***********************************************************************/
copy_str( src, dest)
  char	  src[];
  char	  dest[];
{
  int	  ct;

  ct = 0;
  if (dest) {
    if (!src) src = "";
    for (ct = 0; dest[ct] = src[ct]; ct++);
    }
  return( ct);
}

/***********************************************************************/
/*  procedure:	last_char					       */
/***********************************************************************/
last_char( str)  char  str[];
{ int	  ct;
  ct = str_lenm1( str);
  return( str[ct-1]);
}

/***********************************************************************/
/*  procedure:	set_last_char					       */
/***********************************************************************/
set_last_char( str, ltr)
  char	str[];
  char	ltr;
{
  int	  ct;

  ct = str_lenm1( str);
  if (ct > 0)
    str[ct-1] = ltr;
}

/***********************************************************************/
/*  procedure:	init_term					       */
/***********************************************************************/
init_term( wh_par)
  HWND	     wh_par;
{
  RECT	     rect;

  GetWindowRect( wh_par, (LPRECT)&rect);
  hTerm = CreateWindowEx( 0L, simp_wc_nm, "EPI terminal", WS_OVERLAPPED | WS_THICKFRAME,
		  rect.rt, rect.top, rect.rt-rect.lft, rect.bot-rect.top, 0, (HMENU)0, hInst,  0);
  tfont = GetStockObject( SYSTEM_FONT);
  ShowWindow( hTerm, SW_SHOW);
}

/***********************************************************************/
/*  procedure:	term_paint					       */
/***********************************************************************/
term_paint( HDC dc)
{
  int	     ct, n, ret;
  int	     xc, yc, dy;
  RECT	     rect;
  HANDLE     br, obr, ofnt;
  TEXTMETRIC tm;

  if (hTerm) {
    br	= CreateSolidBrush( RGB( 130, 130, 0));
    obr = SelectObject( dc, br);
    GetClientRect( hTerm, (LPRECT)&rect);
    Rectangle( dc, 0, 0, rect.rt, rect.bot);
    SelectObject( dc, obr);
    DeleteObject( br);
    SetBkMode( dc, TRANSPARENT);
    ofnt = SelectObject( dc, tfont);
    GetTextMetrics( dc, (LPTEXTMETRIC)&tm);
    dy = tm.tmHeight - 3;
    xc = rect.lft + 10;
    yc = rect.top + 1;
    n  = cln;
    for (ct = 0; ct < TLINES; ct++) {
      TextOut( dc, xc, yc, tscrn[n], strlen( tscrn[n]));
      if (n == 0)
	n = TLINES;
      n--;
      yc += dy;
      }
    SelectObject( dc, ofnt);
    }
}

/***********************************************************************/
/*  procedure:	pp						       */
/***********************************************************************/
pp( char * fmt, char p1)
{
  char	 ct, sn, bn, fn, hld_et, hld;
  char	 *sp, *cp;
  char	 ltr, nltr, lng, done;
  int	 *ip;
  long	 int	*lip;
  double *dp;
  char	 str[400];
  static char lfmt[14] = "%s%-";
  static char fchs[] = "dxcsgf";

  if (pp_off)
    return(0);
  sp = &p1;
  str[0] = 0;
  for (ct = bn = 0; fmt[ct]; ct++) {
    ltr  = fmt[ct];
    if (ltr == '%' && fmt[ct+1]) {
      for (ct++, fn=3, done=0; fmt[ct] && !done; ct++) {
	nltr = lcase_ltr( fmt[ct]);
	done = (nltr==' ' || (ltr_sch( fchs, nltr) >= 0));
	if (fn < 12)
	  lfmt[fn++] = nltr;
	}
      ct--;
      lfmt[fn] = 0;
      #if (!WIN32)
        lng = (lfmt[3] == 'l');
      #endif
      switch (nltr) {
	case 'd':
	case 'x':
	  #if (!WIN32)
	  if (lng) {
	    lip = (long int *)sp;
	    sprintf( str, lfmt, str, *lip);
	    sp += 4;
	    break;
	    }
	  #endif
	case 'c':
	case 's':
	  ip = (int *)sp + 1 - 1;
	  cp = (char *)*ip;
	  hld = 0;
	  if (nltr == 's' && cp && str_len(cp) > 100) {
	    hld = cp[100];
	    cp[100] = 0;
	    }
	  sprintf( str, lfmt, str, *ip);

	  if (hld)
	    cp[100] = hld;
	  sp += sizeof(int);
	  break;
	case 'f':
	case 'g':
	  dp = (double *)sp;
	  sprintf( str, lfmt, str, *dp);
	  sp += 8;
	  break;
	default:
	 str[bn++] = nltr;
	}
      bn = str_lenm1( str);
      }
    else {
      str[bn++] = ltr;
      str[bn] = 0;
      }
    }

  cln++;
  if (cln >= TLINES)
    cln = 0;
  if (tmp) {
    fprintf( tmp, "%s", str);
    fflush( tmp);
    }
  if (last_char( str)==0xd || last_char( str)==0xa)
    set_last_char( str, 0);
  str[78] = 0;
  copy_str( str, tscrn[cln]);

  if (hTerm && !pp_fonly) {
    InvalidateRect( hTerm, 0, 0);
    UpdateWindow( hTerm);
    }
}

/***********************************************************************/
/*  procedure:	clear_term					       */
/***********************************************************************/
clear_term()
{
  int	 ct;

  for (ct = 0; ct < TLINES; ct++)
    tscrn[ct][0] = 0;
  cln = 0;
  InvalidateRect( hTerm, NULL, 0);
}
