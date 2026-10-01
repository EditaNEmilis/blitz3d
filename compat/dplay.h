// Compatibility shim for the DirectPlay 4 headers removed from the Windows SDK.
//
// DirectPlay was dropped after DirectX 9: dplayx.dll and dplobby.dll are not
// present on Windows 10 or 11, and CLSID_DirectPlay is no longer registered.
// CoCreateInstance() therefore fails before any interface pointer is obtained,
// so no vtable is ever dereferenced. What follows declares just enough of the
// DirectPlay 4 surface for bbruntime's multiplayer code to compile, and makes
// the one entry point that is a free function fail cleanly.
//
// If you are targeting an OS that still ships dplayx.dll and want real
// networking, delete this directory from the include path and restore the
// genuine DirectX SDK headers instead.

#ifndef B3_COMPAT_DPLAY_H
#define B3_COMPAT_DPLAY_H

#include <windows.h>
#include <guiddef.h>

typedef DWORD DPID;
typedef GUID DPGUID;
typedef const DPGUID *LPCDPGUID;

#define DPID_SERVER			0
#define DPID_ALLPLAYERS		0
#define DPID_BROADCAST		0
#define DPID_SYSMSG			( (DPID)0xFFFFFFFF )
#define DPID_UNKNOWN		( (DPID)0xFFFFFFFF )

// DPLAY_ constants
#define DP_OK					( (HRESULT)0 )
#define DPERR_UNSUPPORTED		( (HRESULT)0x80004001L )
#define DPERR_INVALIDPARAMS		( (HRESULT)0x80070057L )
#define DPERR_NOINTERFACE		( (HRESULT)0x80004002L )
#define DPERR_NOTENABLED		( (HRESULT)0x80004005L )
#define DPERR_NOCONNECTION		( (HRESULT)0x80004004L )
#define DPERR_CONNECTFAILED		( (HRESULT)0x80004008L )
#define DPERR_CONNECTIONLOST	( (HRESULT)0x80004006L )
#define DPERR_SESSIONLOST		( (HRESULT)0x80004006L )
#define DPERR_USERCANCEL		( (HRESULT)0x8000400EL )
#define DPERR_BUFFERTOOSMALL	( (HRESULT)0x8007007AL )
#define DPERR_OUTOFMEMORY		( (HRESULT)0x8007000EL )
#define DPERR_GENERIC			( (HRESULT)0x80004005L )

// DirectPlay::Open
#define DPOpen_Join				0x1
#define DPOpen_Create			0x2
#define DPOpen_SessionJoin		0x4
#define DPOpen_SessionCreate	0x8
#define DPOpen_SessionHost		0x10

#define DPOPEN_JOIN				DPOpen_Join
#define DPOPEN_CREATE			DPOpen_Create
#define DPOPEN_SESSIONJOIN		DPOpen_SessionJoin
#define DPOPEN_SESSIONCREATE	DPOpen_SessionCreate
#define DPOPEN_SESSIONHOST		DPOpen_SessionHost

// DirectPlay::Send
#define DPSEND_GUARANTEED		0x1
#define DPSEND_NOSIGNAL			0x8
#define DPSEND_BROADCAST		DPSEND_GUARANTEED

// DirectPlay::Receive
#define DPRECEIVE_PEER			0x1

// DirectPlay::CreatePlayer
#define DPPLAYER_LOCAL			0x0
#define DPPLAYER_OWNER			0x1
#define DPPLAYER_HOST			0x2

// DirectPlay::EnumPlayers
#define DPENUMPLAYERS_ALL			0xFFFFFFFF
#define DPENUMPLAYERS_LOCALPLAYERS	1
#define DPENUMPLAYERS_REMOTEPLAYERS	2
#define DPENUMPLAYERS_GROUPPLAYERS	4
#define DPENUMPLAYERS_SPECTATORS	8

// DirectPlay::EnumConnections
#define DPCONN_ENUMALL			0xFFFFFFFF

// DirectPlay::EnumSessions
#define DPENUMSESSIONS_ASYNC	0x1
#define DPENUMSESSIONS_PASSIVE	0x4

// DPSESSIONDESC2::dwFlags
#define DPSESSION_KEEPALIVE				0x00000001
#define DPSESSION_MIGRATEHOST			0x00000002
#define DPSESSION_NOMESSAGEID			0x00000004
#define DPSESSION_BYIPADDR				0x00000008
#define DPSESSION_PRIVATE				0x00000020
#define DPSESSION_NOPERSIST			0x00000040
#define DPSESSION_PRESERVED			0x00000080
#define DPSESSION_OPTIMIZELATENCY		0x00010000
#define DPSESSION_DIRECTPLAYPROTOCOL	0x00020000
#define DPSESSION_DIRECTPLAYTCPIP		0x00040000
#define DPSESSION_DIRECTPLAYXNET		0x00080000
#define DPSESSION_DIRECTPLAYMODEM		0x00800000
#define DPSESSION_DIRECTPLAYLANMAN		0x01000000

// system messages delivered through DirectPlay::Receive
#define DPSYS_DESTROYPLAYERORGROUP	3
#define DPSYS_CREATEPLAYERORGROUP	2
#define DPSYS_SESSIONLOST				8
#define DPSYS_HOST						1

typedef struct _DPNAME {
	DWORD	dwSize;
	LPSTR	lpszShortNameA;
	LPWSTR	lpszShortNameW;
} DPNAME, *LPDPNAME, *LPCDPNAME;

typedef struct _DPSESSIONDESC {
	DWORD		dwSize;
	DPGUID		guidInstance;
	DPGUID		guidApplication;
	char *		lpszSessionNameA;
	WCHAR *		lpszSessionNameW;
	DWORD		dwFlags;
	DWORD		dwMaxPlayers;
	DWORD		dwCurrentPlayers;
	DWORD		dwUser1;
	DWORD		dwUser2;
	DWORD		dwUser3;
	DWORD		dwUser4;
} DPSESSIONDESC, *LPDPSESSIONDESC, *LPCDPSESSIONDESC;

typedef struct _DPSESSIONDESC2 {
	DWORD		dwSize;
	DPGUID		guidInstance;
	DPGUID		guidApplication;
	DWORD		dwMaxPlayers;
	DWORD		dwCurrentPlayers;
	DWORD		dwFlags;
	LPSTR		lpszSessionNameA;
	LPWSTR		lpszSessionNameW;
	DWORD		dwUser1;
	DWORD		dwUser2;
	DWORD		dwUser3;
	DWORD		dwUser4;
	DWORD		dwIncomingBandwidth;
	DWORD		dwOutgoingBandwidth;
	DWORD		dwIncomingLatency;
	DWORD		dwOutgoingLatency;
} DPSESSIONDESC2, *LPDPSESSIONDESC2, *LPCDPSESSIONDESC2;

typedef struct _DPMSG_DESTROYPLAYERORGROUP {
	DPID	dpId;
	DWORD	dwReason;
} DPMSG_DESTROYPLAYERORGROUP, *LPDPMSG_DESTROYPLAYERORGROUP;

typedef struct _DPMSG_CREATEPLAYERORGROUP {
	DPID		dpId;
	DPNAME		dpnName;
	DWORD		dwPlayerFlags;
	DWORD		dwHostPlayerId;
	DWORD		dwBroadcastBytesSent;
	DWORD		dwBroadcastBytesReturned;
	LPSTR		lpszLocalNameA;
	LPWSTR		lpszLocalNameW;
	DWORD		dwRemoteDataSize;
	LPVOID		lpRemoteData;
	DWORD		dwLocalDataSize;
	LPVOID		lpLocalData;
} DPMSG_CREATEPLAYERORGROUP, *LPDPMSG_CREATEPLAYERORGROUP;

typedef struct _DPMSG_HOST {
	DWORD	dwFlags;
} DPMSG_HOST, *LPDPMSG_HOST;

typedef struct _DPMSG_SESSIONLOST {
	DWORD	dwReason;
} DPMSG_SESSIONLOST, *LPDPMSG_SESSIONLOST;

typedef struct _DPSERVICE_PROVIDER_INFO {
	DWORD			dwSize;
	DPGUID			guidServiceProvider;
	LPSTR			lpszNameA;
	LPWSTR			lpszNameW;
	LPSTR			lpszPackedDescA;
	LPWSTR			lpszPackedDescW;
	DWORD			dwFlags;
} DPSERVICE_PROVIDER_INFO, *LPDPSERVICE_PROVIDER_INFO;

// CLSID_DirectPlay. Not registered on any modern Windows, so the CoCreateInstance
// in multiplay_setup.cpp fails and the whole feature reports itself unavailable.
DEFINE_GUID( CLSID_DirectPlay, 0x0b1c3010, 0x5d34, 0x11d0, 0x89, 0xc0, 0x00, 0xa0, 0xc9, 0x25, 0x8a, 0xde );
DEFINE_GUID( IID_IDirectPlay4A,   0x286f484d, 0x375e, 0x11d2, 0x85, 0x5f, 0x00, 0xa0, 0xc9, 0x25, 0x8a, 0xde );

typedef BOOL (FAR PASCAL *DPENUMPLAYERSCALLBACK)( DPID idPlayer,DWORD dwPlayerType,LPCDPNAME pdpPlayerName,DWORD dwFlags,LPVOID lpContext );
typedef BOOL (FAR PASCAL *DPENUMCONNECTIONSCALLBACK)( LPCDPGUID pguidSP,LPVOID pvConnection,DWORD cbConnection,LPCDPNAME pdpConnectionName,DWORD dwFlags,LPVOID lpContext );
typedef BOOL (FAR PASCAL *DPENUMSESSIONSCALLBACK)( LPCDPSESSIONDESC2 pdpSessionDesc,LPDWORD pdwTimeout,DWORD dwFlags,LPVOID lpContext );

// Declared abstract on purpose: with dplayx.dll absent there is no object to
// implement these, and no vtable layout to get wrong.
struct IDirectPlay4 : public IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Close() PURE;
	virtual HRESULT STDMETHODCALLTYPE InitializeConnection( LPVOID pvConnectionInfo,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE Open( LPDPSESSIONDESC2 pSessionDesc,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumPlayers( DPID idDevice,DPENUMPLAYERSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE CreatePlayer( DPID * pdpPlayerId,LPCDPNAME pdpName,DWORD dwFlags,DWORD dwHostPlayerId,DWORD dwBroadcastBytesSent,DWORD dwBroadcastBytesReturned ) PURE;
	virtual HRESULT STDMETHODCALLTYPE DestroyPlayer( DPID idPlayer ) PURE;
	virtual HRESULT STDMETHODCALLTYPE Receive( DPID * pdpSender,DPID * pdpPlayer,DWORD dwFlags,LPVOID pvBuffer,LPDWORD pdwDataSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE Send( DPID idPlayer,DPID idRecipient,DWORD dwFlags,LPVOID pvBuffer,DWORD dwDataSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumConnections( LPCDPGUID pguidSessionType,DPENUMCONNECTIONSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumSessions( LPCDPSESSIONDESC2 pSessionDesc,LPCDPGUID pguidApplication,DPENUMSESSIONSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
};

#endif
