#include "../common/Common.h"

// 받은 순서 번호로 ACK 보내기
void sendAck(SOCKET sock, struct sockaddr_in* addr, int seq)
{
	u_long ack = htonl(seq);
	sendto(sock, (char*)&ack, sizeof(ack), 0, (SOCKADDR*)addr, sizeof(*addr));
}

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	struct sockaddr_in server_addr;
	SOCKET sock;

	u64 time_start,time_end;
	int retval;

	if (argc != 3) {
		msg(MSG, "명령 파라미터 잘못 지정됨");
		msg(MSG, "Syntax : UDPFileServer.exe PortNumber FileName");
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(atoi(argv[1]));

	retval = bind(sock, (SOCKADDR*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	msg(SERVER, "UDP 파일 수신 시작됨 !!\n");

	struct sockaddr_in client_addr;
	int len_addr = sizeof(client_addr);
	int BufferNum, totalBufferNum, readBytes;
	long file_size, totalReadBytes;

	char pkt[HDR_SIZE + BUF_SIZE];	// [순서 번호 4바이트][데이터]
	u_long nseq;
	int seq, expected = 0;	// expected : 다음에 받아야 할 번호

	FILE* fp;
	fp = fopen(argv[2], "wb");
	if (fp == NULL) {
		msg(SERVER, "파일 생성 실패");
		exit(1);
	}

	//0번 패킷(파일 크기) 올 때까지 대기
	while (expected == 0) {
		readBytes = recvfrom(sock, pkt, sizeof(pkt), 0, (SOCKADDR*)&client_addr, &len_addr);
		if (readBytes == SOCKET_ERROR) err_quit("recvfrom()");
		if (readBytes <= HDR_SIZE) continue;

		memcpy(&nseq, pkt, HDR_SIZE);
		if (ntohl(nseq) != 0) continue;

		file_size = atol(pkt + HDR_SIZE);
		sendAck(sock, &client_addr, 0);
		expected = 1;
	}

	time_start = GetMicroCounter();

	totalBufferNum = (file_size + BUF_SIZE - 1) / BUF_SIZE;	// 나머지가 있으면 한 블록 더
	BufferNum = 0;
	totalReadBytes = 0;

	//클라이언트가 끊겨도 무한 대기하지 않도록 타임아웃 설정
	DWORD timeout = TIMEOUT_MS * MAX_RETRY;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	while (BufferNum != totalBufferNum) {
		readBytes = recvfrom(sock, pkt, sizeof(pkt), 0, (SOCKADDR*)&client_addr, &len_addr);
		if (readBytes == SOCKET_ERROR) {
			msg(SERVER, "\n수신 시간 초과, 수신 중단");
			break;
		}
		if (readBytes < HDR_SIZE) continue;

		memcpy(&nseq, pkt, HDR_SIZE);
		seq = (int)ntohl(nseq);

		//기다리던 번호일 때만 파일에 쓰기
		if (seq == expected) {
			fwrite(pkt + HDR_SIZE, sizeof(char), readBytes - HDR_SIZE, fp);
			expected++;
			BufferNum++;
			totalReadBytes += readBytes - HDR_SIZE;
			printf("파일 수신중 : %ld/%ldByte(s) [%d%%]\r", totalReadBytes, file_size, ((BufferNum * 100) / totalBufferNum));
		}
		//이미 받은 번호면 ACK가 유실된 것이므로 ACK만 다시 보냄
		if (seq < expected) sendAck(sock, &client_addr, seq);
	}
	time_end = GetMicroCounter();
	printf("\n\n전송시간(Micro seconds) : %d", (int)(time_end - time_start));

	fclose(fp);

	//마지막 ACK가 유실됐으면 클라이언트가 다시 보내므로 잠깐 더 받아줌
	timeout = TIMEOUT_MS * 3;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	while (recvfrom(sock, pkt, sizeof(pkt), 0, (SOCKADDR*)&client_addr, &len_addr) >= HDR_SIZE) {
		memcpy(&nseq, pkt, HDR_SIZE);
		sendAck(sock, &client_addr, (int)ntohl(nseq));
	}

	closesocket(sock);

	WSACleanup();
	return 0;

}
