#include <stdio.h>
#include <stdlib.h>
#include <msquic.h>
#include <string.h>

//msquic.h file include headers files depening on platform type,(Windows or Linux)
//msquic_winkernel & msquic_winuser & msquic_posix.h (our case)
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif 

const QUIC_API_TABLE* MsQuic;
HQUIC Registration;
HQUIC Configuration;
const uint16_t UdpPort = 4567; //unsigned short  value 
const QUIC_REGISTRATION_CONFIG RegConfig = {"quicsample", QUIC_EXECUTION_PROFILE_LOW_LATENCY };
const uint64_t IdleTimeoutMs = 1000; //long int 
const QUIC_BUFFER Alpn = { sizeof("sample") - 1, (uint8_t*)"sample"}; //signed char 
const uint32_t SendBufferLength = 100;

typedef struct QUIC_CREDENTIAL_CONFIG_HELPER {
	QUIC_CREDENTIAL_CONFIG CredConfig;
	union {
		QUIC_CERTIFICATE_FILE CertFile;
	};
} QUIC_CREDENTIAL_CONFIG_HELPER;

void PrintUsage() {
	printf("quicsample -server -cert_file:<> -key_file:<>\n");
	printf("quicsample -client -unsecure -target:[IPaddress|HostName]\n");

}

BOOLEAN GetFlag(int argc, char* argv[], char* name) {
	const size_t nameLen = strlen(name); //unsigned int type 	
	for (int i = 0; i < argc; i++) {
		if(strncmp(argv[i]+1,name,nameLen) == 0 //_strncmp is for Visual Studio
							 //+1 for - sign 
				&& strlen(argv[i]) == nameLen + 1) {  return TRUE; }
	}

return FALSE;
}


const char* GetValue(int argc, char* argv[], char* name) { //Get value after : word 
	const size_t nameLen = strlen(name);
	for (int i=0; i < argc; i++) {
		if(strncmp(argv[i]+1,name,nameLen) == 0 
		&& strlen(argv[i]) > 1+nameLen+1 
		&& *(argv[i] + 1 + nameLen) == ':') {
			return argv[i] + 1  + nameLen + 1;
		}
	

	}

	return NULL;
}

void ServerSend(HQUIC Stream) {
	void* SendBufferRaw = malloc(sizeof(QUIC_BUFFER) + SendBufferLength);
	if (SendBufferRaw == NULL) {
		printf("Sendbuffer Allocation Failed\n");
		MsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT,0 );
		return;
	}
	QUIC_BUFFER* SendBuffer = (QUIC_BUFFER*)SendBufferRaw;
	SendBuffer->Buffer = (uint8_t*)SendBufferRaw + sizeof(QUIC_BUFFER);
	SendBuffer->Length = SendBufferLength;

	printf("[strm][%p] Sending Data...\n", Stream);


	QUIC_STATUS Status;
	if(QUIC_FAILED(Status = MsQuic->StreamSend(Stream, SendBuffer, 1, QUIC_SEND_FLAG_FIN, SendBuffer)))  { 
		printf("StreamSend Failed\n");
		free(SendBufferRaw);
		MsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT,0);
	}

}

QUIC_STATUS ServerStreamCallback(HQUIC Stream, void* Context, QUIC_STREAM_EVENT* Event)
{ 
	UNREFERENCED_PARAMETER(Context);
	switch(Event->Type) {
		case QUIC_STREAM_EVENT_SEND_COMPLETE:
			free(Event->SEND_COMPLETE.ClientContext);
			printf("[strm][%p] Data sent\n", Stream);
			break;
		case QUIC_STREAM_EVENT_RECEIVE:
			uint64_t length = Event->RECEIVE.TotalBufferLength;
			uint8_t* data = Event->RECEIVE.Buffers->Buffer;

			printf("[strm][%p] Data received - TotalBufferLength : %llu Data: %s\n ", Stream, length, data);
			break;
		case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
			printf("[strm][%p] Peer shutdown\n", Stream);
			ServerSend(Stream);
			break;
		case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
			printf("[strm][%p] Peer aborted\n", Stream);
			MsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT,0);
			break;
		case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
			printf("[strm][%p] All done\n", Stream);
			MsQuic->StreamClose(Stream);
			break;
		default:
			break;
	}
	return QUIC_STATUS_SUCCESS;
} 

QUIC_STATUS ClientStreamCallback(HQUIC Stream, void* Context, QUIC_STREAM_EVENT* Event) {
	UNREFERENCED_PARAMETER(Context);
	switch(Event->Type) {
		case QUIC_STREAM_EVENT_SEND_COMPLETE:
			free(Event->SEND_COMPLETE.ClientContext);
			printf("[strm][%p] Data sent\n", Stream);
			break;
		case QUIC_STREAM_EVENT_RECEIVE:
			printf("[strm][%p] Data received\n", Stream);
			break;
		case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
			printf("[strm][%p] Peer shutdown\n", Stream);
			break;
		case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
			printf("[strm][%p] Peer aborted\n", Stream);
			MsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT,0);
			break;
		case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
			printf("[strm][%p] All done\n", Stream);
			MsQuic->StreamClose(Stream);
			break;
		default:
			break;
	}
	return QUIC_STATUS_SUCCESS;
	}


//Callback for connection events from msquic 
QUIC_STATUS ServerConnectionHandler(HQUIC Connection, void* Context, QUIC_CONNECTION_EVENT* Event)
{
	UNREFERENCED_PARAMETER(Context);
	switch (Event->Type) { 
	case QUIC_CONNECTION_EVENT_CONNECTED:
		printf("[conn][%p] Connected\n", Connection);
		MsQuic->ConnectionSendResumptionTicket(Connection, QUIC_SEND_RESUMPTION_FLAG_NONE, 0, NULL);
		break;
	case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
		if(Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status == QUIC_STATUS_CONNECTION_IDLE) {
		printf("[conn][%p] Successfully shut down on idle.\n", Connection);
		} else {

		printf("[conn][%p] Shutdown by transport, 0x%x\n", Connection, Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
		}
		break;
	case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
		printf("[conn][%p] Shut down by peer, 0x%llu.\n", Connection, (unsigned long long)Event->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
			break;
	case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
		printf("[conn][%p] All done\n", Connection);
		MsQuic->ConnectionClose(Connection);
		break;

	case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
		printf("[conn][%p] Peer started\n", Event->PEER_STREAM_STARTED.Stream);
		MsQuic->SetCallbackHandler(Event->PEER_STREAM_STARTED.Stream, (void*)ServerStreamCallback, NULL);
		break;
	case QUIC_CONNECTION_EVENT_RESUMED:
		printf("[conn][%p] Connection resumed\n", Connection);
		break;
	default:
		break;
}
return QUIC_STATUS_SUCCESS;
}

void ClientSend(HQUIC Connection) {
	QUIC_STATUS Status;
	HQUIC Stream = NULL;
	uint8_t* SendBufferRaw;
	QUIC_BUFFER* SendBuffer;

	if(QUIC_FAILED(Status = MsQuic->StreamOpen(Connection, QUIC_STREAM_OPEN_FLAG_NONE, ClientStreamCallback, NULL, &Stream))) {
		printf("Stream open failed\n");
		MsQuic->ConnectionShutdown(Connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,0);
	}
	printf("Starting...\n");

	if(QUIC_FAILED(Status = MsQuic->StreamStart(Stream, QUIC_STREAM_START_FLAG_NONE))) {
		printf("StreamStart failed\n");
		MsQuic->ConnectionShutdown(Connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,0);
	}

	SendBufferRaw = malloc(sizeof(QUIC_BUFFER) + SendBufferLength);

	if (SendBufferRaw == NULL) {
		printf("Sendbuffer Allocation Failed\n");
		MsQuic->ConnectionShutdown(Connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,0);
		return;
	}
	const uint8_t* data = "testing data";
	SendBuffer = (QUIC_BUFFER*)SendBufferRaw;
	SendBuffer->Buffer = (uint8_t*)SendBufferRaw + sizeof(QUIC_BUFFER);
	SendBuffer->Length = SendBufferLength;
	memcpy(SendBuffer->Buffer, data, sizeof(data));

	printf("[strm][%p] Sending Data...\n", Stream);

	if(QUIC_FAILED(Status = MsQuic->StreamSend(Stream, SendBuffer, 1, QUIC_SEND_FLAG_FIN, SendBuffer)))  { 

		printf("StreamSend Failed\n");
		free(SendBufferRaw);
		MsQuic->ConnectionShutdown(Connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE,0);
	}

}


//callback for listener events from MsQuic 
QUIC_STATUS  ServerListenerCallback(HQUIC Listener, void* Context, QUIC_LISTENER_EVENT* Event) { 
	UNREFERENCED_PARAMETER(Listener);
	UNREFERENCED_PARAMETER(Context);

	QUIC_STATUS Status = QUIC_STATUS_NOT_SUPPORTED;

	switch(Event->Type) {
		case QUIC_LISTENER_EVENT_NEW_CONNECTION:
			MsQuic->SetCallbackHandler(Event->NEW_CONNECTION.Connection, (void*)ServerConnectionHandler, NULL);
		Status = MsQuic->ConnectionSetConfiguration(Event->NEW_CONNECTION.Connection, Configuration);
		break;
	default:
		break;

	}
return Status;

}

QUIC_STATUS ClientConnectionCallback(HQUIC Connection, void* Context, QUIC_CONNECTION_EVENT* Event){ 
	UNREFERENCED_PARAMETER(Context);

    switch (Event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        //
        // The handshake has completed for the connection.
        //
        printf("[conn][%p] Connected\n", Connection);
        ClientSend(Connection);
        break;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
        //
        // The connection has been shut down by the transport. Generally, this
        // is the expected way for the connection to shut down with this
        // protocol, since we let idle timeout kill the connection.
        //
        if (Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status == QUIC_STATUS_CONNECTION_IDLE) {
            printf("[conn][%p] Successfully shut down on idle.\n", Connection);
        } else {
            printf("[conn][%p] Shut down by transport, 0x%x\n", Connection, Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
        }
        break;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        //
        // The connection was explicitly shut down by the peer.
        //
        printf("[conn][%p] Shut down by peer, 0x%llu\n", Connection, (unsigned long long)Event->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
        break;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        //
        // The connection has completed the shutdown process and is ready to be
        // safely cleaned up.
        //
        printf("[conn][%p] All done\n", Connection);
        if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
            MsQuic->ConnectionClose(Connection);
        }
        break;
    case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED:
        //
        // A resumption ticket (also called New Session Ticket or NST) was
        // received from the server.
        //
        printf("[conn][%p] Resumption ticket received (%u bytes):\n", Connection, Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength);
        for (uint32_t i = 0; i < Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength; i++) {
            printf("%.2X", (uint8_t)Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicket[i]);
        }
        printf("\n");
        break;
    default:
        break;
    }
    return QUIC_STATUS_SUCCESS;
}

    

BOOLEAN ServerLoadConfiguration(int argc, char* argv[]) {
	QUIC_SETTINGS Settings = {0};

	Settings.IdleTimeoutMs = IdleTimeoutMs;
	Settings.IsSet.IdleTimeoutMs = TRUE; 

	Settings.ServerResumptionLevel = QUIC_SERVER_RESUME_AND_ZERORTT;
	Settings.IsSet.ServerResumptionLevel = TRUE; 

	Settings.PeerBidiStreamCount = 1;
	Settings.IsSet.PeerBidiStreamCount = TRUE;

	QUIC_CREDENTIAL_CONFIG_HELPER Config;
	memset(&Config, 0, sizeof(Config));
	Config.CredConfig.Flags = QUIC_CREDENTIAL_FLAG_NONE; //set default configuration 	

	const char* Cert;
	const char* KeyFile;

	if ((Cert = GetValue(argc,argv, "cert_file")) != NULL && 
		(KeyFile = GetValue(argc,argv,"key_file")) != NULL) {
	Config.CertFile.CertificateFile = (char*)Cert;
	Config.CertFile.PrivateKeyFile = (char*)KeyFile;
	Config.CredConfig.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
	Config.CredConfig.CertificateFile = &Config.CertFile;
	}
	else {
	printf("Must Specify cert\n");
	return FALSE;
	}
	
	QUIC_STATUS Status = QUIC_STATUS_SUCCESS;
	if(QUIC_FAILED(Status = MsQuic->ConfigurationOpen(Registration, &Alpn, 1, &Settings,sizeof(Settings), NULL, &Configuration))) {
printf("ConfigurationOpen Failed\n");	
	return FALSE;
	}
	
	if(QUIC_FAILED(Status = MsQuic->ConfigurationLoadCredential(Configuration,&Config.CredConfig))) {
printf("ConfigurationLoadCredential Failed\n");	
	return FALSE;
	}
	return TRUE;
}

BOOLEAN ClientLoadConfiguration (BOOLEAN Unsecure) {
	QUIC_SETTINGS Settings = {0};
		
	Settings.IdleTimeoutMs = IdleTimeoutMs;
	Settings.IsSet.IdleTimeoutMs = TRUE; 

	QUIC_CREDENTIAL_CONFIG CredConfig;

	memset(&CredConfig, 0, sizeof(CredConfig));
	CredConfig.Type = QUIC_CREDENTIAL_TYPE_NONE;
	CredConfig.Flags = QUIC_CREDENTIAL_FLAG_CLIENT;

	if(Unsecure) {
		CredConfig.Flags |= QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION; //0x00000001 OR 0x00000004 = 0x00000005 
	}

	QUIC_STATUS Status = QUIC_STATUS_SUCCESS;
	if(QUIC_FAILED(Status = MsQuic->ConfigurationOpen(Registration,&Alpn,1,&Settings,sizeof(Settings),NULL,&Configuration))) {
		printf("Configuration Open Failed\n");
		return FALSE;
	}
	if(QUIC_FAILED(Status = MsQuic->ConfigurationLoadCredential(Configuration,&CredConfig))) {
		printf("ConfigurationloadCredential Failed\n");
		return FALSE;
}
return TRUE;
}

void RunClient(int argc, char* argv[]) {
	if(!ClientLoadConfiguration(GetFlag(argc,argv,"unsecure"))) {
		return;
	}
	
	QUIC_STATUS Status = QUIC_STATUS_SUCCESS;
	HQUIC Connection = NULL;

	if(QUIC_FAILED(Status = MsQuic->ConnectionOpen(Registration, ClientConnectionCallback, NULL, &Connection))) {
				printf("Connection Open Failed\n");
				
				}
	const char* Target;
	if((Target = GetValue(argc,argv,"target")) == NULL)  {
		printf("Must specify target\n");
		Status = QUIC_STATUS_INVALID_PARAMETER;
	}

	if(QUIC_FAILED(Status = MsQuic->ConnectionStart(Connection, Configuration, AF_UNSPEC, Target, UdpPort))) {
		printf("Connection Start Failed\n");
	}

}

void RunServer (int argc, char* argv[]) {

	QUIC_STATUS Status;
	HQUIC Listener = NULL;

	//Configures the address used for the listener to listen on all IP 
	//and given UDP port 
	QUIC_ADDR Address = {0};
	Address.Ip.sa_family = AF_UNSPEC;
	Address.Ipv4.sin_port = htons(UdpPort);

	//Load the server configuration based on the command line 
	if(!ServerLoadConfiguration(argc,argv)) {
		return; 
	}

	if(QUIC_FAILED(Status = MsQuic->ListenerOpen(Registration, ServerListenerCallback, NULL, &Listener))) {
		printf("ListenerOpen failed\n");
		goto Error;
	}

	if(QUIC_FAILED(Status = MsQuic->ListenerStart(Listener, &Alpn, 1, &Address))) {
		printf("ListenerStart failed\n");
		goto Error;
	}
	printf ("Press Enter to exit\n");
	getchar();	

Error: 
	if (Listener != NULL) {
		MsQuic->ListenerClose(Listener);

}
}

int main (int argc, char* argv[]) {
	//argument count & arguments value in null-terminator string 
	QUIC_STATUS Status = QUIC_STATUS_SUCCESS;
	//unsigned long Status=0;
	

	//Following code open a handle to the library and get the API function table (MsQuic).
	if (QUIC_FAILED(Status = MsQuicOpen2(&MsQuic))) {

	/*QUIC_FAILED (int x > 0) if return value of MsQuicOpen2 > 0 
	 * MsQuicOpen2 function returns QUIC_STATUS, in which SUCCESS is 0 and failed code are bigger than 0*/
		
		printf("MsQuicOpen2 Failed, 0x%x!\n", Status);
//		goto Error;
	}


	if (QUIC_FAILED(Status = MsQuic->RegistrationOpen(&RegConfig, &Registration))) {
		printf("Registration Failed, 0x%x!\n", Status);
//		goto Error;
	}

	
	if (GetFlag(argc, argv, "help") || GetFlag(argc, argv, "?")) {
			PrintUsage();
	}	
	else if (GetFlag(argc, argv, "server")){
		RunServer(argc, argv);
	}
	else if (GetFlag(argc, argv, "client")) {
		RunClient(argc, argv);
	}

	else {
	PrintUsage();
	}		
	Error:
		if(MsQuic != NULL) {
		if(Configuration != NULL) {
		MsQuic->ConfigurationClose(Configuration);

		}
		if (Registration != NULL) {
		MsQuic->RegistrationClose(Registration);
		}
		MsQuicClose(MsQuic);
	}
	return (int)Status;
}



