// The DirectPlay lobby, written from the published interface.
//
// The lobby is the half of DirectPlay that a game uses to find other games:
// it builds the network addresses, it holds the list of sessions, and it
// launches the game on another machine. The interface is the one the DirectX
// SDK declared, and the vtable order below is the one dplayx.dll implements:
// the IUnknown methods, then IDirectPlayLobby's own methods, then
// IDirectPlayLobby2's, then IDirectPlayLobby3's, each group in the order the
// SDK declared them.
//
// The lobby client is in dplayx.dll, not in dplobby.dll: dplobby.dll was the
// lobby server, and it went away with the SDK. dplayx.dll still exports
// DirectPlayLobbyCreateA, and the runtime calls that directly, because
// CLSID_DirectPlayLobby is not registered either.
//
// The entry point's signature is worth stating, because it is not the one the
// old shim guessed: it takes the interface to ask for nowhere. It always
// hands back IDirectPlayLobby, and IDirectPlayLobby3 is reached from there
// with QueryInterface.

#ifndef B3_COMPAT_DPLOBBY_H
#define B3_COMPAT_DPLOBBY_H

#include "dplay.h"

typedef struct _DPLAPPINFO{
	DWORD dwSize;
	GUID guidApplication;
	union{ LPWSTR lpszAppName; LPSTR lpszAppNameA; };
} DPLAPPINFO,*LPDPLAPPINFO,*LPCDPLAPPINFO;

typedef struct _DPCOMPOUNDADDRESSELEMENT{
	GUID guidDataType;
	DWORD dwDataSize;
	LPVOID lpData;
} DPCOMPOUNDADDRESSELEMENT,*LPDPCOMPOUNDADDRESSELEMENT,*LPCDPCOMPOUNDADDRESSELEMENT;

typedef struct _DPAPPLICATIONDESC{
	DWORD dwSize;
	DWORD dwFlags;
	union{ LPWSTR lpszApplicationName; LPSTR lpszApplicationNameA; };
	GUID guidApplication;
	union{ LPWSTR lpszFilename; LPSTR lpszFilenameA; };
	union{ LPWSTR lpszCommandLine; LPSTR lpszCommandLineA; };
	union{ LPWSTR lpszPath; LPSTR lpszPathA; };
	union{ LPWSTR lpszCurrentDirectory; LPSTR lpszCurrentDirectoryA; };
	LPSTR lpszDescriptionA;
	LPWSTR lpszDescriptionW;
} DPAPPLICATIONDESC,*LPDPAPPLICATIONDESC;

typedef BOOL (FAR PASCAL *DPENUMADDRESSCALLBACK)( REFGUID guidDataType,DWORD dwDataSize,LPCVOID lpData,LPVOID lpContext );
typedef BOOL (FAR PASCAL *DPENUMADDRESSTYPESCALLBACK)( REFGUID guidDataType,LPVOID lpContext,DWORD dwFlags );
typedef BOOL (FAR PASCAL *DPENUMLOCALAPPLICATIONSCALLBACK)( LPCDPLAPPINFO lpAppInfo,LPVOID lpContext,DWORD dwFlags );

//The header of a block of address data.
typedef struct _DPADDRESS{
	GUID guidDataType;
	DWORD dwDataSize;
} DPADDRESS,*LPDPADDRESS;

//IDirectPlayLobby, and the two later versions of it. Only the lobby's
//methods are here, but they are all here: a call that lands one slot out
//lands in the middle of somebody else's function.
struct IDirectPlayLobby : public IUnknown{
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,LPVOID *ppv ) PURE;
	virtual ULONG STDMETHODCALLTYPE AddRef() PURE;
	virtual ULONG STDMETHODCALLTYPE Release() PURE;

	virtual HRESULT STDMETHODCALLTYPE Connect( DWORD dwFlags,LPVOID ppDP,IUnknown *pUnk ) PURE;
	virtual HRESULT STDMETHODCALLTYPE CreateAddress( REFGUID pguidSP,REFGUID guidAddressType,LPCVOID pvData,DWORD dwDataSize,LPVOID pvAddress,LPDWORD pdwAddressSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumAddress( DPENUMADDRESSCALLBACK lpCallback,LPCVOID lpContext,DWORD dwFlags,LPVOID pvReserved ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumAddressTypes( DPENUMADDRESSTYPESCALLBACK lpCallback,REFGUID pguidSP,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumLocalApplications( DPENUMLOCALAPPLICATIONSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetConnectionSettings( DWORD dwFlags,LPVOID pvData,LPDWORD pdwDataSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE ReceiveLobbyMessage( DWORD dwFlags,DWORD dwMessageFlags,LPDWORD pdwStringLength,LPVOID lpBuffer,LPDWORD pdwBufferSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE RunApplication( DWORD dwFlags,LPDWORD pdwSize,LPDPLCONNECTION pConnection,HANDLE hEvent ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SendLobbyMessage( DWORD dwFlags,DWORD dwMessageId,LPVOID lpBuffer,DWORD dwBufferSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetConnectionSettings( DWORD dwFlags,DWORD dwNameLength,LPDPLCONNECTION pConnection ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetLobbyMessageEvent( DWORD dwFlags,DWORD dwMessageId,HANDLE hEvent ) PURE;
};

struct IDirectPlayLobby2 : public IDirectPlayLobby{
	virtual HRESULT STDMETHODCALLTYPE CreateCompoundAddress( LPCDPCOMPOUNDADDRESSELEMENT lpElements,DWORD dwElementCount,LPVOID pvAddress,LPDWORD pdwAddressSize ) PURE;
};

struct IDirectPlayLobby3 : public IDirectPlayLobby{
	virtual HRESULT STDMETHODCALLTYPE ConnectEx( DWORD dwFlags,REFIID riid,LPVOID *ppv,IUnknown *pUnk ) PURE;
	virtual HRESULT STDMETHODCALLTYPE RegisterApplication( DWORD dwFlags,LPVOID pApplicationDesc ) PURE;
	virtual HRESULT STDMETHODCALLTYPE UnregisterApplication( DWORD dwFlags,REFGUID guidApplication ) PURE;
	virtual HRESULT STDMETHODCALLTYPE WaitForConnectionSettings( DWORD dwFlags ) PURE;
};

#endif
