#include "../common/Common.h"

char* SERVERIP = (char*)"127.0.0.1";

// 패킷 보내고 ACK 기다리기, 시간 초과되면 다시 보냄
int sendWithAck(SOCKET sock, struct sockaddr_in* addr, char* pkt, int len, int seq)
{
	u_long ack;
	int retval;

	for (int retry = 0; retry < MAX_RETRY; retry++) {
		sendto(sock, pkt, len, 0, (SOCKADDR*)addr, sizeof(*addr));
		retval = recvfrom(sock, (char*)&ack, sizeof(ack), 0, NULL, NULL);
		if (retval == sizeof(ack) && (int)ntohl(ack) == seq) return 1;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	struct sockaddr_in server_addr;
	SOCKET sock;

	u64 time_start, time_end;

	if (argc != 4) {
		msg(MSG, "명령 파라미터 잘못 지정됨");
		msg(MSG, "Syntax : UDPFileClient.exe IP_Address PortNumber FileName");
		exit(1);
	}

	//윈속 초기화
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	//소켓 생성
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	//ACK 수신 타임아웃 설정
	DWORD timeout = TIMEOUT_MS;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	//소켓 주소 구조체 초기화
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons(atoi(argv[2]));

	msg(CLIENT, "UDP 파일 송신 시작됨 !!\n");

	int BufferNum, totalBufferNum, sendBytes;
	long file_size, totalSendBytes;

	char pkt[HDR_SIZE + BUF_SIZE];	// [순서 번호 4바이트][데이터]
	u_long nseq;
	int seq = 0;

	FILE* fp;
	fp = fopen(argv[3], "rb");
	if (fp == NULL) {
		msg(MSG, notFound);
		exit(1);
	}

	time_start = GetMicroCounter();

	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	totalBufferNum = (file_size + BUF_SIZE - 1) / BUF_SIZE;	// 나머지가 있으면 한 블록 더

	fseek(fp, 0, SEEK_SET);
	BufferNum = 0;
	totalSendBytes = 0;

	//0번 패킷에 파일 크기 보내기
	nseq = htonl(seq);
	memcpy(pkt, &nseq, HDR_SIZE);
	_snprintf(pkt + HDR_SIZE, BUF_SIZE, "%ld", file_size);
	if (!sendWithAck(sock, &server_addr, pkt, HDR_SIZE + (int)strlen(pkt + HDR_SIZE) + 1, seq)) {
		msg(CLIENT, "서버 응답 없음");
		fclose(fp);
		exit(1);
	}
	seq++;

	//1번 패킷부터 파일 데이터
	while ((sendBytes = (int)fread(pkt + HDR_SIZE, sizeof(char), BUF_SIZE, fp)) > 0) {
		nseq = htonl(seq);
		memcpy(pkt, &nseq, HDR_SIZE);

		if (!sendWithAck(sock, &server_addr, pkt, HDR_SIZE + sendBytes, seq)) {
			msg(CLIENT, "\n재전송 횟수 초과, 전송 중단");
			break;
		}
		seq++;
		BufferNum++;
		totalSendBytes += sendBytes;
		printf("파일 송신중 : %ld/%ldByte(s) [%d%%]\r", totalSendBytes, file_size, ((BufferNum * 100) / totalBufferNum));
	}
	time_end = GetMicroCounter();
	printf("\n\n전송시간(Micro seconds) : %d", (int)(time_end - time_start));

	fclose(fp);
	closesocket(sock);

	WSACleanup();
	return 0;
}
