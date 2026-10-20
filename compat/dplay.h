// DirectPlay 4, written from the published interface.
//
// The DirectX SDK is not redistributable and Microsoft retired the download
// in favour of the End-User Runtime, which carries neither headers nor
// d3dxof, so <dplay.h> cannot be had. What is here is the part of DirectPlay
// 4 that bbruntime's multiplayer code uses, written out from the interface
// definition, and it is exact where it matters: the vtable order of
// IDirectPlay4, of IDirectPlayLobby and of IDirectPlayLobby3 is the order the
// library implements, the structures have the layouts the library writes, and
// the constants have the values the library tests.
//
// The vtable order is not a guess and not the order the methods are usually
// talked about in. It is alphabetical within each version of the interface:
// the methods IDirectPlay2 added come first, then the ones IDirectPlay3
// added, then the ones IDirectPlay4 added, and each group is sorted by name.
// A call that lands one slot out is a call into the middle of somebody
// else's function, so the order below is the whole point of this file.
//
// The library itself is still here. dplayx.dll ships with Windows, in
// SysWOW64 for the 32-bit build, and it exports DirectPlayCreate and
// DirectPlayLobbyCreateA. What is gone is the COM registration: CLSID_DirectPlay
// is not in the registry any more, so CoCreateInstance cannot be used to get
// the interface, and the entry points have to be called directly.

#ifndef B3_COMPAT_DPLAY_H
#define B3_COMPAT_DPLAY_H

#include <windows.h>
#include <guiddef.h>
#include <ole2.h>

typedef DWORD DPID;
typedef GUID DPGUID;
typedef const GUID *LPCDPGUID;
typedef DPID *LPDPID;

#define DPID_SYSMSG         0
#define DPID_ALLPLAYERS     0
#define DPID_SERVERPLAYER   1
#define DPID_RESERVEDRANGE  100
#define DPID_UNKNOWN        0xFFFFFFFF

//The error codes. DirectPlay's own facility is 0x877, so its codes are
//MAKE_HRESULT(1,0x877,code) and they are all negative.
#define _FACDP 0x877
#define MAKE_DPHRESULT( code ) MAKE_HRESULT( 1,_FACDP,code )

#define DP_OK                          S_OK
#define DPERR_ALREADYINITIALIZED       MAKE_DPHRESULT( 5 )
#define DPERR_ACCESSDENIED             MAKE_DPHRESULT( 10 )
#define DPERR_ACTIVEPLAYERS            MAKE_DPHRESULT( 20 )
#define DPERR_BUFFERTOOSMALL           MAKE_DPHRESULT( 30 )
#define DPERR_CANTADDPLAYER            MAKE_DPHRESULT( 40 )
#define DPERR_CANTCREATEGROUP          MAKE_DPHRESULT( 50 )
#define DPERR_CANTCREATEPLAYER         MAKE_DPHRESULT( 60 )
#define DPERR_CANTCREATESESSION        MAKE_DPHRESULT( 70 )
#define DPERR_CAPSNOTAVAILABLEYET      MAKE_DPHRESULT( 80 )
#define DPERR_EXCEPTION                MAKE_DPHRESULT( 90 )
#define DPERR_GENERIC                  E_FAIL
#define DPERR_INVALIDFLAGS             MAKE_DPHRESULT( 120 )
#define DPERR_INVALIDOBJECT            MAKE_DPHRESULT( 130 )
#define DPERR_INVALIDPARAM             E_INVALIDARG
#define DPERR_INVALIDPARAMS            DPERR_INVALIDPARAM
#define DPERR_INVALIDPLAYER            MAKE_DPHRESULT( 150 )
#define DPERR_INVALIDGROUP             MAKE_DPHRESULT( 155 )
#define DPERR_NOCAPS                   MAKE_DPHRESULT( 160 )
#define DPERR_NOCONNECTION             MAKE_DPHRESULT( 170 )
#define DPERR_NOMEMORY                 E_OUTOFMEMORY
#define DPERR_OUTOFMEMORY              DPERR_NOMEMORY
#define DPERR_NOMESSAGES               MAKE_DPHRESULT( 190 )
#define DPERR_NONAMESERVERFOUND        MAKE_DPHRESULT( 200 )
#define DPERR_NOPLAYERS                MAKE_DPHRESULT( 210 )
#define DPERR_NOSESSIONS               MAKE_DPHRESULT( 220 )
#define DPERR_PENDING                  E_PENDING
#define DPERR_SENDTOOBIG               MAKE_DPHRESULT( 230 )
#define DPERR_TIMEOUT                  MAKE_DPHRESULT( 240 )
#define DPERR_UNAVAILABLE              MAKE_DPHRESULT( 250 )
#define DPERR_UNSUPPORTED              E_NOTIMPL
#define DPERR_BUSY                     MAKE_DPHRESULT( 270 )
#define DPERR_USERCANCEL               MAKE_DPHRESULT( 280 )
#define DPERR_NOINTERFACE              E_NOINTERFACE
#define DPERR_CANNOTCREATESERVER       MAKE_DPHRESULT( 290 )
#define DPERR_PLAYERLOST               MAKE_DPHRESULT( 300 )
#define DPERR_SESSIONLOST              MAKE_DPHRESULT( 310 )
#define DPERR_UNINITIALIZED            MAKE_DPHRESULT( 320 )
#define DPERR_NONEWPLAYERS             MAKE_DPHRESULT( 330 )
#define DPERR_INVALIDPASSWORD          MAKE_DPHRESULT( 340 )
#define DPERR_CONNECTING               MAKE_DPHRESULT( 350 )
#define DPERR_CONNECTIONLOST           MAKE_DPHRESULT( 360 )
#define DPERR_UNKNOWNMESSAGE           MAKE_DPHRESULT( 370 )
#define DPERR_CANCELFAILED             MAKE_DPHRESULT( 380 )
#define DPERR_INVALIDPRIORITY          MAKE_DPHRESULT( 390 )
#define DPERR_NOTHANDLED               MAKE_DPHRESULT( 400 )
#define DPERR_CANCELLED                MAKE_DPHRESULT( 410 )
#define DPERR_ABORTED                  MAKE_DPHRESULT( 420 )
#define DPERR_NOSERVICEPROVIDER        MAKE_DPHRESULT( 1040 )
#define DPERR_NOTLOBBIED               MAKE_DPHRESULT( 1070 )

//Open
#define DPOPEN_JOIN           0x00000001
#define DPOPEN_CREATE         0x00000002
#define DPOPEN_RETURNSTATUS   0x00000080

//Send
#define DPSEND_GUARANTEED     0x00000001
#define DPSEND_HIGHPRIORITY   0x00000002
#define DPSEND_OPENSTREAM     0x00000008
#define DPSEND_CLOSESTREAM    0x00000010
#define DPSEND_SIGNED         0x00000020
#define DPSEND_ENCRYPTED      0x00000040
#define DPSEND_ASYNC          0x00000200

//Receive
#define DPRECEIVE_ALL         0x00000001
#define DPRECEIVE_TOPLAYER    0x00000002
#define DPRECEIVE_FROMPLAYER  0x00000004
#define DPRECEIVE_PEEK        0x00000008

//EnumPlayers and CreatePlayer
#define DPENUMPLAYERS_ALL           0x00000000
#define DPENUMPLAYERS_LOCAL         0x00000008
#define DPENUMPLAYERS_REMOTE        0x00000010
#define DPENUMPLAYERS_GROUP         0x00000020
#define DPENUMPLAYERS_SESSION       0x00000080
#define DPENUMPLAYERS_SERVERPLAYER  0x00000100
#define DPENUMPLAYERS_SPECTATOR     0x00000200
#define DPENUMPLAYERS_OWNER         0x00002000

#define DPPLAYER_LOCAL        DPENUMPLAYERS_LOCAL
#define DPPLAYER_OWNER        DPENUMPLAYERS_OWNER
#define DPPLAYER_SERVERPLAYER DPENUMPLAYERS_SERVERPLAYER
#define DPPLAYER_SPECTATOR    DPENUMPLAYERS_SPECTATOR

//EnumConnections
#define DPCONNECTION_DIRECTPLAY      0x00000001
#define DPCONNECTION_DIRECTPLAYLOBBY 0x00000002

//EnumSessions
#define DPENUMSESSIONS_AVAILABLE        0x00000001
#define DPENUMSESSIONS_ALL              0x00000002
#define DPENUMSESSIONS_ASYNC            0x00000010
#define DPENUMSESSIONS_STOPASYNC        0x00000020
#define DPENUMSESSIONS_PASSWORDREQUIRED 0x00000040
#define DPENUMSESSIONS_RETURNSTATUS     0x00000080

//The session description's flags
#define DPSESSION_NEWPLAYERSDISABLED   0x00000001
#define DPSESSION_MIGRATEHOST          0x00000004
#define DPSESSION_NOMESSAGEID          0x00000008
#define DPSESSION_JOINDISABLED         0x00000020
#define DPSESSION_KEEPALIVE            0x00000040
#define DPSESSION_NODATAMESSAGES       0x00000080
#define DPSESSION_SECURESERVER         0x00000100
#define DPSESSION_PRIVATE              0x00000200
#define DPSESSION_PASSWORDREQUIRED     0x00000400
#define DPSESSION_MULTICASTSERVER      0x00000800
#define DPSESSION_CLIENTSERVER         0x00001000
#define DPSESSION_DIRECTPLAYPROTOCOL   0x00002000
#define DPSESSION_NOPRESERVEORDER       0x00004000
#define DPSESSION_OPTIMIZELATENCY      0x00008000
#define DPSESSION_ALLOWVOICERETRO      0x00010000
#define DPSESSION_NOSESSIONDESCMESSAGES 0x00020000

//The system messages that arrive through Receive
#define DPSYS_CREATEPLAYERORGROUP   0x0003
#define DPSYS_DESTROYPLAYERORGROUP  0x0005
#define DPSYS_ADDPLAYERTOGROUP      0x0007
#define DPSYS_DELETEPLAYERFROMGROUP 0x0021
#define DPSYS_SESSIONLOST           0x0031
#define DPSYS_HOST                  0x0101
#define DPSYS_SETPLAYERORGROUPDATA  0x0102
#define DPSYS_SETPLAYERORGROUPNAME  0x0103
#define DPSYS_SETSESSIONDESC        0x0104

#define DPPLAYERTYPE_GROUP  0x00000000
#define DPPLAYERTYPE_PLAYER 0x00000001

//The names DirectPlay 1 and 2 used, which some code still uses
#define DPOpen_Join   DPOPEN_JOIN
#define DPOpen_Create DPOPEN_CREATE
#define DPSend_Guaranteed DPSEND_GUARANTEED
#define DPReceive_All DPRECEIVE_ALL
#define DPReceive_TopLayer DPRECEIVE_TOPLAYER
#define DPReceive_FromPlayer DPRECEIVE_FROMPLAYER
#define DPReceive_Peer DPRECEIVE_FROMPLAYER
#define DPReceive_Peek DPRECEIVE_PEEK
#define DPPlayer_Local DPPLAYER_LOCAL
#define DPPlayer_Owner DPPLAYER_OWNER
#define DPPlayer_Host DPPLAYER_SERVERPLAYER
#define DPPlayer_Spectator DPPLAYER_SPECTATOR
#define DPEnumPlayers_All DPENUMPLAYERS_ALL
#define DPEnumPlayers_LocalPlayers DPENUMPLAYERS_LOCAL
#define DPEnumPlayers_RemotePlayers DPENUMPLAYERS_REMOTE
#define DPEnumPlayers_GroupPlayers DPENUMPLAYERS_GROUP
#define DPEnumPlayers_Spectators DPENUMPLAYERS_SPECTATOR
#define DPEnumConnections_All DPCONNECTION_DIRECTPLAY
#define DPEnumSessions_Async DPENUMSESSIONS_ASYNC
#define DPEnumSessions_Passive DPENUMSESSIONS_ALL

//--- the structures -------------------------------------------------------

//The name of a player or a group.
typedef struct _DPNAME{
	DWORD dwSize;
	DWORD dwFlags;
	union{ LPWSTR lpszShortName; LPSTR lpszShortNameA; };
	union{ LPWSTR lpszLongName; LPSTR lpszLongNameA; };
} DPNAME,*LPDPNAME,*LPCDPNAME;

//The description of a session.
typedef struct _DPSESSIONDESC2{
	DWORD dwSize;
	DWORD dwFlags;
	GUID guidInstance;
	GUID guidApplication;
	DWORD dwMaxPlayers;
	DWORD dwCurrentPlayers;
	union{ LPWSTR lpszSessionName; LPSTR lpszSessionNameA; };
	union{ LPWSTR lpszPassword; LPSTR lpszPasswordA; };
	DWORD_PTR dwReserved1;
	DWORD_PTR dwReserved2;
	DWORD_PTR dwUser1;
	DWORD_PTR dwUser2;
	DWORD_PTR dwUser3;
	DWORD_PTR dwUser4;
} DPSESSIONDESC2,*LPDPSESSIONDESC2,*LPCDPSESSIONDESC2;

//Everything needed to connect to a session or create one.
typedef struct _DPLCONNECTION{
	DWORD dwSize;
	DWORD dwFlags;
	LPDPSESSIONDESC2 lpSessionDesc;
	LPDPNAME lpPlayerName;
	GUID guidSP;
	LPVOID lpAddress;
	DWORD dwAddressSize;
} DPLCONNECTION,*LPDPLCONNECTION,*LPCDPLCONNECTION;

//What a DirectPlay object can do.
typedef struct _DPCAPS{
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwMaxBufferSize;
	DWORD dwMaxQueueSize;
	DWORD dwMaxPlayers;
	DWORD dwHundredBaud;
	DWORD dwLatency;
	DWORD dwMaxLocalPlayers;
	DWORD dwHeaderLength;
	DWORD dwTimeout;
} DPCAPS,*LPDPCAPS;

#define DPCAPS_ISHOST               0x00000002
#define DPCAPS_GROUPOPTIMIZED       0x00000008
#define DPCAPS_KEEPALIVEOPTIMIZED   0x00000010
#define DPCAPS_GUARANTEEDOPTIMIZED  0x00000020
#define DPCAPS_GUARANTEEDSUPPORTED  0x00000040
#define DPCAPS_SIGNINGSUPPORTED     0x00000080
#define DPCAPS_ENCRYPTIONSUPPORTED  0x00000100
#define DPCAPS_ASYNCCANCELSUPPORTED    0x00001000
#define DPCAPS_ASYNCCANCELALLSUPPORTED 0x00002000
#define DPCAPS_SENDTIMEOUTSUPPORTED   0x00004000
#define DPCAPS_SENDPRIORITYSUPPORTED   0x00008000
#define DPCAPS_ASYNCSUPPORTED         0x00010000

//The system messages. All of them arrive with dpId as DPID_SYSMSG.
typedef struct _DPMSG_GENERIC{
	DWORD dwType;
} DPMSG_GENERIC,*LPDPMSG_GENERIC;

typedef struct _DPMSG_CREATEPLAYERORGROUP{
	DWORD dwType;
	DWORD dwPlayerType;
	DPID dpId;
	DWORD dwCurrentPlayers;
	LPVOID lpData;
	DWORD dwDataSize;
	DPNAME dpnName;
	DPID dpIdParent;
	DWORD dwFlags;
} DPMSG_CREATEPLAYERORGROUP,*LPDPMSG_CREATEPLAYERORGROUP;

typedef struct _DPMSG_DESTROYPLAYERORGROUP{
	DWORD dwType;
	DWORD dwPlayerType;
	DPID dpId;
	LPVOID lpLocalData;
	DWORD dwLocalDataSize;
	LPVOID lpRemoteData;
	DWORD dwRemoteDataSize;
	DPNAME dpnName;
	DPID dpIdParent;
	DWORD dwFlags;
} DPMSG_DESTROYPLAYERORGROUP,*LPDPMSG_DESTROYPLAYERORGROUP;

typedef DPMSG_GENERIC DPMSG_HOST;
typedef DPMSG_GENERIC DPMSG_SESSIONLOST;

//--- the callbacks --------------------------------------------------------

typedef BOOL (FAR PASCAL *DPENUMPLAYERSCALLBACK)( DPID idPlayer,DWORD dwPlayerType,LPCDPNAME pdpPlayerName,DWORD dwFlags,LPVOID lpContext );
typedef BOOL (FAR PASCAL *DPENUMCONNECTIONSCALLBACK)( LPCDPGUID pguidSP,LPVOID pvConnection,DWORD cbConnection,LPCDPNAME pdpConnectionName,DWORD dwFlags,LPVOID lpContext );
typedef BOOL (FAR PASCAL *DPENUMSESSIONSCALLBACK)( LPCDPSESSIONDESC2 pdpSessionDesc,LPDWORD pdwTimeout,DWORD dwFlags,LPVOID lpContext );

//The SDK's names for the same three
typedef DPENUMPLAYERSCALLBACK LPDPENUMPLAYERSCALLBACK2;
typedef DPENUMCONNECTIONSCALLBACK LPDPENUMCONNECTIONSCALLBACK;
typedef DPENUMSESSIONSCALLBACK LPDPENUMSESSIONSCALLBACK2;

//--- the interface --------------------------------------------------------

//IDirectPlay4. The three IUnknown methods, then the methods IDirectPlay2
//added, then IDirectPlay3's, then IDirectPlay4's, each group in name order.
struct IDirectPlay4 : public IUnknown{
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,LPVOID *ppv ) PURE;
	virtual ULONG STDMETHODCALLTYPE AddRef() PURE;
	virtual ULONG STDMETHODCALLTYPE Release() PURE;

	virtual HRESULT STDMETHODCALLTYPE AddPlayerToGroup( DPID idGroup,DPID idPlayer ) PURE;
	virtual HRESULT STDMETHODCALLTYPE Close() PURE;
	virtual HRESULT STDMETHODCALLTYPE CreateGroup( DPID *pdpGroupId,LPCDPNAME pdpName,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE CreatePlayer( DPID *pdpPlayerId,LPCDPNAME pdpName,DWORD dwFlags,DWORD dwHostPlayerId,DWORD dwBroadcastBytesSent,DWORD dwBroadcastBytesReturned ) PURE;
	virtual HRESULT STDMETHODCALLTYPE DeletePlayerFromGroup( DPID idGroup,DPID idPlayer ) PURE;
	virtual HRESULT STDMETHODCALLTYPE DestroyGroup( DPID idGroup ) PURE;
	virtual HRESULT STDMETHODCALLTYPE DestroyPlayer( DPID idPlayer ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumGroupPlayers( DPID idGroup,LPCDPGUID pguidInstance,DPENUMPLAYERSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumGroups( LPCDPGUID pguidSessionType,DPENUMPLAYERSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumPlayers( DPID idDevice,DPENUMPLAYERSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumSessions( LPDPSESSIONDESC2 pSessionDesc,LPCDPGUID pguidApplication,DPENUMSESSIONSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetCaps( LPDPCAPS pdpCaps,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetGroupData( DPID idGroup,LPVOID pvData,LPDWORD pdwDataSize,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetGroupName( DPID idGroup,LPDPNAME *pdpName ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetMessageCount( DPID idPlayer,LPDWORD pdwCount ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetPlayerAddress( DPID idPlayer,LPVOID pvAddress,LPDWORD pdwAddressSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetPlayerCaps( DPID idPlayer,LPDPCAPS pdpPlayerCaps,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetPlayerData( DPID idPlayer,LPVOID pvData,LPDWORD pdwDataSize,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetPlayerName( DPID idPlayer,LPDPNAME *pdpName ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetSessionDesc( LPDPSESSIONDESC2 pSessionDesc ) PURE;
	virtual HRESULT STDMETHODCALLTYPE Initialize( LPCDPGUID pguidSP ) PURE;
	virtual HRESULT STDMETHODCALLTYPE Open( LPDPSESSIONDESC2 pSessionDesc,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE Receive( DPID *pdpSender,DPID *pdpPlayer,DWORD dwFlags,LPVOID pvBuffer,LPDWORD pdwDataSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE Send( DPID idPlayer,DPID idRecipient,DWORD dwFlags,LPVOID pvBuffer,DWORD dwDataSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetGroupData( DPID idGroup,LPCDPGUID pGroupGuid,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetGroupName( DPID idGroup,LPCDPNAME pdpName ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetPlayerData( DPID idPlayer,LPCDPGUID pPlayerGuid,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetPlayerName( DPID idPlayer,LPCDPNAME pdpName ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetSessionDesc( LPDPSESSIONDESC2 pSessionDesc ) PURE;

	virtual HRESULT STDMETHODCALLTYPE AddGroupToGroup( DPID idGroupTo,DPID idGroup ) PURE;
	virtual HRESULT STDMETHODCALLTYPE CreateGroupInGroup( DPID idParentGroup,DPID *pdpGroupId,LPCDPNAME pdpName,LPVOID pvData,DWORD dwDataSize,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE DeleteGroupFromGroup( DPID idGroupTo,DPID idGroup ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumConnections( LPCDPGUID pguidSessionType,DPENUMCONNECTIONSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE EnumGroupsInGroup( DPID idGroup,LPCDPGUID pguidInstance,DPENUMPLAYERSCALLBACK lpCallback,LPVOID lpContext,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetGroupConnectionSettings( DWORD dwFlags,DPID idGroup,LPVOID pvData,LPDWORD pdwDataSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE InitializeConnection( LPVOID pvConnectionInfo,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SecureOpen( LPCDPSESSIONDESC2 pSessionDesc,DWORD dwFlags,LPVOID pSecurityDesc,LPVOID pCredentials ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SendChatMessage( DPID idPlayer,DPID idRecipient,DWORD dwFlags,LPVOID pvChatMessage ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetGroupConnectionSettings( DWORD dwFlags,DPID idGroup,LPDPLCONNECTION pConnection ) PURE;
	virtual HRESULT STDMETHODCALLTYPE StartSession( DWORD dwFlags,DPID idGroup ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetGroupFlags( DPID idGroup,LPDWORD pdwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetGroupParent( DPID idGroup,LPDPID ppidParent ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetPlayerAccount( DPID idPlayer,DWORD dwFlags,LPVOID pvPlayerAccount,LPDWORD pdwDataSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetPlayerFlags( DPID idPlayer,LPDWORD pdwFlags ) PURE;

	virtual HRESULT STDMETHODCALLTYPE GetGroupOwner( DPID idGroup,LPDPID ppidOwner ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SetGroupOwner( DPID idGroup,DPID idOwner ) PURE;
	virtual HRESULT STDMETHODCALLTYPE SendEx( DPID idPlayer,DPID idRecipient,DWORD dwFlags,LPVOID pvBuffer,DWORD dwDataSize,DWORD dwPriority,DWORD dwTimeout,LPVOID pvContext,DWORD_PTR *pdwMsgID ) PURE;
	virtual HRESULT STDMETHODCALLTYPE GetMessageQueue( DPID idPlayer,DPID idRecipient,DWORD dwFlags,LPDWORD pdwNumberOfMessages,LPDWORD pdwQueueSize ) PURE;
	virtual HRESULT STDMETHODCALLTYPE CancelMessage( DWORD dwMsgID,DWORD dwFlags ) PURE;
	virtual HRESULT STDMETHODCALLTYPE CancelPriority( DWORD dwMinPriority,DWORD dwMaxPriority,DWORD dwFlags ) PURE;
};

//--- the library's entry points -------------------------------------------

//Both of these are exported by dplayx.dll. There is no import library for it
//in the Windows SDK, so they are looked up at run time.
typedef HRESULT (WINAPI *DirectPlayCreate_t)( GUID *guidSP,void **lplpDP,IUnknown *pUnk );
typedef HRESULT (WINAPI *DirectPlayLobbyCreate_t)( GUID *guidSP,void **lplpLobby,IUnknown *pUnk,LPVOID pvReserved,DWORD dwFlags );

//--- the identifiers -------------------------------------------------------

//These are the SDK's values and they are also in the system's dxguid.lib,
//which the runtime already links, so the declarations here only have to
//agree with it.
DEFINE_GUID( CLSID_DirectPlay,0xd1eb6d20,0x8923,0x11d0,0x9d,0x97,0x00,0xa0,0xc9,0x0a,0x43,0xcb );
DEFINE_GUID( IID_IDirectPlay4A,0xab1c531,0x4745,0x11d1,0xa7,0xa1,0x00,0x00,0xf8,0x03,0xab,0xfc );
DEFINE_GUID( IID_IDirectPlayLobby,0xaf465c71,0x9588,0x11cf,0xa0,0x20,0x00,0xaa,0x00,0x61,0x57,0xac );
DEFINE_GUID( IID_IDirectPlayLobby3,0x2db72490,0x652c,0x11d1,0xa7,0xa8,0x00,0x00,0xf8,0x03,0xab,0xfc );

//The TCP/IP service provider, and the address types a lobby can build.
DEFINE_GUID( DPSPGUID_TCPIP,0x36e95ee0,0x8577,0x11cf,0x96,0x0c,0x00,0x80,0xc7,0x53,0x4e,0x82 );
DEFINE_GUID( DPSPGUID_IPX,0x685bc400,0x9d2c,0x11cf,0xa9,0xcd,0x00,0xaa,0x00,0x68,0x86,0xe3 );
DEFINE_GUID( DPAID_TotalSize,0x1318f560,0x912c,0x11d0,0x9d,0xaa,0x00,0xa0,0xc9,0x0a,0x43,0xcb );
DEFINE_GUID( DPAID_ServiceProvider,0x7d916c0,0xe0af,0x11cf,0x9c,0x4e,0x00,0xa0,0xc9,0x05,0x42,0x5e );
DEFINE_GUID( DPAID_LobbyProvider,0x59b95640,0x9667,0x11d0,0xa7,0x7d,0x00,0x00,0xf8,0x03,0xab,0xfc );
DEFINE_GUID( DPAID_INet,0xc4a54da0,0xe0af,0x11cf,0x9c,0x4e,0x00,0xa0,0xc9,0x05,0x42,0x5e );
DEFINE_GUID( DPAID_INetW,0xe63232a0,0x9dbf,0x11d0,0x9c,0xc1,0x00,0xa0,0xc9,0x05,0x42,0x5e );
DEFINE_GUID( DPAID_INetPort,0xe4524541,0x8ea5,0x11d1,0x8a,0x96,0x00,0x60,0x97,0xb0,0x14,0x11 );

#endif
