
// Not HTMLHELP_H: afxhtml.h pulls in the Windows SDK's <htmlhelp.h>, which uses
// that same guard, so sharing it makes this whole file vanish.
#ifndef B3_HTMLHELP_H
#define B3_HTMLHELP_H

// <htmlhelp.h> also does "#define HtmlHelp HtmlHelpA/W", which would rename this
// class out from under itself. Blitz3D never calls the Win32 HtmlHelp() API.
#undef HtmlHelp

class HtmlHelp;

class HelpListener{
public:
	virtual void helpOpen( HtmlHelp *help,const string &file )=0;
	virtual void helpTitleChange( HtmlHelp *help,const string &title )=0;
};

class HtmlHelp : public CHtmlView{
public:
	HtmlHelp( HelpListener *l ):listener(l){}

	string getTitle();

DECLARE_DYNAMIC( HtmlHelp )
DECLARE_MESSAGE_MAP()

	afx_msg BOOL OnEraseBkgnd( CDC *dc );

private:
	virtual void OnTitleChange( LPCTSTR t );
	virtual void OnBeforeNavigate2( LPCTSTR lpszURL, DWORD nFlags, LPCTSTR lpszTargetFrameName, CByteArray& baPostedData, LPCTSTR lpszHeaders, BOOL* pbCancel );

	string title;
	HelpListener *listener;
};

#endif
