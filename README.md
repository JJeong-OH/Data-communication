# Data-communication

데이터통신 수업 과제로 작성한 Winsock 소켓 프로그래밍 코드.
TCP/UDP 에코 서버·클라이언트와 UDP 파일 전송 프로그램을 구현했다.

## 개발 환경
- Windows, Visual Studio
- C++ (Winsock2)

## 파일 구성
| 파일 | 내용 |
|---|---|
| TCPServer, TCPClient | TCP 에코 서버·클라이언트 |
| ThreadTCPServer | 클라이언트마다 스레드를 생성하는 TCP 서버 |
| UDPServer, UDPClient | UDP 에코 서버·클라이언트, 응답 보낸 주소 확인 |
| UDPServer2, UDPClient2 | UDP 파일 전송 |
| Common.h | 오류 출력, 시간 측정 함수 |
| validateIP.h | IP 주소 형식 검사 |

## UDP 파일 전송 실행
UDPServer2.exe 9000 받을파일명
UDPClient2.exe 127.0.0.1 9000 보낼파일명

## UDP 파일 전송 방식
1. 클라이언트가 파일 크기를 먼저 보낸다.
2. 파일을 1024바이트씩 나눠 보내고, 서버의 응답(ACK)을 받은 뒤 다음 블록을 보낸다.
3. 전송이 끝나면 걸린 시간을 마이크로초 단위로 출력한다.

## 아쉬운 점
- 타임아웃이 없어서 패킷이 유실되면 클라이언트가 계속 기다린다.
- 시퀀스 번호가 없어서 중복 패킷을 구분하지 못한다.
- 파일 크기가 1024의 배수면 서버가 블록 하나를 더 기다려서 종료되지 않는다.
