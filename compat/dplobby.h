// Compatibility shim for dplobby.h, which was removed from the Windows SDK
// together with DirectPlay. See dplay.h in this directory for the full story.
//
// DirectPlayLobbyCreate() lived in dplobby.dll. That DLL is not present on
// Windows 10 or 11, so the genuine entry point can never succeed; the inline
// stub below reports DPERR_UNSUPPORTED and the caller bails out cleanly.

#ifndef B3_COMPAT_DPLOBBY_H
#define B3_COMPAT_DPLOBBY_H

#include "dplay.h"

// service provider GUIDs
DEFINE_GUID( DPSPGUID_TCPIP,    0xEBFE7BA0, 0x628A, 0x11D1, 0x89, 0xC0, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( DPSPGUID_IPX,      0xE100ABF0, 0x628A, 0x11D1, 0x89, 0xC0, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( DPSPGUID_MODEM,    0x743F3DC2, 0x628A, 0x11D1, 0x89, 0xC0, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( DPSPGUID_SERIAL,   0x5DE4C3D0, 0x628A, 0x11D1, 0x89, 0xC0, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( DPSPGUID_BROADCAST, 0x218A2DC1, 0x628A, 0x11D1, 0x89, 0xC0, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( DPSPGUID_LANMAN,   0xB51AA10A, 0x628A, 0x11D1, 0x89, 0xC0, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( DPSPGUID_DIRECTPLAY,0x0A781A1D, 0x628A, 0x11D1, 0x89, 0xC0, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( DPSPGUID_XNET,      0xF150DA0C, 0x628A, 0x11D1, 0x89, 0xC0, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );

// DPAID_ address types
#define DPAID_NULL				0x00000000
#define DPAID_INet				0x00000001
#define DPAID_LANMAN			0x00000002
#define DPAID_PHYSICALNET		0x00000004
#define DPAID_DIRECTPLAY			0x00000008
#define DPID_NOLOBBY			( ( DWORD )0xFFFFFFFF )

DEFINE_GUID( CLSID_DirectPlayLobby, 0xFD9C31E0, 0x5C8B, 0x11D1, 0x8B, 0x80, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( IID_IDirectPlayLobby,   0xFC8700A1, 0x5C8B, 0x11D1, 0x8B, 0x80, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( IID_IDirectPlayLobby2,  0xC6651FF0, 0x5CC6, 0x11D1, 0x8B, 0x81, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( IID_IDirectPlayLobby3,  0xFD980C1B, 0x5C8B, 0x11D1, 0x8B, 0x80, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );
DEFINE_GUID( IID_IDirectPlayLobby4, 0x0C1CD409, 0x1D68, 0x11D1, 0x8F, 0x63, 0x00, 0xA0, 0xC9, 0x25, 0x8A, 0xDE );

typedef struct _DPADDRESS {
	DWORD	dwSize;
	DPGUID	guidServiceProvider;
	DWORD	dwFlags;
	WCHAR *	lpszNameW;
	CHAR *	lpszNameA;
	WORD	wPlayerNameSize;
	WCHAR *	lpszPlayerNameW;
	CHAR *	lpszPlayerNameA;
	DWORD	dwFlags2;
} DPADDRESS, *LPDPADDRESS;

struct IDirectPlayLobby : public IUnknown {
	virtual HRESULT STDMETHODCALLTYPE EnumConnections( LPCGUID pguidCT,DPENUMCONNECTIONSCALLBACK lpCallback,LPVOID pvContext,DWORD dwFlags ) PURE;
};

struct IDirectPlayLobby2 : public IDirectPlayLobby {
	virtual HRESULT STDMETHODCALLTYPE RegisterServer( LPCDPSESSIONDESC pdpSessionDesc,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE UnregisterServer( LPCDPSESSIONDESC pdpSessionDesc,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetServiceProviderInfo( LPCDPGUID pguidSP,LPDPSERVICE_PROVIDER_INFO pdpProviderInfo ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetServiceProviderInfo( LPCDPGUID pguidSP,LPDPSERVICE_PROVIDER_INFO pdpProviderInfo ) PURE;
};

struct IDirectPlayLobby3 : public IDirectPlayLobby2 {
	virtual HRESULT STDMETHODCALLTYPE QueryServiceProviderGUID( LPCDPGUID pguidProvider,DPGUID * pguidResult ) PURE;
	virtual HRESULT STDMETHODCALLTYPE CreateAddress( LPCDPGUID pguidProvider,DWORD dwCost,LPCVOID pvInput,DWORD dwSize,LPVOID pvOutput,LPDWORD pdwSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetAddressString( LPCDPGUID pguidProvider,LPVOID pvAddress,DWORD dwAddressSize,LPDWORD pdwSize ) PURE;
};

// Real dplobby.dll entry point. Not present on modern Windows, so always fail.
inline HRESULT DirectPlayLobbyCreate( GUID * guidInstance,IDirectPlayLobby ** ppvLobby,REFIID iid,void * pvOuter,LPUNKNOWN punkOuter ){
	return DPERR_UNSUPPORTED;
}

#endif
